#pragma once
#include <include/Ling.h>
#include <string_view>

namespace ToolbarStyle {
    // One palette across selection, annotation, scrolling and recording toolbars.
    inline UINT iconColor(std::wstring_view id)
    {
        if (id == L"mark" || id == L"pin") return 0xC47D18ff;
        if (id == L"long" || id == L"rect" || id == L"save") return 0x1765ADff;
        if (id == L"video") return 0xC62828ff;
        if (id == L"gif") return 0x087F78ff;
        if (id == L"ocr" || id == L"number") return 0x7040A0ff;
        if (id == L"qrcode" || id == L"text") return 0x4857ABff;
        if (id == L"clipboard") return 0x238552ff;
        if (id == L"close" || id == L"arrow") return 0xC44F4Fff;
        if (id == L"ellipse" || id == L"speaker") return 0x008599ff;
        if (id == L"line") return 0xC47D18ff;
        if (id == L"eraser" || id == L"mic") return 0xAD527Aff;
        if (id == L"mosaic") return 0x66758Cff;
        if (id == L"undo" || id == L"redo") return 0x647D91ff;
        return 0x454545ff;
    }

    inline void applyIconColors(Ling::Button* button, std::wstring_view id)
    {
        button->setColor(iconColor(id));
        button->setHoverColor(id == L"close" ? 0xB91C1Cff : iconColor(id));
    }
}
