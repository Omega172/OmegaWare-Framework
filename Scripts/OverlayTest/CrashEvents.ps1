param([int]$Minutes = 5)
Get-WinEvent -FilterHashtable @{ LogName = 'Application'; Id = 1000; StartTime = (Get-Date).AddMinutes(-$Minutes) } -MaxEvents 5 -ErrorAction SilentlyContinue | ForEach-Object {
    "{0} {1}" -f $_.TimeCreated, (($_.Message -split "`r?`n" | Select-Object -First 9) -join ' | ')
}
