#include "pch.h"
#include "DeviceBatteryItem.h"
#include "DeviceBatteryManager.h"

const wchar_t* DeviceBatteryItem::GetItemName() const
{
    return L"2.4GDeviceBattery";
}

const wchar_t* DeviceBatteryItem::GetItemId() const
{
    return L"Device24GBattery";
}

const wchar_t* DeviceBatteryItem::GetItemLableText() const
{
    return DeviceBatteryManager::Instance().GetLabelText().c_str();
}

const wchar_t* DeviceBatteryItem::GetItemValueText() const
{
    return DeviceBatteryManager::Instance().GetDisplayText().c_str();
}

const wchar_t* DeviceBatteryItem::GetItemValueSampleText() const
{
    return L"100%+";
}
