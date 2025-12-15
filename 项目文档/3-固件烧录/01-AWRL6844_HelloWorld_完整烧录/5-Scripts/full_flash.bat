@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

REM ============================================
REM AWRL6844 完整烧录脚本（官方SDK标准流程）
REM 基于: MMWAVE_L_SDK_06_01_00_01官方文档
REM 一次性烧录SBL和Application到QSPI Flash
REM ============================================

REM 检查参数
if "%~1"=="" (
    echo.
    echo [错误] 请指定COM端口
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

cls
echo.
echo ╔════════════════════════════════════════════════════════════╗
echo ║                                                            ║
echo ║     AWRL6844 完整烧录流程 (官方SDK标准)                    ║
echo ║                                                            ║
echo ║     COM端口: %-45s║
echo ║                                                            ║
echo ╚════════════════════════════════════════════════════════════╝
echo.

set START_TIME=%time%
set PROJECT_ROOT=%SCRIPT_DIR%\..

REM ============================================================
REM Phase 1: 检查固件文件
REM ============================================================
echo [1/3] 检查固件文件...
echo.

cd /d "%SCRIPT_DIR%"

REM 检查SBL固件
if not exist "%PROJECT_ROOT%\1-SBL_Bootloader\sbl.release.appimage" (
    echo   └─ 失败！
    echo.
    echo [错误] SBL固件不存在: sbl.release.appimage
    echo [位置] 1-SBL_Bootloader\sbl.release.appimage
    pause
    exit /b 1
)

REM 获取SBL文件大小
for %%F in ("%PROJECT_ROOT%\1-SBL_Bootloader\sbl.release.appimage") do set SBL_SIZE=%%~zF

echo   ├─ SBL固件: sbl.release.appimage (%SBL_SIZE% bytes)

REM 检查Application固件
if not exist "%PROJECT_ROOT%\2-HelloWorld_App\hello_world_system.release.appimage" (
    echo   └─ 失败！
    echo.
    echo [错误] Application固件不存在: hello_world_system.release.appimage
    echo [位置] 2-HelloWorld_App\hello_world_system.release.appimage
    pause
    exit /b 1
)

REM 获取App文件大小
for %%F in ("%PROJECT_ROOT%\2-HelloWorld_App\hello_world_system.release.appimage") do set APP_SIZE=%%~zF

echo   ├─ App固件: hello_world_system.release.appimage (%APP_SIZE% bytes)

REM 检查烧录工具
if not exist "%PROJECT_ROOT%\3-Tools\arprog_cmdline_6844.exe" (
    echo   └─ 失败！
    echo.
    echo [错误] 烧录工具不存在: arprog_cmdline_6844.exe
    echo [位置] 3-Tools\arprog_cmdline_6844.exe
    pause
    exit /b 1
)

echo   └─ 烧录工具: arprog_cmdline_6844.exe
echo.
echo   [完成] 所有文件检查通过
echo.
if not exist "%PROJECT_ROOT%\2-HelloWorld_App\hello_world_system.release.appimage" (
    echo   └─ 失败！
    echo.
    echo [ERROR] 未找到hello_world_system.release.appimage
    pause
    exit /b 1
)

REM 获取文件大小
for %%F in ("%PROJECT_ROOT%\2-HelloWorld_App\hello_world_system.release.appimage") do set APP_SIZE=%%~zF

echo   └─ 完成 - hello_world_system.release.appimage (%APP_SIZE% bytes)
echo.

REM ============================================================
REM Phase 2: 烧录固件 (官方标准命令)
REM ============================================================
echo [2/3] 烧录固件到Flash...
echo.

echo [重要] 请确认以下硬件设置：
echo   ├─ SOP开关设置为SOP_MODE1 (SOP0=OFF, SOP1=OFF)
echo   ├─ USB连接到%COM_PORT%
echo   └─ 板子已上电
echo.
echo 按任意键继续烧录，或等待10秒自动开始...
timeout /t 10 /nobreak >nul 2>&1

echo.
echo   开始烧录 (使用官方 -cf 参数自动创建Flash头)...
echo   ├─ SBL文件 → Flash 0x2000
echo   └─ App文件 → Flash 0x42000
echo.

cd /d "%PROJECT_ROOT%\3-Tools"

REM 官方标准命令: 使用 -cf 参数自动创建Flash头，一次性烧录SBL和App
arprog_cmdline_6844.exe -p %COM_PORT% ^
  -f1 "%PROJECT_ROOT%\1-SBL_Bootloader\sbl.release.appimage" ^
  -f2 "%PROJECT_ROOT%\2-HelloWorld_App\hello_world_system.release.appimage" ^
  -s SFLASH -c -cf

if errorlevel 1 (
    echo.
    echo   └─ 烧录失败！
    echo.
    echo ╔════════════════════════════════════════════════════════════╗
    echo ║ [错误] 烧录失败，请检查以下项目：                          ║
    echo ╚════════════════════════════════════════════════════════════╝
    echo.
    echo   1. COM端口是否正确？当前: %COM_PORT%
    echo      - 查看设备管理器中的"XDS110 Class Application/User UART"
    echo.
    echo   2. SOP开关是否正确？
    echo      - 应为 SOP_MODE1 (SOP0=OFF, SOP1=OFF)
    echo.
    echo   3. 串口是否被占用？
    echo      - 关闭其他串口终端程序
    echo.
    echo   4. 尝试以下操作：
    echo      - 按下板子的RESET按钮
    echo      - 重新给板子上电
    echo      - 更换USB端口
    echo.
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

echo.
echo   └─ 烧录完成！
echo.

REM ============================================================
REM Phase 3: 完成提示
REM ============================================================
echo [3/3] 烧录流程完成！
echo.

REM 计算耗时
set END_TIME=%time%

echo.
echo ╔════════════════════════════════════════════════════════════╗
echo ║                                                            ║
echo ║     ✓ 烧录完成！                                           ║
echo ║                                                            ║
echo ║     SBL:  已烧录到 0x2000                                  ║
echo ║     App:  已烧录到 0x42000                                 ║
echo ║                                                            ║
echo ╚════════════════════════════════════════════════════════════╝
echo.

echo ========================================
echo   下一步操作
echo ========================================
echo.
echo [1] 断开板子电源
echo.
echo [2] 修改SOP开关为运行模式
echo     从 SOP_MODE1 (SOP0=OFF, SOP1=OFF)
echo     改为 SOP_MODE2 (SOP0=OFF, SOP1=ON)
echo.
echo [3] 重新上电启动
echo.
echo [4] 打开串口终端查看输出
echo     └─ 端口: %COM_PORT%
echo     └─ 波特率: 115200 8N1
echo.
echo [5] 预期输出示例
echo.
echo ----------------------------------------
echo [BOOTLOADER_PROFILE] Boot Media       : FLASH
echo [BOOTLOADER_PROFILE] Boot Image Size  : 220 KB
echo [BOOTLOADER_PROFILE] Cores present    :
echo r5f0-0
echo c66ss0
echo [BOOTLOADER] Booting Cores ...
echo Hello World from r5f0-0 !
echo Hello World from c66ss0 !
echo ----------------------------------------
echo.
echo 如果看到以上输出，说明烧录成功！
echo.

cd /d "%SCRIPT_DIR%"
pause

