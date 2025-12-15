# 👋 HelloWorld Application

> **最简单的AWRL6844应用示例 - 验证系统功能**

---

## 文件说明

### 1. hello_world_system.release.appimage

**文件大小**: ~220KB  
**来源**: `MMWAVE_L_SDK_06_01_00_01/examples/hello_world/`  
**类型**: 多核系统应用（R5F + DSP）

**包含内容**:
- **R5F Core**: 主控逻辑、串口输出
- **DSP Core**: DSP初始化示例
- **系统配置**: FreeRTOS任务调度

---

### 2. metaimage_cfg.release.json

**用途**: HelloWorld Meta Image生成配置

**关键配置项**:
```json
{
  "buildImages": [
    {
      "buildImagePath": "hello_world_r5_img.release.rig",
      "encryptEnable": "no"
    },
    {
      "buildImagePath": "hello_world_dsp_img.release.rig",
      "encryptEnable": "no"
    }
  ],
  "metaImageFile": "hello_world_system.release.appimage"
}
```

**与SBL配置的区别**:
- 包含2个核心镜像（R5F + DSP）
- 不包含Flash Header（由SBL提供）
- 加载地址不同（App区域：0x42000）

---

## HelloWorld功能

### 主要功能

1. **系统初始化**
   - R5F核心启动
   - DSP核心启动
   - 串口初始化

2. **串口输出**
   - 打印"Hello World!"
   - 显示系统信息
   - 输出设备ID

3. **LED控制**
   - GPIO初始化
   - LED闪烁（1Hz）

4. **FreeRTOS任务**
   - 主任务循环
   - 空闲任务

---

## 代码结构

### R5F Core代码

```c
// main.c (简化版)

void main(void)
{
    // 1. 系统初始化
    System_init();
    
    // 2. 串口初始化
    UART_init();
    UART_printf("\n***** Hello World! *****\n");
    
    // 3. DSP核心启动
    DSP_init();
    
    // 4. LED初始化
    GPIO_init();
    
    // 5. 主循环
    while(1)
    {
        GPIO_toggle(LED_PIN);
        Task_sleep(1000);  // 1秒延迟
        UART_printf("Tick\n");
    }
}
```

### DSP Core代码

```c
// dsp_main.c (简化版)

void main(void)
{
    // DSP初始化
    DSP_init();
    
    // 等待R5F命令
    while(1)
    {
        // IPC通信处理
        IPC_processMessages();
        
        // DSP空闲
        Task_sleep(100);
    }
}
```

---

## 生成HelloWorld Meta Image

### Step 1: 提取Build Images

```bash
..\3-Tools\buildImage_creator.exe -i hello_world_system.release.appimage
```

**生成文件**:
- `temp/hello_world_r5_img.release.rig`
- `temp/hello_world_dsp_img.release.rig`

---

### Step 2: 创建Meta Image

```bash
..\3-Tools\metaImage_creator.exe -config metaimage_cfg.release.json
```

**生成文件**:
- `hello_world_meta.bin`

**文件结构**:
```
hello_world_meta.bin:
  ├── Meta Header (~1KB)
  │   ├── Magic: 0x4D535452
  │   ├── Image Count: 2
  │   ├── Image 1 Info (R5F)
  │   └── Image 2 Info (DSP)
  ├── R5F Core Image (~100KB)
  │   ├── Load Address: 0x00000000
  │   ├── Entry Point: 0x00000100
  │   └── Binary Data
  └── DSP Core Image (~50KB)
      ├── Load Address: 0x21000000
      └── Binary Data
```

---

## 烧录到Flash

### 烧录命令

```bash
cd ..\3-Tools
.\arprog_cmdline_6844.exe -p COM3 -f ..\2-HelloWorld_App\hello_world_system.release.appimage -o 0x42000
```

### 参数说明

- `-o 0x42000`: 应用区起始地址（SBL区域结束后对齐）

**为什么是0x42000？**
- Flash Header占用0x0-0x1FFF，SBL占用0x2000-0x41FFF（共256KB）
- 应用从0x42000开始
- SBL会从0x42000读取并加载应用

---

## 串口输出示例

### 完整启动日志

```
**********************************************
*        AWRL6844 Secondary Bootloader      *
*             Version: 1.0.0                *
**********************************************

[SBL] Loading Application from Flash...
[SBL]   Address: 0x00042000
[SBL]   Size: 218,624 bytes
[SBL] Loading R5F image... Done
[SBL] Loading DSP image... Done
[SBL] Starting Application...

**********************************************
*         Hello World Application           *
**********************************************

[APP] System Initialize...
[APP]   R5F Core @ 200 MHz
[APP]   DSP Core @ 450 MHz
[APP]   UART @ 115200 baud
[APP] System Initialize... Done

[APP] Starting DSP Core...
[DSP] DSP Core Started
[DSP] Waiting for commands...

[APP] GPIO Initialize...
[APP]   LED Pin: GPIO45
[APP] GPIO Initialize... Done

Hello World from AWRL6844! 🎉

Device Information:
  Chip ID: 0x68440001
  Revision: 1.0
  Serial Number: 0x12345678
  Temperature: 45°C

System Status:
  R5F Core: Running
  DSP Core: Running
  UART: OK
  GPIO: OK

LED Blinking...
Tick
Tick
Tick
...
```

---

## 修改代码

### 修改输出内容

**修改文件**: `examples/hello_world/xwrL684x-evm/r5fss0-0_freertos/ti-arm-clang/main.c`

```c
// 找到这行
UART_printf("Hello World from AWRL6844!\n");

// 改为
UART_printf("Hello from My Custom App!\n");
```

**重新编译**:
1. 打开CCS (Code Composer Studio)
2. 导入项目
3. 编译生成新的.appimage
4. 重新生成Meta Image
5. 烧录到Flash

---

## 扩展功能

### 添加新功能示例

```c
// 添加温度读取
void readTemperature(void)
{
    float temp = SOC_getTemperature();
    UART_printf("Temperature: %.1f C\n", temp);
}

// 在main循环中调用
while(1)
{
    GPIO_toggle(LED_PIN);
    readTemperature();  // 新增
    Task_sleep(1000);
}
```

### 添加CAN通信

```c
// 初始化CAN
MCAN_init();

// 发送消息
uint8_t txData[8] = {0x01, 0x02, 0x03, 0x04};
MCAN_transmit(txData, 4);
```

---

## 性能特性

### 资源占用

| 资源 | 占用 | 总量 | 百分比 |
|------|------|------|--------|
| Flash | ~220KB | 2MB | 10.7% |
| RAM (R5F) | ~50KB | 512KB | 9.8% |
| RAM (DSP) | ~30KB | 1MB | 2.9% |

### 启动时间

| 阶段 | 耗时 |
|------|------|
| ROM Boot | ~50ms |
| SBL加载 | ~100ms |
| App加载 | ~150ms |
| **总计** | **~300ms** |

---

## 对比其他示例

### HelloWorld vs mmWave Demo

| 特性 | HelloWorld | mmWave Demo |
|------|-----------|-------------|
| 文件大小 | 220KB | 350KB |
| 功能 | 基本I/O | 雷达信号处理 |
| RAM占用 | 80KB | 1.5MB |
| 复杂度 | ⭐ | ⭐⭐⭐⭐ |
| 适合场景 | 系统验证 | 实际应用 |

---

## 常见问题

### Q1: 为什么需要R5F和DSP两个核心？

**A**: 
- **R5F**: 控制逻辑、外设通信
- **DSP**: 高性能信号处理（雷达数据）

HelloWorld中DSP核心是可选的，但保留用于演示多核启动。

### Q2: 如何禁用DSP核心？

**A**: 修改配置文件，移除DSP镜像：
```json
{
  "buildImages": [
    {  // 只保留R5F
      "buildImagePath": "hello_world_r5_img.release.rig"
    }
  ]
}
```

### Q3: 串口输出乱码？

**A**: 检查串口参数：
- 波特率: 115200
- 数据位: 8
- 校验位: None
- 停止位: 1

---

## 下一步

### 学习路径

1. ✅ **HelloWorld** - 系统验证
2. ⏭️ **GPIO Example** - 外设控制
3. ⏭️ **UART Example** - 串口通信
4. ⏭️ **mmWave Demo** - 雷达应用
5. ⏭️ **InCabin Demo** - 车载应用

---

## 相关文档

- [README.md](../README.md) - 项目概述
- [操作指南.md](../操作指南.md) - 烧录步骤
- [1-SBL_Bootloader/README.md](../1-SBL_Bootloader/README.md) - SBL详解

---

**更新日期**: 2025-12-12  
**SDK版本**: 06.01.00.01  
**示例类型**: FreeRTOS + Multi-Core
