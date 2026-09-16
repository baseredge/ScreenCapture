#pragma once

#include <algorithm>
#include <cmath>
#include <initializer_list>

#include "Setting.h"

// 设置页、采集线程和自动停止计时器共用这一份允许值，避免出现“界面显示一种，
// 实际编码又把它改成另一种”的分叉。
namespace MediaConfig
{
    inline int choice(const std::initializer_list<int> values, int value, int def)
    {
        for (const int item : values) {
            if (item == value) return item;
        }
        return def;
    }

    inline int mp4Fps()
    {
        const int value = (int)std::lround(Setting::get()->getMediaNum(L"mp4", L"fps", 30.f));
        return choice({ 5, 10, 15, 30, 60 }, value, 30);
    }

    inline int mp4BitrateKbps()
    {
        const int value = (int)std::lround(Setting::get()->getMediaNum(L"mp4", L"bitrateKbps", 0.f));
        return choice({ 0, 2000, 4000, 8000, 12000 }, value, 0);
    }

    inline int mp4Quality()
    {
        const int value = (int)std::lround(Setting::get()->getMediaNum(L"mp4", L"quality", 50.f));
        return choice({ 40, 50, 60, 80, 100 }, value, 50);
    }

    inline int mp4AudioBitrateKbps()
    {
        const int value = (int)std::lround(Setting::get()->getMediaNum(L"mp4", L"audioBitrateKbps", 192.f));
        return choice({ 128, 192, 256 }, value, 192);
    }

    inline int mp4SampleRate()
    {
        const int value = (int)std::lround(Setting::get()->getMediaNum(L"mp4", L"sampleRate", 44100.f));
        return choice({ 44100, 48000 }, value, 44100);
    }

    inline int mp4MaxMinutes()
    {
        const int value = (int)std::lround(Setting::get()->getMediaNum(L"mp4", L"maxMinutes", 120.f));
        return choice({ 30, 60, 120, 240 }, value, 120);
    }

    inline int gifFps()
    {
        const int value = (int)std::lround(Setting::get()->getMediaNum(L"gif", L"fps", 15.f));
        return choice({ 5, 10, 15, 30, 60 }, value, 15);
    }

    inline int gifQuality()
    {
        const int value = (int)std::lround(Setting::get()->getMediaNum(L"gif", L"quality", 80.f));
        return choice({ 40, 60, 80, 100 }, value, 80);
    }

    inline int gifMaxMinutes()
    {
        const int value = (int)std::lround(Setting::get()->getMediaNum(L"gif", L"maxMinutes", 6.f));
        return choice({ 1, 3, 6, 10 }, value, 6);
    }

    inline int gifRepeat()
    {
        const int value = (int)std::lround(Setting::get()->getMediaNum(L"gif", L"repeat", 0.f));
        return value == -1 ? -1 : 0;
    }
}
