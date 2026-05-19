$files = @(
    'Manual.md',
    'project/Application_solo/Application_solo.vcxproj',
    'project/Application_solo/Application_solo.vcxproj.filters',
    'project/Application/Application.vcxproj',
    'project/Application/Application.vcxproj.filters',
    'project/IrufemiEngine/Engine/Core/Math/MathFunction.cpp',
    'project/IrufemiEngine/Engine/Graphics/Pipeline/RenderGraph/PostProcessPass.cpp',
    'project/IrufemiEngine/Engine/Graphics/PostProcess/PostProcessManager.h',
    'project/IrufemiEngine/IrufemiEngine.vcxproj',
    'project/IrufemiEngine/IrufemiEngine.vcxproj.filters',
    'project/IrufemiEngine/Renderer/Object3D/ObjClass/ObjClass.cpp'
)

foreach ($f in $files) {
    if (Test-Path $f) {
        $content = Get-Content $f -Raw
        $newContent = [regex]::Replace($content, '(?s)<<<<<<< HEAD\r?\n(.*?)\r?\n=======\r?\n(.*?)\r?\n>>>>>>> [^\r\n]*\r?\n', '$1`r`n$2`r`n')
        if ($newContent -ne $content) {
            Write-Host "Fixed: $f"
            Set-Content -Path $f -Value $newContent -Encoding UTF8
        }
    }
}
