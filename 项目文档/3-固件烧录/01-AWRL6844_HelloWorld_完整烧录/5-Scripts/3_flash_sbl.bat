@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

REM ============================================
REM 烧录SBL Bootloader到Flash 0x2000
REM 使用官方SDK标准参数格式
REM ============================================

echo.
echo ========================================
echo   烧录SBL Bootloader
echo ========================================
echo.

REM 检查参数
if "%~1"=="" (
    echo [错误] 缺少COM端口参数！
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
set SBL_IMAGE=%PROJECT_ROOT%\1-SBL_Bootloader\sbl.release.appimage
set FLASH_ADDR_DEC=8192
set FLASH_ADDR_HEX=0x2000

echo [INFO] COM端口: %COM_PORT%
echo [INFO] SBL固件: sbl.release.appimage
echo [INFO] Flash地址: %FLASH_ADDR_HEX% (%FLASH_ADDR_DEC%字节)
echo.

REM 检查工具是否存在
if not exist "%TOOLS_DIR%\arprog_cmdline_6844.exe" (
    echo [错误] 找不到arprog_cmdline_6844.exe
    echo [位置] 3-Tools\arprog_cmdline_6844.exe
    pause
    exit /b 1
)

REM 检查SBL固件是否存在
if not exist "%SBL_IMAGE%" (
    echo [错误] 找不到sbl.release.appimage！
    echo [位置] 1-SBL_Bootloader\sbl.release.appimage
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
echo   1. SOP开关设置为SOP_MODE1
echo      └─ SOP0 = OFF
echo      └─ SOP1 = OFF
echo.
echo   2. USB连接正常
echo      └─ COM端口: %COM_PORT%
echo.
echo   3. 板子已上电
echo.
echo ----------------------------------------
echo.

pause

echo.
echo [INFO] 开始烧录...
echo.

REM 进入工具目录
cd /d "%TOOLS_DIR%"

REM 使用官方标准参数: -f1 -of1 -s SFLASH -c
arprog_cmdline_6844.exe -p %COM_PORT% ^
  -f1 "%SBL_IMAGE%" ^
  -of1 %FLASH_ADDR_DEC% ^
  -s SFLASH -c

if errorlevel 1 (
    echo.
    echo ========================================
    echo [错误] SBL烧录失败！
    echo ========================================
    echo.
    echo [排查步骤]
    echo   1. 检查COM端口是否正确 (当前: %COM_PORT%)
    echo   2. 检查SOP开关是否为SOP_MODE1 (SOP0=OFF, SOP1=OFF)
    echo   3. 按下板子RESET按钮后重试
    echo   4. 检查串口是否被其他程序占用
    echo   5. 重新给板子上电后重试
    echo.
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)
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
