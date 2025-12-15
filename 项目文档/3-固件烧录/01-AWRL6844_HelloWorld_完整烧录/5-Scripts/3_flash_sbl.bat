@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo.
echo ========================================
echo   烧录SBL Bootloader
echo ========================================
echo.

REM 检查参数
if "%~1"=="" (
    echo [ERROR] 缺少COM端口参数！
    echo [用法] %~nx0 COM端口
    echo [示例] %~nx0 COM3
    echo.
    pause
    exit /b 1
)

set COM_PORT=%~1

REM 获取脚本所在目录
set SCRIPT_DIR=%~dp0
set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

REM 项目根目录
cd /d "%SCRIPT_DIR%\.."
set PROJECT_ROOT=%CD%

REM 设置路径
set TOOLS_DIR=%PROJECT_ROOT%\3-Tools
set META_IMAGE=%PROJECT_ROOT%\1-SBL_Bootloader\sbl.release.appimage
set FLASH_ADDR=0x2000

echo [INFO] COM端口: %COM_PORT%
echo [INFO] Meta Image: %META_IMAGE%
echo [INFO] Flash地址: %FLASH_ADDR%
echo.

REM 检查工具是否存在
if not exist "%TOOLS_DIR%\arprog_cmdline_6844.exe" (
    echo [ERROR] 找不到arprog_cmdline_6844.exe
    echo [INFO] 请检查 3-Tools 目录
    pause
    exit /b 1
)

REM 检查Meta Image是否存在
if not exist "%META_IMAGE%" (
    echo [ERROR] 找不到sbl.release.appimage！
    echo [INFO] 请先运行: 1_generate_sbl_meta.bat
    echo.
    pause
    exit /b 1
)

REM 显示硬件设置提醒
echo ========================================
echo   硬件设置检查
echo ========================================
echo.
echo [重要] 请确认以下硬件设置：
echo.
echo   1. SOP开关设置为SOP_MODE1 (S8=OFF, S7=OFF)
echo   2. USB连接正常
echo   3. COM端口: %COM_PORT%
echo   4. 板子已上电
echo.
echo ----------------------------------------
echo.

pause

echo.
echo [INFO] 开始烧录...
echo.

REM 进入工具目录
cd /d "%TOOLS_DIR%"

REM 烧录SBL
arprog_cmdline_6844.exe -p %COM_PORT% -f "%META_IMAGE%" -o %FLASH_ADDR%

if errorlevel 1 (
    echo.
    echo ========================================
    echo [ERROR] SBL烧录失败！
    echo ========================================
    echo.
    echo [排查步骤]
    echo   1. 检查COM端口是否正确
    echo   2. 检查SOP跳线是否为SOP_MODE1 (S8=OFF, S7=OFF)
    echo   3. 按下复位按钮后重试
    echo   4. 检查串口是否被其他程序占用
    echo   5. 重新上电后重试
    echo.
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

echo.
echo ========================================
echo [SUCCESS] SBL烧录完成！
echo ========================================
echo.
echo [烧录信息]
echo   文件: sbl.release.appimage
echo   地址: 0x2000
echo   端口: %COM_PORT%
echo.
echo [下一步]
echo   运行: 4_flash_app.bat %COM_PORT%
echo   烧录HelloWorld应用
echo.

cd /d "%SCRIPT_DIR%"
pause
