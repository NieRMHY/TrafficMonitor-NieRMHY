#pragma once

#include "PluginInterface.h"
#include <string>

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
