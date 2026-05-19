$files = @(
    "Manual.md",
    "project\Application\Application.vcxproj",
    "project\Application\Application.vcxproj.filters",
    "project\Application_solo\Application_solo.vcxproj",
    "project\Application_solo\Application_solo.vcxproj.filters",
    "project\IrufemiEngine\Engine\Core\Math\MathFunction.cpp",
    "project\IrufemiEngine\Engine\Graphics\Pipeline\RenderGraph\PostProcessPass.cpp",
    "project\IrufemiEngine\Engine\Graphics\PostProcess\PostProcessManager.h",
    "project\IrufemiEngine\IrufemiEngine.vcxproj",
    "project\IrufemiEngine\IrufemiEngine.vcxproj.filters",
    "project\IrufemiEngine\Renderer\Object3D\ObjClass\ObjClass.cpp"
)

# Also get all unmerged paths from git
$status = git status --porcelain
foreach ($line in $status) {
    if ($line -match "^UU\s+(.+)") {
        $files += $matches[1]
    }
}
$files = $files | Select-Object -Unique

foreach ($f in $files) {
    if (Test-Path $f) {
        $lines = [System.IO.File]::ReadAllLines($f)
        $outLines = @()
        
        foreach ($line in $lines) {
            if ($line -match "^<<<<<<< HEAD") {
                continue
            } elseif ($line -match "^=======") {
                continue
            } elseif ($line -match "^>>>>>>>") {
                continue
            } else {
                $outLines += $line
            }
        }
        
        $utf8bom = New-Object System.Text.UTF8Encoding($true)
        [System.IO.File]::WriteAllLines($f, $outLines, $utf8bom)
        Write-Host "Resolved: $f"
    }
}
