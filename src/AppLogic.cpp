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
    // 写入当前位置，覆盖最旧日志
    m_logs[m_nextIndex] = { level, ss.str() };
    m_nextIndex = (m_nextIndex + 1) % MAX_LOGS;

    if(m_count < MAX_LOGS) m_count++;
}

// 遍历日志，按时间顺序，从最旧到最新
void AppLogic::ForEachLog(const std::function<void(const LogEntry&)>& cb) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t start = (m_nextIndex + MAX_LOGS - m_count) % MAX_LOGS;
    for(size_t i = 0; i < m_count; i++){
        cb(m_logs[(start + i) % MAX_LOGS]);
    }
}
