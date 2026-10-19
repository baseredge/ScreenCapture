#pragma once
#include <include/Ling.h>
#include <fstream>
#include <array>
#include <filesystem>

// 图像输出相关的工具函数。data 一律要求 BGRA、top-down、行紧凑（步长 = w*4），
// 这也是 WinPin::getImagePixels 交出来的格式。
class Util
{
public:
	// 同时写入 CF_DIBV5（Office / 微信 / WPS 这类原生程序认）和 "PNG" 注册格式
	//（浏览器 / Electron 程序认），两份都带 alpha；开启截图的磁盘中转后还会附带
	// CF_HDROP，让支持“粘贴文件”的窗口拿到真实图片路径。
	static void saveToClipboard(const int w, const int h, BYTE* data);
	static bool saveToFile(const std::wstring& path, const int w, const int h, BYTE* data);
	// 当前截图另存为格式：只返回 png / jpg / bmp 三种受支持的扩展名。
	static std::wstring getCaptureImageExtension();
	// 默认中转基准根目录：跨电脑通用，优先选择非系统盘中可用空间最大的可用分区（如 D:/ScreenCapture 或 E:/ScreenCapture），单分区电脑安全回退至 AppData。
	static std::filesystem::path getDefaultBaseDirectory();
	// 剪贴板磁盘中转目录。配置为空时使用默认基准根目录下的 clipboard 子目录。
	static std::filesystem::path getClipboardRelayDirectory();
	// 录屏独立中转目录。配置为空时使用默认基准根目录下的 video 子目录，不回退到截图目录。
	static std::filesystem::path getVideoRelayDirectory();
	// 为录屏生成独立中转文件路径（带时间戳 record_YYYYMMDD...），确保不重名且不互相覆盖。
	static std::wstring createVideoRelayPath(const std::wstring& ext);
	// 选择一个文件夹或在资源管理器中打开中转目录。
	static std::wstring chooseFolder(HWND owner, const std::wstring& currentPath = L"");
	static bool openFolder(HWND owner, const std::filesystem::path& path);
	// 弹系统另存为对话框，返回空串表示用户取消
	static std::wstring getSaveFilePath(HWND hwnd, const std::wstring& ext = L"png");
	// 以当前时间生成默认文件名，精确到毫秒，避免连续保存时重名
	static std::wstring createFileName(const std::wstring& ext);
	// GDI 抓屏。返回 BGRA、top-down、行紧凑（步长 = w*4），与本类其他函数的入参格式一致
	static std::vector<BYTE> captureScreen(const int x, const int y, const int w, const int h);
	// 把文件路径以 CF_HDROP 写进剪切板，粘贴到资源管理器/聊天窗口就是一个文件
	static void addFileToClipboard(const std::wstring& filePath);
	// 把图存成缓存文件，再交给外部插件 ImageReader.exe 做文字识别。插件先在本 exe
	// 同目录找，再找 %appdata%\ScreenCapture\plugin，都找不到就用默认浏览器打开它的
	// release 页面让用户自己下。缓存图由插件读完后自己删。
	static bool openWithImageReader(const int w, const int h, BYTE* data);
	// 用 quirc 识别图里的二维码，返回识别到的内容，没识别到返回空串。
	// 图里有多个码时用换行拼在一起
	static std::wstring decodeQrCode(const int w, const int h, BYTE* data);
};
