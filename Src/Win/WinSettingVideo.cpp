#include "pch.h"
#include <functional>
#include "../Setting.h"
#include "../MediaConfig.h"
#include "WinSettingVideo.h"

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

    Ling::Button* makeChoiceButton(Ling::Node* options, int value, const std::wstring& text)
    {
        auto btn = options->makeChild<Ling::Button>();
        btn->setId(std::to_wstring(value));
        btn->setText(text);
        btn->setSize(52.f, 30.f);
        btn->setMarginRight(6.f);
        btn->setBorder(1.f, 0xD9D9D9FF);
        btn->setBorderRadius(3.f);
        btn->setFontSize(13.f);
        return btn;
    }
}

WinSettingVideo::WinSettingVideo(Ling::WinBase* parent)
    : Ling::Node(parent),
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
    initFpsCtrls();
    initBitrateCtrls();
    initQualityCtrls();
    initAudioBitrateCtrls();
    initSampleRateCtrls();
    initAudioCtrls();
    initCursorCtrl();
    initDurationCtrls();
}

WinSettingVideo::~WinSettingVideo()
{
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
