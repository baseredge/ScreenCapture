#include "pch.h"
#include "../Lang.h"
#include "WinSetting.h"
#include "WinSettingAbout.h"
#include "../Util.h"
#include "../Setting.h"

namespace {
    std::wstring getAboutLabel(const std::wstring& key)
    {
        if (key == L"fork") {
            return Setting::get()->getLang() == L"en-US" ? L"Fork:" : L"本分支：";
        }
        if (key == L"maintainer") {
            return Setting::get()->getLang() == L"en-US" ? L"Maintainer:" : L"维护者：";
        }
        return Lang::get(L"about." + key);
    }
}

WinSettingAbout::WinSettingAbout(Ling::WinBase* parent):Ling::Node(parent)
{
    std::vector<std::wstring> keys = { L"version",L"project",L"author",L"fork",L"maintainer" };
    for (auto& key : keys)
    {
        auto box = makeChild<Ling::Node>();
        box->setHeight(39.f);
        box->setFlexDirection(Ling::FlexDirection::Row);
        box->setAlignItems(Ling::Align::Center);

        auto label = box->makeChild<Ling::Label>();
        label->setText(getAboutLabel(key));
        label->setHeightPercent(100.f);
        label->setJustifyContent(Ling::Justify::Center);
        label->setFlexGrow(1.f);

        auto btn = box->makeChild<Ling::Button>();
        btn->setId(key);
        if (key == L"version") {
            auto ver = Ling::Util::getVerNum();
            auto verStr = std::format(L"{}.{}.{}", ver[0], ver[1], ver[2]);
            btn->setText(verStr);
        }
        else if (key == L"project") {
            btn->setText(L"github.com/xland/ScreenCapture");
            btn->setColor(0x597ef7ff);
            btn->setHoverColor(0x597ef7ff);
            btn->onClick.add([this](Ling::Button* btn) {
                std::wstring downloadUrl{ L"https://github.com/xland/ScreenCapture" };
                ShellExecute(win->hwnd, L"open", downloadUrl.data(), nullptr, nullptr, SW_SHOWNORMAL);
                });
        }
        else if (key == L"author") {
            btn->setText(Lang::get(L"about.wechat"));
            btn->setColor(0x597ef7ff);
            btn->setHoverColor(0x597ef7ff);
            btn->onClick.add([this](Ling::Button* btn) {
                Ling::Util::setTextToClipboard(L"liulun_007");
                MessageBox(win->hwnd, Lang::get(L"about.copySuccess").data(), Lang::get(L"about.sysTip").data(), MB_OK | MB_ICONINFORMATION);
                });
        }
        else if (key == L"fork") {
            btn->setText(L"github.com/baseredge/ScreenCapture");
            btn->setColor(0x597ef7ff);
            btn->setHoverColor(0x597ef7ff);
            btn->onClick.add([this](Ling::Button* btn) {
                std::wstring downloadUrl{ L"https://github.com/baseredge/ScreenCapture" };
                ShellExecute(win->hwnd, L"open", downloadUrl.data(), nullptr, nullptr, SW_SHOWNORMAL);
                });
        }
        else {
            btn->setText(L"baseredge");
        }
        btn->setAlignItems(Ling::Align::FlexEnd);
        btn->setHeight(28.f);
        btn->setWidth(120.f);
        btn->setBg(0);
        btn->setHoverBg(0);
        btns.push_back(btn);

        auto border = makeChild<Ling::Node>();
        border->setHeight(1.f);
        border->setBg(0xE0E0E0FF);
    }
}

WinSettingAbout::~WinSettingAbout()
{

}
