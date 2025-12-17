@echo off
chcp 65001 >nul
title AWRL6844EVM 固件管理系统

echo.
echo ========================================
echo   AWRL6844EVM 固件智能管理系统
echo ========================================
echo.

:: 检查Python
python --version >nul 2>&1
if errorlevel 1 (
    echo ❌ 错误: 未找到Python，请先安装Python 3.8+
    echo.
    pause
    exit /b 1
)

echo ✅ Python已安装
echo.

:: 检查PyQt6
python -c "import PyQt6" >nul 2>&1
if errorlevel 1 (
    echo ⚠️  警告: 未安装PyQt6
    echo.
    echo 正在安装PyQt6...
    pip install PyQt6 -i https://pypi.tuna.tsinghua.edu.cn/simple
    echo.
)

echo ✅ 依赖检查完成
echo.
echo 🚀 启动程序...
echo.

:: 启动GUI程序
python awrl6844_gui_app.py

if errorlevel 1 (
    echo.
    echo ❌ 程序运行出错
    echo.
    pause
)
