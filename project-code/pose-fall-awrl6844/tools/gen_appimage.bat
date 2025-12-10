@echo off
REM ===================================================================
REM Generate .appimage from .out file for AWRL6844
REM ===================================================================

echo.
echo ========================================
echo   Generating AWRL6844 Boot Image
echo ========================================
echo.

set SDK=C:\ti\MMWAVE_L_SDK_06_01_00_01
set TIARM=C:\ti\ccs2040\ccs\tools\compiler\ti-cgt-armllvm_4.0.4.LTS
set HEX=%TIARM%\bin\tiarmhex.exe
set PROJECT=%~dp0..
set BUILD=%PROJECT%\build
set FIRMWARE=%PROJECT%\firmware

set OUTFILE=%BUILD%\pose_fall_awrl6844.out
set TEMP_PATH=%BUILD%\temp
set CONFIG_PATH=%BUILD%\config

REM 检查 .out 文件
if not exist "%OUTFILE%" (
    echo [ERROR] Output file not found: %OUTFILE%
    echo Please run build first!
    exit /b 1
)

REM 步骤 1: 生成 HEX 文件
echo [1/3] Generating HEX files...
if not exist "%TEMP_PATH%" mkdir "%TEMP_PATH%"

set LINKER_HEX=%FIRMWARE%\mss\linker.cmd
set MEMORY_HEX=%SDK%\examples\mmw_demo\mmwave_demo\xwrL684x-evm\r5fss0-0_freertos\ti-arm-clang\memory_hex.cmd

if not exist "%MEMORY_HEX%" (
    echo [ERROR] memory_hex.cmd not found in SDK
    exit /b 1
)

copy /Y "%MEMORY_HEX%" "%BUILD%\memory_hex.cmd" >nul

cd /d "%BUILD%"
"%HEX%" -romwidth 32 -memwidth 8 -i pose_fall_awrl6844.out memory_hex.cmd
cd /d "%PROJECT%"

REM 检查生成的 HEX 文件 - 使用 8.3 短路径避免空格
set HEX1=%TEMP_PATH%\mmwave_demo_reset_vectors.hex
set HEX2=%TEMP_PATH%\mmwave_demo_tcma_ram.hex
set HEX3=%TEMP_PATH%\mmwave_demo_tcmb_rbl_reserv.hex
set HEX4=%TEMP_PATH%\mmwave_demo_tcmb_ram.hex

if exist "%HEX1%" echo   Found: mmwave_demo_reset_vectors.hex
if exist "%HEX2%" echo   Found: mmwave_demo_tcma_ram.hex

 REM 构建 HEX 文件参数
set HEX_FILES=
if exist "%HEX1%" set HEX_FILES=%HEX_FILES% --input_file "%HEX1%"
if exist "%HEX2%" set HEX_FILES=%HEX_FILES% --input_file "%HEX2%"
if exist "%HEX3%" set HEX_FILES=%HEX_FILES% --input_file "%HEX3%"
if exist "%HEX4%" set HEX_FILES=%HEX_FILES% --input_file "%HEX4%"

REM 步骤 2: 生成核心镜像 (.rig)
echo.
echo [2/3] Generating core image (.rig)...
set CORE_IMG_GEN=%SDK%\tools\MetaImageGen\buildImage_creator.exe
set CORE_IMAGE=%BUILD%\pose_fall_awrl6844_r5.rig

if not exist "%CORE_IMG_GEN%" (
    echo [ERROR] buildImage_creator.exe not found
    echo Path: %CORE_IMG_GEN%
    exit /b 1
)

REM 使用短路径避免空格问题
for %%I in ("%CORE_IMAGE%") do set CORE_IMAGE_SHORT=%%~sI
for %%I in ("%HEX1%") do set HEX1_SHORT=%%~sI
for %%I in ("%HEX2%") do set HEX2_SHORT=%%~sI

"%CORE_IMG_GEN%" -c APPSS --input_file %HEX1_SHORT% --input_file %HEX2_SHORT% --output_file %CORE_IMAGE_SHORT%

if not exist "%CORE_IMAGE%" (
    echo [ERROR] Core image generation failed
    exit /b 1
)

echo   Generated: pose_fall_awrl6844_r5.rig

REM 步骤 3: 生成启动镜像 (.appimage)
echo.
echo [3/3] Generating boot image (.appimage)...

if not exist "%CONFIG_PATH%" mkdir "%CONFIG_PATH%"

REM 定义所有路径
set META_CONFIG=%CONFIG_PATH%\metaimage_cfg.json
set RF_PATCH=%SDK%\firmware\mmwave_dfp\rfsfirmware\xWRL68xx\mmwave_rfs_patch.rig
set BOOTIMAGE=%BUILD%\pose_fall_awrl6844.appimage

REM 转换为正斜杠路径（JSON 兼容）
set JSON_CORE_RIG=%CORE_IMAGE:\=/%
set JSON_RF_PATCH=%RF_PATCH:\=/%
set JSON_BOOT=%BOOTIMAGE:\=/%

(
echo {
echo     "securityType": "gp",
echo     "flashIndex": "1",
echo     "metaHeaderInfo": {
echo         "metaImageType": "multi",
echo         "pbistEnablecontrol": "0",
echo         "sharedRamAllocationControl": "0",
echo         "loggerClear": "0xDD"
echo     },
echo     "buildImages": [
echo         {
echo             "buildImagePath": "%JSON_CORE_RIG%",
echo             "encryptEnable": "no"
echo         },
echo         {
echo             "buildImagePath": "%JSON_RF_PATCH%",
echo             "encryptEnable": "no"
echo         }
echo     ],
echo     "metaImageFile": "%JSON_BOOT%"
echo }
) > "%META_CONFIG%"

set META_IMG_GEN=%SDK%\tools\MetaImageGen\metaImage_creator.exe

if not exist "%META_IMG_GEN%" (
    echo [ERROR] metaImage_creator.exe not found
    echo Path: %META_IMG_GEN%
    exit /b 1
)

cd /d "%SDK%\tools\MetaImageGen"
metaImage_creator.exe --complete_metaimage "%META_CONFIG%"
cd /d "%PROJECT%"

REM 移动生成的 appimage
if exist "%BUILD%\pose_fall_awrl6844.appimage" (
    echo   Generated: pose_fall_awrl6844.appimage
) else (
    echo [ERROR] Boot image generation failed
    exit /b 1
)

REM 清理临时文件
if exist "%TEMP_PATH%" rd /s /q "%TEMP_PATH%"

echo.
echo ========================================
echo   Boot Image Generation Complete!
echo ========================================
echo.
echo Output files:
dir /b "%BUILD%\*.appimage" 2>nul
dir /b "%BUILD%\*.rig" 2>nul
echo.
echo Ready for Uniflash!
echo.
