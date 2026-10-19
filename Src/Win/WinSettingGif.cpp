#include "pch.h"
#include "../Setting.h"
#include "../MediaConfig.h"
#include "../Util.h"
#include "WinSettingGif.h"

namespace {
    std::wstring settingText(const wchar_t* zh, const wchar_t* en)
    {
        return Setting::get()->getLang() == L"en-US" ? en : zh;
    }

    Ling::Node* makeRow(Ling::Node* owner, const std::wstring& text)
    {
        auto row = owner->makeChild<Ling::Node>();
        row->setHeight(44.f);
        row->setFlexDirection(Ling::FlexDirection::Row);
        row->setAlignItems(Ling::Align::Center);
        auto label = row->makeChild<Ling::Label>();
        label->setText(text);
        label->setWidth(150.f);
        label->setHeightPercent(100.f);
        label->setJustifyContent(Ling::Justify::Center);
        return row;
    }

    Ling::Node* makeOptions(Ling::Node* row)
    {
        auto options = row->makeChild<Ling::Node>();
        options->setFlexDirection(Ling::FlexDirection::Row);
        options->setAlignItems(Ling::Align::Center);
        options->setFlexGrow(1.f);
        return options;
    }

    void addBorder(Ling::Node* owner)
    {
        auto border = owner->makeChild<Ling::Node>();
        border->setHeight(1.f);
        border->setBg(0xE0E0E0FF);
    }

    Ling::Button* makeChoiceButton(Ling::Node* options, const std::wstring& id, const std::wstring& text)
    {
        auto btn = options->makeChild<Ling::Button>();
        btn->setId(id);
        btn->setText(text);
        btn->setSize(52.f, 30.f);
        btn->setMarginRight(6.f);
        btn->setBorder(1.f, 0xD9D9D9FF);
        btn->setBorderRadius(3.f);
        btn->setFontSize(13.f);
        return btn;
    }

    Ling::Button* makeChoiceButton(Ling::Node* options, int value, const std::wstring& text)
    {
        return makeChoiceButton(options, std::to_wstring(value), text);
    }
}

WinSettingGif::WinSettingGif(Ling::WinBase* parent)
    : Ling::Node(parent),
      selectedFps(MediaConfig::gifFps()),
      selectedQuality(MediaConfig::gifQuality()),
      selectedLoop(MediaConfig::gifRepeat()),
      selectedDuration(MediaConfig::gifMaxMinutes()),
      fast(Setting::get()->getMediaFlag(L"gif", L"fast", true)),
      cursor(Setting::get()->getMediaFlag(L"gif", L"cursor", true))
{
    initFpsCtrls();
    initQualityCtrls();
    initEncodeCtrls();
    initCursorCtrls();
    initLoopCtrls();
    initDurationCtrls();
    initRelayDirectoryCtrls();
}

WinSettingGif::~WinSettingGif()
{
}

void WinSettingGif::initFpsCtrls()
{
    auto options = makeOptions(makeRow(this, settingText(L"帧率", L"Frame rate")));
    for (const int fps : { 5, 10, 15, 30, 60 }) {
        auto btn = makeChoiceButton(options, fps, std::to_wstring(fps));
        btn->onClick.add([this, fps](Ling::Button*) {
            selectedFps = fps;
            Setting::get()->setMediaNum(L"gif", L"fps", (float)fps);
            applyStyle(fpsBtns, selectedFps);
        });
        fpsBtns.push_back(btn);
    }
    applyStyle(fpsBtns, selectedFps);
    addBorder(this);
}

void WinSettingGif::initQualityCtrls()
{
    auto options = makeOptions(makeRow(this, settingText(L"GIF 质量", L"GIF quality")));
    for (const int quality : { 40, 60, 80, 100 }) {
        auto btn = makeChoiceButton(options, quality, std::to_wstring(quality) + L"%");
        btn->onClick.add([this, quality](Ling::Button*) {
            selectedQuality = quality;
            Setting::get()->setMediaNum(L"gif", L"quality", (float)quality);
            applyStyle(qualityBtns, selectedQuality);
        });
        qualityBtns.push_back(btn);
    }
    applyStyle(qualityBtns, selectedQuality);
    addBorder(this);
}

void WinSettingGif::initEncodeCtrls()
{
    auto row = makeRow(this, settingText(L"快速编码", L"Fast encoding"));
    fastBtn = makeChoiceButton(row, 0, L"");
    fastBtn->setWidth(90.f);
    fastBtn->setId(L"");
    fastBtn->onClick.add([this](Ling::Button*) {
        fast = !fast;
        Setting::get()->setMediaFlag(L"gif", L"fast", fast);
        setToggleButton(fastBtn, fast);
    });
    setToggleButton(fastBtn, fast);
    addBorder(this);
}

void WinSettingGif::initCursorCtrls()
{
    auto row = makeRow(this, settingText(L"鼠标指针", L"Mouse cursor"));
    cursorBtn = makeChoiceButton(row, 0, L"");
    cursorBtn->setWidth(90.f);
    cursorBtn->setId(L"");
    cursorBtn->onClick.add([this](Ling::Button*) {
        cursor = !cursor;
        Setting::get()->setMediaFlag(L"gif", L"cursor", cursor);
        setToggleButton(cursorBtn, cursor);
    });
    setToggleButton(cursorBtn, cursor);
    addBorder(this);
}

void WinSettingGif::initLoopCtrls()
{
    auto options = makeOptions(makeRow(this, settingText(L"播放方式", L"Playback")));
    const std::vector<std::pair<int, std::wstring>> values{
        { -1, settingText(L"播放一次", L"Once") },
        { 0, settingText(L"无限循环", L"Loop") }
    };
    for (const auto& [repeat, text] : values) {
        auto btn = makeChoiceButton(options, repeat, text);
        btn->setWidth(90.f);
        btn->onClick.add([this, repeat](Ling::Button*) {
            selectedLoop = repeat;
            Setting::get()->setMediaNum(L"gif", L"repeat", (float)repeat);
            applyStyle(loopBtns, selectedLoop);
        });
        loopBtns.push_back(btn);
    }
    applyStyle(loopBtns, selectedLoop);
    addBorder(this);
}

void WinSettingGif::initDurationCtrls()
{
    auto options = makeOptions(makeRow(this, settingText(L"自动停止", L"Auto stop")));
    for (const int minutes : { 1, 3, 6, 10 }) {
        auto btn = makeChoiceButton(options, minutes,
            std::to_wstring(minutes) + (Setting::get()->getLang() == L"en-US" ? L" min" : L"分钟"));
        btn->onClick.add([this, minutes](Ling::Button*) {
            selectedDuration = minutes;
            Setting::get()->setMediaNum(L"gif", L"maxMinutes", (float)minutes);
            applyStyle(durationBtns, selectedDuration);
        });
        durationBtns.push_back(btn);
    }
    applyStyle(durationBtns, selectedDuration);
    addBorder(this);
}

void WinSettingGif::initRelayDirectoryCtrls()
{
    auto row = makeRow(this, settingText(L"中转目录", L"Relay directory"));
    auto options = makeOptions(row);
    directoryLabel = options->makeChild<Ling::Label>();
    directoryLabel->setText(Util::getVideoRelayDirectory().wstring());
    directoryLabel->setHeightPercent(100.f);
    directoryLabel->setFlexGrow(1.f);
    directoryLabel->setFontSize(10.f);
    directoryLabel->setColor(0x666666FF);
    directoryLabel->setJustifyContent(Ling::Justify::Center);
    addBorder(this);

    auto actionRow = makeRow(this, settingText(L"目录操作", L"Directory"));
    auto actionOptions = makeOptions(actionRow);
    auto chooseBtn = makeChoiceButton(actionOptions, L"choose", settingText(L"选择目录", L"Choose"));
    chooseBtn->setWidth(82.f);
    chooseBtn->onClick.add([this](Ling::Button*) {
        const auto current = Util::getVideoRelayDirectory().wstring();
        const auto selected = Util::chooseFolder(win->hwnd, current);
        if (selected.empty()) return;
        Setting::get()->setMediaText(L"video", L"relayDirectory", selected);
        directoryLabel->setText(selected);
    });
    auto openBtn = makeChoiceButton(actionOptions, L"open", settingText(L"打开目录", L"Open"));
    openBtn->setWidth(82.f);
    openBtn->onClick.add([this](Ling::Button*) {
        const auto path = Util::getVideoRelayDirectory();
        if (!Util::openFolder(win->hwnd, path)) {
            const auto message = settingText(L"无法打开中转目录", L"Cannot open relay directory");
            const auto title = settingText(L"提示", L"Notice");
            MessageBoxW(win->hwnd, message.c_str(), title.c_str(), MB_OK | MB_ICONWARNING);
        }
    });
    auto resetBtn = makeChoiceButton(actionOptions, L"reset", settingText(L"恢复默认", L"Default"));
    resetBtn->setWidth(82.f);
    resetBtn->onClick.add([this](Ling::Button*) {
        Setting::get()->setMediaText(L"video", L"relayDirectory", L"");
        directoryLabel->setText(Util::getVideoRelayDirectory().wstring());
    });
    addBorder(this);
}

void WinSettingGif::applyStyle(std::vector<Ling::Button*>& buttons, int selected)
{
    for (auto* btn : buttons) {
        const bool active = std::stoi(btn->id) == selected;
        applyToggleStyle(btn, active);
    }
}

void WinSettingGif::applyToggleStyle(Ling::Button* btn, bool selected)
{
    if (selected) {
        btn->setBg(0x597EF7FF);
        btn->setColor(0xFFFFFFFF);
        btn->setHoverBg(0x597EF7FF);
        btn->setHoverColor(0xFFFFFFFF);
    }
    else {
        btn->setBg(0xFFFFFFFF);
        btn->setColor(0x333333FF);
        btn->setHoverBg(0xF2F2F2FF);
        btn->setHoverColor(0x333333FF);
    }
}

void WinSettingGif::setToggleButton(Ling::Button* btn, bool selected)
{
    btn->setText(selected ? settingText(L"开启", L"On") : settingText(L"关闭", L"Off"));
    applyToggleStyle(btn, selected);
}
