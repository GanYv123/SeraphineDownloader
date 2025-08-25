#pragma once
#include <windows.h>
#include <d3d11.h>
#include "Downloader.h"   // �����������ඨ��
#include "FileManager.h"

class AppLogic;
class ImGuiIO;

class UIManager
{
public:
    UIManager(AppLogic& logic);
    ~UIManager();

    bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    void Cleanup();
    void BeginFrame();
    bool RenderUI(AppLogic& logic, HWND hwnd);
    void EndFrame();

private:
    bool m_initialized;
    // ��ģ����Ⱦ����
    void RenderDragWindow(HWND hwnd);
    void RenderFunctionButtons(AppLogic& logic);
    void RenderDownloadProgress();
    void RenderLogOutput(AppLogic& logic);
    bool RenderExitButton(HWND hwnd);
    // ������������
    void AddChineseFont(ImGuiIO& io);

    // ��Ա�����������̬�ֲ�������
    Downloader downloader;
    FileManager fileManager;
    bool dragging = false;
    POINT drag_offset{};

    std::vector<std::string> m_fileList;
    int m_selectedIndex{-1};
};