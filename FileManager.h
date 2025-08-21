#pragma once
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

    // ͬ����ѹ
    void ExtractZip(const std::wstring& zipPath, const std::wstring& destFolder, LogCallback callback = nullptr);
    // �첽��ѹ
    bool ExtractZipAsync(const std::wstring& zipPath, const std::wstring& destFolder, LogCallback callback = nullptr);

    // ���������ݷ�ʽ
    bool CreateShortcut(const std::wstring& relativeExePath, const std::wstring& shortcutName, LogCallback callback = nullptr);

    // ��/�رճ���
    bool RunProgram(const std::wstring& relativeExePath, LogCallback callback = nullptr);
    bool CloseProgram(LogCallback callback = nullptr);

    // ״̬������
    bool IsExtracting() const { return m_extracting; }
    bool IsExtracted() const { return m_extracted; }

private:
    std::mutex m_mutex;
    bool m_extracting = false;
    bool m_extracted = false;

    HANDLE m_processHandle = nullptr;
    DWORD m_processId = 0;
};
