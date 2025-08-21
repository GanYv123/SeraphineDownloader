#include "WindowManager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "resource.h"

// 全局单例指针（方便在 WndProc 中访问当前 WindowManager）
WindowManager* WindowManager::s_instance = nullptr;

// ImGui 的 Win32 窗口消息处理函数（官方封装）
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

WindowManager::WindowManager()
    : m_hwnd(nullptr)
    , m_hInstance(nullptr)
    , m_width(0)
    , m_height(0)
    , m_pd3dDevice(nullptr)
    , m_pd3dDeviceContext(nullptr)
    , m_pSwapChain(nullptr)
    , m_mainRenderTargetView(nullptr)
{
    s_instance = this; // 保存全局实例
}

WindowManager::~WindowManager()
{
    Cleanup();        // 析构时释放资源
    s_instance = nullptr;
}

// 初始化窗口（注册窗口类 + 创建窗口 + 初始化 D3D11）
bool WindowManager::Initialize(HINSTANCE hInstance, int width, int height)
{
    m_hInstance = hInstance;
    m_width = width;
    m_height = height;

    // 注册窗口类
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;   // 指定静态回调
    wc.hInstance = hInstance;
    wc.lpszClassName = _T("ImGuiTemplate");

    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));     // 大图标（任务栏）
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));   // 小图标（标题栏）

    RegisterClassEx(&wc);


    // 创建无边框窗口
    m_hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED, // 置顶 + 支持透明
        wc.lpszClassName,
        _T("ImGui Template"),
        WS_POPUP,                      // 无边框（POPUP 窗口）
        100, 100, width, height,       // 窗口位置和大小
        NULL, NULL, hInstance, NULL
    );

    if(!m_hwnd) return false;

    // 设置窗口透明属性（这里设置为完全不透明）
    SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);

    // 初始化 D3D11
    if(!CreateDeviceD3D()){
        CleanupDeviceD3D();
        DestroyWindow(m_hwnd);
        UnregisterClass(_T("ImGuiTemplate"), m_hInstance);
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hwnd);
    return true;
}

// 清理窗口和 D3D11
void WindowManager::Cleanup()
{
    CleanupDeviceD3D();
    if(m_hwnd){
        DestroyWindow(m_hwnd);
        UnregisterClass(_T("ImGuiTemplate"), m_hInstance);
        m_hwnd = nullptr;
    }
}

// 每帧渲染：清屏 + ImGui 绘制 + 交换缓冲
void WindowManager::Present()
{
    const float clear_color[4] = { 0,0,0,0 }; // 背景色（黑色）
    m_pd3dDeviceContext->OMSetRenderTargets(1, &m_mainRenderTargetView, NULL);
    m_pd3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView, clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    m_pSwapChain->Present(1, 0); // vsync = 1
}

// 创建设备 + 交换链
bool WindowManager::CreateDeviceD3D()
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;                                // 双缓冲
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA 格式
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hwnd;
    sd.SampleDesc.Count = 1;                           // 无多重采样
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;          // 常规丢弃模式

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0
    };

    // 创建设备 + 上下文 + 交换链
    if(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION,
        &sd, &m_pSwapChain, &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget(); // 创建渲染目标
    return true;
}

// 释放 D3D11
void WindowManager::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if(m_pSwapChain){ m_pSwapChain->Release(); m_pSwapChain = nullptr; }
    if(m_pd3dDeviceContext){ m_pd3dDeviceContext->Release(); m_pd3dDeviceContext = nullptr; }
    if(m_pd3dDevice){ m_pd3dDevice->Release(); m_pd3dDevice = nullptr; }
}

// 创建渲染目标（后缓冲 → RTV）
void WindowManager::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    m_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_mainRenderTargetView);
    pBackBuffer->Release();
}

// 释放渲染目标
void WindowManager::CleanupRenderTarget()
{
    if(m_mainRenderTargetView){
        m_mainRenderTargetView->Release();
        m_mainRenderTargetView = nullptr;
    }
}

// 窗口消息处理函数
LRESULT WINAPI WindowManager::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // 先交给 ImGui 的 Win32 实现处理（输入、鼠标等）
    if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch(msg){
    case WM_SIZE:
        if(s_instance && s_instance->m_pd3dDevice != NULL && wParam != SIZE_MINIMIZED){
            // 窗口大小调整 → 重新创建 RenderTarget
            s_instance->CleanupRenderTarget();
            s_instance->m_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam),
                DXGI_FORMAT_UNKNOWN, 0);
            s_instance->CreateRenderTarget();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0); // 关闭程序
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
