# 测试文件编码问题
# 读取flash_tool.py文件的前200行，检查是否有乱码

$filePath = "D:\7.项目资料\Ti雷达项目\项目文档\3-固件工具\01-AWRL6844固件系统工具\5-Scripts\flash_tool.py"

Write-Host "=== 当前PowerShell编码 ===" -ForegroundColor Yellow
Write-Host "InputEncoding: $([Console]::InputEncoding.EncodingName)"
Write-Host "OutputEncoding: $([Console]::OutputEncoding.EncodingName)"
Write-Host "`$OutputEncoding: $($OutputEncoding.EncodingName)"

Write-Host "`n=== 测试1: 使用默认编码读取 ===" -ForegroundColor Yellow
$content1 = Get-Content $filePath -Encoding Default | Select-Object -First 10
$content1 | ForEach-Object { Write-Host $_ }

Write-Host "`n=== 测试2: 使用UTF8编码读取 ===" -ForegroundColor Yellow
$content2 = Get-Content $filePath -Encoding UTF8 | Select-Object -First 10
$content2 | ForEach-Object { Write-Host $_ }

Write-Host "`n=== 结论 ===" -ForegroundColor Green
Write-Host "如果测试1有乱码，测试2正常，说明PowerShell默认编码不是UTF-8"
