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
        return L"1.1.4";
    case TMI_URL:
        return L"https://github.com/zhongyang219/TrafficMonitor";
    default:
        return L"";
    }
}

void DeviceBatteryPlugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data)
{
    if (data && index == EI_LABEL_TEXT_COLOR) { labelColor_.store(wcstoul(data, nullptr, 10)); return; }
    if (data && index == EI_VALUE_TEXT_COLOR) { valueColor_.store(wcstoul(data, nullptr, 10)); return; }
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
        alignColumns_ = GetPrivateProfileIntW(L"display", L"align_columns", 0, configPath.c_str()) != 0;
        itemCount_ = (std::max)(1, (std::min)(DeviceBatteryManager::MaxItems,
            static_cast<int>(GetPrivateProfileIntW(L"display", L"item_count", 1, configPath.c_str()))));
        for (int i = 0; i < itemCount_; ++i)
        {
            batteryItems_[i].SetIndex(i);
            DeviceBatteryManager::Instance(i).LoadConfig(data, i);
        }
    }
}

DeviceBatteryPlugin::ColumnLayout DeviceBatteryPlugin::MeasureColumns(void* dc) const
{
    HDC hdc = static_cast<HDC>(dc);
    ColumnLayout layout;
    if (!hdc) return layout;
    for (int i = 0; i < itemCount_; ++i)
    {
        const auto& manager = DeviceBatteryManager::Instance(i);
        const std::wstring label = manager.GetLabelText() + L": ";
        SIZE size{};
        GetTextExtentPoint32W(hdc, label.c_str(), static_cast<int>(label.size()), &size);
        layout.labelWidth = (std::max)(layout.labelWidth, static_cast<int>(size.cx));
        // Proportional fonts can make 88% wider than 100%; reserve the actual maximum.
        for (int percent = 0; percent <= 100; ++percent)
        {
            const std::wstring value = std::to_wstring(percent) + (manager.ShowChargingMarker() ? L"%+" : L"%");
            GetTextExtentPoint32W(hdc, value.c_str(), static_cast<int>(value.size()), &size);
            layout.valueWidth = (std::max)(layout.valueWidth, static_cast<int>(size.cx));
        }
    }
    TEXTMETRICW metrics{};
    GetTextMetricsW(hdc, &metrics);
    layout.gap = (std::max)(3, static_cast<int>(metrics.tmHeight) / 5);
    return layout;
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
