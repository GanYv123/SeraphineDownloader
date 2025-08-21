#include "WindowManager.h"   // �����ں� DirectX �豸�ķ�װ��
#include "UIManager.h"       // ���� ImGui ��ʼ��/��Ⱦ�ķ�װ��
#include "AppLogic.h"        // Ӧ���߼��ࣨ���Լ���ҵ���߼���������־����ť״̬�ȣ�

// Win32 Ӧ�����
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    // �����ڣ����ڴ�����DX�豸��SwapChain��
    WindowManager windowManager;

    // ���� ImGui UI����ʼ����֡ѭ������Ⱦ��
    UIManager uiManager;

    // ���ҵ���߼��࣬�Ű�ť״̬����־��������������
    AppLogic appLogic;

    // ��ʼ�����ڣ��� 600���� 400��
    if(!windowManager.Initialize(hInstance, 600, 400)){
        return 1;   // ��ʼ��ʧ��ֱ���˳�
    }

    // ��ʼ�� UI����Ҫ���ھ�� + DirectX �豸��
    if(!uiManager.Initialize(windowManager.GetHwnd(),
        windowManager.GetDevice(),
        windowManager.GetDeviceContext())){
        windowManager.Cleanup();
        return 1;   // ��ʼ��ʧ��
    }

    // Windows ��Ϣѭ��
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    bool done = false;

    while(!done){
        // ���� Windows ��Ϣ�����̡���ꡢ�˳��ȣ�
        while(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if(msg.message == WM_QUIT)
                done = true;   // �յ��˳���Ϣ
        }
        if(done) break;

        // ��ʼ�µ�һ֡��ImGui �� BeginFrame��
        uiManager.BeginFrame();

        // ���� UI�������߼������ȥ
        // ��Ҫ����°�ť������ UIManager::RenderUI(appLogic, hwnd) ����д
        if(uiManager.RenderUI(appLogic, windowManager.GetHwnd())){
            done = true;   // ��� UI �߼����� true��������ˡ��˳�����ť�������˳�
        }

        // ���� UI һ֡��ImGui::Render + DX ���ƣ�
        uiManager.EndFrame();

        // ��ʾ����Ļ������ǰ�󻺳�����
        windowManager.Present();
    }

    // ������Դ
    uiManager.Cleanup();
    windowManager.Cleanup();
    return 0;
}
