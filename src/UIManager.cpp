#include "AppLogic.h"
#include "UIManager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <windows.h>

UIManager::UIManager() : m_initialized(false) {}
UIManager::~UIManager() { Cleanup(); }

bool UIManager::Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL;  // 不保存布局文件
    //io.Fonts->AddFontDefault();
    // 添加中文字体
    io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\msyh.ttc", 16.0f, NULL, io.Fonts->GetGlyphRangesChineseFull()
    );
    ImGui::StyleColorsDark();

    if(!ImGui_ImplWin32_Init(hwnd)) return false;
    if(!ImGui_ImplDX11_Init(device, context)){
        ImGui_ImplWin32_Shutdown();
        return false;
    }

    m_initialized = true;
    return true;
}

void UIManager::Cleanup()
{
    if(m_initialized){
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_initialized = false;
    }
}

void UIManager::BeginFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

// 主渲染函数
bool UIManager::RenderUI(AppLogic& logic, HWND hwnd)
{
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

    ImGui::Begin(u8"工具窗口", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);

    RenderDragWindow(hwnd);

    RenderFunctionButtons(logic);

    RenderDownloadProgress();

    RenderLogOutput(logic);

    bool exit = RenderExitButton(hwnd);

    ImGui::End();
    return exit;
}

// ---------------- 窗口拖拽 ----------------
void UIManager::RenderDragWindow(HWND hwnd)
{
    if(ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Left)){
        POINT pt;
        GetCursorPos(&pt);
        if(!dragging) drag_offset = pt;
        int dx = pt.x - drag_offset.x;
        int dy = pt.y - drag_offset.y;
        drag_offset = pt;
        RECT rect;
        GetWindowRect(hwnd, &rect);
        SetWindowPos(hwnd, NULL, rect.left + dx, rect.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        dragging = true;
    } else{
        dragging = false;
    }
}

// ---------------- 功能按钮 ----------------
void UIManager::RenderFunctionButtons(AppLogic& logic)
{
    ImGui::Text(u8"功能操作：");
    ImGui::Spacing();

    float availWidth = ImGui::GetContentRegionAvail().x;
    float buttonHeight = 28.0f;

    // 第一行按钮
    int firstRowButtons = 3;
    float buttonWidth1 = (availWidth - (firstRowButtons - 1) * 5.0f) / firstRowButtons;

    // 下载
    if(ImGui::Button(u8"下载文件", ImVec2(buttonWidth1, buttonHeight))){
        downloader.StartDownload("http://07210d00.cn:8080/download?file=Seraphine.zip", "./Seraphine.zip",logic);
        logic.AddLog(u8"[INFO] 下载开始", LogEntry::Level::Info);
    }
    ImGui::SameLine();

    // 解压
    bool extractEnabled = downloader.IsFinished() && !fileManager.IsExtracted() && !fileManager.IsExtracting();
    if(!extractEnabled) ImGui::BeginDisabled();
    if(ImGui::Button(u8"解压文件", ImVec2(buttonWidth1, buttonHeight))){
        fileManager.ExtractZipAsync(L"Seraphine.zip", L"Seraphine",logic,
            [&](const std::string& msg, int level){ logic.AddLog(msg, (LogEntry::Level)level); });
    }
    if(!extractEnabled) ImGui::EndDisabled();
    ImGui::SameLine();

    // 创建快捷方式
    bool shortcutEnabled = fileManager.IsExtracted();
    if(!shortcutEnabled) ImGui::BeginDisabled();
    if(ImGui::Button(u8"创建桌面快捷方式", ImVec2(buttonWidth1, buttonHeight))){
        fileManager.CreateShortcut(L"Seraphine\\Seraphine\\Seraphine.exe", L"Seraphine",
            [&](const std::string& msg, int level){ logic.AddLog(msg, (LogEntry::Level)level); });
    }
    if(!shortcutEnabled) ImGui::EndDisabled();

    ImGui::NewLine();

    // 第二行按钮
    int secondRowButtons = 2;
    float buttonWidth2 = (availWidth - (secondRowButtons - 1) * 5.0f) / secondRowButtons;

    bool runEnabled = fileManager.IsExtracted();
    if(!runEnabled) ImGui::BeginDisabled();
    if(ImGui::Button(u8"打开程序", ImVec2(buttonWidth2, buttonHeight))){
        fileManager.RunProgram(L"Seraphine\\Seraphine\\Seraphine.exe",
            [&](const std::string& msg, int level){ logic.AddLog(msg, (LogEntry::Level)level); });
    }
    if(!runEnabled) ImGui::EndDisabled();
    ImGui::SameLine();

    if(!runEnabled) ImGui::BeginDisabled();
    if(ImGui::Button(u8"关闭程序", ImVec2(buttonWidth2, buttonHeight))){
        fileManager.CloseProgram(
            [&](const std::string& msg, int level){ logic.AddLog(msg, (LogEntry::Level)level); });
    }
    if(!runEnabled) ImGui::EndDisabled();

    ImGui::Spacing();
}

// ---------------- 下载进度 ----------------
void UIManager::RenderDownloadProgress()
{
    ImGui::Text(u8"下载进度：");
    float progress = downloader.GetProgress();
    if(!downloader.IsDownloading() && downloader.IsFinished()) progress = 1.0f;
    ImGui::ProgressBar(progress, ImVec2(-1.0f, 25.0f));
    ImGui::Text(u8"状态: %s", downloader.GetStatus().c_str());
    ImGui::Separator();
}

// ---------------- 日志输出 ----------------
void UIManager::RenderLogOutput(AppLogic& logic)
{
    ImGui::Text(u8"日志输出：");
    ImGui::BeginChild("LogRegion", ImVec2(0, 130), true, ImGuiWindowFlags_HorizontalScrollbar);
    for(const auto& entry : logic.GetLogs()){
        ImVec4 color = ImVec4(1, 1, 1, 1);
        switch(entry.level){
        case LogEntry::Level::Info:  color = ImVec4(1, 1, 1, 1); break;
        case LogEntry::Level::Warn:  color = ImVec4(1, 1, 0, 1); break;
        case LogEntry::Level::Error: color = ImVec4(1, 0, 0, 1); break;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(entry.text.c_str());
        ImGui::PopStyleColor();
    }
    if(ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    ImGui::Spacing();
}

// ---------------- 退出 / 最小化 ----------------
bool UIManager::RenderExitButton(HWND hwnd)
{
    bool exit = false;

    float availWidth = ImGui::GetContentRegionAvail().x;
    float buttonHeight = 30.0f;
    float buttonWidth = (availWidth - 5.0f) / 2; // 两个按钮平分一行，中间留 5px

    // 退出按钮
    if(ImGui::Button(u8"退出", ImVec2(buttonWidth, buttonHeight))){
        exit = true;
    }

    ImGui::SameLine();

    // 最小化按钮
    if(ImGui::Button(u8"最小化", ImVec2(buttonWidth, buttonHeight))){
        ShowWindow(hwnd, SW_MINIMIZE);
    }

    return exit;
}

void UIManager::EndFrame()
{
    ImGui::Render();
}
