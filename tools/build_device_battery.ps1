param(
    [Parameter(Mandatory=$true)][string]$VCToolsRoot,
    [Parameter(Mandatory=$true)][string]$WindowsSdkRoot,
    [string]$SdkVersion='10.0.22621.0'
)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$source=Join-Path $repo 'PluginDemo\DeviceBatteryPlugin'
$output=Join-Path $repo 'artifacts\DeviceBatteryPlugin'
New-Item -ItemType Directory -Path $output -Force | Out-Null
$compiler=Join-Path $VCToolsRoot 'bin\Hostx64\x64\cl.exe'
$oldInclude=$env:INCLUDE;$oldLib=$env:LIB;$oldPath=$env:PATH
try {
    $env:INCLUDE=(Join-Path $VCToolsRoot 'include')+';'+(Join-Path $WindowsSdkRoot "Include\$SdkVersion\ucrt")+';'+(Join-Path $WindowsSdkRoot "Include\$SdkVersion\um")+';'+(Join-Path $WindowsSdkRoot "Include\$SdkVersion\shared")
    $env:LIB=(Join-Path $VCToolsRoot 'lib\x64')+';'+(Join-Path $WindowsSdkRoot "Lib\$SdkVersion\ucrt\x64")+';'+(Join-Path $WindowsSdkRoot "Lib\$SdkVersion\um\x64")
    $env:PATH=(Split-Path -Parent $compiler)+';'+$env:PATH
    Push-Location $output
    $common=@('/nologo','/std:c++17','/EHsc','/O2','/MT','/utf-8',('/I'+(Join-Path $repo 'include')),('/I'+$source))
    & $compiler @common /LD (Join-Path $source 'DeviceBatteryManager.cpp') (Join-Path $source 'DeviceBatteryItem.cpp') (Join-Path $source 'DeviceBatteryPlugin.cpp') /link winhttp.lib gdi32.lib user32.lib /OUT:DeviceBatteryPlugin.dll
    if($LASTEXITCODE -ne 0){throw 'DeviceBatteryPlugin build failed'}
    & $compiler @common (Join-Path $repo 'tests\device_battery_tests.cpp') (Join-Path $source 'DeviceBatteryManager.cpp') /Fe:device_battery_tests.exe /link winhttp.lib gdi32.lib user32.lib
    if($LASTEXITCODE -ne 0){throw 'Battery test build failed'}
    & (Join-Path $output 'device_battery_tests.exe')
    if($LASTEXITCODE -ne 0){throw 'Battery regression tests failed'}
    Get-FileHash -LiteralPath (Join-Path $output 'DeviceBatteryPlugin.dll') -Algorithm SHA256
} finally {
    Pop-Location
    $env:INCLUDE=$oldInclude;$env:LIB=$oldLib;$env:PATH=$oldPath
}
