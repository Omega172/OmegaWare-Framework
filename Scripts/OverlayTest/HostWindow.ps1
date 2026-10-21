Add-Type @"
using System; using System.Runtime.InteropServices; using System.Text;
public static class OverlayTestWindow {
    public delegate bool EnumProc(IntPtr h, IntPtr l);
    public struct RECT { public int L, T, R, B; }
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc p, IntPtr l);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint p);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll")] public static extern bool PostMessageW(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern uint MapVirtualKeyW(uint c, uint t);
    public static IntPtr Find(uint pid) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((h, l) => { uint p; GetWindowThreadProcessId(h, out p); var s = new StringBuilder(256); GetWindowText(h, s, 256);
            if (p == pid && IsWindowVisible(h) && s.ToString().StartsWith("OmegaGUI Demo")) { found = h; return false; } return true; }, IntPtr.Zero);
        return found;
    }
}
"@ -ErrorAction SilentlyContinue

[OverlayTestWindow]::SetProcessDPIAware() | Out-Null

function Get-HostWindow([int]$ProcessId) {
    [OverlayTestWindow]::Find([uint32]$ProcessId)
}

function Send-HostKey([IntPtr]$Window, [string]$Key) {
    $Codes = @{ home = 0x24; end = 0x23; insert = 0x2D; tab = 0x09; enter = 0x0D; ctrlc = 0x43; ctrla = 0x41 }
    $Code = $Codes[$Key]
    $Extended = if ($Key -in @("home", "end", "insert")) { 1 -shl 24 } else { 0 }
    $Scan = [OverlayTestWindow]::MapVirtualKeyW([uint32]$Code, 0)
    $Down = [IntPtr](1 -bor ($Scan -shl 16) -bor $Extended)
    $Up = [IntPtr]([long]$Down.ToInt64() -bor 0xC0000000L)
    if ($Key -like "ctrl*") { [OverlayTestWindow]::PostMessageW($Window, 0x100, [IntPtr]0x11, [IntPtr]1) | Out-Null; Start-Sleep -Milliseconds 40 }
    [OverlayTestWindow]::PostMessageW($Window, 0x100, [IntPtr]$Code, $Down) | Out-Null
    if ($Key -eq "enter") { [OverlayTestWindow]::PostMessageW($Window, 0x102, [IntPtr]13, [IntPtr]1) | Out-Null }
    Start-Sleep -Milliseconds 60
    [OverlayTestWindow]::PostMessageW($Window, 0x101, [IntPtr]$Code, $Up) | Out-Null
    if ($Key -like "ctrl*") { Start-Sleep -Milliseconds 40; [OverlayTestWindow]::PostMessageW($Window, 0x101, [IntPtr]0x11, [IntPtr]([long]0xC0000001L)) | Out-Null }
    Start-Sleep -Milliseconds 150
}

function Save-HostShot([IntPtr]$Window, [string]$Out) {
    Add-Type -AssemblyName System.Drawing
    [OverlayTestWindow]::SetForegroundWindow($Window) | Out-Null
    Start-Sleep -Milliseconds 700
    $Rect = New-Object OverlayTestWindow+RECT
    [OverlayTestWindow]::GetWindowRect($Window, [ref]$Rect) | Out-Null
    $Bitmap = New-Object System.Drawing.Bitmap ($Rect.R - $Rect.L), ($Rect.B - $Rect.T)
    $Graphics = [System.Drawing.Graphics]::FromImage($Bitmap)
    $Graphics.CopyFromScreen($Rect.L, $Rect.T, 0, 0, $Bitmap.Size)
    $Bitmap.Save($Out)
    $Graphics.Dispose()
    $Bitmap.Dispose()
}

function Get-DataRoot([string]$Arch = "x64") {
    $Repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
    $Name = "OmegaWare"
    $Conf = Join-Path $Repo ".xmake\windows\$Arch\xmake.conf"
    if (Test-Path $Conf) {
        $Match = [regex]::Match((Get-Content $Conf -Raw), '(?m)^\s*framework_name\s*=\s*"([^"]*)"')
        if ($Match.Success -and $Match.Groups[1].Value) { $Name = $Match.Groups[1].Value }
    }
    Join-Path ([Environment]::GetFolderPath("MyDocuments")) $Name
}
