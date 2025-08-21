#include "WindowManager.h"   // 管理窗口和 DirectX 设备的封装类
#include "UIManager.h"       // 管理 ImGui 初始化/渲染的封装类
#include "AppLogic.h"        // 应用逻辑类（你自己的业务逻辑，比如日志、按钮状态等）

// Win32 应用入口
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    // 管理窗口（窗口创建、DX设备、SwapChain）
    WindowManager windowManager;

    // 管理 ImGui UI（初始化、帧循环、渲染）
    UIManager uiManager;

    // 你的业务逻辑类，放按钮状态、日志函数、其他功能
    AppLogic appLogic;

    // 初始化窗口（宽 600，高 400）
    if(!windowManager.Initialize(hInstance, 600, 400)){
        return 1;   // 初始化失败直接退出
    }

    // 初始化 UI（需要窗口句柄 + DirectX 设备）
    if(!uiManager.Initialize(windowManager.GetHwnd(),
        windowManager.GetDevice(),
        windowManager.GetDeviceContext())){
        windowManager.Cleanup();
        return 1;   // 初始化失败
    }

    // Windows 消息循环
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    bool done = false;

    while(!done){
        // 处理 Windows 消息（键盘、鼠标、退出等）
        while(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if(msg.message == WM_QUIT)
                done = true;   // 收到退出消息
        }
        if(done) break;

        // 开始新的一帧（ImGui 的 BeginFrame）
        uiManager.BeginFrame();

        // 绘制 UI，传递逻辑对象进去
        // 你要添加新按钮，就在 UIManager::RenderUI(appLogic, hwnd) 里面写
        if(uiManager.RenderUI(appLogic, windowManager.GetHwnd())){
            done = true;   // 如果 UI 逻辑返回 true（比如点了“退出”按钮），就退出
        }

        // 结束 UI 一帧（ImGui::Render + DX 绘制）
        uiManager.EndFrame();

        // 显示到屏幕（交换前后缓冲区）
        windowManager.Present();
    }

    // 清理资源
    uiManager.Cleanup();
    windowManager.Cleanup();
    return 0;
}
