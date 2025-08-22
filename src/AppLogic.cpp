#include "AppLogic.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include "CpuInfo.h"

AppLogic::AppLogic() : m_state(State::Stopped)
{
    // 获取核心数
    size_t threadCount = CpuInfo::PhysicalCores();
    // 创建线程池
    m_pool = std::make_shared<BS::thread_pool<>>(threadCount);
    // 记录日志
    std::ostringstream oss;
    oss << u8"程序启动成功，线程池线程数: " << threadCount;
    AddLog(oss.str(), LogEntry::Level::Info);
}

AppLogic::~AppLogic() {}

void AppLogic::AddLog(const std::string& message, LogEntry::Level level)
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);

    std::stringstream ss;
    ss << "[" << std::put_time(&tm, "%H:%M:%S") << "] " << message;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.push_back({ level, ss.str() });
}

std::vector<LogEntry> AppLogic::GetLogs() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_logs;
}
