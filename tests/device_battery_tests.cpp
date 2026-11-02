#include "pch.h"
#include "DeviceBatteryManager.h"
#include "PluginInterface.h"
#include <iostream>
#include <stdexcept>

namespace {
int checks = 0;
void Check(bool ok, const char* name) { if (!ok) throw std::runtime_error(name); ++checks; }
void Write(const std::wstring& path, const void* bytes, DWORD size) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create fixture");
    DWORD written = 0; BOOL ok = WriteFile(file, bytes, size, &written, nullptr); CloseHandle(file);
    Check(ok && written == size, "Fixture write");
}
void WriteJson(const std::wstring& path, const std::string& body, DWORD pid = GetCurrentProcessId()) {
    std::string json = "{\"producerPid\":" + std::to_string(pid) + ",\"data\":{\"devices\":" + body + "}}";
    Write(path, json.data(), static_cast<DWORD>(json.size()));
}
void SetTime(const std::wstring& path, FILETIME time) {
    HANDLE file = CreateFileW(path.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    Check(file != INVALID_HANDLE_VALUE, "Open fixture attributes");
    Check(SetFileTime(file, nullptr, nullptr, &time) != 0, "Set fixture time"); CloseHandle(file);
}
const std::string devices = R"([{"id":"ae7","name":"AE7","source":"Dareu","battery":100,"isSleeping":true,"status":"offline"},{"id":"cobra","name":"雷蛇 Cobra","source":"Razer","battery":51,"isShownInTray":true},{"id":"mic","name":"PD200W","source":"Maono","battery":60}])";
}
int wmain(int argc, wchar_t** argv) {
    if (argc == 3 && std::wstring(argv[1]) == L"--live") {
        HMODULE module = LoadLibraryW(L"DeviceBatteryPlugin.dll");
        if (!module) return 2;
        auto factory = reinterpret_cast<ITMPlugin*(*)()>(GetProcAddress(module, "TMPluginGetInstance"));
        if (!factory) { FreeLibrary(module); return 3; }
        ITMPlugin* plugin = factory();
        plugin->OnExtenedInfo(ITMPlugin::EI_CONFIG_DIR, argv[2]);
        plugin->DataRequired();
        int itemCount = 0;
        for (int i = 0; auto* item = plugin->GetItem(i); ++i) {
            std::wstring value(item->GetItemValueText());
            std::wstring line = std::wstring(item->GetItemLableText()) + L": " + value + L"\n";
            int count = WideCharToMultiByte(CP_UTF8, 0, line.data(), static_cast<int>(line.size()), nullptr, 0, nullptr, nullptr);
            std::string utf8(count, '\0');
            WideCharToMultiByte(CP_UTF8, 0, line.data(), static_cast<int>(line.size()), &utf8[0], count, nullptr, nullptr);
            std::cout << utf8;
            ++itemCount;
        }
        FreeLibrary(module);
        return itemCount > 0 ? 0 : 4;
    }
    wchar_t temporary[MAX_PATH]{}; GetTempPathW(MAX_PATH, temporary);
    std::wstring dir = std::wstring(temporary) + L"TrafficMonitorBatteryTest-" + std::to_wstring(GetCurrentProcessId()) + L"\\";
    CreateDirectoryW(dir.c_str(), nullptr);
    std::wstring snapshot = dir + L"battery.json";
    std::wstring exeIni = dir + L"device_battery_tests.exe.ini";
    std::wstring dllIni = dir + L"DeviceBatteryPlugin.dll.ini";
    HMODULE library = nullptr;
    try {
        DeviceBatteryConfig config; DeviceBatteryInfo result; std::wstring error;
        std::string json = "{\"data\":{\"devices\":" + devices + "}}";
        config.source = L"Razer";
        Check(DeviceBatteryManager::ParseAndSelectDevice(json, config, result, error) && result.id == L"cobra" && result.battery == 51, "Select Razer by source");
        config.source = L"dareu";
        Check(DeviceBatteryManager::ParseAndSelectDevice(json, config, result, error) && result.id == L"ae7" && result.isSleeping, "Case insensitive source and cached sleep");
        config.source = L"Maono";
        Check(DeviceBatteryManager::ParseAndSelectDevice(json, config, result, error) && result.battery == 60, "Independent microphone selection");
        config.deviceId = L"missing";
        Check(!DeviceBatteryManager::ParseAndSelectDevice(json, config, result, error), "Missing explicit ID cannot select another device");
        config.deviceId = L"cobra"; config.deviceName = L"AE7";
        Check(DeviceBatteryManager::ParseAndSelectDevice(json, config, result, error) && result.id == L"cobra", "ID takes priority");
        config = DeviceBatteryConfig{}; config.deviceName = L"missing";
        Check(!DeviceBatteryManager::ParseAndSelectDevice(json, config, result, error), "Missing name cannot select another mouse");
        config.deviceName = L"ae7";
        Check(DeviceBatteryManager::ParseAndSelectDevice(json, config, result, error) && result.id == L"ae7", "Case insensitive name match");
        config = DeviceBatteryConfig{};
        Check(DeviceBatteryManager::ParseAndSelectDevice(json, config, result, error) && result.id == L"cobra", "Legacy tray preference");
        config.preferTrayDevice = false;
        Check(DeviceBatteryManager::ParseAndSelectDevice(json, config, result, error) && result.id == L"ae7", "Legacy first battery selection");
        for (const auto& test : std::vector<std::pair<std::string, bool>>{{"0",true},{"100",true},{"null",false},{"101",false},{"-1",false},{"12abc",false},{"\"12\"",false}}) {
            std::string one = "{\"devices\":[{\"id\":\"test\",\"battery\":" + test.first + "}]}";
            result = DeviceBatteryInfo{};
            Check(DeviceBatteryManager::ParseAndSelectDevice(one, config, result, error) && result.hasBattery == test.second, "Battery boundaries and null semantics");
        }
        Check(!DeviceBatteryManager::ParseAndSelectDevice("{}", config, result, error), "Missing devices field rejected");
        Check(!DeviceBatteryManager::ParseAndSelectDevice("{\"devices\":[]}", config, result, error), "Empty source rejected");

        std::wstring settings = L"\xFEFF[api]\r\nsnapshot_file=" + snapshot + L"\r\nrefresh_interval_ms=500\r\nmax_snapshot_age_seconds=900\r\n[display]\r\nitem_count=3\r\n[device0]\r\nsource=Razer\r\nlabel_text=雷蛇\r\n[device1]\r\nsource=Dareu\r\nlabel_text=AE7\r\n[device2]\r\nsource=Maono\r\nlabel_text=PD200W\r\n";
        Write(exeIni, settings.data(), static_cast<DWORD>(settings.size() * sizeof(wchar_t)));
        Write(dllIni, settings.data(), static_cast<DWORD>(settings.size() * sizeof(wchar_t)));
        WriteJson(snapshot, devices);
        auto& manager = DeviceBatteryManager::Instance(); manager.LoadConfig(dir); manager.RefreshIfNeeded();
        Check(manager.GetDisplayText() == L"51%", "Read fresh local snapshot");
        Check(manager.GetLabelText() == L"雷蛇", "UTF16 plugin configuration labels");
        library = LoadLibraryW(L"DeviceBatteryPlugin.dll"); Check(library != nullptr, "Load built plugin");
        auto factory = reinterpret_cast<ITMPlugin*(*)()>(GetProcAddress(library, "TMPluginGetInstance"));
        Check(factory != nullptr, "Plugin export ABI"); ITMPlugin* plugin = factory();
        plugin->OnExtenedInfo(ITMPlugin::EI_CONFIG_DIR, dir.c_str()); plugin->DataRequired();
        Check(std::wstring(plugin->GetItem(0)->GetItemId()) == L"Device24GBattery", "Original item ID preserved");
        Check(std::wstring(plugin->GetItem(1)->GetItemId()) == L"Device24GBattery_2" && std::wstring(plugin->GetItem(2)->GetItemId()) == L"Device24GBattery_3", "Unique per-device item IDs");
        Check(plugin->GetItem(3) == nullptr && plugin->GetItem(-1) == nullptr, "Plugin item bounds");
        Check(std::wstring(plugin->GetItem(0)->GetItemValueText()) == L"51%" && std::wstring(plugin->GetItem(1)->GetItemValueText()) == L"--" && std::wstring(plugin->GetItem(2)->GetItemValueText()) == L"60%", "Three independent batteries; sleeping device is unavailable");

        Check(WritePrivateProfileStringW(L"display", L"trailing_spacer", L"1", dllIni.c_str()) != 0, "Enable optional layout spacer");
        plugin->OnExtenedInfo(ITMPlugin::EI_CONFIG_DIR, dir.c_str()); plugin->DataRequired();
        auto* spacer = plugin->GetItem(3);
        Check(spacer && std::wstring(spacer->GetItemId()) == L"Device24GBatterySpacer" && plugin->GetItem(4) == nullptr, "Spacer appended without changing existing battery IDs");
        Check(std::wstring(spacer->GetItemLableText()).empty() && std::wstring(spacer->GetItemValueText()).empty() && std::wstring(spacer->GetItemValueSampleText()).empty(), "Spacer reserves no text width and draws no fake reading");
        Check(std::wstring(plugin->GetItem(2)->GetItemValueText()) == L"60%", "Spacer does not affect microphone data");
        Check(WritePrivateProfileStringW(L"display", L"trailing_spacer", L"0", dllIni.c_str()) != 0, "Disable optional layout spacer");
        plugin->OnExtenedInfo(ITMPlugin::EI_CONFIG_DIR, dir.c_str());
        Check(plugin->GetItem(3) == nullptr, "Spacer defaults remain backwards compatible");

        WriteJson(snapshot, R"([{"source":"Razer","battery":77,"isCharging":true}])");
        manager.LoadConfig(dir); manager.RefreshIfNeeded(); Check(manager.GetDisplayText() == L"77%+", "Charging indication");
        WriteJson(snapshot, R"([{"source":"Razer","battery":66,"status":"offline"}])");
        manager.LoadConfig(dir); manager.RefreshIfNeeded(); Check(manager.GetDisplayText() == L"--", "Offline historical battery is not displayed as current data");
        WriteJson(snapshot, R"([{"source":"Razer","battery":null}])");
        manager.LoadConfig(dir); manager.RefreshIfNeeded(); Check(manager.GetDisplayText() == L"--", "Unknown value clears old percent");
        WriteJson(snapshot, devices, 0); manager.LoadConfig(dir); manager.RefreshIfNeeded();
        Check(manager.GetDisplayText() == L"--", "Missing producer process rejected");
        WriteJson(snapshot, devices, 1073741823); manager.LoadConfig(dir); manager.RefreshIfNeeded();
        Check(manager.GetDisplayText() == L"--", "Dead producer rejected");
        WriteJson(snapshot, devices);
        FILETIME now{}; GetSystemTimeAsFileTime(&now); ULARGE_INTEGER time{}; time.LowPart=now.dwLowDateTime; time.HighPart=now.dwHighDateTime; time.QuadPart -= 1800ULL*10000000ULL; FILETIME past{time.LowPart,time.HighPart}; SetTime(snapshot,past);
        manager.LoadConfig(dir); manager.RefreshIfNeeded(); Check(manager.GetDisplayText() == L"--", "Expired file rejected");
        WriteJson(snapshot, devices);
        FILETIME created{}, exited{}, kernel{}, user{}; GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user); time.LowPart=created.dwLowDateTime;time.HighPart=created.dwHighDateTime;--time.QuadPart;past={time.LowPart,time.HighPart};SetTime(snapshot,past);
        manager.LoadConfig(dir); manager.RefreshIfNeeded(); Check(manager.GetDisplayText() == L"--", "PID reuse cannot validate an older snapshot");
        WriteJson(snapshot, devices); manager.LoadConfig(dir); manager.RefreshIfNeeded(); Check(manager.GetDisplayText()==L"51%", "Recovers after valid publication");
        DeleteFileW(snapshot.c_str()); manager.LoadConfig(dir);manager.RefreshIfNeeded();Check(manager.GetDisplayText()==L"--", "Provider removal clears stale display");
        std::cout << "PASS: " << checks << " battery integration checks\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        if(library)FreeLibrary(library);DeleteFileW(snapshot.c_str());DeleteFileW(exeIni.c_str());DeleteFileW(dllIni.c_str());RemoveDirectoryW(dir.c_str());return 1;
    }
    if(library)FreeLibrary(library);DeleteFileW(snapshot.c_str());DeleteFileW(exeIni.c_str());DeleteFileW(dllIni.c_str());RemoveDirectoryW(dir.c_str());return 0;
}
