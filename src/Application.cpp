#include "Application.h"
#include <thread> // Sleep
#include <windows.h>

Application::Application()
    : appLogic_(), uiManager_(appLogic_), windowManager_(std::make_unique<WindowManager>())
{
    // 构造函数中按顺序初始化成员
}

int Application::Run(HINSTANCE hInstance)
{
    // 初始化窗口
    if (!windowManager_->Initialize(hInstance, 600, 400))
    {
        return 1;
    }

    // 初始化 UI
    if (!uiManager_.Initialize(windowManager_->GetHwnd(),
                               windowManager_->GetDevice(),
                               windowManager_->GetDeviceContext()))
    {
        return 1;
    }

    // 消息循环
    MSG msg{};
    bool isDone = false;

    while (!isDone)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                isDone = true;
        }
        else
        {
            uiManager_.BeginFrame();
            if (uiManager_.RenderUI(appLogic_, windowManager_->GetHwnd()))
                isDone = true;
            uiManager_.EndFrame();
            windowManager_->Present();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // 不用显式 Cleanup，成员对象析构自动释放
    return 0;
}
