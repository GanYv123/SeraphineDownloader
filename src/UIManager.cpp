#include "AppLogic.h"
#include "UIManager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <windows.h>

UIManager::UIManager(AppLogic& logic) : m_initialized(false) {
    // ����״̬����߳�
    fileManager.StartMonitoring(logic);
    fileManager.UpdateMonitoredFile(L"");
}
UIManager::~UIManager() { Cleanup(); }

bool UIManager::Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
     
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL;  // �����沼���ļ�
    // �����������
    AddChineseFont(io);

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

// ����Ⱦ����
bool UIManager::RenderUI(AppLogic& logic, HWND hwnd)
{
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

    ImGui::Begin(u8"���ߴ���", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);

    RenderDragWindow(hwnd);

    RenderFunctionButtons(logic);

    RenderDownloadProgress();

    RenderLogOutput(logic);

    bool exit = RenderExitButton(hwnd);

    ImGui::End();
    return exit;
}

// ---------------- ������ק ----------------
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

// ---------------- ���ܰ�ť -----------------
void UIManager::RenderFunctionButtons(AppLogic& logic)
{
    float availWidth = ImGui::GetContentRegionAvail().x;
    float buttonHeight = 24.0f;
    float spacing = 4.0f;
    float fourthWidth = (availWidth - 3 * spacing) / 4;

    // ---------- ���ѡ���ļ� ----------
    const bool hasSelected = (m_selectedIndex >= 0 && m_selectedIndex < m_fileList.size());
    static std::string lastSelectedFile;
    static std::wstring wsSelectedFile;

    if(hasSelected && m_fileList[m_selectedIndex] != lastSelectedFile){
        lastSelectedFile = m_fileList[m_selectedIndex];
        wsSelectedFile = std::wstring(lastSelectedFile.begin(), lastSelectedFile.end());

        // ���� FileManager ���·��
        fileManager.UpdateMonitoredFile(wsSelectedFile);
    }

    // ---------- ״̬�� ----------
    ImGui::Text(u8"ѹ����: %s | �ѽ�ѹ: %s",
        fileManager.IsExitZip() ? u8"����" : u8"������",
        fileManager.IsExtracted() ? u8"��" : u8"��");
    ImGui::Separator();

    // ---------- �ļ������� ----------
    float thirdWidthDownload = (availWidth - 2 * spacing) / 3;
    if(ImGui::Button(u8"ˢ���ļ��б�", ImVec2(thirdWidthDownload, buttonHeight))){
        m_fileList = downloader.FetchFileList("http://07210d00.cn:8080/list");
        m_selectedIndex = m_fileList.empty() ? -1 : 0;
        logic.AddLog(u8"[INFO] �ļ��б�ˢ�����", LogEntry::Level::Info);
    }

    ImGui::SameLine();
    const char* preview = hasSelected ? lastSelectedFile.c_str() : u8"�볢��ˢ���ļ��б�";
    ImGui::SetNextItemWidth(thirdWidthDownload);
    if(ImGui::BeginCombo("##fileCombo", preview)){
        if(m_fileList.empty()) ImGui::Selectable(u8"���ļ�", false);
        else{
            for(int i = 0; i < m_fileList.size(); i++){
                bool isSelected = (i == m_selectedIndex);
                if(ImGui::Selectable(m_fileList[i].c_str(), isSelected)) m_selectedIndex = i;
                if(isSelected) ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if(ImGui::Button(u8"�����ļ�", ImVec2(thirdWidthDownload, buttonHeight)) && hasSelected){
        std::string url = "http://07210d00.cn:8080/download?file=" + lastSelectedFile;
        std::string savePath = "./" + lastSelectedFile;
        downloader.StartDownload(url, savePath, logic);
        logic.AddLog(u8"[INFO] ��ʼ����: " + lastSelectedFile, LogEntry::Level::Info);
    }

    ImGui::Spacing();

    // ---------- ���ܰ�ť������ѹ / ��ݷ�ʽ / �� / �رգ� ----------
    bool extractEnabled = hasSelected && fileManager.IsExitZip();
    if(!extractEnabled) ImGui::BeginDisabled();
    if(ImGui::Button(u8"��ѹ�ļ�", ImVec2(fourthWidth, buttonHeight)) && hasSelected){
        fileManager.ExtractZipAsync(wsSelectedFile, wsSelectedFile.substr(0, wsSelectedFile.find_last_of(L'.')), logic,
            [&](const std::string& msg, int level){ logic.AddLog(msg, (LogEntry::Level)level); });
    }
    if(!extractEnabled) ImGui::EndDisabled();
    ImGui::SameLine();

    bool shortcutEnabled = fileManager.IsExtracted();
    if(!shortcutEnabled) ImGui::BeginDisabled();
    if(ImGui::Button(u8"���������ݷ�ʽ", ImVec2(fourthWidth, buttonHeight))){
        std::wstring exeName = wsSelectedFile.substr(0, wsSelectedFile.find_last_of(L'.'));
        std::wstring exePath = exeName + L"\\" + exeName;
        fileManager.CreateShortcut(exePath, wsSelectedFile.substr(0, wsSelectedFile.find_last_of(L'.')),
            [&](const std::string& msg, int level){ logic.AddLog(msg, (LogEntry::Level)level); });
    }
    if(!shortcutEnabled) ImGui::EndDisabled();
    ImGui::SameLine();

    if(!shortcutEnabled) ImGui::BeginDisabled();
    if(ImGui::Button(u8"�򿪳���", ImVec2(fourthWidth, buttonHeight))){
        std::wstring exeName = wsSelectedFile.substr(0, wsSelectedFile.find_last_of(L'.'));
        std::wstring exePath = exeName + L"\\" + exeName;
        fileManager.RunProgram(exePath,
            [&](const std::string& msg, int level){ logic.AddLog(msg, (LogEntry::Level)level); });
    }
    if(!shortcutEnabled) ImGui::EndDisabled();
    ImGui::SameLine();

    if(!shortcutEnabled) ImGui::BeginDisabled();
    if(ImGui::Button(u8"�رճ���", ImVec2(fourthWidth, buttonHeight))){
        fileManager.CloseProgram(
            [&](const std::string& msg, int level){ logic.AddLog(msg, (LogEntry::Level)level); });
    }
    if(!shortcutEnabled) ImGui::EndDisabled();
}

// ---------------- ���ؽ��� ----------------
void UIManager::RenderDownloadProgress()
{
    ImGui::Text(u8"���ؽ��ȣ�");

    float progress = downloader.GetProgress();
    bool finished = !downloader.IsDownloading() && downloader.IsFinished();

    // ---------- ��̬��������ɫ ----------
    ImVec4 barColor;
    if(finished){
        barColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // �����ɫ
    } else if(progress < 0.5f){
        float t = progress / 0.5f;
        barColor = ImVec4(1.0f, t, 0.0f, 1.0f); // ��->��
    } else{
        float t = (progress - 0.5f) / 0.5f;
        barColor = ImVec4(1.0f - t, 1.0f, 0.0f, 1.0f); // ��->��
    }

    // ---------- ���ƽ����� ----------
    ImVec2 barSize(-1.0f, 25.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
    ImGui::ProgressBar(progress, barSize, ""); // ������
    ImGui::PopStyleColor();

    // ---------- ���� ----------
    char overlay[32];
    if(finished){
        strcpy_s(overlay, u8"���!");
    } else{
        int percent = static_cast<int>(progress * 100);
        sprintf_s(overlay, "%d%%", percent);
    }

    // ---------- �������� ----------
    ImVec2 pos = ImGui::GetItemRectMin();
    ImVec2 size = ImGui::GetItemRectSize();
    ImVec2 textSize = ImGui::CalcTextSize(overlay);
    ImVec2 textPos = ImVec2(pos.x + (size.x - textSize.x) * 0.5f,
        pos.y + (size.y - textSize.y) * 0.5f);

    // ---------- ������ɫ������ <50% ��ɫ��>=50% ��ɫ ----------
    ImVec4 textColor = (progress < 0.5f) ? ImVec4(1, 1, 1, 1) : ImVec4(0, 0, 0, 1);
    ImU32 col = ImGui::ColorConvertFloat4ToU32(textColor);
    ImGui::GetWindowDrawList()->AddText(textPos, col, overlay);

    // ---------- ״̬��Ϣ ----------
    ImGui::Text(u8"״̬: %s", downloader.GetStatus().c_str());
    ImGui::Separator();
}


// ---------------- ��־��� ----------------
void UIManager::RenderLogOutput(AppLogic& logic)
{
    ImGui::Text(u8"��־�����");
    ImGui::BeginChild("LogRegion", ImVec2(0, 160), true, ImGuiWindowFlags_HorizontalScrollbar);
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

// ---------------- �˳� / ��С�� ----------------
bool UIManager::RenderExitButton(HWND hwnd)
{
    bool exit = false;

    float availWidth = ImGui::GetContentRegionAvail().x;
    float buttonHeight = 30.0f;
    float buttonWidth = (availWidth - 5.0f) / 2; // ������ťƽ��һ�У��м��� 5px

    // �˳���ť
    if(ImGui::Button(u8"�˳�", ImVec2(buttonWidth, buttonHeight))){
        exit = true;
    }

    ImGui::SameLine();

    // ��С����ť
    if(ImGui::Button(u8"��С��", ImVec2(buttonWidth, buttonHeight))){
        ShowWindow(hwnd, SW_MINIMIZE);
    }

    return exit;
}

void UIManager::AddChineseFont(ImGuiIO& io)
{
    wchar_t* windir = nullptr;
    size_t len = 0;
    if(_wdupenv_s(&windir, &len, L"WINDIR") != 0 || !windir)
        return;

    // ƴ������·��
    std::wstring fontPath = std::wstring(windir) + L"\\Fonts\\msyh.ttc";
    free(windir);

    // ת UTF-8
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, fontPath.c_str(), (int)fontPath.size(), NULL, 0, NULL, NULL);
    std::string fontPathUtf8(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, fontPath.c_str(), (int)fontPath.size(), fontPathUtf8.data(), size_needed, NULL, NULL);

    // ��������
    io.Fonts->AddFontFromFileTTF(fontPathUtf8.c_str(), 16.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
}

void UIManager::EndFrame()
{
    ImGui::Render();
}


