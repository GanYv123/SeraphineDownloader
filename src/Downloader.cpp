#include "Downloader.h"
#include "AppLogic.h"
#include <windows.h>
#include <winhttp.h>
#include <fstream>
#include <chrono>
#include <iostream>

#pragma comment(lib, "winhttp.lib")

Downloader::Downloader() : m_downloading(false), m_progress(0.0f), m_success(false) {}
Downloader::~Downloader() = default;

void Downloader::StartDownload(const std::string& url, const std::string& savePath, AppLogic& logic)
{
    if(m_downloading) return;
    
    m_downloading = true;
    m_progress = 0.0f;
    m_success = false;

    if(m_logCallback) m_logCallback("Start downloading: " + url, 0);

    // 提交到线程池，任务内部包装 try-catch
    logic.SubmitTask([this, url, savePath]() {
        try{
            DownloadThread(url, savePath);
        } catch(const std::exception& e){
            if(m_logCallback) m_logCallback(std::string("Task exception: ") + e.what(), 2);
        } catch(...){
            if(m_logCallback) m_logCallback("Unknown task exception", 2);
        }
        m_downloading = false; // 确保状态重置
        });
}

std::string Downloader::GetStatus() const
{
    if(m_downloading) return "Downloading...";
    return m_success ? "Download complete!" : "Idle/Failed";
}

void Downloader::DownloadThread(const std::string& url, const std::string& savePath)
{
    try{
        URL_COMPONENTS components = { sizeof(URL_COMPONENTS) };
        wchar_t hostName[256], urlPath[1024];
        components.lpszHostName = hostName;
        components.dwHostNameLength = _countof(hostName);
        components.lpszUrlPath = urlPath;
        components.dwUrlPathLength = _countof(urlPath);

        std::wstring wurl(url.begin(), url.end());
        if(!WinHttpCrackUrl(wurl.c_str(), 0, 0, &components)){
            if(m_logCallback) m_logCallback("Failed to parse URL", 2);
            return;
        }

        HINTERNET hSession = WinHttpOpen(L"MyDownloader/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
        HINTERNET hConnect = WinHttpConnect(hSession, components.lpszHostName,
            components.nPort, 0);
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", components.lpszUrlPath,
            NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
            (components.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0);

        if(!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)){
            if(m_logCallback) m_logCallback("Failed to send request", 2);
            return;
        }
        WinHttpReceiveResponse(hRequest, NULL);

        DWORD contentLength = 0, size = sizeof(contentLength);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &contentLength, &size, WINHTTP_NO_HEADER_INDEX);

        std::ofstream out(savePath, std::ios::binary);
        DWORD dwDownloaded = 0;
        DWORD totalDownloaded = 0;
        BYTE buffer[4096];

        DWORD lastLogged = 0;
        auto lastTime = std::chrono::steady_clock::now();

        while(WinHttpReadData(hRequest, buffer, sizeof(buffer), &dwDownloaded) && dwDownloaded > 0){
            out.write((char*)buffer, dwDownloaded);
            totalDownloaded += dwDownloaded;
            if(contentLength > 0)
                m_progress = (float)totalDownloaded / (float)contentLength;

            auto now = std::chrono::steady_clock::now();
            if(totalDownloaded - lastLogged > 50 * 1024 || std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count() > 500){
                lastLogged = totalDownloaded;
                lastTime = now;
                if(m_logCallback){
                    char buf[128];
                    sprintf_s(buf, "Downloaded: %u/%u bytes (%.2f%%)", totalDownloaded, contentLength, m_progress * 100);
                    m_logCallback(buf, 0); // Info
                }
            }
        }
        out.close();

        m_success = true;
        if(m_logCallback) m_logCallback("Download complete: " + savePath, 1); // Success

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
    } catch(const std::exception& e){
        if(m_logCallback) m_logCallback(std::string("Exception: ") + e.what(), 2);
    } catch(...){
        if(m_logCallback) m_logCallback("Unknown exception occurred", 2);
    }
}
