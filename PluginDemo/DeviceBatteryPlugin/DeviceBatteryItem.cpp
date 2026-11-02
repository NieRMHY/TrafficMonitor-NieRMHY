#include "pch.h"
#include "DeviceBatteryItem.h"
#include "DeviceBatteryManager.h"

void DeviceBatteryItem::SetIndex(int index)
{
    index_ = index;
    id_ = index == 0 ? L"Device24GBattery" : L"Device24GBattery_" + std::to_wstring(index + 1);
}

const wchar_t* DeviceBatteryItem::GetItemName() const
{
    return DeviceBatteryManager::Instance(index_).GetLabelText().c_str();
}

const wchar_t* DeviceBatteryItem::GetItemId() const
{
    return id_.c_str();
}

const wchar_t* DeviceBatteryItem::GetItemLableText() const
{
    return DeviceBatteryManager::Instance(index_).GetLabelText().c_str();
}

const wchar_t* DeviceBatteryItem::GetItemValueText() const
{
    return DeviceBatteryManager::Instance(index_).GetDisplayText().c_str();
}

const wchar_t* DeviceBatteryItem::GetItemValueSampleText() const
{
    return L"100%+";
}
