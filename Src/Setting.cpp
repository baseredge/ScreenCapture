#include "pch.h"
#include <include/Ling.h>
#include "Setting.h"
#include "Util.h"
#include "Lang.h"
#include "Win/WinCap.h"
#include "App.h"

namespace {
    std::unique_ptr<Setting> setting;
    constexpr int capShortcutMsgId{ 100 };
    // 配置文件的默认内容。媒体配置按实际输出格式分开，避免 MP4 的码率等参数
    // 意外套到 GIF 上。早期的 video 节点仍由 getMedia* 兼容读取。
    constexpr std::wstring_view defaultConfig{ LR"""({"common":{"autoStart":false,"language":"zh-CN"},"shortcutKey":{"cap":"Ctrl+Alt+A"},"capture":{"imageFormat":"png","jpegQuality":95,"clipboardFileRelay":false,"clipboardDirectory":""},"mp4":{"fps":30,"quality":50,"bitrateKbps":0,"audioBitrateKbps":192,"sampleRate":44100,"systemAudio":true,"microphone":false,"cursor":true,"maxMinutes":120},"gif":{"fps":15,"quality":80,"fast":true,"cursor":true,"repeat":0,"maxMinutes":6}})""" };
}


Setting::Setting() :dataPath{ initDataPath() }, configPath{ initConfigPath() }
{
    if (std::filesystem::exists(configPath)) {
        auto content = Ling::Util::readFileText(configPath);
        if (content.empty() || content.find_first_not_of(L" \t\r\n") == std::wstring::npos) {
            configObj = JsonObject::Parse(defaultConfig);
            save();
            return;
        }
        JsonObject obj{ nullptr };
        if (JsonObject::TryParse(content, obj)) {
            configObj = obj;
            return;
        }
        MessageBox(nullptr, L"config.json parse error，use default config", L"ScreenCapture", MB_OK | MB_ICONWARNING);
    }
    configObj = JsonObject::Parse(defaultConfig); 
}



Setting::~Setting()
{

}

void Setting::init()
{
    auto ptr = new Setting();
    setting.reset(ptr);
}

void Setting::dispose()
{
    setting.reset();
}

Setting* Setting::get()
{
    return setting.get();
}

std::filesystem::path Setting::getDataPath()
{
    return dataPath; //复制一份路径对象，不允许就地修改
}

const JsonObject Setting::getConfigObj()
{
    return configObj;
}

void Setting::setShortcutKey(const std::wstring& type, const std::vector<std::wstring>& keys)
{
    std::wstring str;
    for (size_t i = 0; i < keys.size(); i++)
    {
        str += L"+" + keys[i];
    }
    str.erase(0,1);
    auto shortcutKey = configObj.GetNamedObject(L"shortcutKey");
    shortcutKey.SetNamedValue(type, JsonValue::CreateStringValue(str));
    auto app = Ling::App::get();
    app->unRegHotKey(capShortcutMsgId);
    app->regHotKey(str, capShortcutMsgId);
    save();
}

std::wstring Setting::getShortcutKey(const std::wstring& type)
{
    // 一路用带默认值的重载：启动时 ensureDefaults 已经补齐过，这里只是别让运行期
    // 意外（配置被外部改动、问了个没配过的 type）变成一次崩溃
    auto obj = configObj.GetNamedObject(L"shortcutKey", nullptr);
    if (!obj) return L"";
    return std::wstring{ obj.GetNamedString(type, L"") };
}

void Setting::setAutoStart(bool autoStart)
{
    std::wstring runKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    if (autoStart) {
        wchar_t buffer[MAX_PATH];
        GetModuleFileName(nullptr, buffer, MAX_PATH);
        auto curPath = std::filesystem::path(buffer);
        std::wstring commandLine = std::format(L"\"{}\" --auto-start", curPath.wstring());
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, runKey.data(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegSetValueEx(hKey, L"ScreenCapture", 0, REG_SZ, (const BYTE*)commandLine.data(), (commandLine.size() + 1) * sizeof(wchar_t));
            RegCloseKey(hKey);
        }
    }
    else {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, runKey.data(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegDeleteValue(hKey, L"ScreenCapture");
            RegCloseKey(hKey);
        }
    }
    auto common = configObj.GetNamedObject(L"common", nullptr);
    if (!common) {
        common = JsonObject();
        configObj.SetNamedValue(L"common", common);
    }
    common.SetNamedValue(L"autoStart", JsonValue::CreateBooleanValue(autoStart));
    save();
}

bool Setting::getAutoStart()
{
    auto common = configObj.GetNamedObject(L"common", nullptr);
    return common && common.GetNamedBoolean(L"autoStart", false);
}

std::filesystem::path Setting::initDataPath()
{
    PWSTR pathTmp;
    auto hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &pathTmp);
    if (FAILED(hr)) {
        _ASSERT_EXPR(FALSE, L"get roaming path，error");
        return L"";
    }
    auto dataPath = std::filesystem::path{ pathTmp };
    CoTaskMemFree(pathTmp);
    dataPath.append("ScreenCapture");
    if (!std::filesystem::exists(dataPath)) {
        if (!std::filesystem::create_directories(dataPath)) {
            _ASSERT_EXPR(FALSE, L"create data path，error");
        }
    }
    return dataPath;
}

std::filesystem::path Setting::initConfigPath()
{
    // 与插件的查找顺序一致（见 Util.cpp 里的 findImageReader）：先看 exe 同目录。
    // 只有那份文件本来就存在时才认它 —— 不存在就不要在程序目录里新建，
    // 装在 Program Files 下时那儿通常没有写权限，况且默认位置该是 appdata
    wchar_t buffer[MAX_PATH]{};
    GetModuleFileName(nullptr, buffer, MAX_PATH);
    auto path = std::filesystem::path{ buffer }.parent_path().append(L"config.json");
    if (std::filesystem::exists(path)) return path;
    auto fallback = this->dataPath; //复制一份路径对象，append 会就地改
    return fallback.append(L"config.json");
}

void Setting::save()
{
    std::wstring str{ configObj.Stringify() };
    Ling::Util::saveFile(configPath.wstring(), str);
}

std::wstring Setting::getLang()
{
    auto common = configObj.GetNamedObject(L"common", nullptr);
    if (!common) return L"zh-CN";
    return std::wstring{ common.GetNamedString(L"language", L"zh-CN") };
}

void Setting::setLang(const std::wstring& langCode)
{
    auto common = setting->configObj.GetNamedObject(L"common", nullptr);
    if (!common) {
        common = JsonObject();
        setting->configObj.SetNamedValue(L"common", common);
    }
    common.SetNamedValue(L"language", JsonValue::CreateStringValue(langCode));
    setting->save();
	Lang::get()->initLang(langCode);
}

JsonObject Setting::getToolObj(const std::wstring& tool)
{
    // 用带默认值的重载：这两层在旧配置文件里都不存在，直接 GetNamedObject 会抛异常，
    // 值被手工改成非对象时它也一样返回默认值，不会炸
    auto root = configObj.GetNamedObject(L"toolPin", nullptr);
    if (!root) {
        root = JsonObject();
        configObj.SetNamedValue(L"toolPin", root);
    }
    auto obj = root.GetNamedObject(tool, nullptr);
    if (!obj) {
        obj = JsonObject();
        root.SetNamedValue(tool, obj);
    }
    return obj;
}

bool Setting::getToolFlag(const std::wstring& tool, const std::wstring& key, bool def)
{
    return getToolObj(tool).GetNamedBoolean(key, def);
}

void Setting::setToolFlag(const std::wstring& tool, const std::wstring& key, bool val)
{
    getToolObj(tool).SetNamedValue(key, JsonValue::CreateBooleanValue(val));
    save();
}

float Setting::getToolNum(const std::wstring& tool, const std::wstring& key, float def)
{
    return static_cast<float>(getToolObj(tool).GetNamedNumber(key, def));
}

void Setting::setToolNum(const std::wstring& tool, const std::wstring& key, float val)
{
    getToolObj(tool).SetNamedValue(key, JsonValue::CreateNumberValue(val));
    save();
}

JsonObject Setting::getMediaObj(const std::wstring& media)
{
    auto obj = configObj.GetNamedObject(media, nullptr);
    if (!obj) {
        obj = JsonObject();
        configObj.SetNamedValue(media, obj);
    }
    return obj;
}

float Setting::getMediaNum(const std::wstring& media, const std::wstring& key, float def)
{
    auto obj = configObj.GetNamedObject(media, nullptr);
    if (obj && obj.HasKey(key)) {
        try {
            return static_cast<float>(obj.GetNamedNumber(key, def));
        }
        catch (...) {
            // 配置文件可能被用户手工改成了错误类型，按缺失项处理。
        }
    }
    // 早期设置页把 MP4 配置写在 video 下。只对 MP4 做回退，避免一项旧配置
    // 同名时污染 GIF 或截图配置。
    if (media == L"mp4") {
        auto legacy = configObj.GetNamedObject(L"video", nullptr);
        if (legacy && legacy.HasKey(key)) {
            try {
                return static_cast<float>(legacy.GetNamedNumber(key, def));
            }
            catch (...) {
            }
        }
    }
    return def;
}

void Setting::setMediaNum(const std::wstring& media, const std::wstring& key, float val)
{
    getMediaObj(media).SetNamedValue(key, JsonValue::CreateNumberValue(val));
    save();
}

bool Setting::getMediaFlag(const std::wstring& media, const std::wstring& key, bool def)
{
    auto obj = configObj.GetNamedObject(media, nullptr);
    if (obj && obj.HasKey(key)) {
        try {
            return obj.GetNamedBoolean(key, def);
        }
        catch (...) {
        }
    }
    if (media == L"mp4") {
        auto legacy = configObj.GetNamedObject(L"video", nullptr);
        if (legacy && legacy.HasKey(key)) {
            try {
                return legacy.GetNamedBoolean(key, def);
            }
            catch (...) {
            }
        }
    }
    return def;
}

void Setting::setMediaFlag(const std::wstring& media, const std::wstring& key, bool val)
{
    getMediaObj(media).SetNamedValue(key, JsonValue::CreateBooleanValue(val));
    save();
}

std::wstring Setting::getMediaText(const std::wstring& media, const std::wstring& key, const std::wstring& def)
{
    auto obj = configObj.GetNamedObject(media, nullptr);
    if (!obj || !obj.HasKey(key)) return def;
    try {
        return std::wstring{ obj.GetNamedString(key, def) };
    }
    catch (...) {
        return def;
    }
}

void Setting::setMediaText(const std::wstring& media, const std::wstring& key, const std::wstring& val)
{
    getMediaObj(media).SetNamedValue(key, JsonValue::CreateStringValue(val));
    save();
}

float Setting::getVideoNum(const std::wstring& key, float def)
{
    auto video = configObj.GetNamedObject(L"video", nullptr);
    if (!video) return def;
    return static_cast<float>(video.GetNamedNumber(key, def));
}

void Setting::setVideoNum(const std::wstring& key, float val)
{
    auto video = configObj.GetNamedObject(L"video", nullptr);
    if (!video) {
        video = JsonObject();
        configObj.SetNamedValue(L"video", video);
    }
    video.SetNamedValue(key, JsonValue::CreateNumberValue(val));
    save();
}

bool Setting::getVideoFlag(const std::wstring& key, bool def)
{
    auto video = configObj.GetNamedObject(L"video", nullptr);
    if (!video) return def;
    return video.GetNamedBoolean(key, def);
}

void Setting::setVideoFlag(const std::wstring& key, bool val)
{
    auto video = configObj.GetNamedObject(L"video", nullptr);
    if (!video) {
        video = JsonObject();
        configObj.SetNamedValue(L"video", video);
    }
    video.SetNamedValue(key, JsonValue::CreateBooleanValue(val));
    save();
}

long long Setting::getUpdateCheckDay()
{
    auto common = configObj.GetNamedObject(L"common", nullptr);
    if (!common) return 0;
    return static_cast<long long>(common.GetNamedNumber(L"updateCheckDay", 0));
}

void Setting::setUpdateCheckDay(long long day)
{
    auto common = configObj.GetNamedObject(L"common", nullptr);
    if (!common) return;
    // 这项不写进 defaultConfig：它是程序自己的记账，不是给用户改的配置
    common.SetNamedValue(L"updateCheckDay", JsonValue::CreateNumberValue(static_cast<double>(day)));
    save();
}

void Setting::initShortcutKeys()
{
    auto lingApp = Ling::App::get();
    // 取不到就用默认的那个组合：热键注册不上顶多是快捷键不好用，不该让程序起不来
    std::wstring capStr{ getShortcutKey(L"cap") };
    if (capStr.empty()) capStr = L"Ctrl+Alt+A";
    lingApp->regHotKey(capStr, capShortcutMsgId);

    lingApp->onHotKey.add([this](UINT msg) {
        if (msg == capShortcutMsgId) {
            WinCap::init();
        }
    });
    lingApp->onSecondInstance.add([this]() {
        WinCap::init();
    });
}
