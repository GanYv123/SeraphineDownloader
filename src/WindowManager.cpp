#include "WindowManager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "resource.h"

// ȫ�ֵ���ָ�루������ WndProc �з��ʵ�ǰ WindowManager��
WindowManager* WindowManager::s_instance = nullptr;

// ImGui �� Win32 ������Ϣ���������ٷ���װ��
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
    s_instance = this; // ����ȫ��ʵ��
}

WindowManager::~WindowManager()
{
    Cleanup();        // ����ʱ�ͷ���Դ
    s_instance = nullptr;
}

// ��ʼ�����ڣ�ע�ᴰ���� + �������� + ��ʼ�� D3D11��
bool WindowManager::Initialize(HINSTANCE hInstance, int width, int height)
{
    m_hInstance = hInstance;
    m_width = width;
    m_height = height;

    // ע�ᴰ����
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;   // ָ����̬�ص�
    wc.hInstance = hInstance;
    wc.lpszClassName = _T("ImGuiTemplate");

    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));     // ��ͼ�꣨��������
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));   // Сͼ�꣨��������

    RegisterClassEx(&wc);


    // �����ޱ߿򴰿�
    m_hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED, // �ö� + ֧��͸��
        wc.lpszClassName,
        _T("ImGui Template"),
        WS_POPUP,                      // �ޱ߿�POPUP ���ڣ�
        100, 100, width, height,       // ����λ�úʹ�С
        NULL, NULL, hInstance, NULL
    );

    if(!m_hwnd) return false;

    // ���ô���͸�����ԣ���������Ϊ��ȫ��͸����
    SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);

    // ��ʼ�� D3D11
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

// �����ں� D3D11
void WindowManager::Cleanup()
{
    CleanupDeviceD3D();
    if(m_hwnd){
        DestroyWindow(m_hwnd);
        UnregisterClass(_T("ImGuiTemplate"), m_hInstance);
        m_hwnd = nullptr;
    }
}

// ÿ֡��Ⱦ������ + ImGui ���� + ��������
void WindowManager::Present()
{
    const float clear_color[4] = { 0,0,0,0 }; // ����ɫ����ɫ��
    m_pd3dDeviceContext->OMSetRenderTargets(1, &m_mainRenderTargetView, NULL);
    m_pd3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView, clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    m_pSwapChain->Present(1, 0); // vsync = 1
}

// �����豸 + ������
bool WindowManager::CreateDeviceD3D()
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;                                // ˫����
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA ��ʽ
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hwnd;
    sd.SampleDesc.Count = 1;                           // �޶��ز���
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;          // ���涪��ģʽ

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0
    };

    // �����豸 + ������ + ������
    if(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION,
        &sd, &m_pSwapChain, &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget(); // ������ȾĿ��
    return true;
}

// �ͷ� D3D11
void WindowManager::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if(m_pSwapChain){ m_pSwapChain->Release(); m_pSwapChain = nullptr; }
    if(m_pd3dDeviceContext){ m_pd3dDeviceContext->Release(); m_pd3dDeviceContext = nullptr; }
    if(m_pd3dDevice){ m_pd3dDevice->Release(); m_pd3dDevice = nullptr; }
}

// ������ȾĿ�꣨�󻺳� �� RTV��
void WindowManager::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    m_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_mainRenderTargetView);
    pBackBuffer->Release();
}

// �ͷ���ȾĿ��
void WindowManager::CleanupRenderTarget()
{
    if(m_mainRenderTargetView){
        m_mainRenderTargetView->Release();
        m_mainRenderTargetView = nullptr;
    }
}

// ������Ϣ������
LRESULT WINAPI WindowManager::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // �Ƚ��� ImGui �� Win32 ʵ�ִ������롢���ȣ�
    if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch(msg){
    case WM_SIZE:
        if(s_instance && s_instance->m_pd3dDevice != NULL && wParam != SIZE_MINIMIZED){
            // ���ڴ�С���� �� ���´��� RenderTarget
            s_instance->CleanupRenderTarget();
            s_instance->m_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam),
                DXGI_FORMAT_UNKNOWN, 0);
            s_instance->CreateRenderTarget();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0); // �رճ���
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
