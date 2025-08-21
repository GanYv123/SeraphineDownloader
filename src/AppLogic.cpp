#include "AppLogic.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

AppLogic::AppLogic() : m_state(State::Stopped)
{
    AddLog(u8"程序启动成功", LogEntry::Level::Info);
}

AppLogic::~AppLogic() {}

void AppLogic::OnStartClicked()
{
    if(m_state == State::Stopped || m_state == State::Paused){
        m_state = State::Running;
        AddLog(u8"点击 Start 按钮", LogEntry::Level::Info);
    }
}

void AppLogic::OnPauseClicked()
{
    if(m_state == State::Running){
        m_state = State::Paused;
        AddLog(u8"点击 Pause 按钮", LogEntry::Level::Info);
    } else if(m_state == State::Paused){
        m_state = State::Stopped;
        AddLog(u8"点击 Stop 按钮", LogEntry::Level::Info);
    }
}

std::string AppLogic::GetStatusText() const
{
    switch(m_state){
    case State::Stopped: return "Stopped";
    case State::Running: return "Running";
    case State::Paused:  return "Paused";
    default: return "Unknown";
    }
}

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
