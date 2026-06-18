@echo off
setlocal enabledelayedexpansion

REM ========================================================
REM IrufemiEngine Offline Shader Compiler Script
REM ========================================================
REM Usage: CompileShaders.bat <ShadersDirectory> [DxcExePath]
REM ========================================================

set "SHADERS_DIR=%~1"
set "DXC_PATH=%~2"

if "%SHADERS_DIR%"=="" (
    echo [Error] Shaders directory not specified.
    exit /b 1
)

if "%DXC_PATH%"=="" (
    set "DXC_PATH=dxc.exe"
)

echo [Info] Starting offline shader compilation...
echo [Info] Shaders Directory: %SHADERS_DIR%
echo [Info] DXC Path: %DXC_PATH%

pushd "%SHADERS_DIR%"

for /r %%f in (*.hlsl) do (
    set "FILENAME=%%~nxf"
    set "FILEBASENAME=%%~nf"
    set "PROFILE="

    echo.!FILENAME!| findstr /I "\.VS\.hlsl$" >nul
    if not errorlevel 1 set "PROFILE=vs_6_0"

    echo.!FILENAME!| findstr /I "\.PS\.hlsl$" >nul
    if not errorlevel 1 set "PROFILE=ps_6_0"

    echo.!FILENAME!| findstr /I "\.CS\.hlsl$" >nul
    if not errorlevel 1 set "PROFILE=cs_6_0"

    echo.!FILENAME!| findstr /I "\.GS\.hlsl$" >nul
    if not errorlevel 1 set "PROFILE=gs_6_0"

    echo.!FILENAME!| findstr /I "\.HS\.hlsl$" >nul
    if not errorlevel 1 set "PROFILE=hs_6_0"

    echo.!FILENAME!| findstr /I "\.DS\.hlsl$" >nul
    if not errorlevel 1 set "PROFILE=ds_6_0"

    if not "!PROFILE!"=="" (
        echo [DXC] Compiling !FILENAME! as !PROFILE!...
        "%DXC_PATH%" /Zpr /O3 /T !PROFILE! /E main "%%f" /Fo "%%~dpnf.cso"
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
