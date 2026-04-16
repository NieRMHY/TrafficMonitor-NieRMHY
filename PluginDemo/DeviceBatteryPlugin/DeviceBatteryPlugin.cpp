#include "pch.h"
#include "DeviceBatteryPlugin.h"
#include "DeviceBatteryManager.h"

DeviceBatteryPlugin& DeviceBatteryPlugin::Instance()
{
    static DeviceBatteryPlugin instance;
    return instance;
}

IPluginItem* DeviceBatteryPlugin::GetItem(int index)
{
    if (index == 0)
    {
        return &batteryItem_;
    }
    return nullptr;
}

void DeviceBatteryPlugin::DataRequired()
{
    DeviceBatteryManager::Instance().RefreshIfNeeded();
}

const wchar_t* DeviceBatteryPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:
        return L"2.4GDeviceBattery";
    case TMI_DESCRIPTION:
        return L"Read device battery from unified HTTP API";
    case TMI_AUTHOR:
        return L"Custom";
    case TMI_COPYRIGHT:
        return L"Copyright (C) 2026";
    case TMI_VERSION:
        return L"1.0.0";
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
    }
}

const wchar_t* DeviceBatteryPlugin::GetTooltipInfo()
{
    return DeviceBatteryManager::Instance().GetTooltipText().c_str();
}

ITMPlugin* TMPluginGetInstance()
{
    return &DeviceBatteryPlugin::Instance();
}
