@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo.
echo ========================================
echo   生成SBL Meta Image
echo ========================================
echo.

REM 获取脚本所在目录
set SCRIPT_DIR=%~dp0
set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

REM 项目根目录
cd /d "%SCRIPT_DIR%\.."
set PROJECT_ROOT=%CD%

REM 设置路径
set SBL_DIR=%PROJECT_ROOT%\1-SBL_Bootloader
set TOOLS_DIR=%PROJECT_ROOT%\3-Tools
set OUTPUT_DIR=%PROJECT_ROOT%\4-Generated

echo [INFO] 当前目录: %CD%
echo [INFO] SBL目录: %SBL_DIR%
echo [INFO] 工具目录: %TOOLS_DIR%
echo [INFO] 输出目录: %OUTPUT_DIR%
echo.

REM 检查工具是否存在
if not exist "%TOOLS_DIR%\buildImage_creator.exe" (
    echo [ERROR] 找不到buildImage_creator.exe
    echo [INFO] 请检查 3-Tools 目录
    pause
    exit /b 1
)

if not exist "%TOOLS_DIR%\metaImage_creator.exe" (
    echo [ERROR] 找不到metaImage_creator.exe
    echo [INFO] 请检查 3-Tools 目录
    pause
    exit /b 1
)

REM 检查SBL文件是否存在
if not exist "%SBL_DIR%\sbl.release.appimage" (
    echo [ERROR] 找不到sbl.release.appimage
    echo [INFO] 请检查 1-SBL_Bootloader 目录
    pause
    exit /b 1
)

if not exist "%SBL_DIR%\metaimage_cfg.release.json" (
    echo [ERROR] 找不到metaimage_cfg.release.json
    echo [INFO] 请检查 1-SBL_Bootloader 目录
    pause
    exit /b 1
)

REM 创建输出目录
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

REM 进入SBL目录
echo [INFO] 进入SBL目录...
cd /d "%SBL_DIR%"

REM 创建temp目录
if not exist temp mkdir temp

echo.
echo [1/2] 提取Build Images...
echo.

REM 运行buildImage_creator
"%TOOLS_DIR%\buildImage_creator.exe" -i sbl.release.appimage

if errorlevel 1 (
    echo.
    echo [ERROR] buildImage_creator执行失败！
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

REM 检查是否生成了.rig文件
if not exist "temp\*.rig" (
    echo.
    echo [ERROR] 未找到生成的.rig文件
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

echo.
echo [2/2] 生成Meta Image...
echo.

REM 运行metaImage_creator
"%TOOLS_DIR%\metaImage_creator.exe" -config metaimage_cfg.release.json

if errorlevel 1 (
    echo.
    echo [ERROR] metaImage_creator执行失败！
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

REM 检查输出文件
if not exist "%OUTPUT_DIR%\sbl_meta.bin" (
    echo.
    echo [ERROR] 未找到生成的sbl_meta.bin
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

echo.
echo ========================================
echo [SUCCESS] SBL Meta Image生成完成！
echo ========================================
echo.
echo [输出文件] %OUTPUT_DIR%\sbl_meta.bin

REM 显示文件大小
for %%F in ("%OUTPUT_DIR%\sbl_meta.bin") do (
    echo [文件大小] %%~zF bytes
)

echo.
echo [下一步] 运行 3_flash_sbl.bat 烧录SBL
echo.

cd /d "%SCRIPT_DIR%"
pause
