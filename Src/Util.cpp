#include "pch.h"
#include <wincodec.h>
#include <shobjidl.h>
#include <format>
#include <fstream>
#include "Util.h"
#include "Lang.h"
#include "Setting.h"
#include "quirc/quirc.h"

using Microsoft::WRL::ComPtr;

namespace {
	// 把 BGRA top-down 像素编码成 PNG 写进 stream。saveToClipboard 和 saveToFile 共用这段。
	bool encodePng(IStream* stream, const int w, const int h, BYTE* data)
	{
		UINT rowBytes = (UINT)w * 4;
		UINT imgBytes = rowBytes * (UINT)h;
		ComPtr<IWICImagingFactory> factory;
		auto hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.GetAddressOf()));
		if (FAILED(hr)) return false;
		ComPtr<IWICBitmapEncoder> encoder;
		hr = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.GetAddressOf());
		if (FAILED(hr)) return false;
		hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
		if (FAILED(hr)) return false;
		ComPtr<IWICBitmapFrameEncode> frame;
		hr = encoder->CreateNewFrame(frame.GetAddressOf(), nullptr);
		if (FAILED(hr)) return false;
		hr = frame->Initialize(nullptr);
		if (FAILED(hr)) return false;
		hr = frame->SetSize((UINT)w, (UINT)h);
		if (FAILED(hr)) return false;
		WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppBGRA;
		hr = frame->SetPixelFormat(&fmt);
		if (FAILED(hr) || !IsEqualGUID(fmt, GUID_WICPixelFormat32bppBGRA)) return false;
		hr = frame->WritePixels((UINT)h, rowBytes, imgBytes, data);
		if (FAILED(hr)) return false;
		hr = frame->Commit();
		if (FAILED(hr)) return false;
		return SUCCEEDED(encoder->Commit());
	}

	// JPEG/BMP 不接受带 alpha 的 32 位格式作为稳定的公共输出格式，统一转换成
	// 紧凑的 BGR24。截图入参仍保持 BGRA，剪切板也继续走上面的 PNG + DIB 多格式方案。
	bool encodeBgrImage(IStream* stream, const GUID& container, const int w, const int h,
		BYTE* data, int jpegQuality)
	{
		// 24bpp 的每行按 BMP/WIC 的习惯补到 4 字节边界，避免宽度不是 4 的倍数时
		// 某些编码器拒绝不对齐的 stride。
		const UINT rowBytes = ((UINT)w * 3 + 3u) & ~3u;
		const UINT imgBytes = rowBytes * (UINT)h;
		std::vector<BYTE> bgr((size_t)imgBytes);
		for (int row = 0; row < h; ++row) {
			const BYTE* src = data + (size_t)row * (size_t)w * 4;
			BYTE* dst = bgr.data() + (size_t)row * rowBytes;
			for (int col = 0; col < w; ++col) {
				dst[0] = src[0];
				dst[1] = src[1];
				dst[2] = src[2];
				src += 4;
				dst += 3;
			}
		}

		ComPtr<IWICImagingFactory> factory;
		auto hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(factory.GetAddressOf()));
		if (FAILED(hr)) return false;
		ComPtr<IWICBitmapEncoder> encoder;
		hr = factory->CreateEncoder(container, nullptr, encoder.GetAddressOf());
		if (FAILED(hr)) return false;
		hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
		if (FAILED(hr)) return false;
		ComPtr<IWICBitmapFrameEncode> frame;
		ComPtr<IPropertyBag2> options;
		hr = encoder->CreateNewFrame(frame.GetAddressOf(), options.GetAddressOf());
		if (FAILED(hr)) return false;

		if (IsEqualGUID(container, GUID_ContainerFormatJpeg) && options) {
			PROPBAG2 option{};
			option.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
			VARIANT value{};
			VariantInit(&value);
			value.vt = VT_R4;
			value.fltVal = std::clamp(jpegQuality, 1, 100) / 100.0f;
			options->Write(1, &option, &value);
			VariantClear(&value);
		}

		hr = frame->Initialize(options.Get());
		if (FAILED(hr)) return false;
		hr = frame->SetSize((UINT)w, (UINT)h);
		if (FAILED(hr)) return false;
		WICPixelFormatGUID fmt = GUID_WICPixelFormat24bppBGR;
		hr = frame->SetPixelFormat(&fmt);
		if (FAILED(hr) || !IsEqualGUID(fmt, GUID_WICPixelFormat24bppBGR)) return false;
		hr = frame->WritePixels((UINT)h, rowBytes, imgBytes, bgr.data());
		if (FAILED(hr)) return false;
		hr = frame->Commit();
		if (FAILED(hr)) return false;
		return SUCCEEDED(encoder->Commit());
	}

	std::wstring imageExtension(const std::wstring& path)
	{
		auto ext = std::filesystem::path(path).extension().wstring();
		std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t ch) { return (wchar_t)towlower(ch); });
		if (ext == L".jpeg") ext = L".jpg";
		return ext;
	}

	HGLOBAL makeFileDropData(const std::wstring& filePath)
	{
		if (filePath.empty() || filePath.size() > (SIZE_MAX / sizeof(wchar_t)) - 2) return nullptr;
		const SIZE_T pathBytes = (filePath.size() + 2) * sizeof(wchar_t);
		if (pathBytes > SIZE_MAX - sizeof(DROPFILES)) return nullptr;
		const SIZE_T totalSize = sizeof(DROPFILES) + pathBytes;
		auto hGlobal = GlobalAlloc(GMEM_MOVEABLE, totalSize);
		if (!hGlobal) return nullptr;
		auto pDropFiles = static_cast<DROPFILES*>(GlobalLock(hGlobal));
		if (!pDropFiles) {
			GlobalFree(hGlobal);
			return nullptr;
		}
		pDropFiles->pFiles = sizeof(DROPFILES);
		pDropFiles->fWide = TRUE;
		auto dest = reinterpret_cast<wchar_t*>(pDropFiles + 1);
		CopyMemory(dest, filePath.c_str(), filePath.size() * sizeof(wchar_t));
		dest[filePath.size()] = L'\0';
		dest[filePath.size() + 1] = L'\0';
		GlobalUnlock(hGlobal);
		return hGlobal;
	}

	std::wstring saveClipboardRelayImage(const int w, const int h, BYTE* data)
	{
		// 中转文件固定使用 PNG：它同时适合 Codex、浏览器和聊天窗口；“另存为”仍然
		// 遵循截图页里的 PNG/JPG/BMP 设置。中转文件不自动删除，便于一次选取多张发送。
		auto directory = Util::getClipboardRelayDirectory();
		std::error_code ec;
		std::filesystem::create_directories(directory, ec);
		if (ec) return L"";
		const auto basePath = directory / (std::wstring{ L"clipboard_" } + Util::createFileName(L"png"));
		auto path = basePath;
		for (unsigned int index = 1; std::filesystem::exists(path, ec) && !ec; ++index) {
			path = directory / (basePath.stem().wstring() + L"_" + std::to_wstring(index) + basePath.extension().wstring());
		}
		if (ec) return L"";
		if (!Util::saveToFile(path.wstring(), w, h, data)) {
			std::filesystem::remove(path, ec);
			return L"";
		}
		return path.wstring();
	}

	// quirc 交出来的是裸字节流：BYTE 类型的二维码现实中基本都是 UTF-8（微信、支付宝
	// 生成的都是），Kanji 类型按 ISO 18004 规定是 Shift-JIS。所以先按 UTF-8 严格解，
	// 解不通再退回对应的本地代码页，避免把中文变成一堆问号
	std::wstring qrPayloadToWStr(const uint8_t* payload, const int len, const int dataType)
	{
		if (len <= 0) return L"";
		auto convert = [payload, len](UINT codePage, DWORD flags) {
			auto str = (const char*)payload;
			auto count = MultiByteToWideChar(codePage, flags, str, len, nullptr, 0);
			if (count <= 0) return std::wstring();
			std::wstring result(count, 0);
			MultiByteToWideChar(codePage, flags, str, len, result.data(), count);
			return result;
		};
		auto result = convert(CP_UTF8, MB_ERR_INVALID_CHARS);
		if (!result.empty()) return result;
		return convert(dataType == QUIRC_DATA_TYPE_KANJI ? 932 : CP_ACP, 0);
	}

	// 插件的查找顺序：先本 exe 同目录（绿色包一起解压的情况），
	// 再 %appdata%\ScreenCapture\plugin（后来单独下载的情况）
	std::filesystem::path findImageReader()
	{
		wchar_t buffer[MAX_PATH]{};
		GetModuleFileName(nullptr, buffer, MAX_PATH);
		auto path = std::filesystem::path{ buffer }.parent_path().append(L"ImageReader.exe");
		if (std::filesystem::exists(path)) return path;
		path = Setting::get()->getDataPath().append(L"plugin").append(L"ImageReader.exe");
		if (std::filesystem::exists(path)) return path;
		return {};
	}
}

void Util::saveToClipboard(const int w, const int h, BYTE* data)
{
	if (w <= 0 || h <= 0 || !data) return;
	std::wstring relayPath;
	if (auto setting = Setting::get(); setting &&
		setting->getMediaFlag(L"capture", L"clipboardFileRelay", false)) {
		relayPath = saveClipboardRelayImage(w, h, data);
	}
	DWORD rowBytes = (DWORD)w * 4;
	DWORD imgBytes = rowBytes * (DWORD)h;

	// ---------- 1) PNG 编码到内存流 ----------
	ComPtr<IStream> pngStream;
	if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, pngStream.GetAddressOf()))) return;
	if (!encodePng(pngStream.Get(), w, h, data)) return;
	// 流内部的 HGLOBAL 尺寸可能大于实际字节数，拷一份精确大小的出来给剪切板
	STATSTG stat{};
	if (FAILED(pngStream->Stat(&stat, STATFLAG_NONAME))) return;
	SIZE_T pngSize = (SIZE_T)stat.cbSize.QuadPart;
	if (pngSize == 0) return;
	HGLOBAL hPngSrc{ nullptr };
	if (FAILED(GetHGlobalFromStream(pngStream.Get(), &hPngSrc)) || !hPngSrc) return;
	auto srcPtr = GlobalLock(hPngSrc);
	if (!srcPtr) return;
	HGLOBAL hPng = GlobalAlloc(GMEM_MOVEABLE, pngSize);
	if (!hPng) { GlobalUnlock(hPngSrc); return; }
	auto dstPtr = GlobalLock(hPng);
	if (!dstPtr) { GlobalUnlock(hPngSrc); GlobalFree(hPng); return; }
	CopyMemory(dstPtr, srcPtr, pngSize);
	GlobalUnlock(hPng);
	GlobalUnlock(hPngSrc);

	// ---------- 2) 构造 CF_DIBV5（带 alpha） ----------
	HGLOBAL hDibV5 = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPV5HEADER) + imgBytes);
	if (!hDibV5) { GlobalFree(hPng); return; }
	auto pv5 = static_cast<BYTE*>(GlobalLock(hDibV5));
	if (!pv5) { GlobalFree(hDibV5); GlobalFree(hPng); return; }
	auto bv5 = reinterpret_cast<BITMAPV5HEADER*>(pv5);
	*bv5 = {};
	bv5->bV5Size = sizeof(BITMAPV5HEADER);
	bv5->bV5Width = w;
	bv5->bV5Height = -h;                  // 负 = top-down
	bv5->bV5Planes = 1;
	bv5->bV5BitCount = 32;
	bv5->bV5Compression = BI_BITFIELDS;   // 让接收端识别 alpha
	bv5->bV5SizeImage = imgBytes;
	bv5->bV5RedMask = 0x00FF0000;
	bv5->bV5GreenMask = 0x0000FF00;
	bv5->bV5BlueMask = 0x000000FF;
	bv5->bV5AlphaMask = 0xFF000000;
	bv5->bV5CSType = LCS_sRGB;
	bv5->bV5Intent = LCS_GM_GRAPHICS;
	CopyMemory(pv5 + sizeof(BITMAPV5HEADER), data, imgBytes);
	GlobalUnlock(hDibV5);

	// ---------- 3) 构造 CF_DIB（24bpp、BI_RGB、自下而上） ----------
	// 老软件（比如 Illustrator 2020）只认最传统的这一种 DIB：注册格式 PNG 它不查，
	// CF_DIBV5 它不认，32bpp + BI_BITFIELDS 和 top-down 也读不了。系统虽然能从 CF_DIBV5
	// 合成出 CF_DIB，合成出来的仍是那份带 alpha 的 32 位数据，一样不合它的口味。
	// 所以显式再放一份最保守的：丢掉 alpha 写成 24 位，行按 4 字节对齐，自下而上排列
	DWORD dibRowBytes = ((DWORD)w * 3 + 3) & ~3u;
	DWORD dibImgBytes = dibRowBytes * (DWORD)h;
	HGLOBAL hDib = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + dibImgBytes);
	if (!hDib) { GlobalFree(hDibV5); GlobalFree(hPng); return; }
	auto pDib = static_cast<BYTE*>(GlobalLock(hDib));
	if (!pDib) { GlobalFree(hDib); GlobalFree(hDibV5); GlobalFree(hPng); return; }
	auto bi = reinterpret_cast<BITMAPINFOHEADER*>(pDib);
	*bi = {};
	bi->biSize = sizeof(BITMAPINFOHEADER);
	bi->biWidth = w;
	bi->biHeight = h;                     // 正 = 自下而上
	bi->biPlanes = 1;
	bi->biBitCount = 24;
	bi->biCompression = BI_RGB;
	bi->biSizeImage = dibImgBytes;
	auto dibPixels = pDib + sizeof(BITMAPINFOHEADER);
	for (int row = 0; row < h; row++) {
		auto src = data + (size_t)row * rowBytes;                 //入参是 top-down
		auto dst = dibPixels + (size_t)(h - 1 - row) * dibRowBytes;
		for (int col = 0; col < w; col++) {
			dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];     //BGRA -> BGR
			src += 4;
			dst += 3;
		}
	}
	GlobalUnlock(hDib);

	// ---------- 4) 写入剪切板 ----------
	if (!OpenClipboard(nullptr)) {
		GlobalFree(hDib);
		GlobalFree(hDibV5);
		GlobalFree(hPng);
		return;
	}
	EmptyClipboard();
	// SetClipboardData 成功后 HGLOBAL 归剪切板所有，不能再 GlobalFree；失败了才要自己释放
	if (!relayPath.empty()) {
		auto hDrop = makeFileDropData(relayPath);
		if (!hDrop || !SetClipboardData(CF_HDROP, hDrop)) {
			if (hDrop) GlobalFree(hDrop);
		}
	}
	if (!SetClipboardData(CF_DIBV5, hDibV5)) {
		GlobalFree(hDibV5);
	}
	if (!SetClipboardData(CF_DIB, hDib)) {
		GlobalFree(hDib);
	}
	UINT cfPng = RegisterClipboardFormatW(L"PNG");
	if (cfPng == 0 || !SetClipboardData(cfPng, hPng)) {
		GlobalFree(hPng);
	}
	CloseClipboard();
}

bool Util::saveToFile(const std::wstring& path, const int w, const int h, BYTE* data)
{
	if (path.empty() || w <= 0 || h <= 0 || !data) return false;
	ComPtr<IWICImagingFactory> factory;
	auto hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.GetAddressOf()));
	if (FAILED(hr)) return false;
	ComPtr<IWICStream> stream;
	hr = factory->CreateStream(stream.GetAddressOf());
	if (FAILED(hr)) return false;
	hr = stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE);
	if (FAILED(hr)) return false;
	const auto ext = imageExtension(path);
	if (ext == L".jpg") {
		int quality = 95;
		if (auto setting = Setting::get()) {
			quality = (int)std::lround(setting->getMediaNum(L"capture", L"jpegQuality", 95.f));
		}
		return encodeBgrImage(stream.Get(), GUID_ContainerFormatJpeg, w, h, data, quality);
	}
	if (ext == L".bmp") {
		return encodeBgrImage(stream.Get(), GUID_ContainerFormatBmp, w, h, data, 100);
	}
	// 未知扩展名按 PNG 处理，兼容 OCR 插件及旧调用方直接传入无扩展名路径的行为。
	return encodePng(stream.Get(), w, h, data);
}

std::wstring Util::getCaptureImageExtension()
{
	std::wstring format = L"png";
	if (auto setting = Setting::get()) format = setting->getMediaText(L"capture", L"imageFormat", L"png");
	std::transform(format.begin(), format.end(), format.begin(), [](wchar_t ch) { return (wchar_t)towlower(ch); });
	if (format == L"jpeg") format = L"jpg";
	if (format != L"jpg" && format != L"bmp") format = L"png";
	return format;
}

std::filesystem::path Util::getClipboardRelayDirectory()
{
	auto setting = Setting::get();
	if (!setting) return {};
	const auto configured = setting->getMediaText(L"capture", L"clipboardDirectory", L"");
	if (configured.empty()) {
		return setting->getDataPath().append(L"clipboard");
	}
	std::filesystem::path path{ configured };
	if (path.is_relative()) path = Setting::get()->getDataPath() / path;
	return path;
}

std::wstring Util::chooseFolder(HWND owner, const std::wstring& currentPath)
{
	std::wstring result;
	ComPtr<IFileOpenDialog> dialog;
	auto hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(dialog.GetAddressOf()));
	if (FAILED(hr)) return result;
	DWORD options{ 0 };
	if (FAILED(dialog->GetOptions(&options))) return result;
	if (FAILED(dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM))) return result;
	if (!currentPath.empty()) {
		ComPtr<IShellItem> currentItem;
		if (SUCCEEDED(SHCreateItemFromParsingName(currentPath.c_str(), nullptr,
			IID_PPV_ARGS(currentItem.GetAddressOf()))) && currentItem) {
			dialog->SetFolder(currentItem.Get());
		}
	}
	if (FAILED(dialog->Show(owner))) return result;
	ComPtr<IShellItem> item;
	if (FAILED(dialog->GetResult(item.GetAddressOf())) || !item) return result;
	PWSTR filePath{ nullptr };
	if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &filePath)) || !filePath) return result;
	result = filePath;
	CoTaskMemFree(filePath);
	return result;
}

bool Util::openFolder(HWND owner, const std::filesystem::path& path)
{
	std::error_code ec;
	std::filesystem::create_directories(path, ec);
	if (ec) return false;
	return reinterpret_cast<INT_PTR>(ShellExecuteW(owner, L"open", path.wstring().c_str(),
		nullptr, nullptr, SW_SHOWNORMAL)) > 32;
}

std::wstring Util::getSaveFilePath(HWND hwnd, const std::wstring& ext)
{
	std::wstring result;
	ComPtr<IFileSaveDialog> saveDialog;
	auto hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(saveDialog.GetAddressOf()));
	if (FAILED(hr)) return result;
	DWORD dwFlags{ 0 };
	saveDialog->GetOptions(&dwFlags);
	saveDialog->SetOptions(dwFlags | FOS_OVERWRITEPROMPT | FOS_STRICTFILETYPES);
	auto pattern = L"*." + ext;
	auto typeName = Lang::get(L"util.file");
	COMDLG_FILTERSPEC filterSpec[]{ { typeName.c_str(), pattern.c_str() } };
	saveDialog->SetFileTypes(_countof(filterSpec), filterSpec);
	saveDialog->SetFileTypeIndex(1);
	saveDialog->SetDefaultExtension(ext.c_str());
	auto fileName = createFileName(ext);
	saveDialog->SetFileName(fileName.c_str());
	// 用户取消时 Show 返回 HRESULT_FROM_WIN32(ERROR_CANCELLED)，一样走 FAILED 分支
	hr = saveDialog->Show(hwnd);
	if (FAILED(hr)) return result;
	ComPtr<IShellItem> item;
	hr = saveDialog->GetResult(item.GetAddressOf());
	if (FAILED(hr)) return result;
	PWSTR filePath{ nullptr };
	hr = item->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
	if (FAILED(hr)) return result;
	result = filePath;
	CoTaskMemFree(filePath);
	return result;
}

std::wstring Util::createFileName(const std::wstring& ext)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	return std::format(L"{:04d}{:02d}{:02d}{:02d}{:02d}{:02d}{:03d}.{}",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, ext);
}

std::vector<BYTE> Util::captureScreen(const int x, const int y, const int w, const int h)
{
	std::vector<BYTE> data;
	if (w <= 0 || h <= 0) return data;
	HDC hScreen = GetDC(nullptr);
	HDC hDC = CreateCompatibleDC(hScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
	auto oldObj = SelectObject(hDC, hBitmap);
	BitBlt(hDC, 0, 0, w, h, hScreen, x, y, SRCCOPY);
	ReleaseDC(nullptr, hScreen);
	data.resize((size_t)w * 4 * h);
	BITMAPINFO bmi{};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = w;
	// 负高度 = top-down，第一行就是屏幕最上面那行，省掉后续所有翻转
	bmi.bmiHeader.biHeight = -h;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	GetDIBits(hDC, hBitmap, 0, h, data.data(), &bmi, DIB_RGB_COLORS);
	SelectObject(hDC, oldObj);
	DeleteDC(hDC);
	DeleteObject(hBitmap);
	return data;
}

void Util::addFileToClipboard(const std::wstring& filePath)
{
	if (!OpenClipboard(nullptr)) return;
	EmptyClipboard();
	// DROPFILES 之后紧跟双 \0 结尾的路径列表，这里只放一条
	auto hGlobal = makeFileDropData(filePath);
	if (!hGlobal) {
		CloseClipboard();
		return;
	}
	auto pDropFiles = static_cast<DROPFILES*>(GlobalLock(hGlobal));
	if (!pDropFiles) {
		GlobalFree(hGlobal);
		CloseClipboard();
		return;
	}
	pDropFiles->pFiles = sizeof(DROPFILES);
	pDropFiles->fWide = TRUE;
	auto dest = reinterpret_cast<wchar_t*>(pDropFiles + 1);
	wcscpy_s(dest, filePath.length() + 1, filePath.c_str());
	dest[filePath.length() + 1] = L'\0';
	GlobalUnlock(hGlobal);
	// 成功后 HGLOBAL 归剪切板所有，只在失败时自己释放
	if (!SetClipboardData(CF_HDROP, hGlobal)) {
		GlobalFree(hGlobal);
	}
	CloseClipboard();
}

bool Util::openWithImageReader(const int w, const int h, BYTE* data)
{
	auto exePath = findImageReader();
	if (exePath.empty()) {
		// 插件没装，直接把用户带到下载页，不再多弹一层提示
		ShellExecute(nullptr, L"open", L"https://github.com/xland/ImageReader/releases", nullptr, nullptr, SW_SHOWNORMAL);
		return false;
	}
	auto imgPath = Setting::get()->getDataPath().append(L"ocr_" + createFileName(L"png")).wstring();
	if (!saveToFile(imgPath, w, h, data)) return false;
	// --del-image=true：插件读完自己把缓存图删掉，免得在数据目录里越攒越多
	auto cmd = std::format(L"\"{}\" --image-path=\"{}\" --del-image=true", exePath.wstring(), imgPath);
	// 工作目录设成插件所在目录，它才找得到自己身边的依赖
	auto workDir = exePath.parent_path().wstring();
	STARTUPINFO si{ .cb = sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi{};
	if (!CreateProcess(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, workDir.data(), &si, &pi)) {
		std::error_code ec;
		std::filesystem::remove(imgPath, ec); //插件没起来，别留下垃圾文件
		return false;
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return true;
}

std::wstring Util::decodeQrCode(const int w, const int h, BYTE* data)
{
	std::wstring result;
	if (w <= 0 || h <= 0 || !data) return result;
	// quirc_new 和 quirc_resize 是这个库里唯一会申请内存的两个函数，选区大的时候
	// 那块灰度缓冲不小，所以下面每条返回路径都得走到 quirc_destroy
	auto qr = quirc_new();
	if (!qr) return result;
	if (quirc_resize(qr, w, h) < 0) {
		quirc_destroy(qr);
		return result;
	}
	// quirc_begin 给的就是它内部那块缓冲，一个像素一字节，直接把灰度写进去
	int bufW{ 0 }, bufH{ 0 };
	auto buffer = quirc_begin(qr, &bufW, &bufH);
	const size_t count = (size_t)w * h;
	for (size_t i = 0; i < count; i++) {
		auto px = data + i * 4; //入参是 BGRA
		buffer[i] = (uint8_t)((px[2] * 77 + px[1] * 150 + px[0] * 29) >> 8);
	}
	quirc_end(qr);
	auto codeCount = quirc_count(qr);
	for (int i = 0; i < codeCount; i++) {
		quirc_code code{};
		quirc_data qrData{};
		quirc_extract(qr, i, &code);
		auto err = quirc_decode(&code, &qrData);
		if (err == QUIRC_ERROR_DATA_ECC) {
			// 可能是镜像的码（ISO 18004:2015 允许），翻过来再试一次
			quirc_flip(&code);
			err = quirc_decode(&code, &qrData);
		}
		if (err != QUIRC_SUCCESS) continue;
		auto text = qrPayloadToWStr(qrData.payload, qrData.payload_len, qrData.data_type);
		if (text.empty()) continue;
		if (!result.empty()) result += L"\n";
		result += text;
	}
	quirc_destroy(qr);
	return result;
}

