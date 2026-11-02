#include "pch.h"
#include "DeviceBatteryManager.h"

#include <sstream>

namespace
{
extern "C" IMAGE_DOS_HEADER __ImageBase;

std::wstring Trim(const std::wstring& text)
{
    const wchar_t* spaces = L" \t\r\n";
    const size_t begin = text.find_first_not_of(spaces);
    if (begin == std::wstring::npos)
    {
        return L"";
    }
    const size_t end = text.find_last_not_of(spaces);
    return text.substr(begin, end - begin + 1);
}

std::wstring ToLower(const std::wstring& text)
{
    std::wstring result = text;
    std::transform(result.begin(), result.end(), result.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(towlower(ch));
    });
    return result;
}

bool ParseJsonStringLiteral(const std::string& text, size_t beginPos, size_t& endPos, std::string& value)
{
    if (beginPos >= text.size() || text[beginPos] != '"')
    {
        return false;
    }

    std::string output;
    bool escaped = false;
    for (size_t i = beginPos + 1; i < text.size(); ++i)
    {
        const char ch = text[i];
        if (escaped)
        {
            switch (ch)
            {
            case '"': output.push_back('"'); break;
            case '\\': output.push_back('\\'); break;
            case '/': output.push_back('/'); break;
            case 'b': output.push_back('\b'); break;
            case 'f': output.push_back('\f'); break;
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            default: output.push_back(ch); break;
            }
            escaped = false;
            continue;
        }

        if (ch == '\\')
        {
            escaped = true;
            continue;
        }
        if (ch == '"')
        {
            endPos = i + 1;
            value = output;
            return true;
        }
        output.push_back(ch);
    }
    return false;
}

bool ExtractJsonValueRaw(const std::string& objectText, const std::string& key, std::string& valueRaw)
{
    const std::string marker = "\"" + key + "\"";
    const size_t keyPos = objectText.find(marker);
    if (keyPos == std::string::npos)
    {
        return false;
    }

    size_t cursor = keyPos + marker.size();
    while (cursor < objectText.size() && std::isspace(static_cast<unsigned char>(objectText[cursor])) != 0)
    {
        ++cursor;
    }
    if (cursor >= objectText.size() || objectText[cursor] != ':')
    {
        return false;
    }
    ++cursor;
    while (cursor < objectText.size() && std::isspace(static_cast<unsigned char>(objectText[cursor])) != 0)
    {
        ++cursor;
    }
    if (cursor >= objectText.size())
    {
        return false;
    }

    if (objectText[cursor] == '"')
    {
        size_t valueEnd = cursor;
        std::string parsedString;
        if (!ParseJsonStringLiteral(objectText, cursor, valueEnd, parsedString))
        {
            return false;
        }
        valueRaw = objectText.substr(cursor, valueEnd - cursor);
        return true;
    }

    size_t valueEnd = cursor;
    while (valueEnd < objectText.size())
    {
        const char ch = objectText[valueEnd];
        if (ch == ',' || ch == '}' || ch == ']')
        {
            break;
        }
        ++valueEnd;
    }
    valueRaw = objectText.substr(cursor, valueEnd - cursor);
    return true;
}

bool GetJsonString(const std::string& objectText, const std::string& key, std::wstring& outValue, bool& isNull)
{
    std::string rawValue;
    if (!ExtractJsonValueRaw(objectText, key, rawValue))
    {
        return false;
    }

    const std::wstring trimmed = Trim(DeviceBatteryManager::Utf8ToWide(rawValue));
    if (trimmed == L"null")
    {
        isNull = true;
        outValue.clear();
        return true;
    }

    if (rawValue.size() < 2 || rawValue.front() != '"' || rawValue.back() != '"')
    {
        return false;
    }

    size_t endPos = 0;
    std::string unescaped;
    if (!ParseJsonStringLiteral(rawValue, 0, endPos, unescaped))
    {
        return false;
    }

    outValue = DeviceBatteryManager::Utf8ToWide(unescaped);
    isNull = false;
    return true;
}

bool GetJsonInt(const std::string& objectText, const std::string& key, int& outValue, bool& isNull)
{
    std::string rawValue;
    if (!ExtractJsonValueRaw(objectText, key, rawValue))
    {
        return false;
    }

    const std::wstring trimmed = Trim(DeviceBatteryManager::Utf8ToWide(rawValue));
    if (trimmed == L"null")
    {
        isNull = true;
        return true;
    }

    try
    {
        size_t used = 0;
        outValue = std::stoi(trimmed, &used);
        if (used != trimmed.size()) return false;
        isNull = false;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool GetJsonBool(const std::string& objectText, const std::string& key, bool& outValue)
{
    std::string rawValue;
    if (!ExtractJsonValueRaw(objectText, key, rawValue))
    {
        return false;
    }
    const std::wstring trimmed = ToLower(Trim(DeviceBatteryManager::Utf8ToWide(rawValue)));
    if (trimmed == L"true")
    {
        outValue = true;
        return true;
    }
    if (trimmed == L"false")
    {
        outValue = false;
        return true;
    }
    return false;
}

bool ExtractDevicesArray(const std::string& jsonText, std::string& devicesArray)
{
    const std::string marker = "\"devices\"";
    const size_t markerPos = jsonText.find(marker);
    if (markerPos == std::string::npos)
    {
        return false;
    }

    size_t cursor = markerPos + marker.size();
    while (cursor < jsonText.size() && std::isspace(static_cast<unsigned char>(jsonText[cursor])) != 0)
    {
        ++cursor;
    }
    if (cursor >= jsonText.size() || jsonText[cursor] != ':')
    {
        return false;
    }

    ++cursor;
    while (cursor < jsonText.size() && std::isspace(static_cast<unsigned char>(jsonText[cursor])) != 0)
    {
        ++cursor;
    }
    if (cursor >= jsonText.size() || jsonText[cursor] != '[')
    {
        return false;
    }

    size_t depth = 0;
    bool inString = false;
    bool escaped = false;
    for (size_t i = cursor; i < jsonText.size(); ++i)
    {
        const char ch = jsonText[i];
        if (inString)
        {
            if (escaped)
            {
                escaped = false;
            }
            else if (ch == '\\')
            {
                escaped = true;
            }
            else if (ch == '"')
            {
                inString = false;
            }
            continue;
        }

        if (ch == '"')
        {
            inString = true;
            continue;
        }
        if (ch == '[')
        {
            ++depth;
            continue;
        }
        if (ch == ']')
        {
            --depth;
            if (depth == 0)
            {
                devicesArray = jsonText.substr(cursor, i - cursor + 1);
                return true;
            }
        }
    }

    return false;
}

std::vector<std::string> SplitDeviceObjects(const std::string& devicesArray)
{
    std::vector<std::string> objects;

    size_t depth = 0;
    bool inString = false;
    bool escaped = false;
    size_t objectBegin = 0;

    for (size_t i = 0; i < devicesArray.size(); ++i)
    {
        const char ch = devicesArray[i];
        if (inString)
        {
            if (escaped)
            {
                escaped = false;
            }
            else if (ch == '\\')
            {
                escaped = true;
            }
            else if (ch == '"')
            {
                inString = false;
            }
            continue;
        }

        if (ch == '"')
        {
            inString = true;
            continue;
        }

        if (ch == '{')
        {
            if (depth == 0)
            {
                objectBegin = i;
            }
            ++depth;
            continue;
        }

        if (ch == '}')
        {
            if (depth == 0)
            {
                continue;
            }
            --depth;
            if (depth == 0)
            {
                objects.emplace_back(devicesArray.substr(objectBegin, i - objectBegin + 1));
            }
        }
    }

    return objects;
}

bool ParseSingleDevice(const std::string& objectText, DeviceBatteryInfo& info)
{
    bool isNull = false;
    GetJsonString(objectText, "id", info.id, isNull);
    GetJsonString(objectText, "name", info.name, isNull);
    GetJsonString(objectText, "renamedName", info.renamedName, isNull);

    int batteryValue = -1;
    if (GetJsonInt(objectText, "battery", batteryValue, isNull))
    {
        if (!isNull && batteryValue >= 0 && batteryValue <= 100)
        {
            info.hasBattery = true;
            info.battery = batteryValue;
        }
    }

    GetJsonBool(objectText, "isCharging", info.isCharging);
    GetJsonBool(objectText, "isSleeping", info.isSleeping);
    GetJsonBool(objectText, "isBatteryUnsupported", info.isBatteryUnsupported);
    GetJsonBool(objectText, "isShownInTray", info.isShownInTray);
    GetJsonString(objectText, "source", info.source, isNull);
    GetJsonString(objectText, "status", info.status, isNull);

    return true;
}

bool IsMatchByName(const DeviceBatteryInfo& info, const std::wstring& nameFilter)
{
    if (nameFilter.empty())
    {
        return false;
    }
    const std::wstring normalizedFilter = ToLower(nameFilter);
    const std::wstring renamed = ToLower(info.renamedName);
    const std::wstring rawName = ToLower(info.name);
    return renamed == normalizedFilter || rawName == normalizedFilter;
}

} // namespace

DeviceBatteryManager& DeviceBatteryManager::Instance(int index)
{
    static DeviceBatteryManager instances[MaxItems];
    return instances[(std::max)(0, (std::min)(index, MaxItems - 1))];
}

std::wstring DeviceBatteryManager::Utf8ToWide(const std::string& text)
{
    if (text.empty())
    {
        return L"";
    }

    const int wcharCount = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (wcharCount <= 0)
    {
        return L"";
    }

    std::wstring result(wcharCount, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &result[0], wcharCount);
    return result;
}

std::wstring DeviceBatteryManager::GetModuleFileNameOnly()
{
    wchar_t modulePath[MAX_PATH]{};
    const HMODULE moduleHandle = reinterpret_cast<HMODULE>(&__ImageBase);
    GetModuleFileNameW(moduleHandle, modulePath, MAX_PATH);
    std::wstring fullPath = modulePath;
    const size_t separatorPos = fullPath.find_last_of(L"\\/");
    if (separatorPos == std::wstring::npos)
    {
        return fullPath;
    }
    return fullPath.substr(separatorPos + 1);
}

std::wstring DeviceBatteryManager::BuildConfigPath(const std::wstring& configDir)
{
    std::wstring result;
    if (!configDir.empty())
    {
        result = configDir;
        if (result.back() != L'\\' && result.back() != L'/')
        {
            result.push_back(L'\\');
        }
    }
    result += GetModuleFileNameOnly();
    result += L".ini";
    return result;
}

void DeviceBatteryManager::LoadConfig(const std::wstring& configDir, int index)
{
    configPath_ = BuildConfigPath(configDir);
    config_ = DeviceBatteryConfig{};
    lastRequestTick_ = 0;

    wchar_t buffer[512]{};
    GetPrivateProfileStringW(L"api", L"host", config_.host.c_str(), buffer, static_cast<DWORD>(std::size(buffer)), configPath_.c_str());
    config_.host = Trim(buffer);
    if (config_.host.empty())
    {
        config_.host = L"127.0.0.1";
    }

    config_.port = GetPrivateProfileIntW(L"api", L"port", 18080, configPath_.c_str());
    config_.timeoutMs = GetPrivateProfileIntW(L"api", L"timeout_ms", 1500, configPath_.c_str());
    config_.refreshIntervalMs = GetPrivateProfileIntW(L"api", L"refresh_interval_ms", 3000, configPath_.c_str());
    config_.preferTrayDevice = GetPrivateProfileIntW(L"api", L"prefer_tray_device", 1, configPath_.c_str()) != 0;

    GetPrivateProfileStringW(L"api", L"token", L"", buffer, static_cast<DWORD>(std::size(buffer)), configPath_.c_str());
    config_.token = Trim(buffer);

    GetPrivateProfileStringW(L"api", L"device_id", L"", buffer, static_cast<DWORD>(std::size(buffer)), configPath_.c_str());
    config_.deviceId = Trim(buffer);

    GetPrivateProfileStringW(L"api", L"device_name", L"", buffer, static_cast<DWORD>(std::size(buffer)), configPath_.c_str());
    config_.deviceName = Trim(buffer);

    GetPrivateProfileStringW(L"api", L"endpoint", config_.endpoint.c_str(), buffer, static_cast<DWORD>(std::size(buffer)), configPath_.c_str());
    config_.endpoint = Trim(buffer);
    if (config_.endpoint.empty())
    {
        config_.endpoint = L"/api/v1/status";
    }

    GetPrivateProfileStringW(L"display", L"label_text", config_.labelText.c_str(), buffer, static_cast<DWORD>(std::size(buffer)), configPath_.c_str());
    config_.labelText = Trim(buffer);
    if (config_.labelText.empty())
    {
        config_.labelText = L"2.4G";
    }

    GetPrivateProfileStringW(L"api", L"snapshot_file", L"", buffer, static_cast<DWORD>(std::size(buffer)), configPath_.c_str());
    if (buffer[0] != L'\0')
    {
        wchar_t expanded[32768]{};
        DWORD length = ExpandEnvironmentStringsW(buffer, expanded, static_cast<DWORD>(std::size(expanded)));
        if (length == 0 || length > std::size(expanded)) config_.snapshotFile = buffer;
        else config_.snapshotFile = expanded;
    }
    config_.maxSnapshotAgeSeconds = (std::max)(30, (std::min)(3600,
        static_cast<int>(GetPrivateProfileIntW(L"api", L"max_snapshot_age_seconds", 900, configPath_.c_str()))));

    const std::wstring section = L"device" + std::to_wstring(index);
    auto readSelection = [&](const wchar_t* key, std::wstring& value) {
        GetPrivateProfileStringW(section.c_str(), key, value.c_str(), buffer, static_cast<DWORD>(std::size(buffer)), configPath_.c_str());
        value = Trim(buffer);
    };
    readSelection(L"device_id", config_.deviceId);
    readSelection(L"device_name", config_.deviceName);
    readSelection(L"source", config_.source);
    readSelection(L"label_text", config_.labelText);
}

bool DeviceBatteryManager::ReadSnapshotFile(std::string& jsonText) const
{
    HANDLE file = CreateFileW(config_.snapshotFile.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    FILETIME modified{}, now{};
    bool ok = GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart <= 262144 &&
        GetFileTime(file, nullptr, nullptr, &modified);
    GetSystemTimeAsFileTime(&now);
    auto ticks = [](FILETIME ft) { ULARGE_INTEGER t{}; t.LowPart = ft.dwLowDateTime; t.HighPart = ft.dwHighDateTime; return t.QuadPart; };
    if (ok && ticks(now) > ticks(modified) + static_cast<ULONGLONG>(config_.maxSnapshotAgeSeconds) * 10000000ULL) ok = false;
    if (ok)
    {
        jsonText.resize(static_cast<size_t>(size.QuadPart));
        DWORD read = 0;
        ok = ReadFile(file, &jsonText[0], static_cast<DWORD>(jsonText.size()), &read, nullptr) && read == jsonText.size();
    }
    CloseHandle(file);
    if (!ok) return false;
    int pid = 0;
    bool isNull = false;
    if (!GetJsonInt(jsonText, "producerPid", pid, isNull) || isNull || pid <= 0) return false;
    HANDLE producer = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
    if (producer == nullptr) return false;
    FILETIME created{}, exited{}, kernel{}, user{};
    ok = WaitForSingleObject(producer, 0) == WAIT_TIMEOUT &&
        GetProcessTimes(producer, &created, &exited, &kernel, &user) && ticks(created) <= ticks(modified);
    CloseHandle(producer);
    return ok;
}

bool DeviceBatteryManager::FetchStatusJson(std::string& jsonText) const
{
    if (!config_.snapshotFile.empty()) return ReadSnapshotFile(jsonText);
    HINTERNET session = WinHttpOpen(L"TrafficMonitorDeviceBatteryPlugin/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == nullptr)
    {
        return false;
    }

    DWORD timeout = static_cast<DWORD>((std::max)(config_.timeoutMs, 500));
    WinHttpSetTimeouts(session, timeout, timeout, timeout, timeout);

    HINTERNET connection = WinHttpConnect(session, config_.host.c_str(), static_cast<INTERNET_PORT>(config_.port), 0);
    if (connection == nullptr)
    {
        WinHttpCloseHandle(session);
        return false;
    }

    HINTERNET request = WinHttpOpenRequest(connection, L"GET", config_.endpoint.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (request == nullptr)
    {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    std::wstring headers;
    if (!config_.token.empty())
    {
        headers = L"X-Api-Token: " + config_.token + L"\r\n";
    }

    BOOL sendResult = WinHttpSendRequest(
        request,
        headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str(),
        headers.empty() ? 0 : static_cast<DWORD>(headers.length()),
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0);

    bool success = false;
    if (sendResult == TRUE && WinHttpReceiveResponse(request, nullptr) == TRUE)
    {
        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);
        if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX) == TRUE && statusCode == 200)
        {
            std::string payload;
            DWORD availableSize = 0;
            while (WinHttpQueryDataAvailable(request, &availableSize) == TRUE && availableSize > 0)
            {
                if (availableSize > 262144 || payload.size() > 262144 - availableSize) { payload.clear(); break; }
                std::string buffer(availableSize, '\0');
                DWORD downloaded = 0;
                if (WinHttpReadData(request, &buffer[0], availableSize, &downloaded) != TRUE)
                {
                    payload.clear();
                    break;
                }
                buffer.resize(downloaded);
                payload.append(buffer);
            }
            if (!payload.empty())
            {
                jsonText = payload;
                success = true;
            }
        }
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return success;
}

bool DeviceBatteryManager::ParseAndSelectDevice(const std::string& jsonText, const DeviceBatteryConfig& config, DeviceBatteryInfo& selectedInfo, std::wstring& errorText)
{
    std::string devicesArray;
    if (!ExtractDevicesArray(jsonText, devicesArray))
    {
        errorText = L"Missing devices field in response";
        return false;
    }

    std::vector<std::string> objects = SplitDeviceObjects(devicesArray);
    if (objects.empty())
    {
        errorText = L"No visible devices";
        return false;
    }

    std::vector<DeviceBatteryInfo> deviceList;
    deviceList.reserve(objects.size());
    for (const std::string& objectText : objects)
    {
        DeviceBatteryInfo info;
        if (ParseSingleDevice(objectText, info))
        {
            deviceList.push_back(info);
        }
    }
    if (deviceList.empty())
    {
        errorText = L"Failed to parse device list";
        return false;
    }

    auto selectById = [&]() -> const DeviceBatteryInfo* {
        if (config.deviceId.empty())
        {
            return nullptr;
        }
        for (const auto& device : deviceList)
        {
            if (device.id == config.deviceId)
            {
                return &device;
            }
        }
        return nullptr;
    };

    auto selectByName = [&]() -> const DeviceBatteryInfo* {
        if (config.deviceName.empty())
        {
            return nullptr;
        }
        for (const auto& device : deviceList)
        {
            if (IsMatchByName(device, config.deviceName))
            {
                return &device;
            }
        }
        return nullptr;
    };

    auto selectTray = [&]() -> const DeviceBatteryInfo* {
        for (const auto& device : deviceList)
        {
            if (device.isShownInTray && device.hasBattery)
            {
                return &device;
            }
        }
        return nullptr;
    };

    auto selectFirstWithBattery = [&]() -> const DeviceBatteryInfo* {
        for (const auto& device : deviceList)
        {
            if (device.hasBattery)
            {
                return &device;
            }
        }
        return nullptr;
    };

    const DeviceBatteryInfo* selected = selectById();
    if (selected == nullptr && config.deviceId.empty())
    {
        selected = selectByName();
    }
    if (selected == nullptr && config.deviceId.empty() && config.deviceName.empty() && !config.source.empty())
    {
        for (const auto& device : deviceList)
            if (ToLower(device.source) == ToLower(config.source)) { selected = &device; break; }
    }
    // Explicit selections never fall back to an unrelated peripheral when one disappears.
    if (selected == nullptr && (!config.deviceId.empty() || !config.deviceName.empty() || !config.source.empty()))
    {
        errorText = L"Selected device is unavailable";
        return false;
    }
    if (selected == nullptr && config.preferTrayDevice)
    {
        selected = selectTray();
    }
    if (selected == nullptr)
    {
        selected = selectFirstWithBattery();
    }
    if (selected == nullptr)
    {
        selected = &deviceList.front();
    }

    selectedInfo = *selected;
    return true;
}

void DeviceBatteryManager::UpdateDisplay(const DeviceBatteryInfo& selectedInfo)
{
    std::lock_guard<std::mutex> guard(textMutex_);
    if (selectedInfo.hasBattery && !selectedInfo.isSleeping && selectedInfo.status != L"offline")
    {
        displayText_ = std::to_wstring(selectedInfo.battery) + L"%";
        if (selectedInfo.isCharging)
        {
            displayText_ += L"+";
        }
    }
    else
    {
        displayText_ = L"--";
    }

    std::wstring deviceTitle = selectedInfo.renamedName.empty() ? selectedInfo.name : selectedInfo.renamedName;
    if (deviceTitle.empty())
    {
        deviceTitle = L"Unnamed device";
    }

    std::wstringstream stream;
    stream << deviceTitle << L" battery: " << displayText_;
    if (selectedInfo.isSleeping)
    {
        stream << L" (sleeping)";
    }
    else if (selectedInfo.status == L"offline") stream << L" (offline; cached)";
    if (selectedInfo.isBatteryUnsupported)
    {
        stream << L" (battery unsupported)";
    }
    tooltipText_ = stream.str();
}

void DeviceBatteryManager::RefreshIfNeeded()
{
    const std::uint64_t nowTick = GetTickCount64();
    if (lastRequestTick_ != 0)
    {
        const std::uint64_t elapsed = nowTick - lastRequestTick_;
        if (elapsed < static_cast<std::uint64_t>((std::max)(config_.refreshIntervalMs, 500)))
        {
            return;
        }
    }
    lastRequestTick_ = nowTick;

    std::string jsonText;
    if (!FetchStatusJson(jsonText))
    {
        SetUnavailable(L"Battery source unavailable, expired, or publisher stopped");
        return;
    }

    DeviceBatteryInfo selectedInfo;
    std::wstring errorText;
    if (!ParseAndSelectDevice(jsonText, config_, selectedInfo, errorText))
    {
        SetUnavailable(L"Battery data: " + errorText);
        return;
    }
    UpdateDisplay(selectedInfo);
}

const std::wstring& DeviceBatteryManager::GetDisplayText() const
{
    thread_local std::wstring copy;
    std::lock_guard<std::mutex> guard(textMutex_);
    copy = displayText_;
    return copy;
}

const std::wstring& DeviceBatteryManager::GetTooltipText() const
{
    thread_local std::wstring copy;
    std::lock_guard<std::mutex> guard(textMutex_);
    copy = tooltipText_;
    return copy;
}

void DeviceBatteryManager::SetUnavailable(const std::wstring& reason)
{
    std::lock_guard<std::mutex> guard(textMutex_);
    displayText_ = L"--";
    tooltipText_ = reason;
}

const std::wstring& DeviceBatteryManager::GetLabelText() const
{
    return config_.labelText;
}
