#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>

class Downloader
{
public:
    using LogCallback = std::function<void(const std::string& message, int level)>;

    Downloader();
    ~Downloader();

    void StartDownload(const std::string& url, const std::string& savePath);

    float GetProgress() const { return m_progress; }
    std::string GetStatus() const;
    bool IsDownloading() const { return m_downloading; }
    bool IsFinished() const { return m_success && !m_downloading; }

    void SetLogCallback(LogCallback cb) { m_logCallback = cb; }

private:
    void DownloadThread(const std::string& url, const std::string& savePath);

    std::thread m_thread;
    std::atomic<bool> m_downloading;
    std::atomic<float> m_progress; // 下载进度
    std::atomic<bool> m_success;   // 是否成功
    LogCallback m_logCallback;
};
