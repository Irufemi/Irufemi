Add-Type -AssemblyName System.Drawing

$width  = 512
$height = 512
$splitX = 307  # 左60% = 本体, 右40% = 砲身

$bmp = New-Object System.Drawing.Bitmap($width, $height)
$g   = [System.Drawing.Graphics]::FromImage($bmp)

# 本体色：ネイビーブルー
$navyColor = [System.Drawing.Color]::FromArgb(255, 13, 31, 92)
$navyBrush = New-Object System.Drawing.SolidBrush($navyColor)
$g.FillRectangle($navyBrush, 0, 0, $splitX, $height)

# 砲身色：シアンブルー
$cyanColor = [System.Drawing.Color]::FromArgb(255, 0, 191, 255)
$cyanBrush = New-Object System.Drawing.SolidBrush($cyanColor)
$g.FillRectangle($cyanBrush, $splitX, 0, ($width - $splitX), $height)

$navyBrush.Dispose()
$cyanBrush.Dispose()
$g.Dispose()

$outPath = "c:\Users\k024g\Desktop\TD3_1\project\Application\resources\texture\player\playerCannon.png"
$bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

Write-Host "Done: $outPath"
