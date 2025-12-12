@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo.
echo ========================================
echo   烧录HelloWorld应用
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
set OUTPUT_DIR=%PROJECT_ROOT%\4-Generated
set META_IMAGE=%OUTPUT_DIR%\hello_world_meta.bin
set FLASH_ADDR=0x40000

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
    echo [ERROR] 找不到hello_world_meta.bin！
    echo [INFO] 请先运行: 2_generate_app_meta.bat
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
echo   1. SOP开关仍为SOP_MODE1 (S8=OFF, S7=OFF)
echo   2. USB连接正常
echo   3. COM端口: %COM_PORT%
echo   4. 板子已上电
echo   5. SBL已烧录完成
echo.
echo ----------------------------------------
echo.

pause

echo.
echo [INFO] 开始烧录...
echo.

REM 进入工具目录
cd /d "%TOOLS_DIR%"

REM 烧录App
arprog_cmdline_6844.exe -p %COM_PORT% -f "%META_IMAGE%" -o %FLASH_ADDR%

if errorlevel 1 (
    echo.
    echo ========================================
    echo [ERROR] 应用烧录失败！
    echo ========================================
    echo.
    echo [排查步骤]
    echo   1. 检查COM端口是否正确
    echo   2. 检查SOP跳线是否为SOP4
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
echo [SUCCESS] HelloWorld应用烧录完成！
echo ========================================
echo.
echo [烧录信息]
echo   文件: hello_world_meta.bin
echo   地址: 0x40000
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
