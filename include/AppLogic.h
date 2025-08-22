#pragma once
#include <string>
#include <vector>
#include <mutex>

struct LogEntry
{
    enum class Level { Info, Warn, Error } level;
    std::string text;
};

class AppLogic
{
public:
    enum class State { Stopped, Running, Paused };

    AppLogic();
    ~AppLogic();

    // 添加日志
    void AddLog(const std::string& message, LogEntry::Level level = LogEntry::Level::Info);

    // 获取日志列表（线程安全）
    std::vector<LogEntry> GetLogs() const;

private:
    State m_state;
    mutable std::mutex m_mutex;
    std::vector<LogEntry> m_logs;
};
