# SeraphineDownloader

A simple **Win32 + DirectX11 + ImGui** framework for GUI applications with built-in file downloading support.

---

## 📖 项目简介

本项目封装了以下功能模块：

- **WindowManager**：窗口创建、DirectX11 设备管理、消息循环封装  
- **UIManager**：ImGui 初始化、帧循环和渲染  
- **AppLogic**：应用逻辑（按钮状态、日志、功能处理等）  
- **Downloader**：基于 WinHTTP 的文件下载模块（支持进度条、日志回调）  
- **FileManager**：文件解压与本地处理  

应用启动后，可一键从云端下载 `Seraphine.zip`，并实时显示下载进度与日志。

---

## 🎬 演示效果

👉 ![1](D:\projects\Vs\SeraphineDownloader\docs\1.gif)

---

## ⚙️ 编译环境要求

- **Visual Studio 2019 / 2022**
- **Windows 10 或更高**
- **C++17** 或更高
- **Windows 10 SDK**（包含 DirectX SDK，无需单独安装）

---

## 🛠️ 编译方法

1. 打开项目 `.sln` 文件  
2. 确认项目属性配置：
   - **C++ → 常规 → Character Set** → `Use Unicode Character Set`  
   - **Linker → System → Subsystem** → `Windows (/SUBSYSTEM:WINDOWS)`  
   - 链接库：`d3d11.lib`, `dxgi.lib`, `d3dcompiler.lib`, `winhttp.lib`  
3. 编译 **Debug** 或 **Release**，生成 `.exe` 文件  

---

## 🚀 功能扩展方向

- 支持多文件下载队列  
- 支持断点续传  
- 下载完成后自动解压并安装  
- UI 主题美化与国际化支持  

---

## 📄 License

本项目基于 MIT 协议开源，欢迎自由使用与修改。
