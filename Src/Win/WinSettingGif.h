#pragma once
#include <include/Ling.h>

class WinSettingGif : public Ling::Node
{
public:
    WinSettingGif(Ling::WinBase* parent);
    ~WinSettingGif();

private:
    void initFpsCtrls();
    void initQualityCtrls();
    void initEncodeCtrls();
    void initCursorCtrls();
    void initLoopCtrls();
    void initDurationCtrls();
    void applyStyle(std::vector<Ling::Button*>& buttons, int selected);
    void applyToggleStyle(Ling::Button* btn, bool selected);
    void setToggleButton(Ling::Button* btn, bool selected);

private:
    std::vector<Ling::Button*> fpsBtns;
    std::vector<Ling::Button*> qualityBtns;
    std::vector<Ling::Button*> loopBtns;
    std::vector<Ling::Button*> durationBtns;
    Ling::Button* fastBtn{ nullptr };
    Ling::Button* cursorBtn{ nullptr };
    int selectedFps{ 15 };
    int selectedQuality{ 80 };
    int selectedLoop{ 0 };
    int selectedDuration{ 6 };
    bool fast{ true };
    bool cursor{ true };
};
