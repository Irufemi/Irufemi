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
        
        # Replace the literal `r`n string that I accidentally injected!
        $content = $content.Replace("`"r`"n", "`r`n")
        $content = $content.Replace('`r`n', "`r`n")
        
        # Fix any remaining git conflict markers properly
        $content = [regex]::Replace($content, '(?s)<<<<<<< HEAD\r?\n(.*?)\r?\n=======\r?\n(.*?)\r?\n>>>>>>> [^\r\n]*\r?\n', '$1`r`n$2`r`n')
        
        # NOTE: In PowerShell double-quotes "$1`r`n$2`r`n", the $1 is interpreted as variable $1! 
        # To avoid this, we use a script block for MatchEvaluator, or we use standard regex replacement.
        # Actually, in single quotes '$1' is the regex group 1. But `r`n in single quotes is literal backtick r backtick n.
        # So we should build the replacement string with double quotes but escape the $!
        $content = [regex]::Replace($content, '(?s)<<<<<<< HEAD\r?\n(.*?)\r?\n=======\r?\n(.*?)\r?\n>>>>>>> [^\r\n]*\r?\n', "`$1`r`n`$2`r`n")
        
        Set-Content -Path $f -Value $content -Encoding UTF8
    }
}
