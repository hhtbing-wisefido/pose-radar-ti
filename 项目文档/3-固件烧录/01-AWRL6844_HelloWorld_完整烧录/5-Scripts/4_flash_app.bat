@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

REM ============================================
REM 烧录Application到Flash 0x42000
REM 使用官方SDK标准参数格式
REM ============================================

echo.
echo ========================================
echo   烧录HelloWorld应用
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
set APP_IMAGE=%PROJECT_ROOT%\2-HelloWorld_App\hello_world_system.release.appimage
set FLASH_ADDR_DEC=270336
set FLASH_ADDR_HEX=0x42000

echo [INFO] COM端口: %COM_PORT%
echo [INFO] App固件: hello_world_system.release.appimage
echo [INFO] Flash地址: %FLASH_ADDR_HEX% (%FLASH_ADDR_DEC%字节)
echo.

REM 检查工具是否存在
if not exist "%TOOLS_DIR%\arprog_cmdline_6844.exe" (
    echo [错误] 找不到arprog_cmdline_6844.exe
    echo [位置] 3-Tools\arprog_cmdline_6844.exe
    pause
    exit /b 1
)

REM 检查App固件是否存在
if not exist "%APP_IMAGE%" (
    echo [错误] 找不到hello_world_system.release.appimage！
    echo [位置] 2-HelloWorld_App\hello_world_system.release.appimage
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
echo   1. SOP开关仍为SOP_MODE1
echo      └─ SOP0 = OFF
echo      └─ SOP1 = OFF
echo.
echo   2. USB连接正常
echo      └─ COM端口: %COM_PORT%
echo.
echo   3. 板子已上电
echo.
echo   4. SBL已烧录完成
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
  -f1 "%APP_IMAGE%" ^
  -of1 %FLASH_ADDR_DEC% ^
  -s SFLASH -c

if errorlevel 1 (
    echo.
    echo ========================================
    echo [错误] Application烧录失败！
    echo ========================================
    echo.
    echo [排查步骤]
    echo   1. 检查COM端口是否正确 (当前: %COM_PORT%)
    echo   2. 检查SOP开关是否仍为SOP_MODE1 (SOP0=OFF, SOP1=OFF)
    echo   3. 按下板子RESET按钮后重试
    echo   4. 检查串口是否被其他程序占用
    echo   5. 重新给板子上电后重试
    echo.
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

echo.
echo ========================================
echo [SUCCESS] HelloWorld应用烧录完成！
echo ========================================
echo.
echo [烧录信息]
echo   文件: hello_world_system.release.appimage
echo   地址: 0x42000
echo   端口: %COM_PORT%
echo.
echo ========================================
echo   后续操作步骤
echo ========================================
echo.
echo [1] 断开板子电源
echo.
echo [2] 修改SOP开关
echo     从 SOP_MODE1 (S8=OFF, S7=OFF)
echo     改为 SOP_MODE2 (S8=OFF, S7=ON)
echo.
echo [3] 重新上电
echo.
echo [4] 打开串口终端
echo     波特率: 115200
echo     数据位: 8
echo     停止位: 1
echo     校验: 无
echo     端口: %COM_PORT%
echo.
echo [5] 期望输出
echo     ----------------------------------------
echo     [BOOTLOADER_PROFILE] Boot Media: FLASH
echo     [BOOTLOADER] Booting Cores ...
echo     Hello World from r5f0-0 !
echo     Hello World from c66ss0 !
echo     Hello World from r5f0-1 !
echo     ----------------------------------------
echo.

cd /d "%SCRIPT_DIR%"
pause
