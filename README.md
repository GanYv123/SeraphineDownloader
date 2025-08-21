# ImGuiTemplate

A simple Win32 + DirectX11 + ImGui framework for GUI applications.

## 项目简介

本项目封装了窗口管理、DirectX 11 设备初始化以及 ImGui UI 渲染，方便快速开发 Windows 桌面应用程序。  

主要模块：
- `WindowManager`：窗口创建、DX11 设备管理、消息循环封装  
- `UIManager`：ImGui 初始化、帧循环和渲染  
- `AppLogic`：应用逻辑（按钮状态、日志、功能处理等）

支持：
- 自定义窗口大小和位置
- 任务栏图标和窗口标题栏图标设置
- ImGui 按钮和自定义控件渲染
- 版本信息和发布信息自定义

---

## 编译要求

- Visual Studio 2019/2022
- Windows 10 或更高
- DirectX SDK（Windows 10 SDK 通常足够）
- C++17 或更高

---

## 编译方法

1. 打开项目 `.sln` 文件  
2. 确认项目属性：
   - C++ → 常规 → `Character Set` 设置为 `Use Unicode Character Set`  
   - 链接器 → 系统 → `Subsystem` 设置为 `Windows (/SUBSYSTEM:WINDOWS)`  
3. 编译 Debug 或 Release，生成 `.exe`
