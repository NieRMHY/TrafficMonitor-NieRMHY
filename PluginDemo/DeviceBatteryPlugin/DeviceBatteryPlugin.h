#pragma once

#include "PluginInterface.h"
#include "DeviceBatteryItem.h"
#include "DeviceBatteryManager.h"

class DeviceBatteryPlugin : public ITMPlugin
{
public:
    static DeviceBatteryPlugin& Instance();

    IPluginItem* GetItem(int index) override;
    void DataRequired() override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;
    void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override;
    const wchar_t* GetTooltipInfo() override;

private:
    DeviceBatteryPlugin() = default;

private:
    DeviceBatteryItem batteryItems_[DeviceBatteryManager::MaxItems]{};
    DeviceBatterySpacerItem spacerItem_{};
    int itemCount_{1};
    bool trailingSpacer_{};
};

#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance();
#ifdef __cplusplus
}
#endif
