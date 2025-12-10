# Ti SDK Environment Check (实际环境检查)
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  Ti Radar SDK Environment Check" -ForegroundColor Green
Write-Host "========================================`n" -ForegroundColor Cyan

$allPassed = $true
$tiRoot = "C:\ti"

# Check C:\ti directory
Write-Host "[1/6] Checking C:\ti directory..." -ForegroundColor Yellow
if (Test-Path $tiRoot) {
    Write-Host "  [OK] C:\ti exists" -ForegroundColor Green
    $items = Get-ChildItem $tiRoot -Directory | Select-Object Name
    Write-Host "  Found:" -ForegroundColor Cyan
    foreach ($item in $items) {
        Write-Host "    - $($item.Name)" -ForegroundColor White
    }
} else {
    Write-Host "  [FAIL] C:\ti not found!" -ForegroundColor Red
    $allPassed = $false
}

# Check mmWave L SDK
Write-Host "`n[2/6] Checking MMWAVE-L-SDK..." -ForegroundColor Yellow
$sdkPath = "$tiRoot\MMWAVE_L_SDK_06_01_00_01"
if (Test-Path $sdkPath) {
    Write-Host "  [OK] MMWAVE-L-SDK found: $sdkPath" -ForegroundColor Green
} else {
    Write-Host "  [FAIL] MMWAVE-L-SDK not found!" -ForegroundColor Red
    Write-Host "  Expected: $sdkPath" -ForegroundColor Yellow
    $allPassed = $false
}

# Check CCS
Write-Host "`n[3/6] Checking Code Composer Studio..." -ForegroundColor Yellow
$ccsPath = "$tiRoot\ccs2040"
if (Test-Path $ccsPath) {
    Write-Host "  [OK] CCS found: $ccsPath" -ForegroundColor Green
} else {
    Write-Host "  [FAIL] CCS not found!" -ForegroundColor Red
    Write-Host "  Expected: $ccsPath" -ForegroundColor Yellow
    $allPassed = $false
}

# Check Radar Toolbox
Write-Host "`n[4/6] Checking Radar Toolbox..." -ForegroundColor Yellow
$toolboxPath = "$tiRoot\radar_toolbox_3_30_00_06"
if (Test-Path $toolboxPath) {
    Write-Host "  [OK] Radar Toolbox found: $toolboxPath" -ForegroundColor Green
} else {
    Write-Host "  [FAIL] Radar Toolbox not found!" -ForegroundColor Red
    $allPassed = $false
}

# Check firmware directory
Write-Host "`n[5/6] Checking project firmware..." -ForegroundColor Yellow
$firmwareDir = Join-Path (Split-Path -Parent $PSScriptRoot) "firmware"
if (Test-Path $firmwareDir) {
    Write-Host "  [OK] firmware/ directory exists" -ForegroundColor Green
    
    # Check key files
    $keyFiles = @(
        "common\pose_types.h",
        "common\pose_config.h",
        "mss\pose_mss.h",
        "mss\pose_mss.c"
    )
    foreach ($file in $keyFiles) {
        $filePath = Join-Path $firmwareDir $file
        if (Test-Path $filePath) {
            Write-Host "  [OK] $file" -ForegroundColor Green
        } else {
            Write-Host "  [MISS] $file" -ForegroundColor Yellow
        }
    }
} else {
    Write-Host "  [FAIL] firmware/ not found!" -ForegroundColor Red
    $allPassed = $false
}

# Check COM ports (hardware)
Write-Host "`n[6/6] Checking hardware (COM ports)..." -ForegroundColor Yellow
$ports = [System.IO.Ports.SerialPort]::getportnames()
if ($ports.Count -gt 0) {
    Write-Host "  [INFO] Found COM ports: $($ports -join ', ')" -ForegroundColor Cyan
    if ($ports -contains "COM3" -and $ports -contains "COM4") {
        Write-Host "  [INFO] AWRL6844 typical ports detected (COM3, COM4)" -ForegroundColor Green
    }
} else {
    Write-Host "  [WARN] No COM ports detected (hardware not connected?)" -ForegroundColor Yellow
}

# Summary
Write-Host "`n========================================" -ForegroundColor Cyan
if ($allPassed) {
    Write-Host "  ✅ All checks PASSED!" -ForegroundColor Green
    Write-Host "  Ready to start Phase 3.2 SDK integration" -ForegroundColor Green
} else {
    Write-Host "  ❌ Some checks FAILED!" -ForegroundColor Red
    Write-Host "  Please install missing components from C:\ti" -ForegroundColor Red
}
Write-Host "========================================`n" -ForegroundColor Cyan

# Return exit code
if ($allPassed) { exit 0 } else { exit 1 }
