#include "FileManager.h"
#include <atlbase.h>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <shlobj.h>
#include <shobjidl.h>
#include <sstream>
#include <thread>
#include <windows.h>

// miniz 单文件库
#define MINIZ_HEADER_FILE_ONLY
#include "miniz.h"

FileManager::FileManager() {}
FileManager::~FileManager() { StopMonitoring(); }

// ----------------- 同步解压 -----------------
void FileManager::ExtractZip(const std::wstring &zipPath,
                             const std::wstring &destFolder,
                             LogCallback callback)
{
    std::string zipPathA(zipPath.begin(), zipPath.end());
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (!mz_zip_reader_init_file(&zip_archive, zipPathA.c_str(), 0))
    {
        if (callback)
            callback(u8"[ERROR] 打开 ZIP 文件失败", 2);
        return;
    }

    int numFiles = (int) mz_zip_reader_get_num_files(&zip_archive);

    for (int i = 0; i < numFiles; i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
            continue;

        std::filesystem::path fullPath = std::filesystem::path(destFolder) / file_stat.m_filename;

        if (file_stat.m_is_directory)
        {
            std::filesystem::create_directories(fullPath);
            if (callback)
                callback(u8"[INFO] 创建文件夹: " + fullPath.string(), 1);
        }
        else
        {
            std::filesystem::create_directories(fullPath.parent_path());
            if (mz_zip_reader_extract_to_file(&zip_archive, i, fullPath.string().c_str(), 0))
            {
                if (callback)
                    callback(u8"[INFO] 解压文件: " + fullPath.string(), 1);
            }
            else
            {
                if (callback)
                    callback(u8"[ERROR] 解压失败: " + fullPath.string(), 2);
            }
        }
    }

    mz_zip_reader_end(&zip_archive);
    m_extracted.store(true);
    if (callback)
        callback(u8"[INFO] 解压完成", 1);
}

// ----------------- 异步解压 -----------------
bool FileManager::ExtractZipAsync(const std::wstring &zipPath,
                                  const std::wstring &destFolder,
                                  AppLogic &logic,
                                  LogCallback callback)
{
    if (!m_isExistZip)
    {
        if (callback)
            callback(u8"[ERROR] 解压失败 压缩包不存在", 1);
        m_extracting.store(false);
        return false;
    }

    if (m_extracting.load() || m_extracted.load())
    {
        if (callback)
            callback(u8"[WARN] 正在解压或已解压完成", 1);
        return false;
    }
    m_extracting.store(true);

    logic.SubmitTask(
        [zipPath, destFolder, callback, this]
        {
            try
            {
                ExtractZip(zipPath, destFolder, callback);
            }
            catch (...)
            {
                if (callback)
                    callback(u8"[ERROR] 解压失败", 1);
            }
            m_extracting.store(false);
        });

    if (callback)
        callback(u8"[INFO] 异步解压启动", 1);
    return true;
}

// ----------------- 快捷方式（支持传入文件夹） -----------------
bool FileManager::CreateShortcut(const std::wstring &folderPath,
                                 const std::wstring &shortcutName,
                                 LogCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 获取 exe 文件夹的完整路径
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileName(NULL, exePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    std::filesystem::path targetDir = exeDir / folderPath;

    if (!std::filesystem::exists(targetDir) || !std::filesystem::is_directory(targetDir))
    {
        if (callback)
            callback(u8"[ERROR] 文件夹不存在或不是目录", 2);
        return false;
    }

    // 遍历文件夹寻找第一个 exe 文件
    std::filesystem::path exeFile;
    for (auto &entry : std::filesystem::directory_iterator(targetDir))
    {
        if (entry.is_regular_file() && entry.path().extension() == L".exe")
        {
            exeFile = entry.path();
            break;
        }
    }

    if (exeFile.empty())
    {
        if (callback)
            callback(u8"[ERROR] 文件夹中没有 exe 文件", 2);
        return false;
    }

    // 初始化 COM
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        if (callback)
            callback(u8"[ERROR] CoInitialize 失败", 2);
        return false;
    }

    CComPtr<IShellLink> pShellLink;
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellLink));
    if (FAILED(hr))
    {
        CoUninitialize();
        if (callback)
            callback(u8"[ERROR] 创建 ShellLink 失败", 2);
        return false;
    }

    pShellLink->SetPath(exeFile.wstring().c_str());
    pShellLink->SetWorkingDirectory(exeFile.parent_path().wstring().c_str());

    CComPtr<IPersistFile> pPersistFile;
    hr = pShellLink.QueryInterface(&pPersistFile);
    if (FAILED(hr))
    {
        CoUninitialize();
        if (callback)
            callback(u8"[ERROR] QueryInterface 失败", 2);
        return false;
    }

    wchar_t desktopPath[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath)))
    {
        CoUninitialize();
        if (callback)
            callback(u8"[ERROR] 获取桌面路径失败", 2);
        return false;
    }

    std::filesystem::path shortcutFile =
        std::filesystem::path(desktopPath) / (shortcutName + L".lnk");
    hr = pPersistFile->Save(shortcutFile.wstring().c_str(), TRUE);
    if (FAILED(hr))
    {
        CoUninitialize();
        if (callback)
            callback(u8"[ERROR] 保存快捷方式失败", 2);
        return false;
    }

    CoUninitialize();
    if (callback)
        callback(u8"[INFO] 快捷方式创建成功", 1);
    return true;
}

// ----------------- 打开程序（支持传入文件夹） -----------------
bool FileManager::RunProgram(const std::wstring &folderPath, LogCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_processHandle)
    {
        if (callback)
            callback(u8"[WARN] 程序已在运行", 1);
        return false;
    }

    // 获取当前 exe 的目录
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    std::filesystem::path targetDir = exeDir / folderPath;

    if (callback)
        callback(u8"[INFO] 检查目录: " + targetDir.u8string(), 1);

    // 检查目录是否存在
    if (!std::filesystem::exists(targetDir) || !std::filesystem::is_directory(targetDir))
    {
        if (callback)
            callback(u8"[ERROR] 文件夹不存在或不是目录", 2);
        return false;
    }

    // 遍历文件夹寻找第一个 exe 文件
    std::filesystem::path exeFile;
    for (auto &entry : std::filesystem::directory_iterator(targetDir))
    {
        if (entry.is_regular_file() && entry.path().extension() == L".exe")
        {
            exeFile = entry.path();
            break;
        }
    }

    if (exeFile.empty())
    {
        if (callback)
            callback(u8"[ERROR] 文件夹中没有 exe 文件", 2);
        return false;
    }

    if (callback)
        callback(u8"[INFO] 找到可执行文件: " + exeFile.u8string(), 1);

    // ---------- 尝试 CreateProcess ----------
    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi{};

    if (!CreateProcessW(exeFile.wstring().c_str(),
                        nullptr,
                        nullptr,
                        nullptr,
                        FALSE,
                        0,
                        nullptr,
                        nullptr,
                        &si,
                        &pi))
    {
        DWORD errCode = GetLastError();

        if (errCode == ERROR_ELEVATION_REQUIRED)
        {
            if (callback)
                callback(u8"[WARN] 程序需要管理员权限，尝试使用 ShellExecute 提权", 1);

            HINSTANCE hInst = ShellExecuteW(nullptr,
                                            L"runas",                  // 请求管理员权限
                                            exeFile.wstring().c_str(), // 可执行文件
                                            nullptr,                   // 参数
                                            exeFile.parent_path().wstring().c_str(), // 工作目录
                                            SW_SHOWNORMAL);

            if ((INT_PTR) hInst <= 32)
            {
                if (callback)
                    callback(u8"[ERROR] ShellExecute 提权启动失败", 2);
                return false;
            }

            if (callback)
                callback(u8"[INFO] 已通过 ShellExecute 启动程序（无法跟踪进程句柄）", 1);
            return true; // 不用 m_processHandle
        }

        // 其他错误
        LPWSTR errMsg = nullptr;
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr,
                       errCode,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPWSTR) &errMsg,
                       0,
                       nullptr);

        std::wstring wmsg = errMsg ? errMsg : L"未知错误";
        if (errMsg)
            LocalFree(errMsg);

        std::string utf8msg(wmsg.begin(), wmsg.end());
        if (callback)
            callback(u8"[ERROR] 启动程序失败 (" + std::to_string(errCode) + "): " + utf8msg, 2);
        return false;
    }

    // ---------- 成功启动 ----------
    m_processHandle = pi.hProcess;
    m_processId = pi.dwProcessId;

    if (callback)
        callback(u8"[INFO] 程序启动成功 (PID=" + std::to_string(m_processId) + ")", 1);
    return true;
}

// ----------------- 关闭程序 -----------------
bool FileManager::CloseProgram(LogCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_processHandle)
    {
        if (callback)
            callback(u8"[WARN] 程序未运行或是 ShellExecute 启动的（不可控）请手动关闭", 1);
        return false;
    }

    TerminateProcess(m_processHandle, 0);
    CloseHandle(m_processHandle);
    m_processHandle = nullptr;
    m_processId = 0;
    if (callback)
        callback(u8"[INFO] 程序已关闭", 1);
    return true;
}

// ----------------- 状态监控 -----------------
void FileManager::StartMonitoring(AppLogic &logic)
{
    m_stopMonitor.store(false);

    logic.SubmitTask(
        [this]
        {
            while (!m_stopMonitor.load())
            {
                m_isExistZip.store(std::filesystem::exists(m_monitoredZip));
                bool extracted = std::filesystem::exists(m_monitoredDest) &&
                                 !std::filesystem::is_empty(m_monitoredDest);

                if (!m_isExistZip)
                {
                    m_extracting.store(false);
                    m_extracted.store(false);
                }
                else if (extracted)
                {
                    m_extracting.store(false);
                    m_extracted.store(true);
                }
                else if (m_isExistZip && !extracted)
                {
                    m_extracting.store(false);
                    m_extracted.store(false);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
}

void FileManager::StopMonitoring()
{
    m_stopMonitor.store(true);
    if (m_monitorThread.joinable())
        m_monitorThread.join();
}

void FileManager::UpdateMonitoredFile(const std::wstring &zipPath)
{
    std::lock_guard<std::mutex> lock(m_monitorMutex);
    m_monitoredZip = zipPath;
    m_monitoredDest = zipPath.substr(0, zipPath.find_last_of(L'.')); // 假设解压到同名文件夹
}