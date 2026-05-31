# PowerShell script to search for Blender and run the wind-up mechanism generator script.

$blenderPath = "blender" # Fallback to PATH environment variable

# Search for Blender in typical Program Files paths
$candidates = @(
    "C:\Program Files\Blender Foundation\Blender 4.3\blender.exe",
    "C:\Program Files\Blender Foundation\Blender 4.2\blender.exe",
    "C:\Program Files\Blender Foundation\Blender 4.1\blender.exe",
    "C:\Program Files\Blender Foundation\Blender 4.0\blender.exe",
    "C:\Program Files\Blender Foundation\Blender 3.6\blender.exe",
    "C:\Program Files\Blender Foundation\Blender 3.5\blender.exe",
    "C:\Program Files\Blender Foundation\Blender 3.4\blender.exe",
    "C:\Program Files\Blender Foundation\Blender 3.3\blender.exe",
    "C:\Program Files\Blender Foundation\Blender 3.2\blender.exe",
    "C:\Program Files\Blender Foundation\Blender 3.1\blender.exe",
    "C:\Program Files\Blender Foundation\Blender 3.0\blender.exe"
)

$found = $false
foreach ($c in $candidates) {
    if (Test-Path $c) {
        $blenderPath = $c
        $found = $true
        break
    }
}

if ($found) {
    Write-Host "Found Blender: $blenderPath" -ForegroundColor Green
} else {
    Write-Host "Blender was not found in common C:\Program Files paths. Will attempt to run using 'blender' in PATH." -ForegroundColor Yellow
}

$scriptPath = "c:\Users\k024g\Desktop\TD3_1\scratch\generate_windup_mechanism.py"

if (-not (Test-Path $scriptPath)) {
    Write-Error "Script not found at $scriptPath"
    exit 1
}

Write-Host "Running Blender script: $scriptPath..."
& $blenderPath --background --python $scriptPath

if ($LASTEXITCODE -eq 0) {
    Write-Host "Successfully generated Wind-up Mechanism!" -ForegroundColor Green
    Write-Host "Output files are stored at: c:\Users\k024g\Desktop\TD3_1\project\Application\resources\model\player\windup_mechanism.obj" -ForegroundColor Green
} else {
    Write-Error "Blender script execution failed with exit code $LASTEXITCODE"
}
