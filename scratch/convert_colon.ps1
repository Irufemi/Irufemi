Add-Type -AssemblyName System.Drawing
$imagePath = "c:\Users\k024g\Desktop\TD3_1\project\Application\resources\texture\inGame\colon.png"
$bmp = [System.Drawing.Bitmap]::FromFile($imagePath)
$newBmp = New-Object System.Drawing.Bitmap $bmp.Width, $bmp.Height

for ($y = 0; $y -lt $bmp.Height; $y++) {
    for ($x = 0; $x -lt $bmp.Width; $x++) {
        $pixel = $bmp.GetPixel($x, $y)
        if ($pixel.A -gt 0) {
            # アルファ値を維持したまま、RGBを白（255, 255, 255）にする
            $newColor = [System.Drawing.Color]::FromArgb($pixel.A, 255, 255, 255)
            $newBmp.SetPixel($x, $y, $newColor)
        } else {
            $newBmp.SetPixel($x, $y, $pixel)
        }
    }
}

$bmp.Dispose()
# 元のファイルを上書きするために一時ファイルに保存してから上書きコピーする
$tempPath = "c:\Users\k024g\Desktop\TD3_1\scratch\temp_colon.png"
$newBmp.Save($tempPath, [System.Drawing.Imaging.ImageFormat]::Png)
$newBmp.Dispose()

Copy-Item -Path $tempPath -Destination $imagePath -Force
Remove-Item -Path $tempPath

Write-Output "Conversion completed successfully!"
