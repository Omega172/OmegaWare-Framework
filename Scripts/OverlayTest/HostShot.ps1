param(
    [Parameter(Mandatory)][int]$ProcessId,
    [Parameter(Mandatory)][string]$Out
)
. (Join-Path $PSScriptRoot "HostWindow.ps1")
$Window = Get-HostWindow $ProcessId
if ($Window -eq [IntPtr]::Zero) { "no window"; exit 1 }
Save-HostShot $Window $Out
"saved $Out"
