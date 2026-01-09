# 📡 AWRL6844 健康检测雷达配置文件

## ✅ SDK Visualizer 兼容

**v3.0更新（2026-01-09）：CLI已更新为L-SDK标准格式，完全兼容SDK Visualizer！**

| 配置文件 | 兼容性 | 说明 |
|---------|-------|------|
| `health_detect_standard.cfg` | ✅ **推荐** | L-SDK标准格式，SDK Visualizer兼容 |
| `health_detect_4T4R.cfg` | ⚠️ 旧格式 | 保留参考，建议使用standard版本 |

---

## 📋 支持的命令

### L-SDK标准命令（SDK Visualizer使用）

```支持的命令
✅ sensorStart / sensorStop       - 传感器控制
✅ channelCfg                      - 通道配置
✅ chirpComnCfg                    - Chirp通用配置
✅ chirpTimingCfg                  - Chirp时序配置
✅ frameCfg                        - 帧配置
✅ guiMonitor                      - GUI监视器选择
✅ cfarProcCfg                     - CFAR处理配置
✅ cfarFovCfg                      - CFAR视场配置
✅ aoaProcCfg / aoaFovCfg          - AOA配置
✅ clutterRemoval                  - 杂波移除
✅ factoryCalibCfg / runtimeCalibCfg - 校准配置
✅ antGeometryBoard                - 天线几何
✅ adcDataSource / adcLogging      - ADC配置
✅ lowPowerCfg                     - 低功耗配置
✅ apllFreqShiftEn                 - APLL频偏
✅ adcDataDitherCfg                - ADC抖动
✅ gpAdcMeasConfig                 - GP ADC配置
```

### 健康检测扩展命令

```扩展命令
✅ presenceCfg                     - 人员存在检测配置
✅ help / version                  - 帮助/版本信息
```

### 旧版兼容命令

```旧版命令（仍然支持）
✅ profileCfg                      - 旧版Profile配置
✅ chirpCfg                        - 旧版Chirp配置
✅ cfarCfg                         - 旧版CFAR配置
```

---

## 🚀 使用方法

### 方式1：SDK Visualizer（推荐）

1. 打开SDK Visualizer
2. 连接设备（选择正确的COM端口）
3. 加载 `health_detect_standard.cfg`
4. 点击"Send Config"发送配置
5. 观察点云数据

### 方式2：串口终端

1. 打开串口终端（PuTTY/Tera Term）
2. 连接CLI端口，波特率115200
3. 确认SOP跳线为运行模式（S7-OFF, S8-ON）
4. 按S2复位键
5. 等待看到 `mmwDemo:/>` 提示符
6. 逐行发送 `health_detect_standard.cfg` 中的命令

---

## 📊 配置参数说明

### 基本参数

| 参数 | 值 | 说明 |
|------|-----|------|
| 频率 | 60 GHz | 起始频率 |
| 通道 | 4T4R | 全通道模式 |
| ADC采样数 | 256 | 范围分辨率相关 |
| Chirp数 | 64 | 每帧chirp数 |
| 帧周期 | 100 ms | 10 Hz帧率 |
| 检测范围 | 0.25-9.0 m | CFAR范围 |

### 适用场景

- 🏠 室内人员存在检测
- 💓 健康监测（呼吸/心跳）
- 👥 近距离目标检测

---

## 📁 文件说明

| 文件 | 说明 |
|------|------|
| `health_detect_standard.cfg` | **主配置文件** - L-SDK标准格式 |
| `health_detect_4T4R.cfg` | 旧版配置，保留参考 |
| `README.md` | 本文档 |

---

## 🔄 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| v3.0 | 2026-01-09 | CLI更新为L-SDK标准格式，SDK Visualizer兼容 |
| v2.0 | 2026-01-09 | 自定义CLI格式（已废弃） |
| v1.0 | 2026-01-08 | 初始版本 |
