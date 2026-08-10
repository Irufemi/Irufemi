@echo off
setlocal enabledelayedexpansion

REM ========================================================
REM IrufemiEngine Offline Shader Compiler Script
REM ========================================================
REM Usage: CompileShaders.bat <ShadersDirectory> <OutputDirectory> [DxcExePath]
REM ========================================================

set "SHADERS_DIR=%~1"
set "OUTPUT_DIR=%~2"
set "DXC_PATH=%~3"

if "%SHADERS_DIR%"=="" (
    echo [Error] Shaders directory not specified.
    exit /b 1
)

if "%OUTPUT_DIR%"=="" (
    echo [Error] Output directory not specified.
    exit /b 1
)

if "%DXC_PATH%"=="" (
    set "DXC_PATH=dxc.exe"
)

echo [Info] Starting offline shader compilation...
echo [Info] Shaders Directory: %SHADERS_DIR%
echo [Info] Output Directory: %OUTPUT_DIR%
echo [Info] DXC Path: %DXC_PATH%

pushd "%SHADERS_DIR%"

for /r %%f in (*.hlsl) do (
    set "FILENAME=%%~nxf"
    set "FILEBASENAME=%%~nf"
    set "PROFILE="

    if /i "!FILENAME:~-8!"==".VS.hlsl" set "PROFILE=vs_6_0"
    if /i "!FILENAME:~-8!"==".PS.hlsl" set "PROFILE=ps_6_0"
    if /i "!FILENAME:~-8!"==".CS.hlsl" set "PROFILE=cs_6_0"
    if /i "!FILENAME:~-8!"==".GS.hlsl" set "PROFILE=gs_6_0"
    if /i "!FILENAME:~-8!"==".HS.hlsl" set "PROFILE=hs_6_0"
    if /i "!FILENAME:~-8!"==".DS.hlsl" set "PROFILE=ds_6_0"

    if not "!PROFILE!"=="" (
        echo [DXC] Compiling !FILENAME! as !PROFILE!...
        if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
        "%DXC_PATH%" /Zpr /O3 /T !PROFILE! /E main "%%f" /Fo "%OUTPUT_DIR%\%%~nf.cso" -I "%SHADERS_DIR%" -I "%~dp0IrufemiEngine\EngineResources\shaders"
        if errorlevel 1 (
            echo [Error] Failed to compile !FILENAME!
            popd
            exit /b 1
        )
    ) else (
        echo [Skip] %%f (Unknown profile)
    )
)

popd
echo [Info] All shaders compiled successfully.
exit /b 0
