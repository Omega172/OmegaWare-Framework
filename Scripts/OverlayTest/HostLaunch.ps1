param(
    [Parameter(Mandatory)][string]$Backend,
    [string]$Arch = "x64",
    [string]$Mode = "Debug",
    [string]$Proxy = "version",
    [switch]$Probe,
    [double]$ProbeDepth = 0.999,
    [string]$GuiRoot = ""
)
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "HostWindow.ps1")
$Repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
if (-not $GuiRoot) { $GuiRoot = Join-Path $Repo "Source\Third-Party\OmegaWareProjects\OmegaWare-GUI" }
$Demo = Join-Path $GuiRoot "Build\$Mode\$Arch\OmegaGUI-Demo.exe"
$ProxyDll = Join-Path $Repo "Build\$Mode\$Arch\OmegaWareProxy\$Proxy.dll"
foreach ($File in @($Demo, $ProxyDll)) {
    if (-not (Test-Path $File)) { throw "missing $File, build it first" }
}

$Dir = Join-Path $Repo "Build\OverlayTest\$Backend-$Arch-$Mode"
$Leftover = @(Get-Process OmegaGUI-Demo -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$Dir*" })
$Leftover | Stop-Process -Force -ErrorAction SilentlyContinue
$Leftover | ForEach-Object { $_.WaitForExit(10000) | Out-Null }
for ($i = 0; $i -lt 20 -and (Test-Path $Dir); $i++) {
    Remove-Item -Recurse -Force $Dir -ErrorAction SilentlyContinue
    if (Test-Path $Dir) { Start-Sleep -Milliseconds 500 }
}
New-Item -ItemType Directory -Force $Dir | Out-Null
Copy-Item $Demo $Dir
Copy-Item $ProxyDll $Dir

$Profiles = Join-Path (Get-DataRoot $Arch) "OmegaGUI-Demo\Configs\Profiles"
New-Item -ItemType Directory -Force $Profiles | Out-Null
$Enabled = if ($Probe) { "true" } else { "false" }
$Depth = $ProbeDepth.ToString([System.Globalization.CultureInfo]::InvariantCulture)
[System.IO.File]::WriteAllText((Join-Path $Profiles "default.json"), "{ ""version"": 1, ""values"": { ""example.depth_probe"": $Enabled, ""example.depth_probe_depth"": $Depth } }")

$Process = Start-Process -FilePath (Join-Path $Dir "OmegaGUI-Demo.exe") -ArgumentList "--backend=$Backend", "--hidden" -WorkingDirectory $Dir -PassThru
[pscustomobject]@{ ProcessId = $Process.Id; Dir = $Dir; Started = $Process.StartTime }
