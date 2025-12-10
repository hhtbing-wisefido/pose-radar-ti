@echo off
REM Compile SysConfig generated files

set CC=C:\ti\ccs2040\ccs\tools\compiler\ti-cgt-armllvm_4.0.4.LTS\bin\tiarmclang.exe
set SDK=C:\ti\MMWAVE_L_SDK_06_01_00_01
set FREERTOS=%SDK%\source\kernel\freertos\FreeRTOS-Kernel
set FREERTOS_CFG=%SDK%\source\kernel\freertos\config\xwrL684x\r5f
set FREERTOS_PORT=%SDK%\source\kernel\freertos\portable\TI_ARM_CLANG\ARM_CR5F

set PROJECT=%~dp0..
set FIRMWARE=%PROJECT%\firmware
set BUILD=%PROJECT%\build

set CFLAGS=-mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -O2 -g -Wall -c
set CFLAGS=%CFLAGS% -I"%FIRMWARE%\common" -I"%FIRMWARE%\mss" -I"%FIRMWARE%\mss\generated" -I"%FIRMWARE%\model"
set CFLAGS=%CFLAGS% -I"%SDK%\source" -I"%SDK%\firmware\mmwave_dfp"
set CFLAGS=%CFLAGS% -I"%FREERTOS%\include" -I"%FREERTOS_CFG%" -I"%FREERTOS_PORT%"
set CFLAGS=%CFLAGS% -DSOC_XWRL684X -DAWRL6844 -DUSE_TI_CNN_CLASSIFIER

echo Compiling SysConfig files...

for %%f in (
    ti_drivers_config.c
    ti_drivers_open_close.c
    ti_board_config.c
    ti_board_open_close.c
    ti_dpl_config.c
    ti_pinmux_config.c
    ti_power_clock_config.c
) do (
    echo   %%f
    "%CC%" %CFLAGS% -o "%BUILD%\%%~nf.o" "%FIRMWARE%\mss\generated\%%f"
)

echo Done!
