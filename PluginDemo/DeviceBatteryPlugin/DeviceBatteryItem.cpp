#include "pch.h"
#include "DeviceBatteryItem.h"
#include "DeviceBatteryManager.h"
#include "DeviceBatteryPlugin.h"

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
    auto& manager = DeviceBatteryManager::Instance(index_);
    const auto& value = manager.GetDisplayText();
    manager.RecordRenderedValue(value);
    return value.c_str();
}

const wchar_t* DeviceBatteryItem::GetItemValueSampleText() const
{
    return DeviceBatteryManager::Instance(index_).ShowChargingMarker() ? L"100%+" : L"100%";
}

bool DeviceBatteryItem::IsCustomDraw() const
{
    return DeviceBatteryPlugin::Instance().AlignColumns();
}

int DeviceBatteryItem::GetItemWidthEx(void* dc) const
{
    return DeviceBatteryPlugin::Instance().MeasureColumns(dc).Total();
}

void DeviceBatteryItem::DrawItem(void* dc, int x, int y, int w, int h, bool darkMode)
{
    HDC hdc = static_cast<HDC>(dc);
    if (!hdc || w <= 0 || h <= 0) return;
    const auto& plugin = DeviceBatteryPlugin::Instance();
    const auto columns = plugin.MeasureColumns(dc);
    const int saved = SaveDC(hdc);
    if (saved == 0) return;
    IntersectClipRect(hdc, x, y, x + w, y + h);
    SetBkMode(hdc, TRANSPARENT);
    const UINT format = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX;
    RECT labelRect{ x + columns.gap, y, x + columns.gap + columns.labelWidth, y + h };
    RECT valueRect{ labelRect.right, y, x + w - columns.gap, y + h };
    const std::wstring label = DeviceBatteryManager::Instance(index_).GetLabelText() + L": ";
    SetTextColor(hdc, plugin.LabelColor());
    DrawTextW(hdc, label.c_str(), static_cast<int>(label.size()), &labelRect, format);
    SetTextColor(hdc, plugin.ValueColor());
    DrawTextW(hdc, GetItemValueText(), -1, &valueRect, format);
    RestoreDC(hdc, saved);
}
