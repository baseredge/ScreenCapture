#include "pch.h"
#include <functional>
#include "../Setting.h"
#include "../MediaConfig.h"
#include "../Util.h"
#include "WinSettingVideo.h"
#include "CapVideo.h"

namespace {
    std::wstring settingText(const wchar_t* zh, const wchar_t* en)
    {
        return Setting::get()->getLang() == L"en-US" ? en : zh;
    }

    Ling::Node* makeRow(Ling::Node* owner, const std::wstring& text)
    {
        auto row = owner->makeChild<Ling::Node>();
        row->setHeight(38.f);
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

WinSettingVideo::WinSettingVideo(Ling::WinBase* parent)
    : Ling::Node(parent),
      selectedCodec(MediaConfig::mp4Codec()),
      selectedFps(MediaConfig::mp4Fps()),
      selectedBitrate(MediaConfig::mp4BitrateKbps()),
      selectedQuality(MediaConfig::mp4Quality()),
      selectedAudioBitrate(MediaConfig::mp4AudioBitrateKbps()),
      selectedSampleRate(MediaConfig::mp4SampleRate()),
      selectedDuration(MediaConfig::mp4MaxMinutes()),
      systemAudio(Setting::get()->getMediaFlag(L"mp4", L"systemAudio", true)),
      microphone(Setting::get()->getMediaFlag(L"mp4", L"microphone", false)),
      cursor(Setting::get()->getMediaFlag(L"mp4", L"cursor", true))
{
    initCodecCtrls();
    initRelayDirectoryCtrls();
    initFpsCtrls();
    initBitrateCtrls();
    initQualityCtrls();
    initAudioBitrateCtrls();
    initSampleRateCtrls();
    initAudioCtrls();
    initCursorCtrl();
    initDurationCtrls();
    initEncoderInfo();
}

WinSettingVideo::~WinSettingVideo()
{
}

void WinSettingVideo::initCodecCtrls()
{
    auto options = makeOptions(makeRow(this, settingText(L"视频编码", L"Video codec")));
    const std::vector<std::wstring> labels{ settingText(L"自动", L"Auto"), L"H.264", L"H.265" };
    const std::vector<std::wstring> values{ L"auto", L"h264", L"h265" };
    for (int index = 0; index < 3; ++index) {
        auto btn = makeChoiceButton(options, index, labels[index]);
        btn->setWidth(65.f);
        btn->onClick.add([this, index, value = values[index]](Ling::Button*) {
            selectedCodec = index;
            Setting::get()->setMediaText(L"mp4", L"codec", value);
            applyStyle(codecBtns, selectedCodec);
        });
        codecBtns.push_back(btn);
    }
    applyStyle(codecBtns, selectedCodec);
    addBorder(this);
}

void WinSettingVideo::initRelayDirectoryCtrls()
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

void WinSettingVideo::initEncoderInfo()
{
    auto row = makeRow(this, settingText(L"最近实际编码", L"Last encoder"));
    row->setHeight(72.f);
    auto details = makeOptions(row);
    details->setFlexDirection(Ling::FlexDirection::Column);
    const auto info = CapVideo::lastEncoderInfo();
    auto addLine = [details](const std::wstring& text) {
        auto label = details->makeChild<Ling::Label>();
        label->setText(text);
        label->setHeight(22.f);
        label->setWidthPercent(100.f);
        label->setFontSize(12.f);
        label->setColor(0x526478ff);
    };
    if (info.codec.empty()) {
        addLine(settingText(L"录制后显示实际编码和加速方式", L"Shown after recording starts"));
        addLine(settingText(L"自动优先 H.265，不可用时回退 H.264", L"Auto: H.265 first, then H.264"));
    }
    else {
        const auto mode = info.hardware == 1 ? settingText(L"硬件加速", L"Hardware")
            : info.hardware == 0 ? settingText(L"软件编码", L"Software")
            : settingText(L"加速方式未知", L"Acceleration unknown");
        addLine(info.codec + L" · " + mode);
        addLine(info.name.empty() ? settingText(L"系统未提供编码器名称", L"Encoder name not provided") : info.name);
    }
    addLine(settingText(L"本次运行的最近记录；重新打开此页可刷新", L"This session; reopen this page to refresh"));
}

void WinSettingVideo::initFpsCtrls()
{
    auto options = makeOptions(makeRow(this, settingText(L"帧率", L"Frame rate")));
    for (const int fps : { 5, 10, 15, 30, 60 }) {
        auto btn = makeChoiceButton(options, fps, std::to_wstring(fps));
        btn->onClick.add([this, fps](Ling::Button*) {
            selectedFps = fps;
            Setting::get()->setMediaNum(L"mp4", L"fps", (float)fps);
            applyStyle(fpsBtns, selectedFps);
        });
        fpsBtns.push_back(btn);
    }
    applyStyle(fpsBtns, selectedFps);
    addBorder(this);
}

void WinSettingVideo::initBitrateCtrls()
{
    auto options = makeOptions(makeRow(this, settingText(L"视频码率", L"Video bitrate")));
    const std::vector<std::pair<int, std::wstring>> values{
        { 0, settingText(L"自动", L"Auto") }, { 2000, L"2M" }, { 4000, L"4M" },
        { 8000, L"8M" }, { 12000, L"12M" }
    };
    for (const auto& [bitrate, text] : values) {
        auto btn = makeChoiceButton(options, bitrate, text);
        btn->onClick.add([this, bitrate](Ling::Button*) {
            selectedBitrate = bitrate;
            Setting::get()->setMediaNum(L"mp4", L"bitrateKbps", (float)bitrate);
            applyStyle(bitrateBtns, selectedBitrate);
        });
        bitrateBtns.push_back(btn);
    }
    applyStyle(bitrateBtns, selectedBitrate);
    addBorder(this);
}

void WinSettingVideo::initQualityCtrls()
{
    auto options = makeOptions(makeRow(this, settingText(L"视频质量", L"Video quality")));
    for (const int quality : { 40, 50, 60, 80, 100 }) {
        auto btn = makeChoiceButton(options, quality, std::to_wstring(quality) + L"%");
        btn->onClick.add([this, quality](Ling::Button*) {
            selectedQuality = quality;
            Setting::get()->setMediaNum(L"mp4", L"quality", (float)quality);
            applyStyle(qualityBtns, selectedQuality);
        });
        qualityBtns.push_back(btn);
    }
    applyStyle(qualityBtns, selectedQuality);
    addBorder(this);
}

void WinSettingVideo::initAudioBitrateCtrls()
{
    auto options = makeOptions(makeRow(this, settingText(L"音频码率", L"Audio bitrate")));
    for (const int bitrate : { 128, 192, 256 }) {
        auto btn = makeChoiceButton(options, bitrate, std::to_wstring(bitrate) + L"k");
        btn->onClick.add([this, bitrate](Ling::Button*) {
            selectedAudioBitrate = bitrate;
            Setting::get()->setMediaNum(L"mp4", L"audioBitrateKbps", (float)bitrate);
            applyStyle(audioBitrateBtns, selectedAudioBitrate);
        });
        audioBitrateBtns.push_back(btn);
    }
    applyStyle(audioBitrateBtns, selectedAudioBitrate);
    addBorder(this);
}

void WinSettingVideo::initSampleRateCtrls()
{
    auto options = makeOptions(makeRow(this, settingText(L"音频采样率", L"Audio sample rate")));
    const std::vector<std::pair<int, std::wstring>> values{
        { 44100, L"44.1k" }, { 48000, L"48k" }
    };
    for (const auto& [sampleRate, text] : values) {
        auto btn = makeChoiceButton(options, sampleRate, text);
        btn->onClick.add([this, sampleRate](Ling::Button*) {
            selectedSampleRate = sampleRate;
            Setting::get()->setMediaNum(L"mp4", L"sampleRate", (float)sampleRate);
            applyStyle(sampleRateBtns, selectedSampleRate);
        });
        sampleRateBtns.push_back(btn);
    }
    applyStyle(sampleRateBtns, selectedSampleRate);
    addBorder(this);
}

void WinSettingVideo::initAudioCtrls()
{
    auto makeToggle = [this](const std::wstring& text, bool selected, Ling::Button** outBtn,
        std::function<void(bool)> onChanged) {
        auto row = makeRow(this, text);
        auto btn = makeChoiceButton(row, 0, L"");
        btn->setWidth(90.f);
        btn->setId(L"");
        btn->onClick.add([this, btn, selected, onChanged](Ling::Button*) mutable {
            selected = !selected;
            onChanged(selected);
            setToggleButton(btn, selected);
        });
        setToggleButton(btn, selected);
        *outBtn = btn;
        addBorder(this);
    };

    makeToggle(settingText(L"系统声音", L"System audio"), systemAudio, &systemAudioBtn,
        [this](bool value) {
            systemAudio = value;
            Setting::get()->setMediaFlag(L"mp4", L"systemAudio", value);
        });
    makeToggle(settingText(L"麦克风", L"Microphone"), microphone, &microphoneBtn,
        [this](bool value) {
            microphone = value;
            Setting::get()->setMediaFlag(L"mp4", L"microphone", value);
        });
}

void WinSettingVideo::initCursorCtrl()
{
    auto row = makeRow(this, settingText(L"鼠标指针", L"Mouse cursor"));
    cursorBtn = makeChoiceButton(row, 0, L"");
    cursorBtn->setWidth(90.f);
    cursorBtn->setId(L"");
    cursorBtn->onClick.add([this](Ling::Button*) {
        cursor = !cursor;
        Setting::get()->setMediaFlag(L"mp4", L"cursor", cursor);
        setToggleButton(cursorBtn, cursor);
    });
    setToggleButton(cursorBtn, cursor);
    addBorder(this);
}

void WinSettingVideo::initDurationCtrls()
{
    auto options = makeOptions(makeRow(this, settingText(L"自动停止", L"Auto stop")));
    for (const int minutes : { 30, 60, 120, 240 }) {
        auto btn = makeChoiceButton(options, minutes,
            std::to_wstring(minutes) + (Setting::get()->getLang() == L"en-US" ? L" min" : L"分钟"));
        btn->onClick.add([this, minutes](Ling::Button*) {
            selectedDuration = minutes;
            Setting::get()->setMediaNum(L"mp4", L"maxMinutes", (float)minutes);
            applyStyle(durationBtns, selectedDuration);
        });
        durationBtns.push_back(btn);
    }
    applyStyle(durationBtns, selectedDuration);
    addBorder(this);
}

void WinSettingVideo::applyStyle(std::vector<Ling::Button*>& buttons, int selected)
{
    for (auto* btn : buttons) {
        const bool active = std::stoi(btn->id) == selected;
        applyToggleStyle(btn, active);
    }
}

void WinSettingVideo::applyToggleStyle(Ling::Button* btn, bool selected)
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

void WinSettingVideo::setToggleButton(Ling::Button* btn, bool selected)
{
    btn->setText(selected ? settingText(L"开启", L"On") : settingText(L"关闭", L"Off"));
    applyToggleStyle(btn, selected);
}
