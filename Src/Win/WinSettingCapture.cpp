#include "pch.h"
#include "../Setting.h"
#include "../Util.h"
#include "WinSettingCapture.h"

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
        btn->setHoverBg(0xF2F2F2FF);
        btn->setHoverColor(0x333333FF);
        return btn;
    }

    std::wstring readFormat()
    {
        auto value = Setting::get()->getMediaText(L"capture", L"imageFormat", L"png");
        std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) { return (wchar_t)towlower(ch); });
        if (value == L"jpeg") value = L"jpg";
        return value == L"jpg" || value == L"bmp" ? value : L"png";
    }

    int readQuality()
    {
        const int value = (int)std::lround(Setting::get()->getMediaNum(L"capture", L"jpegQuality", 95.f));
        for (const int quality : { 80, 90, 95, 100 }) {
            if (value == quality) return quality;
        }
        return 95;
    }

}

WinSettingCapture::WinSettingCapture(Ling::WinBase* parent)
    : Ling::Node(parent), selectedFormat(readFormat()), selectedQuality(readQuality()),
      clipboardFileRelay(Setting::get()->getMediaFlag(L"capture", L"clipboardFileRelay", false))
{
    initFormatCtrls();
    initClipboardRelayCtrl();
    initRelayDirectoryCtrls();
    initJpegQualityCtrls();
}

WinSettingCapture::~WinSettingCapture()
{
}

void WinSettingCapture::initFormatCtrls()
{
    auto options = makeOptions(makeRow(this, settingText(L"保存格式", L"File format")));
    const std::vector<std::pair<std::wstring, std::wstring>> values{
        { L"png", L"PNG" }, { L"jpg", L"JPG" }, { L"bmp", L"BMP" }
    };
    for (const auto& [format, text] : values) {
        auto btn = makeChoiceButton(options, format, text);
        btn->onClick.add([this, format](Ling::Button*) {
            selectedFormat = format;
            Setting::get()->setMediaText(L"capture", L"imageFormat", format);
            applyFormatStyle();
        });
        formatBtns.push_back(btn);
    }
    applyFormatStyle();
    addBorder(this);
}

void WinSettingCapture::initClipboardRelayCtrl()
{
    auto row = makeRow(this, settingText(L"剪贴板磁盘中转", L"Clipboard file relay"));
    relayBtn = makeChoiceButton(row, L"relay", L"");
    relayBtn->setWidth(90.f);
    relayBtn->onClick.add([this](Ling::Button*) {
        clipboardFileRelay = !clipboardFileRelay;
        Setting::get()->setMediaFlag(L"capture", L"clipboardFileRelay", clipboardFileRelay);
        setToggleButton(relayBtn, clipboardFileRelay);
    });
    setToggleButton(relayBtn, clipboardFileRelay);
    addBorder(this);
}

void WinSettingCapture::initRelayDirectoryCtrls()
{
    auto row = makeRow(this, settingText(L"中转目录", L"Relay directory"));
    auto options = makeOptions(row);
    directoryLabel = options->makeChild<Ling::Label>();
    directoryLabel->setText(Util::getClipboardRelayDirectory().wstring());
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
        const auto current = Util::getClipboardRelayDirectory().wstring();
        const auto selected = Util::chooseFolder(win->hwnd, current);
        if (selected.empty()) return;
        Setting::get()->setMediaText(L"capture", L"clipboardDirectory", selected);
        directoryLabel->setText(selected);
    });
    auto openBtn = makeChoiceButton(actionOptions, L"open", settingText(L"打开目录", L"Open"));
    openBtn->setWidth(82.f);
    openBtn->onClick.add([this](Ling::Button*) {
        const auto path = Util::getClipboardRelayDirectory();
        if (!Util::openFolder(win->hwnd, path)) {
            const auto message = settingText(L"无法打开中转目录", L"Cannot open relay directory");
            const auto title = settingText(L"提示", L"Notice");
            MessageBoxW(win->hwnd, message.c_str(), title.c_str(), MB_OK | MB_ICONWARNING);
        }
    });
    auto resetBtn = makeChoiceButton(actionOptions, L"reset", settingText(L"恢复默认", L"Default"));
    resetBtn->setWidth(82.f);
    resetBtn->onClick.add([this](Ling::Button*) {
        Setting::get()->setMediaText(L"capture", L"clipboardDirectory", L"");
        directoryLabel->setText(Util::getClipboardRelayDirectory().wstring());
    });
    addBorder(this);

    auto hintRow = makeRow(this, settingText(L"多图发送", L"Multiple images"));
    auto hintOptions = makeOptions(hintRow);
    auto hint = hintOptions->makeChild<Ling::Label>();
    hint->setText(settingText(L"每张截图单独保留，可打开目录后多选发送", L"Each screenshot is kept; open the directory to select multiple files."));
    hint->setHeightPercent(100.f);
    hint->setFlexGrow(1.f);
    hint->setFontSize(11.f);
    hint->setColor(0x666666FF);
    hint->setJustifyContent(Ling::Justify::Center);
    addBorder(this);
}

void WinSettingCapture::initJpegQualityCtrls()
{
    auto options = makeOptions(makeRow(this, settingText(L"JPG 质量", L"JPG quality")));
    for (const int quality : { 80, 90, 95, 100 }) {
        auto btn = makeChoiceButton(options, std::to_wstring(quality), std::to_wstring(quality) + L"%");
        btn->onClick.add([this, quality](Ling::Button*) {
            selectedQuality = quality;
            Setting::get()->setMediaNum(L"capture", L"jpegQuality", (float)quality);
            applyQualityStyle();
        });
        qualityBtns.push_back(btn);
    }
    applyQualityStyle();
    addBorder(this);
}

void WinSettingCapture::applyFormatStyle()
{
    for (auto* btn : formatBtns) applyToggleStyle(btn, btn->id == selectedFormat);
}

void WinSettingCapture::applyQualityStyle()
{
    for (auto* btn : qualityBtns) applyToggleStyle(btn, std::stoi(btn->id) == selectedQuality);
}

void WinSettingCapture::applyToggleStyle(Ling::Button* btn, bool selected)
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

void WinSettingCapture::setToggleButton(Ling::Button* btn, bool selected)
{
    btn->setText(selected ? settingText(L"开启", L"On") : settingText(L"关闭", L"Off"));
    applyToggleStyle(btn, selected);
}
