#include "pch.h"
#include "DeviceBatteryManager.h"
#include "PluginInterface.h"
#include <iostream>
#include <stdexcept>
#include <cstdio>

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
void WriteHeartbeat(const std::wstring& path, const std::string& observed, const std::string& body) {
    std::string json = "{\"producerPid\":" + std::to_string(GetCurrentProcessId()) +
        ",\"data\":{\"heartbeatIntervalMs\":3000,\"observedAtUtc\":\"" + observed + "\",\"devices\":" + body + "}}";
    Write(path, json.data(), static_cast<DWORD>(json.size()));
}
std::string UtcText() {
    SYSTEMTIME utc{}; GetSystemTime(&utc); char text[32]{};
    sprintf_s(text, "%04u-%02u-%02uT%02u:%02u:%02uZ", utc.wYear, utc.wMonth, utc.wDay, utc.wHour, utc.wMinute, utc.wSecond);
    return text;
}
void SetTime(const std::wstring& path, FILETIME time) {
    HANDLE file = CreateFileW(path.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    Check(file != INVALID_HANDLE_VALUE, "Open fixture attributes");
    Check(SetFileTime(file, nullptr, nullptr, &time) != 0, "Set fixture time"); CloseHandle(file);
}
const std::string devices = R"([{"id":"ae7","name":"AE7","source":"Dareu","battery":100,"isSleeping":true,"status":"offline"},{"id":"cobra","name":"雷蛇 Cobra","source":"Razer","battery":51,"isShownInTray":true},{"id":"mic","name":"PD200W","source":"Maono","battery":60}])";
void CheckBatteryDrawing(ITMPlugin* plugin) {
    for (int fontHeight : {16,24,32}) {
        HDC dc=CreateCompatibleDC(nullptr);
        HFONT font=CreateFontW(-fontHeight,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei");
        HGDIOBJ oldFont=SelectObject(dc,font);
        const int width=plugin->GetItem(0)->GetItemWidthEx(dc), lineHeight=fontHeight+10;
        Check(width>0,"Measure battery cell using the actual host font");
        for(int item=1;item<4;++item)Check(plugin->GetItem(item)->GetItemWidthEx(dc)==width,"All battery rows reserve identical label/value columns");
        const int canvasWidth=2*width+24,canvasHeight=2*lineHeight+8;
        BITMAPINFO bitmapInfo{};bitmapInfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biWidth=canvasWidth;bitmapInfo.bmiHeader.biHeight=-canvasHeight;
        bitmapInfo.bmiHeader.biPlanes=1;bitmapInfo.bmiHeader.biBitCount=32;bitmapInfo.bmiHeader.biCompression=BI_RGB;
        void* rawPixels=nullptr;HBITMAP bitmap=CreateDIBSection(dc,&bitmapInfo,DIB_RGB_COLORS,&rawPixels,nullptr,0);
        HGDIOBJ oldBitmap=SelectObject(dc,bitmap);auto* pixels=static_cast<DWORD*>(rawPixels);
        std::fill(pixels,pixels+canvasWidth*canvasHeight,0);
        plugin->OnExtenedInfo(ITMPlugin::EI_LABEL_TEXT_COLOR,L"255");
        plugin->OnExtenedInfo(ITMPlugin::EI_VALUE_TEXT_COLOR,L"65280");
        const COLORREF original=SetTextColor(dc,RGB(10,20,30));
        int firstValueX=-1;
        for(int item=0;item<4;++item){
            const int x=8+(item/2)*(width+8),y=4+(item%2)*lineHeight;
            plugin->GetItem(item)->DrawItem(dc,x,y,width,lineHeight,true);
            GdiFlush();int minimum=width,maximum=-1;
            for(int py=y;py<y+lineHeight;++py)for(int px=x;px<x+width;++px)
                if((pixels[py*canvasWidth+px]&0xFFFFFF)==0x00FF00){minimum=(std::min)(minimum,px-x);maximum=(std::max)(maximum,px-x);}
            Check(maximum>=minimum,"Battery value is actually rendered");
            if(firstValueX<0)firstValueX=minimum;
            Check(minimum==firstValueX,"100 percent starts at the same pixel for mouse, mic and gamepad");
            Check(maximum<width-2,"100 percent preserves a trailing gap inside its own cell");
        }
        Check(GetTextColor(dc)==RGB(10,20,30),"Plugin restores the host drawing context");SetTextColor(dc,original);
        bool outsideClean=true;
        for(int y=0;y<canvasHeight;++y)for(int x=0;x<canvasWidth;++x){
            const bool inside=(x>=8&&x<8+width||x>=16+width&&x<16+2*width)&&y>=4&&y<4+2*lineHeight;
            if(!inside&&(pixels[y*canvasWidth+x]&0xFFFFFF)!=0)outsideClean=false;
        }
        Check(outsideClean,"Battery drawing cannot enter the adjacent column or clock area");
        if(fontHeight==16){
            const wchar_t* colors[]={L"7915580",L"12753652",L"5551359",L"15853276"};
            std::fill(pixels,pixels+canvasWidth*canvasHeight,0x22252B);
            for(int item=0;item<4;++item){plugin->OnExtenedInfo(ITMPlugin::EI_LABEL_TEXT_COLOR,colors[item]);plugin->OnExtenedInfo(ITMPlugin::EI_VALUE_TEXT_COLOR,colors[item]);
                plugin->GetItem(item)->DrawItem(dc,8+(item/2)*(width+8),4+(item%2)*lineHeight,width,lineHeight,true);}
            GdiFlush();BITMAPFILEHEADER header{};header.bfType=0x4D42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);
            header.bfSize=header.bfOffBits+canvasWidth*canvasHeight*4;FILE* file=nullptr;
            if(fopen_s(&file,"battery-layout-preview.bmp","wb")==0){fwrite(&header,sizeof(header),1,file);fwrite(&bitmapInfo.bmiHeader,sizeof(BITMAPINFOHEADER),1,file);fwrite(pixels,4,canvasWidth*canvasHeight,file);fclose(file);}
        }
        SelectObject(dc,oldBitmap);DeleteObject(bitmap);SelectObject(dc,oldFont);DeleteObject(font);DeleteDC(dc);
    }
}
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
        Check(WritePrivateProfileStringW(L"display", L"show_charging_marker", L"0", exeIni.c_str()) != 0, "Enable compact battery percentages");
        manager.LoadConfig(dir); manager.RefreshIfNeeded();
        Check(manager.GetDisplayText() == L"77%" && manager.GetTooltipText().find(L"charging") != std::wstring::npos, "Compact percent keeps charging information in tooltip");
        Check(WritePrivateProfileStringW(L"display", L"show_charging_marker", L"0", dllIni.c_str()) != 0, "Enable compact plugin layout");
        plugin->OnExtenedInfo(ITMPlugin::EI_CONFIG_DIR, dir.c_str()); plugin->DataRequired();
        Check(std::wstring(plugin->GetItem(0)->GetItemValueSampleText()) == L"100%" && std::wstring(plugin->GetItem(0)->GetItemValueText()) == L"77%", "Narrow sample reserves full percentage without hidden plus width");
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
        WriteJson(snapshot, devices);
        Sleep(550); manager.RefreshIfNeeded();
        Check(manager.GetDisplayText() == L"51%", "Previously missing snapshot recovers without reloading plugin configuration");
        WriteJson(snapshot, R"([{"source":"Razer","battery":88,"status":"online"}])");
        Sleep(550); manager.RefreshIfNeeded();
        Check(manager.GetDisplayText() == L"88%", "Existing reader follows subsequent exported battery changes");
        WriteJson(snapshot, devices, 1073741823);
        Sleep(550); manager.RefreshIfNeeded();
        Check(manager.GetDisplayText() == L"--" && manager.GetTooltipText().find(L"publisher") != std::wstring::npos, "Publisher failure has a useful diagnostic reason");
        WriteJson(snapshot, devices);
        Sleep(550); manager.RefreshIfNeeded();
        Check(manager.GetDisplayText() == L"51%", "Reader recovers from publisher restart without plugin restart");
        WriteHeartbeat(snapshot, "2000-01-01T00:00:00.0000000Z", devices);
        manager.LoadConfig(dir); manager.RefreshIfNeeded();
        Check(manager.GetDisplayText() == L"--" && manager.GetTooltipText().find(L"observations") != std::wstring::npos,
            "A fresh heartbeat cannot make expired hardware observations current");
        WriteHeartbeat(snapshot, "invalid", devices);
        manager.LoadConfig(dir); manager.RefreshIfNeeded();
        Check(manager.GetDisplayText() == L"--", "Malformed observation timestamps are unavailable");
        WriteHeartbeat(snapshot, UtcText(), devices);
        Sleep(550); manager.RefreshIfNeeded();
        Check(manager.GetDisplayText() == L"51%", "Fresh heartbeat and observations recover without reloading");
        GetSystemTimeAsFileTime(&now); time.LowPart=now.dwLowDateTime;time.HighPart=now.dwHighDateTime;
        time.QuadPart -= 20ULL*10000000ULL; past={time.LowPart,time.HighPart};SetTime(snapshot,past);
        manager.LoadConfig(dir);manager.RefreshIfNeeded();
        Check(manager.GetDisplayText() == L"--" && manager.GetTooltipText().find(L"heartbeat expired") != std::wstring::npos,
            "Stalled heartbeat is rejected before the hardware observation timeout");
        WriteHeartbeat(snapshot, UtcText(), devices);
        Sleep(550);manager.RefreshIfNeeded();
        Check(manager.GetDisplayText() == L"51%", "Heartbeat restoration reconnects the existing reader");
        Check(WritePrivateProfileStringW(L"api", L"diagnostics", L"1", exeIni.c_str()) != 0, "Enable diagnostics while running");
        Sleep(550);manager.RefreshIfNeeded();
        std::wstring diagnostic = exeIni + L".0.poll.txt";
        WIN32_FILE_ATTRIBUTE_DATA before{}, after{};
        Check(GetFileAttributesExW(diagnostic.c_str(), GetFileExInfoStandard, &before) != 0, "Diagnostics enable without restarting the host");
        Check(WritePrivateProfileStringW(L"api", L"diagnostics", L"0", exeIni.c_str()) != 0, "Disable diagnostics while running");
        Sleep(550);manager.RefreshIfNeeded();
        Check(GetFileAttributesExW(diagnostic.c_str(), GetFileExInfoStandard, &after) != 0 &&
            CompareFileTime(&before.ftLastWriteTime, &after.ftLastWriteTime) == 0, "Diagnostics stop without restarting the host");
        DeleteFileW(diagnostic.c_str());
        Check(WritePrivateProfileStringW(L"display", L"item_count", L"4", dllIni.c_str()) != 0, "Add fourth device");
        Check(WritePrivateProfileStringW(L"display", L"align_columns", L"1", dllIni.c_str()) != 0, "Enable fixed battery column geometry");
        Check(WritePrivateProfileStringW(L"device2", L"label_text", L"MIC", dllIni.c_str()) != 0, "Use the compact microphone label");
        Check(WritePrivateProfileStringW(L"device3", L"source", L"Flydigi", dllIni.c_str()) != 0, "Select gamepad by source");
        Check(WritePrivateProfileStringW(L"device3", L"label_text", L"手柄", dllIni.c_str()) != 0, "Configure short gamepad label");
        const std::string fourDevices=R"([{"source":"Razer","battery":100},{"source":"Dareu","battery":100},{"source":"Maono","battery":100},{"source":"Flydigi","battery":100}])";
        WriteJson(snapshot,fourDevices);plugin->OnExtenedInfo(ITMPlugin::EI_CONFIG_DIR,dir.c_str());plugin->DataRequired();
        Check(plugin->GetItem(3) && std::wstring(plugin->GetItem(3)->GetItemId()) == L"Device24GBattery_4" && plugin->GetItem(4)==nullptr,
            "Gamepad occupies the old empty lower row without moving hardware items");
        Check(std::wstring(plugin->GetItem(3)->GetItemValueText()) == L"100%", "Gamepad receives its own battery value");
        Check(plugin->GetItem(0)->IsCustomDraw() && plugin->GetItem(3)->IsCustomDraw(), "Opt-in battery layout uses measured columns instead of whitespace padding");
        CheckBatteryDrawing(plugin);
        WriteJson(snapshot,R"([{"source":"Razer","battery":9},{"source":"Dareu","battery":100},{"source":"Maono","battery":59},{"source":"Flydigi","battery":null}])");
        plugin->OnExtenedInfo(ITMPlugin::EI_CONFIG_DIR,dir.c_str());plugin->DataRequired();
        Check(std::wstring(plugin->GetItem(0)->GetItemValueText())==L"9%"&&std::wstring(plugin->GetItem(1)->GetItemValueText())==L"100%"&&
            std::wstring(plugin->GetItem(2)->GetItemValueText())==L"59%"&&std::wstring(plugin->GetItem(3)->GetItemValueText())==L"--","Mixed-width values and unknown gamepad remain independent");
        Check(WritePrivateProfileStringW(L"display",L"align_columns",L"0",dllIni.c_str())!=0,"Disable optional battery layout");
        plugin->OnExtenedInfo(ITMPlugin::EI_CONFIG_DIR,dir.c_str());
        Check(!plugin->GetItem(0)->IsCustomDraw(),"Original host-rendered layout remains the default");
        std::cout << "PASS: " << checks << " battery integration checks\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        if(library)FreeLibrary(library);DeleteFileW(snapshot.c_str());DeleteFileW(exeIni.c_str());DeleteFileW(dllIni.c_str());RemoveDirectoryW(dir.c_str());return 1;
    }
    if(library)FreeLibrary(library);DeleteFileW(snapshot.c_str());DeleteFileW(exeIni.c_str());DeleteFileW(dllIni.c_str());RemoveDirectoryW(dir.c_str());return 0;
}
