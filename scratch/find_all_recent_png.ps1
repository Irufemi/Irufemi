$targetDirs = @(
    "C:\Users\k024g\Desktop", 
    "C:\Users\k024g\Downloads", 
    "C:\Users\k024g\Documents", 
    "C:\Users\k024g\Pictures", 
    "C:\Users\k024g\Desktop\TD3_1",
    "C:\Users\k024g\OneDrive",
    "C:\Users\k024g\AppData\Local\Aseprite"
)
foreach ($dir in $targetDirs) {
    if (Test-Path $dir) {
        Get-ChildItem -Path $dir -Recurse -File -Filter "*.png" -ErrorAction SilentlyContinue | Where-Object {
            $_.FullName -notmatch "Library|PackageCache|Temp|obj|bin|\.git" -and
            ($_.Name -like "*record*" -or $_.Name -like "*best*" -or $_.Name -like "*this*")
        } | Select-Object -Property FullName, LastWriteTime | Out-String | Write-Output
    }
}
