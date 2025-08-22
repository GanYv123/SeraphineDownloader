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

    // 异步解压
    bool ExtractZipAsync(const std::wstring& zipPath, const std::wstring& destFolder, 
        AppLogic& logic,LogCallback callback = nullptr);

    // 创建桌面快捷方式
    bool CreateShortcut(const std::wstring& relativeExePath, const std::wstring& shortcutName, LogCallback callback = nullptr);

    // 打开/关闭程序
    bool RunProgram(const std::wstring& relativeExePath, LogCallback callback = nullptr);
    bool CloseProgram(LogCallback callback = nullptr);

    // 状态访问器
    bool IsExtracting() const { return m_extracting.load(); }
    bool IsExtracted() const { return m_extracted.load(); }
private:
    // 同步解压
    void ExtractZip(const std::wstring& zipPath, const std::wstring& destFolder, LogCallback callback = nullptr);

private:
    std::mutex m_mutex;
    //bool m_extracting = false; // 使用 atomic 重构
    std::atomic<bool> m_extracting;
    //bool m_extracted = false;
    std::atomic<bool> m_extracted;

    HANDLE m_processHandle = nullptr;
    DWORD m_processId = 0;
};
