#pragma once

#include "PluginInterface.h"
#include <string>

// Optional empty lower row keeps the final battery in a normal horizontal cell.
class DeviceBatterySpacerItem : public IPluginItem
{
public:
    const wchar_t* GetItemName() const override { return L"电量布局占位（空白）"; }
    const wchar_t* GetItemId() const override { return L"Device24GBatterySpacer"; }
    const wchar_t* GetItemLableText() const override { return L""; }
    const wchar_t* GetItemValueText() const override { return L""; }
    const wchar_t* GetItemValueSampleText() const override { return L""; }
};

class DeviceBatteryItem : public IPluginItem
{
public:
    void SetIndex(int index);
    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;
private:
    int index_{};
    std::wstring id_{L"Device24GBattery"};
};
