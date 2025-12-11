# 📡 Ti雷达项目

> AWRL6844 Pose and Fall Detection 移植

**更新日期**: 2025-12-09

---

## 📁 项目结构

```
Ti雷达项目/
 project-code/              # 代码
    pose-fall-awrl6844/   # 移植项目
 项目文档/                  # 文档 (主线)
    2-开发记录/           # 开发日志
 知识库/                    # 参考资料 (只读)
 README.md                 # 本文件
```

---

## 🚀 当前进度

| Phase | 状态 | 说明 |
|-------|------|------|
| 1-4 | ✅ 完成 | 规划、配置、固件、模型 |
| 5 | ✅ 完成 | FreeRTOS 编译、Appimage 生成 |
| **6** | **⏳ 进行中** | **硬件烧录与测试** |
| 7 | 待开始 | 优化调参 |

---

## 📚 主线文档

**最新计划**: `项目文档/2-开发记录/2025-12-11/2025-12-11_0900_完美Next_Step计划.md`

**开发记录**: `项目文档/2-开发记录/`

**代码**: `project-code/pose-fall-awrl6844/`

---

## ⏭️ 下一步 (Phase 6)

1. ✅ 硬件准备（AWRL6844 EVM + Uniflash）
2. ⏳ 固件烧录（pose_fall_awrl6844.appimage）
3. ⏳ 串口监控与日志验证
4. ⏳ 功能测试（雷达 + DPU + AI 推理）
5. ⏳ 性能测试与报告

**详细步骤**: 见 `2025-12-11_0900_完美Next_Step计划.md`

---

## 📦 关键产出

- ✅ **固件**: `build/pose_fall_awrl6844.appimage` (100 KB)
- ✅ **SOP 配置**: 烧录=00, 运行=01
- ✅ **GitHub**: https://github.com/hhtbing-wisefido/pose-radar-ti

---

**最后更新**: 2025-12-11
