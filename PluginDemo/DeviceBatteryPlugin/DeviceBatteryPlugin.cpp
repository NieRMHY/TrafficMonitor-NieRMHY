#include "pch.h"
#include "DeviceBatteryPlugin.h"
#include "DeviceBatteryManager.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

DeviceBatteryPlugin& DeviceBatteryPlugin::Instance()
{
    static DeviceBatteryPlugin instance;
    return instance;
}

IPluginItem* DeviceBatteryPlugin::GetItem(int index)
{
    if (index >= 0 && index < itemCount_)
    {
        return &batteryItems_[index];
    }
    if (trailingSpacer_ && index == itemCount_) return &spacerItem_;
    return nullptr;
}

void DeviceBatteryPlugin::DataRequired()
{
    for (int i = 0; i < itemCount_; ++i) DeviceBatteryManager::Instance(i).RefreshIfNeeded();
}

const wchar_t* DeviceBatteryPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:
        return L"2.4GDeviceBattery";
    case TMI_DESCRIPTION:
        return L"Read multiple device batteries from HTTP or a FerrisPulse local snapshot";
    case TMI_AUTHOR:
        return L"Custom";
    case TMI_COPYRIGHT:
        return L"Copyright (C) 2026";
    case TMI_VERSION:
        return L"1.1.1";
    case TMI_URL:
        return L"https://github.com/zhongyang219/TrafficMonitor";
    default:
        return L"";
    }
}

void DeviceBatteryPlugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data)
{
    if (index == EI_CONFIG_DIR && data != nullptr)
    {
        DeviceBatteryManager::Instance().LoadConfig(data);
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);
        const std::wstring module(path);
        std::wstring directory(data);
        if (!directory.empty() && directory.back() != L'\\' && directory.back() != L'/') directory += L'\\';
        const std::wstring configPath = directory + module.substr(module.find_last_of(L"\\/") + 1) + L".ini";
        trailingSpacer_ = GetPrivateProfileIntW(L"display", L"trailing_spacer", 0, configPath.c_str()) != 0;
        itemCount_ = (std::max)(1, (std::min)(DeviceBatteryManager::MaxItems,
            static_cast<int>(GetPrivateProfileIntW(L"display", L"item_count", 1, configPath.c_str()))));
        for (int i = 0; i < itemCount_; ++i)
        {
            batteryItems_[i].SetIndex(i);
            DeviceBatteryManager::Instance(i).LoadConfig(data, i);
        }
    }
}

const wchar_t* DeviceBatteryPlugin::GetTooltipInfo()
{
    thread_local std::wstring text;
    text.clear();
    for (int i = 0; i < itemCount_; ++i)
    {
        if (i > 0) text += L"\r\n";
        text += DeviceBatteryManager::Instance(i).GetLabelText() + L": " + DeviceBatteryManager::Instance(i).GetTooltipText();
    }
    return text.c_str();
}

ITMPlugin* TMPluginGetInstance()
{
    return &DeviceBatteryPlugin::Instance();
}
