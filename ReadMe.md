简体中文 | [English](./Doc/ReadMe.en-US.md) | [Русский](./Doc/ReadMe.ru-RU.md) | [Bahasa Indonesia](./Doc/ReadMe.id-ID.md)

![banner](./Doc/banner.png)

# ScreenCapture

一个轻量、快速的 Windows 截图与录屏工具。本仓库是基于 [xland/ScreenCapture](https://github.com/xland/ScreenCapture) 的个人维护分支。

## 项目关系

- 原项目：[xland/ScreenCapture](https://github.com/xland/ScreenCapture)
- 当前维护者：`baseredge`
- 当前仓库：[baseredge/ScreenCapture](https://github.com/baseredge/ScreenCapture)（Private）
- 原项目的作者信息、版权声明和许可证保持不变；本分支只在此基础上增加和维护功能。

## 功能

- 截图、绘图标注、滚动截图（截长图）。
- 屏幕录制：MP4 和 GIF。
- 截图格式：PNG、JPG、BMP；JPG 质量可配置。
- 截图剪贴板文件中转：截图先保存到指定目录，再复制到剪贴板；每张截图独立保留，便于在支持磁盘图片的应用中多选发送。
- MP4 设置：帧率、质量/码率、音频码率、采样率、系统声音、麦克风、鼠标指针和自动停止时间。
- GIF 设置：帧率、质量、快速编码、鼠标指针、循环方式和自动停止时间。
- 文字识别（OCR）和二维码识别插件。
- 取景框、颜色拾取、箭头、矩形、椭圆、文本、马赛克、橡皮擦等标注工具。
- 支持多语言和命令行启动。

## 使用

直接运行：

```text
x64\Release\ScreenCapture.exe
```

设置窗口可分别配置截图、MP4 和 GIF，不同媒体的参数互不污染。

## 配置与便携模式

默认配置目录：`%APPDATA%\ScreenCapture`。

- 配置文件：`%APPDATA%\ScreenCapture\config.json`
- 剪贴板中转目录：可在“设置 → 截图”中选择、打开或恢复默认目录。
- 语言文件：`%APPDATA%\ScreenCapture\Lang`
- OCR 插件：`%APPDATA%\ScreenCapture\plugin\ImageReader.exe`

如果在 `ScreenCapture.exe` 同目录创建 `config.json`，程序会优先使用程序目录中的配置，适合便携使用。`Lang` 和 `ImageReader.exe` 也可以放在程序目录下。

## 命令行

```text
:: 截图完成后退出
ScreenCapture.exe --auto-quit=true

:: 框选完成后直接进入指定功能
ScreenCapture.exe --enter=pin
ScreenCapture.exe --enter=long
ScreenCapture.exe --enter=video
ScreenCapture.exe --enter=ocr
ScreenCapture.exe --enter=qr

:: 仅注册托盘图标
ScreenCapture.exe --enter=tray
```

## 编译

要求：

- Windows 10 1803 或更高版本。
- Visual Studio 2026，安装“使用 C++ 的桌面开发”。
- [Ling](https://github.com/xland/Ling) GUI 框架。
- GIF 编码依赖 [gifski](https://github.com/ImageOptim/gifski)。

当前工程按以下目录关系查找外部源码和库：

```text
D:\aiDo\CPP\
├─ ScreenCapture\
├─ Ling\
└─ gifski\
```

先确保 `gifski\target\release\gifski.lib` 存在，然后使用 Visual Studio 或 MSBuild 编译：

```text
msbuild ScreenCapture.slnx /t:Build /p:Configuration=Release /p:Platform=x64
```

输出文件：`x64\Release\ScreenCapture.exe`。

## 自动构建

每次向 `main` 推送后，GitHub Actions 会在 Windows Runner 上自动构建，并更新私有仓库中的 `latest` Release。Release 中包含 Windows x64 ZIP 包和 SHA-256 校验文件。

## 许可证与第三方组件

- 原项目代码继续使用仓库中的 [MIT License](./LICENSE)，不得删除原版权和许可文本。
- [Ling](https://github.com/xland/Ling) 使用 MIT 许可证。
- GIF 功能链接 [gifski](https://github.com/ImageOptim/gifski)，其许可证为 AGPL-3.0-or-later；该依赖的许可证和源码义务不能被本项目的 MIT 声明覆盖。
- `Src/quirc` 保留其自身的版权和许可声明。

本分支不包含支付、捐赠或赞助二维码，也不设置付费入口；保留原项目归属信息是为了尊重来源和方便追溯。
