# AWRL6844 Pose and Fall Detection - Build Script
# 编译 MSS (R5F) 代码

$ErrorActionPreference = "Continue"

# 编译器路径
$TIARM = "C:\ti\ccs2040\ccs\tools\compiler\ti-cgt-armllvm_4.0.4.LTS"
$CC = "$TIARM\bin\tiarmclang.exe"

# SDK 路径
$SDK = "C:\ti\MMWAVE_L_SDK_06_01_00_01"

# 项目路径 (tools/ 的上级目录)
$PROJECT = Split-Path $PSScriptRoot -Parent
$FIRMWARE = "$PROJECT\firmware"
$BUILD = "$PROJECT\build"

# 创建 build 目录
if (-not (Test-Path $BUILD)) {
    New-Item -ItemType Directory -Path $BUILD -Force | Out-Null
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  AWRL6844 Pose and Fall Detection Build" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 检查编译器
if (-not (Test-Path $CC)) {
    Write-Host "[ERROR] TI ARM Clang not found: $CC" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Compiler: $CC" -ForegroundColor Green

# FreeRTOS 路径
$FREERTOS = "$SDK\source\kernel\freertos\FreeRTOS-Kernel"
$FREERTOS_CFG = "$SDK\source\kernel\freertos\config\xwrL684x\r5f"
$FREERTOS_PORT = "$SDK\source\kernel\freertos\portable\TI_ARM_CLANG\ARM_CR5F"

# 编译选项 (MSS - Cortex-R5F)
$CFLAGS = @(
    "-mcpu=cortex-r5",
    "-mfloat-abi=hard",
    "-mfpu=vfpv3-d16",
    "-mlittle-endian",
    "-O2",
    "-g",
    "-Wall",
    "-c",
    # 项目头文件
    "-I`"$FIRMWARE\common`"",
    "-I`"$FIRMWARE\mss`"",
    "-I`"$FIRMWARE\mss\generated`",
    "-I`"$FIRMWARE\model`"",
    # SDK 源码
    "-I`"$SDK\source`"",
    "-I`"$SDK\firmware\mmwave_dfp`"",
    # FreeRTOS
    "-I`"$FREERTOS\include`"",
    "-I`"$FREERTOS_CFG`"",
    "-I`"$FREERTOS_PORT`"",
    # 平台定义
    "-DSOC_XWRL684X",
    "-DAWRL6844",
    "-DUSE_TI_CNN_CLASSIFIER"
)

Write-Host ""
Write-Host "[1/3] Compiling MSS files..." -ForegroundColor Yellow

# 编译 common 文件 (只有头文件，跳过)

# 编译 model 文件
$modelFiles = @(
    "$FIRMWARE\model\pose_model_wrapper.c"
)

foreach ($src in $modelFiles) {
    if (Test-Path $src) {
        $obj = "$BUILD\" + [System.IO.Path]::GetFileNameWithoutExtension($src) + ".o"
        $srcName = [System.IO.Path]::GetFileName($src)
        Write-Host "  Compiling $srcName..." -NoNewline
        
        $args = $CFLAGS + @("-o", "`"$obj`"", "`"$src`"")
        $result = & $CC $args 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host " OK" -ForegroundColor Green
        } else {
            Write-Host " FAILED" -ForegroundColor Red
            Write-Host $result -ForegroundColor Red
        }
    }
}

# 编译 SysConfig 生成的文件
$syscfgFiles = @(
    "$FIRMWARE\mss\generated\ti_drivers_config.c",
    "$FIRMWARE\mss\generated\ti_drivers_open_close.c",
    "$FIRMWARE\mss\generated\ti_board_config.c",
    "$FIRMWARE\mss\generated\ti_board_open_close.c",
    "$FIRMWARE\mss\generated\ti_dpl_config.c",
    "$FIRMWARE\mss\generated\ti_pinmux_config.c",
    "$FIRMWARE\mss\generated\ti_power_clock_config.c"
)

foreach ($src in $syscfgFiles) {
    if (Test-Path $src) {
        $obj = "$BUILD\" + [System.IO.Path]::GetFileNameWithoutExtension($src) + ".o"
        $srcName = [System.IO.Path]::GetFileName($src)
        Write-Host "  Compiling $srcName..." -NoNewline
        
        $args = $CFLAGS + @("-I`"$FIRMWARE\mss\generated`"", "-o", "`"$obj`"", "`"$src`"")
        $result = & $CC $args 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host " OK" -ForegroundColor Green
        } else {
            Write-Host " FAILED" -ForegroundColor Red
            Write-Host $result -ForegroundColor Red
        }
    }
}

# 编译 MSS 文件
$mssFiles = @(
    "$FIRMWARE\mss\main.c",
    "$FIRMWARE\mss\pose_mss.c"
)

foreach ($src in $mssFiles) {
    if (Test-Path $src) {
        $obj = "$BUILD\" + [System.IO.Path]::GetFileNameWithoutExtension($src) + ".o"
        $srcName = [System.IO.Path]::GetFileName($src)
        Write-Host "  Compiling $srcName..." -NoNewline
        
        $args = $CFLAGS + @("-o", "`"$obj`"", "`"$src`"")
        $result = & $CC $args 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host " OK" -ForegroundColor Green
        } else {
            Write-Host " FAILED" -ForegroundColor Red
            Write-Host $result -ForegroundColor Red
        }
    }
}

Write-Host ""
Write-Host "[2/4] Build Summary:" -ForegroundColor Yellow
$objFiles = Get-ChildItem "$BUILD\*.o" -ErrorAction SilentlyContinue
if ($objFiles) {
    Write-Host "  Generated $($objFiles.Count) object files" -ForegroundColor Green
    foreach ($obj in $objFiles) {
        Write-Host "    - $($obj.Name)" -ForegroundColor Gray
    }
} else {
    Write-Host "  No object files generated" -ForegroundColor Red
    Write-Host ""
    Write-Host "[FAILED] Compilation failed, skipping link" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[3/4] Linking..." -ForegroundColor Yellow

# 链接器
$LNK = "$TIARM\bin\tiarmclang.exe"
$LINKER_CMD = "$FIRMWARE\mss\linker.cmd"

# SDK 库路径
$SDK_LIB = "$SDK\source"
$DFP_LIB = "$SDK\firmware\mmwave_dfp"

# 链接选项
$LFLAGS = @(
    "-mcpu=cortex-r5",
    "-mfloat-abi=hard",
    "-mfpu=vfpv3-d16",
    "-Wl,--ram_model",
    "-Wl,--reread_libs",
    "-Wl,--diag_suppress=10063",
    # 库搜索路径
    "-Wl,-i`"$SDK_LIB\kernel\freertos\lib`"",
    "-Wl,-i`"$SDK_LIB\drivers\lib`"",
    "-Wl,-i`"$SDK_LIB\datapath\lib`"",
    "-Wl,-i`"$SDK_LIB\alg\gtrack\lib`"",
    "-Wl,-i`"$SDK_LIB\board\lib`"",
    "-Wl,-i`"$SDK_LIB\control\lib`"",
    "-Wl,-i`"$SDK_LIB\utils\lib`"",
    "-Wl,-i`"$DFP_LIB\mmwavelink\lib\xWRL68XX`"",
    "-Wl,-i`"$DFP_LIB\fecsslib\lib\xWRL68XX`"",
    "-Wl,-i`"$TIARM\lib`""
)

# 需要链接的库
$LIBS = @(
    "-lfreertos.xwrL684x.r5f.ti-arm-clang.release.lib",
    "-ldatapath_tracker3d.xwrL684x.r5f.ti-arm-clang.release.lib",
    "-lalg_gtrack3d.xwrL684x.r5f.ti-arm-clang.release.lib",
    "-ldrivers.xwrL684x.r5f.ti-arm-clang.release.lib",
    "-lboard.xwrL684x.r5f.ti-arm-clang.release.lib",
    "-lcontrol.xwrL684x.r5f.ti-arm-clang.release.lib",
    "-lutils.xwrL684x.r5f.ti-arm-clang.release.lib",
    "-lmmwavelink_r5.lib",
    "-lfecss_ram_r5.lib",
    "-llibc.a"
)

# 输出文件
$OUTPUT = "$BUILD\pose_fall_awrl6844.out"

# 收集所有 .o 文件
$objList = (Get-ChildItem "$BUILD\*.o" | ForEach-Object { "`"$($_.FullName)`"" }) -join " "

# 构建链接命令
$linkArgs = $LFLAGS + @("-o", "`"$OUTPUT`"", "`"$LINKER_CMD`"") + $objList.Split(" ") + $LIBS

Write-Host "  Linking to pose_fall_awrl6844.out..." -NoNewline
$linkResult = & $LNK $linkArgs 2>&1

if ($LASTEXITCODE -eq 0) {
    Write-Host " OK" -ForegroundColor Green
    $outSize = (Get-Item $OUTPUT).Length / 1024
    Write-Host "  Output: $OUTPUT ($([math]::Round($outSize, 1)) KB)" -ForegroundColor Green
} else {
    Write-Host " FAILED" -ForegroundColor Red
    Write-Host $linkResult -ForegroundColor Red
    Write-Host ""
    Write-Host "[NOTE] Link requires SysConfig generated files (ti_drivers_config.c, etc.)" -ForegroundColor Yellow
    Write-Host "       Run SysConfig or use SDK example project for full build." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "[4/4] Done!" -ForegroundColor Cyan
