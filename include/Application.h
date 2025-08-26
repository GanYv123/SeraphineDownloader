#pragma once
#include "AppLogic.h"
#include "UIManager.h"
#include "WindowManager.h"
#include <memory>

class Application
{
public:
    Application();
    ~Application() = default;

    int Run(HINSTANCE hInstance);

private:
    // 成员对象按依赖顺序声明
    AppLogic appLogic_;
    UIManager uiManager_;
    std::unique_ptr<WindowManager> windowManager_;
};
