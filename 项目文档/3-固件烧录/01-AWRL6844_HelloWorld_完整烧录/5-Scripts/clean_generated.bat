@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo.
echo ========================================
echo   清理生成文件
echo ========================================
echo.

REM 获取脚本所在目录
set SCRIPT_DIR=%~dp0
set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

REM 项目根目录
cd /d "%SCRIPT_DIR%\.."
set PROJECT_ROOT=%CD%

echo [INFO] 项目目录: %PROJECT_ROOT%
echo.

echo [警告] 将删除以下内容：
echo.
echo   - 1-SBL_Bootloader\temp\
echo   - 2-HelloWorld_App\temp\
echo   - 4-Generated\temp\
echo   - 4-Generated\*.bin
echo   - 4-Generated\readback\
echo.

set /p CONFIRM="是否继续？(Y/N): "

if /i not "%CONFIRM%"=="Y" (
    echo.
    echo [INFO] 已取消清理操作
    pause
    exit /b 0
)

echo.
echo [INFO] 开始清理...
echo.

set DELETED_COUNT=0

REM 清理SBL temp目录
if exist "%PROJECT_ROOT%\1-SBL_Bootloader\temp" (
    echo [删除] 1-SBL_Bootloader\temp\
    rmdir /s /q "%PROJECT_ROOT%\1-SBL_Bootloader\temp"
    set /a DELETED_COUNT+=1
)

REM 清理App temp目录
if exist "%PROJECT_ROOT%\2-HelloWorld_App\temp" (
    echo [删除] 2-HelloWorld_App\temp\
    rmdir /s /q "%PROJECT_ROOT%\2-HelloWorld_App\temp"
    set /a DELETED_COUNT+=1
)

REM 清理Generated temp目录
if exist "%PROJECT_ROOT%\4-Generated\temp" (
    echo [删除] 4-Generated\temp\
    rmdir /s /q "%PROJECT_ROOT%\4-Generated\temp"
    set /a DELETED_COUNT+=1
)

REM 清理Generated readback目录
if exist "%PROJECT_ROOT%\4-Generated\readback" (
    echo [删除] 4-Generated\readback\
    rmdir /s /q "%PROJECT_ROOT%\4-Generated\readback"
    set /a DELETED_COUNT+=1
)

REM 清理Meta Image文件
if exist "%PROJECT_ROOT%\4-Generated\*.bin" (
    echo [删除] 4-Generated\*.bin
    del /q "%PROJECT_ROOT%\4-Generated\*.bin" 2>nul
    set /a DELETED_COUNT+=1
)

echo.
echo ========================================
echo [SUCCESS] 清理完成！
echo ========================================
echo.
echo [统计] 删除了 !DELETED_COUNT! 项内容
echo.
echo [INFO] 保留的文件：
echo   - 源文件 (.appimage)
echo   - 配置文件 (.json)
echo   - 工具文件 (.exe)
echo   - 文档文件 (.md)
echo.

cd /d "%SCRIPT_DIR%"
pause
