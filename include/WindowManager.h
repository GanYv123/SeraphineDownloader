#pragma once
#include <d3d11.h>
#include <tchar.h>
#include <windows.h>

class WindowManager
{
public:
    WindowManager();
    ~WindowManager();

    bool Initialize(HINSTANCE hInstance, int width, int height);
    void Cleanup();
    void Present();

    HWND GetHwnd() const { return m_hwnd; }
    ID3D11Device *GetDevice() const { return m_pd3dDevice; }
    ID3D11DeviceContext *GetDeviceContext() const { return m_pd3dDeviceContext; }

    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    bool CreateDeviceD3D();
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();

    HWND m_hwnd;
    HINSTANCE m_hInstance;
    int m_width, m_height;

    ID3D11Device *m_pd3dDevice;
    ID3D11DeviceContext *m_pd3dDeviceContext;
    IDXGISwapChain *m_pSwapChain;
    ID3D11RenderTargetView *m_mainRenderTargetView;

    static WindowManager *s_instance;
};