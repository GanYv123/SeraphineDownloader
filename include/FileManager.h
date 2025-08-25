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

    // 异步解压
    bool ExtractZipAsync(const std::wstring& zipPath, const std::wstring& destFolder, AppLogic& logic, LogCallback callback = nullptr);

    // 创建桌面快捷方式
    bool CreateShortcut(const std::wstring& folderPath, const std::wstring& shortcutName, LogCallback callback = nullptr);

    // 打开/关闭程序
    bool RunProgram(const std::wstring& folderPath, LogCallback callback = nullptr);
    bool CloseProgram(LogCallback callback = nullptr);

    // 状态访问器
    bool IsExtracting() const { return m_extracting.load(); }
    bool IsExtracted()  const { return m_extracted.load();  }
    bool IsExitZip()    const { return m_isExistZip.load(); }

    // 启动/停止监控
    void StartMonitoring(AppLogic& logic);
    void StopMonitoring();
    // 更新监控文件
    void UpdateMonitoredFile(const std::wstring& zipPath);

private:
    // 同步解压
    void ExtractZip(const std::wstring& zipPath, const std::wstring& destFolder, LogCallback callback = nullptr);

private:
    std::mutex m_mutex;
    std::atomic<bool> m_extracting  { false };
    std::atomic<bool> m_extracted   { false };
    std::atomic<bool> m_isExistZip  { false };

    HANDLE m_processHandle = nullptr;
    DWORD m_processId = 0;
    std::mutex m_monitorMutex;
    // 监控线程
    std::atomic<bool> m_stopMonitor{ false };
    std::thread m_monitorThread;
    std::wstring m_monitoredZip;
    std::wstring m_monitoredDest;
};
