#pragma once
#include "AppLogic.h"
#include "WindowManager.h"
#include "UIManager.h"
#include <memory>

class Application
{
public:
    Application()
        : uiManager_(appLogic_) {
    }   // uiManager_ ��������� appLogic_ ��
    ~Application() = default;

    int Run(HINSTANCE hInstance);

private:
    WindowManager windowManager_;
    AppLogic appLogic_;
    UIManager uiManager_;
};
