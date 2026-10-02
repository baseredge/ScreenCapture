# 2026-10-02 录屏交互验测

此记录对应最初的文字按钮版本。最终版本已恢复图标按钮、补充 GIF 图标、统一工具条配色，并区分原作者和当前维护者；最终版本编译与重启成功，用户已确认图标和配色效果。后续按用户要求由用户验证，不将下方历史测试结果视作最终版本的完整验测。

构建：`./build-local.ps1`，VS 2022/v143 + 本地 ATL，Ling/Yoga 同工具链重编，构建退出码 0。

程序：`x64/LocalRelease/ScreenCapture.exe`。旧版进程已退出，新版以 `--enter=tray` 重启。

测试：`.build-deps/qa_recording.py` 使用自建的移动方块画面，通过 Windows 窗口消息操作实际界面；框选后单击对应录屏按钮直接进入录制，无格式选择和倒计时。MP4 用 OpenCV 解码，GIF 用 Pillow 验证多帧及画面变化。保存到文件实际操作系统另存为对话框，丢弃检查临时文件删除。

| 格式 | 操作 | 结果 | 证据 |
|---|---|---|---|
| MP4 | copy | 通过 | 14 帧，8864 字节，696×348，可解码且画面变化 |
| GIF | copy | 通过 | 15 帧，7650 字节，700×350，可解码且画面变化 |
| MP4 | discard | 通过 | 正常退出；丢弃动作确认临时文件已删除 |
| GIF | discard | 通过 | 正常退出；丢弃动作确认临时文件已删除 |
| MP4 | escape | 通过 | 正常退出 |
| GIF | escape | 通过 | 正常退出 |
| MP4 | save | 通过 | 14 帧，7698 字节，696×348，可解码且画面变化 |
| GIF | save | 通过 | 15 帧，7648 字节，700×350，可解码且画面变化 |

产物与原始结果：`.build-deps/qa/results.json`、`recording.mp4`、`recording.gif`、`saved.mp4`、`saved.gif`。选区工具条截图为 `.build-deps/qa/mp4-selection-toolbar.png`，已检查录视频/GIF文字及按功能配色。录制工具条有捕获排除属性，不把空白的外部截图用作界面证据。

700×350 的选区输出 GIF 为 700×350；MP4 为 696×348，沿用录制库在显示器局部坐标中向内对齐的既有行为。

另外验证启动与 Esc 退出，退出码 0。未覆盖声音内容、多屏/混合 DPI、长时间录制。

回退保存点：`before-quick-record-20261002`。原 `x64/Release/ScreenCapture.exe` 保留。Ling 的 v143 兼容修改原文件备份为 `.build-deps/Ling-Util.h.before-v143`。
