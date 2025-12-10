@echo off
REM GitHub 推送脚本
REM 使用方法：编辑此文件，填写您的 GitHub 仓库地址，然后运行

echo ========================================
echo   GitHub 推送脚本
echo ========================================
echo.

REM ===== 配置区域 - 请修改以下变量 =====
REM 示例：set GITHUB_REPO=https://github.com/username/ti-radar-awrl6844.git
set GITHUB_REPO=

REM ===== 请勿修改以下内容 =====

if "%GITHUB_REPO%"=="" (
    echo [错误] 请先配置 GITHUB_REPO 变量
    echo.
    echo 打开此文件，找到 "set GITHUB_REPO=" 这行
    echo 填写您的 GitHub 仓库地址
    echo 示例：set GITHUB_REPO=https://github.com/username/repo.git
    echo.
    pause
    exit /b 1
)

echo [1/4] 检查 Git 状态...
git status

echo.
echo [2/4] 检查远程仓库配置...
git remote -v

REM 检查是否已配置远程仓库
git remote | findstr "origin" >nul
if errorlevel 1 (
    echo [3/4] 添加远程仓库: %GITHUB_REPO%
    git remote add origin %GITHUB_REPO%
) else (
    echo [3/4] 远程仓库已存在，更新地址
    git remote set-url origin %GITHUB_REPO%
)

echo.
echo [4/4] 推送到 GitHub...
echo 分支：master
echo 仓库：%GITHUB_REPO%
echo.

git push -u origin master

if errorlevel 1 (
    echo.
    echo [失败] 推送失败，可能原因：
    echo   - GitHub 仓库不存在
    echo   - 没有权限
    echo   - 网络问题
    echo.
    echo 请检查：
    echo 1. GitHub 仓库地址是否正确
    echo 2. 是否已在 GitHub 创建该仓库
    echo 3. 是否配置了 SSH 密钥或访问令牌
    echo.
) else (
    echo.
    echo [成功] 推送完成！
    echo.
    echo 知识库目录已忽略，不会上传到 GitHub
    echo.
)

pause
