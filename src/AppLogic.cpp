#include "AppLogic.h"
#include "CpuInfo.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

AppLogic::AppLogic() : m_state(State::Stopped)
{
    size_t threadCount = CpuInfo::PhysicalCores();
    m_pool = std::make_shared<BS::thread_pool<>>(threadCount);
    std::ostringstream oss;
    oss << u8"线程池已启动 thread num: " << threadCount;
    AddLog(oss.str(), LogEntry::Level::Info);
}

AppLogic::~AppLogic() {}

void AppLogic::AddLog(const std::string &message, LogEntry::Level level)
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);

    std::stringstream ss;
    ss << "[" << std::put_time(&tm, "%H:%M:%S") << "] " << message;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.push_back({level, ss.str()});
}

std::vector<LogEntry> AppLogic::GetLogs() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_logs;
}