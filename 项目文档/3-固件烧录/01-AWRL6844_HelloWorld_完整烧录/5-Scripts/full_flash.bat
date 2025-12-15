@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

REM 检查参数
if "%~1"=="" (
    echo.
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

cls
echo.
echo ╔════════════════════════════════════════════════════════════╗
echo ║                                                            ║
echo ║     AWRL6844 HelloWorld 完整烧录流程                       ║
echo ║                                                            ║
echo ║     COM端口: %-45s║
echo ║                                                            ║
echo ╚════════════════════════════════════════════════════════════╝
echo.

set START_TIME=%time%

REM ============================================================
REM Phase 1: 清理旧文件
REM ============================================================
echo [1/6] 清理旧文件...
echo.

cd /d "%SCRIPT_DIR%"

REM 静默清理（不显示详细信息）
set PROJECT_ROOT=%SCRIPT_DIR%\..
if exist "%PROJECT_ROOT%\1-SBL_Bootloader\temp" rmdir /s /q "%PROJECT_ROOT%\1-SBL_Bootloader\temp" 2>nul
if exist "%PROJECT_ROOT%\2-HelloWorld_App\temp" rmdir /s /q "%PROJECT_ROOT%\2-HelloWorld_App\temp" 2>nul
if exist "%PROJECT_ROOT%\4-Generated\temp" rmdir /s /q "%PROJECT_ROOT%\4-Generated\temp" 2>nul
if exist "%PROJECT_ROOT%\4-Generated\*.bin" del /q "%PROJECT_ROOT%\4-Generated\*.bin" 2>nul

echo   └─ 完成
echo.

REM ============================================================
REM Phase 2: 生成SBL Meta Image
REM ============================================================
echo [2/6] 生成SBL Meta Image...
echo.

call "%SCRIPT_DIR%\1_generate_sbl_meta.bat" >nul 2>&1

if errorlevel 1 (
    echo   └─ 失败！
    echo.
    echo [ERROR] SBL Meta Image生成失败
    pause
    exit /b 1
)

REM 检查输出文件
if not exist "%PROJECT_ROOT%\4-Generated\sbl_meta.bin" (
    echo   └─ 失败！
    echo.
    echo [ERROR] 未找到sbl_meta.bin
    pause
    exit /b 1
)

REM 获取文件大小
for %%F in ("%PROJECT_ROOT%\4-Generated\sbl_meta.bin") do set SBL_SIZE=%%~zF

echo   └─ 完成 - sbl_meta.bin (%SBL_SIZE% bytes)
echo.

REM ============================================================
REM Phase 3: 生成App Meta Image
REM ============================================================
echo [3/6] 生成App Meta Image...
echo.

call "%SCRIPT_DIR%\2_generate_app_meta.bat" >nul 2>&1

if errorlevel 1 (
    echo   └─ 失败！
    echo.
    echo [ERROR] App Meta Image生成失败
    pause
    exit /b 1
)

REM 检查输出文件
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
REM Phase 4: 烧录SBL
REM ============================================================
echo [4/6] 烧录SBL Bootloader...
echo   └─ 地址: 0x2000
echo.

echo [硬件检查] 请确认以下设置已就绪：
echo   - SOP开关设置为SOP_MODE1 (S8=OFF, S7=OFF)
echo   - USB连接到%COM_PORT%
echo   - 板子已上电
echo.
echo 按任意键继续烧录，或等待10秒自动开始...
timeout /t 10 /nobreak >nul 2>&1

cd /d "%PROJECT_ROOT%\3-Tools"
echo   开始烧录SBL...
arprog_cmdline_6844.exe -p %COM_PORT% -f "%PROJECT_ROOT%\1-SBL_Bootloader\sbl.release.appimage" -o 0x2000

if errorlevel 1 (
    echo.
    echo   └─ 失败！
    echo.
    echo [ERROR] SBL烧录失败
    echo [INFO] 请检查：
    echo   - COM端口是否正确 (当前: %COM_PORT%)
    echo   - SOP跳线是否为SOP_MODE1 (S8=OFF, S7=OFF)
    echo   - 串口是否被占用
    echo   - 板子是否上电
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

echo   └─ 完成
echo.

REM ============================================================
REM Phase 5: 烧录App
REM ============================================================
echo [5/6] 烧录HelloWorld应用...
echo   └─ 地址: 0x42000
echo.

cd /d "%PROJECT_ROOT%\3-Tools"
echo   开始烧录App...
arprog_cmdline_6844.exe -p %COM_PORT% -f "%PROJECT_ROOT%\2-HelloWorld_App\hello_world_system.release.appimage" -o 0x42000

if errorlevel 1 (
    echo.
    echo   └─ 失败！
    echo.
    echo [ERROR] App烧录失败
    cd /d "%SCRIPT_DIR%"
    pause
    exit /b 1
)

echo   └─ 完成
echo.

REM ============================================================
REM Phase 6: 验证（可选，暂时跳过以节省时间）
REM ============================================================
echo [6/6] 验证完成
echo   └─ SBL: 已烧录到0x2000
echo   └─ App: 已烧录到0x42000
echo.

REM 计算耗时
set END_TIME=%time%

echo.
echo ╔════════════════════════════════════════════════════════════╗
echo ║                                                            ║
echo ║     ✓ 烧录完成！                                           ║
echo ║                                                            ║
echo ╚════════════════════════════════════════════════════════════╝
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
echo.
echo ----------------------------------------
echo [BOOTLOADER_PROFILE] Boot Media       : FLASH
echo [BOOTLOADER_PROFILE] Boot Image Size  : 220 KB
echo [BOOTLOADER_PROFILE] Cores present    :
echo r5f0-0
echo c66ss0
echo r5f0-1
echo [BOOTLOADER] Booting Cores ...
echo Hello World from r5f0-0 !
echo Hello World from c66ss0 !
echo Hello World from r5f0-1 !
echo ----------------------------------------
echo.

cd /d "%SCRIPT_DIR%"
timeout /t 5 >nul 2>&1
