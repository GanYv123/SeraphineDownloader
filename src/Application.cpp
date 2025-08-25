#include "Application.h"

int Application::Run(HINSTANCE hInstance)
{
    // 初始化窗口
    if(!windowManager_.Initialize(hInstance, 600, 400)){
        return 1;
    }

    // 初始化 UI
    if(!uiManager_.Initialize(windowManager_.GetHwnd(),
        windowManager_.GetDevice(),
        windowManager_.GetDeviceContext())){
        windowManager_.Cleanup();
        return 1;
    }

    // 消息循环
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    bool isDone = false;

    while(!isDone){
        if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if(msg.message == WM_QUIT)
                isDone = true;
        } else{
            uiManager_.BeginFrame();
            if(uiManager_.RenderUI(appLogic_, windowManager_.GetHwnd())){
                isDone = true;
            }
            uiManager_.EndFrame();
            windowManager_.Present();
            Sleep(1);
        }
    }

    uiManager_.Cleanup();
    windowManager_.Cleanup();
    return 0;
}