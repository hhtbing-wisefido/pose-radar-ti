@echo off
REM Link pose-fall-awrl6844 firmware

set LNK=C:\ti\ccs2040\ccs\tools\compiler\ti-cgt-armllvm_4.0.4.LTS\bin\tiarmclang.exe
set SDK=C:\ti\MMWAVE_L_SDK_06_01_00_01
set PROJECT=%~dp0..
set BUILD=%PROJECT%\build
set LINKER_CMD=%PROJECT%\firmware\mss\linker.cmd
set OUTPUT=%BUILD%\pose_fall_awrl6844.out

echo Linking...

"%LNK%" ^
  -mcpu=cortex-r5 ^
  -mfloat-abi=hard ^
  -mfpu=vfpv3-d16 ^
  -Wl,--ram_model ^
  -Wl,--reread_libs ^
  -Wl,--diag_suppress=10063 ^
  -Wl,-i"%SDK%\source\kernel\freertos\lib" ^
  -Wl,-i"%SDK%\source\drivers\lib" ^
  -Wl,-i"%SDK%\source\datapath\lib" ^
  -Wl,-i"%SDK%\source\alg\gtrack\lib" ^
  -Wl,-i"%SDK%\source\board\lib" ^
  -Wl,-i"%SDK%\source\control\lib" ^
  -Wl,-i"%SDK%\source\utils\lib" ^
  -Wl,-i"%SDK%\firmware\mmwave_dfp\mmwavelink\lib\xWRL68XX" ^
  -Wl,-i"%SDK%\firmware\mmwave_dfp\fecsslib\lib\xWRL68XX" ^
  -Wl,-i"C:\ti\ccs2040\ccs\tools\compiler\ti-cgt-armllvm_4.0.4.LTS\lib" ^
  -o "%OUTPUT%" ^
  "%LINKER_CMD%" ^
  "%BUILD%\main.o" ^
  "%BUILD%\pose_model_wrapper.o" ^
  "%BUILD%\pose_mss.o" ^
  "%BUILD%\gtrack_alloc.o" ^
  "%BUILD%\ti_drivers_config.o" ^
  "%BUILD%\ti_drivers_open_close.o" ^
  "%BUILD%\ti_board_config.o" ^
  "%BUILD%\ti_board_open_close.o" ^
  "%BUILD%\ti_dpl_config.o" ^
  "%BUILD%\ti_pinmux_config.o" ^
  "%BUILD%\ti_power_clock_config.o" ^
  -lfreertos.xwrL684x.r5f.ti-arm-clang.release.lib ^
  -ldatapath_tracker3d.xwrL684x.r5f.ti-arm-clang.release.lib ^
  -lalg_gtrack3d.xwrL684x.r5f.ti-arm-clang.release.lib ^
  -ldrivers.xwrL684x.r5f.ti-arm-clang.release.lib ^
  -lboard.xwrL684x.r5f.ti-arm-clang.release.lib ^
  -lcontrol.xwrL684x.r5f.ti-arm-clang.release.lib ^
  -lutils.xwrL684x.r5f.ti-arm-clang.release.lib ^
  -lmmwavelink_r5.lib ^
  -lfecss_ram_r5.lib ^
  -llibc.a

if %ERRORLEVEL% EQU 0 (
    echo Link successful!
    echo Output: %OUTPUT%
) else (
    echo Link failed!
    exit /b 1
)
