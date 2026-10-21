param(
    [Parameter(Mandatory)][int]$ProcessId,
    [string[]]$Keys = @(),
    [string]$Text = "",
    [int[]]$Click = @(),
    [int[]]$Drag = @()
)
. (Join-Path $PSScriptRoot "HostWindow.ps1")
$Window = Get-HostWindow $ProcessId
if ($Window -eq [IntPtr]::Zero) { "no window"; exit 1 }
[OverlayTestWindow]::SetForegroundWindow($Window) | Out-Null
Start-Sleep -Milliseconds 300
function MakeLParam([int]$X, [int]$Y) { [IntPtr](($Y -shl 16) -bor ($X -band 0xFFFF)) }
if ($Click.Count -eq 2) {
    [OverlayTestWindow]::PostMessageW($Window, 0x200, [IntPtr]0, (MakeLParam $Click[0] $Click[1])) | Out-Null; Start-Sleep -Milliseconds 80
    [OverlayTestWindow]::PostMessageW($Window, 0x201, [IntPtr]1, (MakeLParam $Click[0] $Click[1])) | Out-Null; Start-Sleep -Milliseconds 80
    [OverlayTestWindow]::PostMessageW($Window, 0x202, [IntPtr]0, (MakeLParam $Click[0] $Click[1])) | Out-Null; Start-Sleep -Milliseconds 150
}
if ($Drag.Count -eq 4) {
    [OverlayTestWindow]::PostMessageW($Window, 0x200, [IntPtr]0, (MakeLParam $Drag[0] $Drag[1])) | Out-Null; Start-Sleep -Milliseconds 80
    [OverlayTestWindow]::PostMessageW($Window, 0x201, [IntPtr]1, (MakeLParam $Drag[0] $Drag[1])) | Out-Null; Start-Sleep -Milliseconds 80
    for ($i = 1; $i -le 8; $i++) {
        $X = $Drag[0] + [int](($Drag[2] - $Drag[0]) * $i / 8)
        $Y = $Drag[1] + [int](($Drag[3] - $Drag[1]) * $i / 8)
        [OverlayTestWindow]::PostMessageW($Window, 0x200, [IntPtr]1, (MakeLParam $X $Y)) | Out-Null; Start-Sleep -Milliseconds 40
    }
    [OverlayTestWindow]::PostMessageW($Window, 0x202, [IntPtr]0, (MakeLParam $Drag[2] $Drag[3])) | Out-Null; Start-Sleep -Milliseconds 150
}
foreach ($Character in $Text.ToCharArray()) {
    [OverlayTestWindow]::PostMessageW($Window, 0x102, [IntPtr][int]$Character, [IntPtr]1) | Out-Null
    Start-Sleep -Milliseconds 30
}
foreach ($Key in $Keys) { Send-HostKey $Window $Key }
"ok"
