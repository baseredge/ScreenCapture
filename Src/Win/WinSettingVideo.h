#pragma once
#include <include/Ling.h>

class WinSettingVideo : public Ling::Node
{
public:
    WinSettingVideo(Ling::WinBase* parent);
    ~WinSettingVideo();

private:
    void initFpsCtrls();
    void initBitrateCtrls();
    void initQualityCtrls();
    void initAudioBitrateCtrls();
    void initSampleRateCtrls();
    void initAudioCtrls();
    void initCursorCtrl();
    void initDurationCtrls();
    void applyStyle(std::vector<Ling::Button*>& buttons, int selected);
    void applyToggleStyle(Ling::Button* btn, bool selected);
    void setToggleButton(Ling::Button* btn, bool selected);

private:
    std::vector<Ling::Button*> fpsBtns;
    std::vector<Ling::Button*> bitrateBtns;
    std::vector<Ling::Button*> qualityBtns;
    std::vector<Ling::Button*> audioBitrateBtns;
    std::vector<Ling::Button*> sampleRateBtns;
    std::vector<Ling::Button*> durationBtns;
    Ling::Button* systemAudioBtn{ nullptr };
    Ling::Button* microphoneBtn{ nullptr };
    Ling::Button* cursorBtn{ nullptr };
    int selectedFps{ 30 };
    int selectedBitrate{ 0 };
    int selectedQuality{ 50 };
    int selectedAudioBitrate{ 192 };
    int selectedSampleRate{ 44100 };
    int selectedDuration{ 120 };
    bool systemAudio{ true };
    bool microphone{ false };
    bool cursor{ true };
};
