# 📦 AWRL6844 健康检测项目代码

> **项目**: AWRL6844雷达健康检测
> **创建日期**: 2026-01-05
> **状态**: 🚧 开发中

---

## 📊 项目概览

本目录包含AWRL6844雷达健康检测的固件代码，实现人存检测、姿态检测、跌倒检测等功能。

---

## 📁 目录结构

```
AWRL6844_HealthDetect/
├── README.md                     # 本文件
├── src/                          # 源代码目录（待开发）
│   ├── main.c                    # 主程序入口
│   ├── health_detect.c/h         # 健康检测主模块
│   ├── pose_detect.c/h           # 姿态检测
│   ├── fall_detect.c/h           # 跌倒检测
│   ├── background_learn.c/h      # 背景学习
│   ├── zone_config.c/h           # 区域配置
│   ├── breath_detect.c/h         # 呼吸检测(Phase 2)
│   └── people_count.c/h          # 人数统计(Phase 2)
├── lib/                          # 依赖库（从SDK复制）
│   ├── cnn_classifier.lib        # CNN分类器库
│   ├── intrusionDetection.lib    # 入侵检测库
│   └── occupancyClassifier.lib   # 占用分类库
├── radar_config_file/            # 雷达配置文件 ⭐
│   ├── README.md                 # 配置文件说明
│   ├── profile_health.cfg        # 健康检测专用雷达配置
│   └── zone_default.cfg          # 默认区域配置
└── docs/                         # 项目文档
    ├── API参考.md
    └── 使用手册.md
```

---

## 📋 文件索引

| 目录/文件 | 说明 | 状态 |
|----------|------|------|
| `src/` | 源代码目录 | 🔲 待创建 |
| `lib/` | 依赖库目录 | 🔲 待创建 |
| `radar_config_file/` | 雷达配置文件 | ✅ 已创建 |
| `docs/` | 项目文档 | 🔲 待创建 |

---

## 🔗 相关文档

- **方案规划**: `项目文档/3-固件工具/08-AWRL6844雷达健康检测实现方案/`
- **配置参数研究**: `项目文档/3-固件工具/05-雷达配置参数研究/`
- **SDK固件研究**: `项目文档/3-固件工具/06-SDK固件研究/`
- **数据集**: `知识库/Pose_And_Fall_Detection/`

---

## 📅 开发计划

| Phase | 内容 | 状态 |
|-------|------|------|
| Phase 1A | 核心功能（人存/姿态/跌倒检测） | 🚧 开发中 |
| Phase 1B | 辅助功能（杂波移除/背景学习/区域划分） | 🔲 待开发 |
| Phase 2 | 扩展功能（呼吸检测/人数统计） | 🔲 待开发 |

---

> 📝 详细实施步骤见 `08-AWRL6844雷达健康检测实现方案/AWRL6844雷达健康检测-03-实施目录大纲.md`
