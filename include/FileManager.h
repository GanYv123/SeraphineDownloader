#pragma once
#include "AppLogic.h"
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <windows.h>

class FileManager
{
public:
    using LogCallback = std::function<void(const std::string&, int)>;

    FileManager();
    ~FileManager();

    // �첽��ѹ
    bool ExtractZipAsync(const std::wstring& zipPath, const std::wstring& destFolder, AppLogic& logic, LogCallback callback = nullptr);

    // ���������ݷ�ʽ
    bool CreateShortcut(const std::wstring& folderPath, const std::wstring& shortcutName, LogCallback callback = nullptr);

    // ��/�رճ���
    bool RunProgram(const std::wstring& folderPath, LogCallback callback = nullptr);
    bool CloseProgram(LogCallback callback = nullptr);

    // ״̬������
    bool IsExtracting() const { return m_extracting.load(); }
    bool IsExtracted()  const { return m_extracted.load();  }
    bool IsExitZip()    const { return m_isExistZip.load(); }

    // ����/ֹͣ���
    void StartMonitoring(AppLogic& logic);
    void StopMonitoring();
    // ���¼���ļ�
    void UpdateMonitoredFile(const std::wstring& zipPath);

private:
    // ͬ����ѹ
    void ExtractZip(const std::wstring& zipPath, const std::wstring& destFolder, LogCallback callback = nullptr);

private:
    std::mutex m_mutex;
    std::atomic<bool> m_extracting  { false };
    std::atomic<bool> m_extracted   { false };
    std::atomic<bool> m_isExistZip  { false };

    HANDLE m_processHandle = nullptr;
    DWORD m_processId = 0;
    std::mutex m_monitorMutex;
    // ����߳�
    std::atomic<bool> m_stopMonitor{ false };
    std::thread m_monitorThread;
    std::wstring m_monitoredZip;
    std::wstring m_monitoredDest;
};
