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
    bool AlignColumns() const { return alignColumns_; }
    struct ColumnLayout
    {
        int labelWidth{}, valueWidth{}, gap{};
        int Total() const { return labelWidth + valueWidth + 2 * gap; }
    };
    ColumnLayout MeasureColumns(void* dc) const;
    unsigned long LabelColor() const { return labelColor_.load(); }
    unsigned long ValueColor() const { return valueColor_.load(); }

private:
    DeviceBatteryPlugin() = default;

private:
    DeviceBatteryItem batteryItems_[DeviceBatteryManager::MaxItems]{};
    DeviceBatterySpacerItem spacerItem_{};
    int itemCount_{1};
    bool trailingSpacer_{};
    bool alignColumns_{};
    std::atomic<unsigned long> labelColor_{ 0xFFFFFF }, valueColor_{ 0xFFFFFF };
};

#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance();
#ifdef __cplusplus
}
#endif
