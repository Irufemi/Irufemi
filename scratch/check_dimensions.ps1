Add-Type -AssemblyName System.Drawing
$imgThis = [System.Drawing.Bitmap]::FromFile("c:\Users\k024g\Desktop\TD3_1\project\Application\resources\texture\inGame\This record.png")
$imgBest = [System.Drawing.Bitmap]::FromFile("c:\Users\k024g\Desktop\TD3_1\project\Application\resources\texture\inGame\best record.png")
Write-Output "This record: Width=$($imgThis.Width), Height=$($imgThis.Height)"
Write-Output "Best record: Width=$($imgBest.Width), Height=$($imgBest.Height)"
$imgThis.Dispose()
$imgBest.Dispose()
