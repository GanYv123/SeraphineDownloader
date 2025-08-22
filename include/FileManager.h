#pragma once
#include "AppLogic.h"
#include <string>
#include <functional>
#include <mutex>
#include <windows.h>


class FileManager
{
public:
    using LogCallback = std::function<void(const std::string&, int)>;

    FileManager();
    ~FileManager();

    // �첽��ѹ
    bool ExtractZipAsync(const std::wstring& zipPath, const std::wstring& destFolder, 
        AppLogic& logic,LogCallback callback = nullptr);

    // ���������ݷ�ʽ
    bool CreateShortcut(const std::wstring& relativeExePath, const std::wstring& shortcutName, LogCallback callback = nullptr);

    // ��/�رճ���
    bool RunProgram(const std::wstring& relativeExePath, LogCallback callback = nullptr);
    bool CloseProgram(LogCallback callback = nullptr);

    // ״̬������
    bool IsExtracting() const { return m_extracting.load(); }
    bool IsExtracted() const { return m_extracted.load(); }
private:
    // ͬ����ѹ
    void ExtractZip(const std::wstring& zipPath, const std::wstring& destFolder, LogCallback callback = nullptr);

private:
    std::mutex m_mutex;
    //bool m_extracting = false; // ʹ�� atomic �ع�
    std::atomic<bool> m_extracting;
    //bool m_extracted = false;
    std::atomic<bool> m_extracted;

    HANDLE m_processHandle = nullptr;
    DWORD m_processId = 0;
};
