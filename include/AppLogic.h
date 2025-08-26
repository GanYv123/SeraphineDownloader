#pragma once
#include "BS_thread_pool.hpp" // 线程池
#include <future>

struct LogEntry
{
    enum class Level
    {
        Info,
        Warn,
        Error
    } level;
    std::string text;
};

class AppLogic
{
public:
    enum class State
    {
        Stopped,
        Running,
        Paused
    };

    AppLogic();
    ~AppLogic();

    // 添加日志
    void AddLog(const std::string &message, LogEntry::Level level = LogEntry::Level::Info);

    // 获取日志列表（线程安全）
    std::vector<LogEntry> GetLogs() const;

    // 对外暴露任务提交接口
    template <typename F, typename... Args>
    auto SubmitTask(F &&f, Args &&...args)
        -> std::future<typename std::invoke_result_t<F, Args...>>;

private:
    State m_state;
    mutable std::mutex m_mutex;
    std::vector<LogEntry> m_logs;
    std::shared_ptr<BS::thread_pool<>> m_pool;
};

template <typename F, typename... Args>
auto AppLogic::SubmitTask(F &&f, Args &&...args)
    -> std::future<typename std::invoke_result_t<F, Args...>>
{
    return m_pool->submit_task(std::forward<F>(f), std::forward<Args>(args)...);
}
