param([string]$Arch = "x64", [string[]]$Skip = @(), [Parameter(ValueFromRemainingArguments = $true)][string[]]$Rest = @())
$Skip = @($Skip) + @($Rest) -split ","
$Repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$Config = Get-Item (Join-Path $Repo ".xmake\windows\$Arch\xmake.conf") -ErrorAction SilentlyContinue
if (-not $Config) { return }
$Text = Get-Content $Config.FullName -Raw
$Arguments = @()
foreach ($Name in @("arch", "mode", "proxy", "engine", "game", "unreal_sdk", "framework_name", "framework_version", "process")) {
    if ($Skip -contains $Name) { continue }
    $Match = [regex]::Match($Text, "(?m)^\s*$Name\s*=\s*(?:""([^""]*)""|\[\[(.*?)\]\])")
    if ($Match.Success) {
        $Value = if ($Match.Groups[1].Success) { $Match.Groups[1].Value } else { $Match.Groups[2].Value }
        $Arguments += "--$Name=`"$Value`""
    }
}
$Arguments -join " "
