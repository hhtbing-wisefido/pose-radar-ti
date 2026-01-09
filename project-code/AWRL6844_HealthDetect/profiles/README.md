# 📡 AWRL6844 健康检测雷达配置文件

## 🚨 重要说明

**❗ HealthDetect固件使用自定义CLI命令格式，与标准mmw_demo不兼容！**

| 配置文件 | 兼容性 | 说明 |
|---------|-------|------|
| `health_detect_simple.cfg` | ✅ **推荐** | 适配HealthDetect固件CLI |
| `health_detect_4T4R.cfg` | ❌ 不兼容 | mmw_demo格式，不适用于本固件 |

### 🔴 "Error in Setting up device" 错误原因

如果SDK Visualizer显示这个错误，说明：
1. 配置文件中包含固件不识别的命令
2. HealthDetect固件CLI只支持以下命令：

```支持的命令
✅ sensorStart / sensorStop
✅ profileCfg
✅ chirpCfg
✅ frameCfg
✅ channelCfg
✅ cfarCfg
✅ presenceCfg
✅ help / version
```

```不支持的命令（mmw_demo专用）
❌ apllFreqShiftEn
❌ chirpComnCfg / chirpTimingCfg
❌ guiMonitor
❌ cfarProcCfg / cfarFovCfg
❌ aoaProcCfg / aoaFovCfg
❌ factoryCalibCfg / runtimeCalibCfg
❌ lowPowerCfg
... 等等
```

---

## 📋 配置文件列表

| 文件名 | 用途 | 状态 |
|--------|------|------|
| `health_detect_simple.cfg` | HealthDetect固件专用配置 | ✅ 推荐使用 |
| `health_detect_4T4R.cfg` | mmw_demo标准格式（参考） | ⚠️ 不适用于本固件 |

## 🚀 使用方法

### 方式1：串口终端发送（推荐）

1. 打开串口终端（PuTTY/Tera Term）
2. 连接CLI端口（如COM3），波特率115200
3. 确认SOP跳线为运行模式（S7-OFF, S8-ON）
4. 按S2复位键
5. 等待看到固件启动信息
6. 发送 `help` 确认固件响应
7. 逐行发送 `health_detect_simple.cfg` 中的命令

### 方式2：PowerShell脚本发送

```powershell
# 使用PowerShell发送配置
$port = New-Object System.IO.Ports.SerialPort COM3,115200
$port.Open()

Get-Content "health_detect_simple.cfg" | ForEach-Object {
    if ($_ -notmatch "^%" -and $_.Trim() -ne "") {
        $port.WriteLine($_)
        Write-Host "Sent: $_"
        Start-Sleep -Milliseconds 100
    }
}

$port.Close()
```

### 方式3：Python脚本发送

```python
import serial
import time

port = serial.Serial('COM3', 115200)

with open('health_detect_simple.cfg', 'r') as f:
    for line in f:
        line = line.strip()
        if line and not line.startswith('%'):
            port.write((line + '\r\n').encode())
            print(f'Sent: {line}')
            time.sleep(0.1)

port.close()
```

## ⚙️ 配置参数说明

### 关键参数

| 参数 | 值 | 说明 |
|------|-----|------|
| `channelCfg` | 153 255 0 | 4T4R模式 |
| `framePeriodicity` | 100ms | 10Hz帧率 |
| `cfarFovCfg 0` | 0.25~9.0m | 距离范围 |
| `clutterRemoval` | 0 | 关闭杂波移除（保留静态目标） |
| `lowPowerCfg` | 1 | 低功耗模式 |

### 健康检测优化

- **帧率**：10Hz足够检测呼吸(0.2-0.5Hz)和心跳(1-2Hz)
- **距离范围**：0.25-9m覆盖室内场景
- **杂波移除**：关闭，保留静态目标用于人员存在检测

## 📚 参考来源

- 基于 `mmw_demo/profiles/profile_4T4R_tdm.cfg`
- SDK版本：MMWAVE_L_SDK 06.01.00.01

## 📅 更新记录

| 日期 | 版本 | 说明 |
|------|------|------|
| 2026-01-09 | v1.0 | 初始版本，基于mmw_demo优化 |
