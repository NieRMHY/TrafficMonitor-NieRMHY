#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

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
    bool showChargingMarker{ true };
    bool diagnostics{ false };
    std::wstring source{};
    std::wstring snapshotFile{};
    int maxSnapshotAgeSeconds{ 900 };
    int maxHeartbeatAgeSeconds{ 15 };
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
    std::wstring source{};
    std::wstring status{ L"online" };
};

class DeviceBatteryManager
{
public:
    static constexpr int MaxItems = 8;
    static DeviceBatteryManager& Instance(int index = 0);
    static std::wstring Utf8ToWide(const std::string& text);

    void LoadConfig(const std::wstring& configDir, int index = 0);
    void RefreshIfNeeded();

    const std::wstring& GetDisplayText() const;
    const std::wstring& GetTooltipText() const;
    const std::wstring& GetLabelText() const;
    bool ShowChargingMarker() const { return config_.showChargingMarker; }
    void RecordRenderedValue(const std::wstring& value) const;
    static bool ParseAndSelectDevice(const std::string& jsonText, const DeviceBatteryConfig& config, DeviceBatteryInfo& selectedInfo, std::wstring& errorText);

private:
    DeviceBatteryManager() = default;

    bool FetchStatusJson(std::string& jsonText) const;
    bool ReadSnapshotFile(std::string& jsonText) const;
    void SetUnavailable(const std::wstring& reason);
    void WriteDiagnostic(const wchar_t* stage, const std::wstring& value, const std::wstring& detail) const;

    void UpdateDisplay(const DeviceBatteryInfo& selectedInfo);
    static std::wstring GetModuleFileNameOnly();
    static std::wstring BuildConfigPath(const std::wstring& configDir);

private:
    DeviceBatteryConfig config_{};
    std::wstring configPath_{};
    std::wstring displayText_{ L"--" };
    std::wstring tooltipText_{ L"Device battery: --" };
    std::uint64_t lastRequestTick_{ 0 };
    mutable std::mutex textMutex_;
    mutable std::wstring sourceError_;
    int itemIndex_{};
    std::atomic<bool> diagnosticsEnabled_{ false };
};
