$paths = @("C:\Users\k024g\Desktop", "C:\Users\k024g\Downloads", "C:\Users\k024g\Documents", "C:\Users\k024g\Desktop\TD3_1")
foreach ($path in $paths) {
    if (Test-Path $path) {
        Get-ChildItem -Path $path -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
            ($_.Name -like "*record*" -or $_.Name -like "*best*" -or $_.Name -like "*this*") -and
            $_.Extension -eq ".png"
        } | Select-Object -Property FullName, LastWriteTime | Out-String | Write-Output
    }
}
