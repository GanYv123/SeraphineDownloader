#include "FileManager.h"
#include <iostream>
#include <filesystem>
#include <thread>
#include <atlbase.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <windows.h>

// miniz ���ļ���
#define MINIZ_HEADER_FILE_ONLY
#include "miniz.h"

FileManager::FileManager() {}
FileManager::~FileManager() {
    CloseProgram();
}

// ----------------- ��ѹ -----------------
void FileManager::ExtractZip(const std::wstring& zipPath, const std::wstring& destFolder, LogCallback callback)
{
    std::string zipPathA(zipPath.begin(), zipPath.end());
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if(!mz_zip_reader_init_file(&zip_archive, zipPathA.c_str(), 0)){
        if(callback) callback(u8"[ERROR] �� ZIP �ļ�ʧ��", 2);
        return;
    }

    int numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);

    for(int i = 0; i < numFiles; i++){
        mz_zip_archive_file_stat file_stat;
        if(!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;

        std::filesystem::path fullPath = std::filesystem::path(destFolder) / file_stat.m_filename;

        if(file_stat.m_is_directory){
            std::filesystem::create_directories(fullPath);
            if(callback) callback(u8"[INFO] �����ļ���: " + fullPath.string(), 1);
        } else{
            std::filesystem::create_directories(fullPath.parent_path());
            if(mz_zip_reader_extract_to_file(&zip_archive, i, fullPath.string().c_str(), 0)){
                if(callback) callback(u8"[INFO] ��ѹ�ļ�: " + fullPath.string(), 1);
            } else{
                if(callback) callback(u8"[ERROR] ��ѹʧ��: " + fullPath.string(), 2);
            }
        }
    }

    mz_zip_reader_end(&zip_archive);
    m_extracted = true;
    if(callback) callback(u8"[INFO] ��ѹ���", 1);
}

bool FileManager::ExtractZipAsync(const std::wstring& zipPath, const std::wstring& destFolder, LogCallback callback)
{
    if(m_extracting || m_extracted){
        if(callback) callback(u8"[WARN] ���ڽ�ѹ���ѽ�ѹ���", 1);
        return false;
    }
    m_extracting = true;

    std::thread([=](){
        ExtractZip(zipPath, destFolder, callback);
        m_extracting = false;
        }).detach();

    if(callback) callback(u8"[INFO] �첽��ѹ����", 1);
    return true;
}

// ----------------- ��ݷ�ʽ -----------------
bool FileManager::CreateShortcut(const std::wstring& relativeExePath, const std::wstring& shortcutName, LogCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    HRESULT hr = CoInitialize(NULL);
    if(FAILED(hr)){
        if(callback) callback(u8"[ERROR] CoInitialize ʧ��", 2);
        return false;
    }

    // ��ǰ EXE ����Ŀ¼
    wchar_t exePath[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, exePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();

    std::filesystem::path targetFullPath = exeDir / relativeExePath;

    CComPtr<IShellLink> pShellLink;
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellLink));
    if(FAILED(hr)){ CoUninitialize(); if(callback) callback(u8"[ERROR] ���� ShellLink ʧ��", 2); return false; }

    pShellLink->SetPath(targetFullPath.wstring().c_str());
    pShellLink->SetWorkingDirectory(targetFullPath.parent_path().wstring().c_str());

    CComPtr<IPersistFile> pPersistFile;
    hr = pShellLink.QueryInterface(&pPersistFile);
    if(FAILED(hr)){ CoUninitialize(); if(callback) callback(u8"[ERROR] QueryInterface ʧ��", 2); return false; }

    wchar_t desktopPath[MAX_PATH];
    if(!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath))){
        CoUninitialize(); if(callback) callback("[ERROR] ��ȡ����·��ʧ��", 2); return false;
    }

    std::filesystem::path shortcutFile = std::filesystem::path(desktopPath) / (shortcutName + L".lnk");
    hr = pPersistFile->Save(shortcutFile.wstring().c_str(), TRUE);
    if(FAILED(hr)){ CoUninitialize(); if(callback) callback(u8"[ERROR] �����ݷ�ʽʧ��", 2); return false; }

    CoUninitialize();
    if(callback) callback(u8"[INFO] ��ݷ�ʽ�����ɹ�", 1);
    return true;
}

// ----------------- ��/�رճ��� -----------------
bool FileManager::RunProgram(const std::wstring& relativeExePath, LogCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if(m_processHandle){
        if(callback) callback(u8"[WARN] ������������", 1);
        return false;
    }

    // ��ǰ EXE ����Ŀ¼
    wchar_t exePath[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, exePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    std::filesystem::path targetFullPath = exeDir / relativeExePath;

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if(!CreateProcess(targetFullPath.wstring().c_str(), nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)){
        if(callback) callback(u8"[ERROR] ��������ʧ��", 2);
        return false;
    }

    m_processHandle = pi.hProcess;
    m_processId = pi.dwProcessId;
    if(callback) callback(u8"[INFO] ���������ɹ�", 1);
    return true;
}

bool FileManager::CloseProgram(LogCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if(!m_processHandle){
        if(callback) callback(u8"[WARN] ����δ����", 1);
        return false;
    }

    TerminateProcess(m_processHandle, 0);
    CloseHandle(m_processHandle);
    m_processHandle = nullptr;
    m_processId = 0;
    if(callback) callback(u8"[INFO] �����ѹر�", 1);
    return true;
}
