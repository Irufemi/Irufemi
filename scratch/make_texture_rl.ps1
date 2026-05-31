Add-Type -AssemblyName System.Drawing

$width  = 512
$height = 512
$splitY = 256  # 上下で50%ずつに分割

$bmp = New-Object System.Drawing.Bitmap($width, $height)
$g   = [System.Drawing.Graphics]::FromImage($bmp)

# 1. 画像の上半分 (Y: 0 ~ 255) -> UVの V: 0.5 ~ 1.0 (チューブ領域)
# 発射管チューブ色：シアンブルー (#00BFFF)
$cyanColor = [System.Drawing.Color]::FromArgb(255, 0, 191, 255)
$cyanBrush = New-Object System.Drawing.SolidBrush($cyanColor)
$g.FillRectangle($cyanBrush, 0, 0, $width, $splitY)

# 2. 画像の下半分 (Y: 256 ~ 511) -> UVの V: 0.0 ~ 0.5 (ポッドケース本体領域)
# ポッドケース本体色：ネイビーブルー (#0D1F5C)
$navyColor = [System.Drawing.Color]::FromArgb(255, 13, 31, 92)
$navyBrush = New-Object System.Drawing.SolidBrush($navyColor)
$g.FillRectangle($navyBrush, 0, $splitY, $width, ($height - $splitY))

$navyBrush.Dispose()
$cyanBrush.Dispose()
$g.Dispose()

$outPath = "c:\Users\k024g\Desktop\TD3_1\project\Application\resources\texture\player\playerRocketLauncher.png"

# 親ディレクトリの存在確認と作成
$parentDir = Split-Path $outPath
if (-not (Test-Path $parentDir)) {
    New-Item -ItemType Directory -Force -Path $parentDir | Out-Null
}

$bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

Write-Host "Done: $outPath"
