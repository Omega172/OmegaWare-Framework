param(
    [string[]]$Configs = @("x64 Release", "x86 Release"),
    [string[]]$Backends = @("dx9", "dx10", "dx11", "dx12", "opengl", "vulkan"),
    [string]$Proxy = "version",
    [switch]$Probe
)
$Configs = @($Configs | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$Backends = @($Backends | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$Repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$OutDir = Join-Path $Repo "Build\OverlayTest"
New-Item -ItemType Directory -Force $OutDir | Out-Null
$Out = Join-Path $OutDir "HostMatrix.log"
"start $(Get-Date -Format HH:mm:ss)" | Set-Content $Out
$Failed = 0
foreach ($Config in $Configs) {
    $Arch, $Mode = $Config -split ' '
    foreach ($Backend in $Backends) {
        $Shot = Join-Path $OutDir "$Backend-$Arch-$Mode.png"
        $Lines = & (Join-Path $PSScriptRoot "HostCycle.ps1") -Backend $Backend -Arch $Arch -Mode $Mode -Proxy $Proxy -Probe:$Probe -Shot $Shot 2>&1
        if ($LASTEXITCODE -ne 0) { ++$Failed }
        $Lines | ForEach-Object { $_; $_ | Add-Content $Out }
    }
}
"done $(Get-Date -Format HH:mm:ss), $Failed failed" | Tee-Object -FilePath $Out -Append
if ($Failed) { exit 1 }
