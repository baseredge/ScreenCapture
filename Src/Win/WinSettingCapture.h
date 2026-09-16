#pragma once
#include <include/Ling.h>

class WinSettingCapture : public Ling::Node
{
public:
    WinSettingCapture(Ling::WinBase* parent);
    ~WinSettingCapture();

private:
    void initFormatCtrls();
    void initClipboardRelayCtrl();
    void initRelayDirectoryCtrls();
    void initJpegQualityCtrls();
    void applyFormatStyle();
    void applyQualityStyle();
    void applyToggleStyle(Ling::Button* btn, bool selected);
    void setToggleButton(Ling::Button* btn, bool selected);

private:
    std::vector<Ling::Button*> formatBtns;
    std::vector<Ling::Button*> qualityBtns;
    Ling::Button* relayBtn{ nullptr };
    Ling::Label* directoryLabel{ nullptr };
    std::wstring selectedFormat{ L"png" };
    int selectedQuality{ 95 };
    bool clipboardFileRelay{ false };
};
