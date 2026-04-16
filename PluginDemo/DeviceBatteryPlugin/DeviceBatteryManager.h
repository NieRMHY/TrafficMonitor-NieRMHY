#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct DeviceBatteryConfig
{
    std::wstring host{ L"127.0.0.1" };
    std::wstring endpoint{ L"/api/v1/status" };
    std::wstring token{};
    std::wstring deviceId{};
    std::wstring deviceName{};
    std::wstring labelText{ L"2.4G" };
    int port{ 18080 };
    int timeoutMs{ 1500 };
    int refreshIntervalMs{ 3000 };
    bool preferTrayDevice{ true };
};

struct DeviceBatteryInfo
{
    std::wstring id{};
    std::wstring name{};
    std::wstring renamedName{};
    int battery{ -1 };
    bool hasBattery{ false };
    bool isCharging{ false };
    bool isSleeping{ false };
    bool isBatteryUnsupported{ false };
    bool isShownInTray{ false };
};

class DeviceBatteryManager
{
public:
    static DeviceBatteryManager& Instance();
    static std::wstring Utf8ToWide(const std::string& text);

    void LoadConfig(const std::wstring& configDir);
    void RefreshIfNeeded();

    const std::wstring& GetDisplayText() const;
    const std::wstring& GetTooltipText() const;
    const std::wstring& GetLabelText() const;

private:
    DeviceBatteryManager() = default;

    bool FetchStatusJson(std::string& jsonText) const;
    bool ParseAndSelectDevice(const std::string& jsonText, DeviceBatteryInfo& selectedInfo, std::wstring& errorText) const;

    void UpdateDisplay(const DeviceBatteryInfo& selectedInfo);
    static std::wstring GetModuleFileNameOnly();
    static std::wstring BuildConfigPath(const std::wstring& configDir);

private:
    DeviceBatteryConfig config_{};
    std::wstring configPath_{};
    std::wstring displayText_{ L"--" };
    std::wstring tooltipText_{ L"Device battery: --" };
    std::uint64_t lastRequestTick_{ 0 };
};
