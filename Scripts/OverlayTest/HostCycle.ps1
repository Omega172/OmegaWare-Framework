param(
    [Parameter(Mandatory)][string]$Backend,
    [string]$Arch = "x64",
    [string]$Mode = "Release",
    [string]$Proxy = "version",
    [switch]$Probe,
    [double]$ProbeDepth = 0.999,
    [string]$Shot = "",
    [int]$AttachSeconds = 30
)
. (Join-Path $PSScriptRoot "HostWindow.ps1")
$Api = @{ dx9 = "Direct3D 9"; dx10 = "Direct3D 10"; dx11 = "Direct3D 11"; dx12 = "Direct3D 12"; opengl = "OpenGL"; vulkan = "Vulkan" }[$Backend]
$Launch = & (Join-Path $PSScriptRoot "HostLaunch.ps1") -Backend $Backend -Arch $Arch -Mode $Mode -Proxy $Proxy -Probe:$Probe -ProbeDepth $ProbeDepth
$ProcessId = $Launch.ProcessId
$LogDir = Join-Path (Get-DataRoot $Arch) "OmegaGUI-Demo\Logs"
$Log = $null
for ($i = 0; $i -lt 40 -and -not $Log; $i++) {
    Start-Sleep -Milliseconds 250
    $Log = Get-ChildItem $LogDir -Filter *.log -ErrorAction SilentlyContinue | Where-Object { $_.Name -notlike "*_ERRORS*" -and $_.LastWriteTime -ge $Launch.Started.AddSeconds(-2) } | Sort-Object LastWriteTime | Select-Object -Last 1 -ExpandProperty FullName
}

$Deadline = (Get-Date).AddSeconds($AttachSeconds)
$Attached = $false
while ($Log -and (Get-Date) -lt $Deadline) {
    if (Select-String -Path $Log -Pattern "gui (initialized on|reattached to) $Api" -Quiet) { $Attached = $true; break }
    if (-not (Get-Process -Id $ProcessId -ErrorAction SilentlyContinue)) { break }
    Start-Sleep -Milliseconds 500
}
Start-Sleep 2

$Window = Get-HostWindow $ProcessId
if ($Shot -and $Window -ne [IntPtr]::Zero) {
    Save-HostShot $Window $Shot
}
if ($Window -ne [IntPtr]::Zero) {
    [OverlayTestWindow]::SetForegroundWindow($Window) | Out-Null
    Start-Sleep -Milliseconds 300
    Send-HostKey $Window "end"
}

$Deadline = (Get-Date).AddSeconds(10)
$Unloaded = $false
while ($Log -and (Get-Date) -lt $Deadline) {
    if (Select-String -Path $Log -Pattern "unloaded, the proxy stays loaded" -Quiet) { $Unloaded = $true; break }
    Start-Sleep -Milliseconds 500
}
Start-Sleep 2

$Process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
$Alive = [bool]$Process
$Responding = $Alive -and $Process.Responding
$Ready = @(if ($Log) { Select-String -Path $Log -Pattern "rendering|presented|hooking anyway|scene depth" | ForEach-Object { $_.Line } })
$Problems = @(if ($Log) { Select-String -Path $Log -Pattern "\[(warning|error)\]" | ForEach-Object { $_.Line } })
$Pass = $Attached -and $Unloaded -and $Responding
"{0} {1} {2} {3}: attached={4} unloaded={5} alive={6} responding={7}" -f $(if ($Pass) { "PASS" } else { "FAIL" }), $Backend, $Arch, $Mode, $Attached, $Unloaded, $Alive, $Responding
$Ready | ForEach-Object { "    $_" }
$Problems | ForEach-Object { "    $_" }
if ($Process) { Stop-Process -Id $ProcessId -Force }
exit $(if ($Pass) { 0 } else { 1 })
