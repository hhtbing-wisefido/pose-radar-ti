@echo off
REM ===================================================================
REM AWRL6844 Pose Detection Firmware - Complete Build Script
REM ===================================================================

echo.
echo ========================================
echo   AWRL6844 Firmware Complete Build
echo ========================================
echo.

REM 步骤 1: 编译 SysConfig 生成的文件
echo [1/3] Compiling SysConfig files...
call tools\compile_syscfg.bat
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] SysConfig compilation failed!
    exit /b 1
)

REM 步骤 2: 编译项目源文件
echo.
echo [2/3] Compiling project source files...

set CC=C:\ti\ccs2040\ccs\tools\compiler\ti-cgt-armllvm_4.0.4.LTS\bin\tiarmclang.exe
set SDK=C:\ti\MMWAVE_L_SDK_06_01_00_01
set FREERTOS=%SDK%\source\kernel\freertos\FreeRTOS-Kernel
set FREERTOS_CFG=%SDK%\source\kernel\freertos\config\xwrL684x\r5f
set FREERTOS_PORT=%SDK%\source\kernel\freertos\portable\TI_ARM_CLANG\ARM_CR5F

set CFLAGS=-mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -O2 -g -Wall -c
set CFLAGS=%CFLAGS% -I"firmware\common" -I"firmware\mss" -I"firmware\mss\generated" -I"firmware\model"
set CFLAGS=%CFLAGS% -I"%SDK%\source" -I"%SDK%\firmware\mmwave_dfp"
set CFLAGS=%CFLAGS% -I"%FREERTOS%\include" -I"%FREERTOS_CFG%" -I"%FREERTOS_PORT%"
set CFLAGS=%CFLAGS% -DSOC_XWRL684X -DAWRL6844 -DUSE_TI_CNN_CLASSIFIER

for %%f in (
    firmware\mss\main.c
    firmware\mss\pose_mss.c
    firmware\mss\gtrack_alloc.c
    firmware\model\pose_model_wrapper.c
) do (
    echo   Compiling %%~nxf...
    "%CC%" %CFLAGS% -o "build\%%~nf.o" "%%f"
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Compilation failed: %%~nxf
        exit /b 1
    )
)

REM 步骤 3: 链接生成固件
echo.
echo [3/4] Linking firmware...
call tools\link.bat
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Linking failed!
    exit /b 1
)

REM 步骤 4: 生成 appimage
echo.
echo [4/4] Generating boot image...
call tools\gen_appimage.bat
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Boot image generation failed!
    exit /b 1
)

echo.
echo ========================================
echo   Build Complete!
echo ========================================
echo.
echo Output files:
echo   - pose_fall_awrl6844.out (ELF)
echo   - pose_fall_awrl6844_r5.rig (Core Image)
echo   - pose_fall_awrl6844.appimage (Boot Image - Ready for Uniflash!)
echo.
dir build\pose_fall_awrl6844.* | findstr /v "目录"
echo.
