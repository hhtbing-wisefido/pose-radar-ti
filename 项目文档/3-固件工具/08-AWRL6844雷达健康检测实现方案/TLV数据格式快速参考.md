# 🔍 TI雷达固件TLV数据格式快速参考

## 标准Demo vs InCabin Demo - TLV Type ID对照表

| 数据类型 | 标准mmWave Demo | InCabin Demo | 兼容性 |
|---------|----------------|--------------|--------|
| **点云数据** | Type = 1<br/>`DETECTED_POINTS` | Type = 3001<br/>`POINT_CLOUD` | ❌ 不兼容 |
| **Range Profile** | Type = 2<br/>`RANGE_PROFILE` | Type = 2<br/>`RANGE_PROFILE` | ✅ 兼容 |
| **Stats统计** | Type = 6<br/>`STATS` | Type = 6<br/>`STATS` | ✅ 兼容 |
| **占用特征** | ❌ 无 | Type = 3002<br/>`OCCUPANCY_FEATURES` | - |
| **分类结果** | ❌ 无 | Type = 1041<br/>`CLASSIFICATION_RES` | - |
| **身高估计** | ❌ 无 | Type = 1042<br/>`HEIGHT_ESTIMATION` | - |
| **入侵检测** | ❌ 无 | Type = 12, 13<br/>`INTRUSION_DET_*` | - |

---

## 🎯 工具兼容性矩阵

| 固件 | 配置文件 | SDK Visualizer | InCabin GUI | 说明 |
|-----|---------|----------------|-------------|------|
| **mmwave_demo<br/>.release.appimage** | 6844_profile_<br/>4T4R_tdm.cfg | ✅ 能用 | ❌ 不能 | 标准Demo使用Type=1 |
| **InCabin固件** | incabin_<br/>compatible.cfg | ❌ 不能 | ✅ 能用 | InCabin使用Type=3001 |
| **自定义固件<br/>(基于标准Demo)** | 自定义.cfg | ✅ 能用 | ❌ 不能 | 遵循标准Demo TLV协议 |
| **自定义固件<br/>(基于InCabin)** | 自定义.cfg | ❌ 不能 | ✅ 能用 | 遵循InCabin TLV协议 |

---

## 🔴 关键点总结

### 为什么InCabin不能用SDK Visualizer？

```
SDK Visualizer解析逻辑：
if (tlv_type == 1):      # 标准Demo的点云
    parse_point_cloud()  # ✅ 能解析
    display_points()     # ✅ 显示点云

elif (tlv_type == 3001): # InCabin的点云
    skip()               # ❌ 不认识这个ID
                         # ❌ 跳过这个TLV块
                         # ❌ 导致Points Detected = 0
```

### 为什么InCabin需要独有格式？

1. **AI处理输出** - 需要传输CNN分类器的结果
2. **多区域监控** - 需要传输每个座位的占用状态
3. **身高估计** - CPD模式需要区分婴儿/成人
4. **数据优化** - 量化数据节省UART带宽

### 正确的测试方法

| 你的固件 | 应该用的工具 | 工具路径 |
|---------|------------|---------|
| 标准mmWave Demo | SDK Visualizer | `C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\visualizer\` |
| InCabin Demo | InCabin GUI | `C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src\occupancy_demo_gui.exe` |

---

## 📚 详细文档

完整技术分析请参考: [Part14-TLV数据格式与工具兼容性完整指南](../06-SDK固件研究/Part14-TLV数据格式与工具兼容性完整指南.md)

---

## 💡 快速诊断

**症状**: 雷达运行，但SDK Visualizer显示Points Detected = 0

**检查步骤**:

1. ✅ **确认固件类型**
   ```
   如果是InCabin固件 → 必须用InCabin GUI ⚠️
   如果是标准Demo固件 → 可以用SDK Visualizer ✅
   ```

2. ✅ **查看WebSocket日志**
   ```
   如果看到大量数据包 → 数据在传输，但工具无法解析
   如果无数据包 → 硬件/配置问题
   ```

3. ✅ **检查Range Profile**
   ```
   如果Range Profile有峰值 → 雷达工作正常，是工具兼容性问题
   如果Range Profile平坦 → 硬件或CFAR参数问题
   ```

---

**结论**: 工具选择必须匹配固件类型！🎯
