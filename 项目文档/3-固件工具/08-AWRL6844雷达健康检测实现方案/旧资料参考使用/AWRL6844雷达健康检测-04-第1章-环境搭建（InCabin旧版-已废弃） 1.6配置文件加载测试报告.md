# 项目文档\3-固件工具\08-AWRL6844雷达健康检测实现方案🧪 AWRL6844 配置文件加载测试报告

**测试日期**：2026年1月6日
**固件版本**：自编译 InCabin Demo（基于SDK 06.01.00.01）
**测试工具**：SDK Visualizer
**硬件状态**：SOP开关设置为FUNCTIONAL模式，已复位

---

## 📋 测试环境

### 硬件配置

### 硬件配置

**SOP跳线设置**：

![SOP设置](image/AWRL6844雷达健康检测-04-第1章-环境搭建1.6配置文件加载测试报告/1767691373902.png)

- S7: ON（FUNCTIONAL模式）
- S8: ON（FUNCTIONAL模式）

**串口连接**：

- 配置端口：COM3 @ 115200
- 数据端口：COM4 @ 1250000

---

## 🧪 测试配置文件列表

### 通用配置文件

| 编号 | 文件                          | 路径                                                                                      | 用途                      |
| ---- | ----------------------------- | ----------------------------------------------------------------------------------------- | ------------------------- |
| 1    | `6844_profile_4T4R_tdm.cfg` | `C:\ti\MMWAVE_L_SDK_06_01_00_01\tools\visualizer\tmp\`                                  | SDK Visualizer默认配置 ✅ |
| 2    | `6844_profile_4T4R_tdm.cfg` | `C:\ti\radar_toolbox_3_30_00_06\tools\mmwave_data_recorder\src\cfg\`                    | 数据采集工具配置 ✅       |
| 3    | `xWRL6844_4T4R_tdm.cfg`     | `C:\ti\radar_toolbox_3_30_00_06\tools\Adc_Data_Capture_Tool_DCA1000_CLI\chirp_configs\` | ADC采集配置               |

#### InCabin Demo专用配置

| 编号 | 文件                           | 路径                                                                                            | 用途              |
| ---- | ------------------------------ | ----------------------------------------------------------------------------------------------- | ----------------- |
| 4    | `cpd.cfg`                    | `C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src\chirpConfigs6844\` | 儿童存在检测(CPD) |
| 5    | `sbr.cfg`                    | 同上                                                                                            | 安全带提醒(SBR)   |
| 6    | `intrusion_detection.cfg`    | 同上                                                                                            | 入侵检测          |
| 7    | `intrusion_detection_LP.cfg` | 同上                                                                                            | 低功耗入侵检测    |

以下是发送配置文件后的结果:

编号1,编号2 结果如下:

**Please update to latest demo (on the latest HW, may not apply to older HW)!**

The device is running an older version of mmWave SDK (05.06.00.00) and is not guaranteed to work correctly with the current Visualizer. Please flash the latest demo that can be found under <SDK_INSTALL_ROOT>/examples/mmw_demo/ `<demo>`/prebuilt_binaries/`<device>`/*.appimage.

![1767691651042](image/AWRL6844雷达健康检测-04-第1章-环境搭建1.6配置文件加载测试报告/1767691651042.png)

编号3,4,5,6,7结果如下:

**Error in Setting up device**

Please try again.

![1767691823725](image/AWRL6844雷达健康检测-04-第1章-环境搭建1.6配置文件加载测试报告/1767691823725.png)![1767691866689](image/AWRL6844雷达健康检测-04-第1章-环境搭建1.6配置文件加载测试报告/1767691866689.png)

![1767691889658](image/AWRL6844雷达健康检测-04-第1章-环境搭建1.6配置文件加载测试报告/1767691889658.png)![1767691903080](image/AWRL6844雷达健康检测-04-第1章-环境搭建1.6配置文件加载测试报告/1767691903080.png)

![1767691941590](image/AWRL6844雷达健康检测-04-第1章-环境搭建1.6配置文件加载测试报告/1767691941590.png)

对于你的解决方案实操下来结果如下:

### 方案1：使用InCabin Demo专用GUI（强烈推荐）

### RE: 无法在win10启动,启动时一闪而过

然后,使用选项A：安装MATLAB Runtime

RE: 正常启动,因为此应用是依赖matlab启动的,

1. 导入配置文件 `cpd.cfg`, 雷达可以启动.
2. 导入配置文件6844_profile_4T4R_tdm.cfg,错误如下:

GUI Path: C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src
系统找不到指定的路径。
Opening configuration file C:\ti\MMWAVE_L_SDK_06_01_00_01\mmwave_l_sdk_06_01_00_01\tools\visualizer\tmp\6844_profile_4T4R_tdm.cfg ...
无法识别的字段名称 "runningMode"。

出错 parseCfgL6xxx (第 167 行)

出错 occupancy_demo_gui (第 175 行)

MATLAB:nonExistentField

而使用选项B：烧录预编译固件验证硬件C:\ti\radar_toolbox_3_30_00_06\source\ti\examples\Automotive_InCabin_Security_and_Safety\AWRL6844_InCabin_Demos\prebuilt_binaries\demo_in_cabin_sensing_6844_system.release.appimage 固件,

启动occupancy_demo_gui.exe后导入配置文件 `cpd.cfg,反而无法启动雷达, 错误如下:`

GUI Path: C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src
系统找不到指定的路径。
Opening configuration file C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src\chirpConfigs6844\cpd.cfg ...
--------------------------Figures Are Being Prepared----------------------------
-------------------------------Figures Created----------------------------------
Sending configuration from C:\ti\radar_toolbox_3_30_00_06\tools\visualizers\AWRL6844_Incabin_GUI\src\chirpConfigs6844\cpd.cfg file to Target EVM 1 ...
sensorStop 0
警告: 在 'readline' 的超时期限内未返回指定的数据量。
'serialport' unable to read any data. For more information on possible reasons, see <a href="matlab: helpview('matlab', 'serialport_nodata')"'>serialport 读取警告 `</a>`.
错误使用 contains
第一个参数 必须为文本。

出错 sendCfgToDevice (第 39 行)

出错 occupancy_demo_gui (第 944 行)

MATLAB:string:MustBeCharCellArrayOrString

所以放弃选项B,

当前已确认:

C:\Users\Administrator\workspace_ccstheia\demo_in_cabin_sensing_6844_system\Release\demo_in_cabin_sensing_6844_system.release.appimage 使用此固件,

结论:

方案1选项A OK. 但此方案对我来说只能做验证使用.无法用到后续的项目中.

方案1选项B 失败, 放弃选项B

### 方案2：忽略SDK Visualizer的版本警告

RE:忽略没用. 忽略后雷达没有数据输出.

结论: 继续找出原因并修正, 后续也需要使用这个测试.

### 方案3：创建兼容的通用配置文件

**RE:**

**Error in Setting up device**

Please try again.

结论: 方案3没有意义,不要再提出此方案

通过更改mmw_cli.h ,修改：`5.6.0.0` → `6.1.0.1后重新编译并烧录后测试:`

1. 使用InCabin GUI + `cpd.cfg` 验证雷达依然正常工作, 但不是工作的主线. 使用SDK Visualizer才是通用的主线.
2. 使用SDK Visualizer

   2.1`cpd.cfg 依然出错,**Error in Setting up device**

Please try again.`

    2.2`6844_profile_4T4R_tdm.cfg 虽然可以导入配置文件,也没有版本报警,但雷达也未启动.`

你这个解决方案就是傻逼方案.

1. 自定义CLI命令（runningMode, sigProcChainCommonCfg, cuboidDef等 这些只是自定义的命令,还能跳出官方的SDK吗? 这些真的与启动雷达有关么?你他妈的就没有深入分析.
2. 你确认消除SDK Visualizer的版本警告就可以启动雷达了吗? 错,消除版本警告只是掩耳盗铃,没有找出真正的原因肯定还有深层次的原因.
3. 这个方案均采用基于InCabin Demo扩展雷达检测系统, 有问题解决当前的问题,而不是绕路走.
4. 我现在的要求是在SDK Visualizer上使用  在 `6844_profile_4T4R_tdm.cfg版本基础上修正后,找出无法启动雷达的原因.`
5. `cpd.cfg 能启动雷达, 但内部有自定义命令, 但也跳不出官方sdk 标准. 所以可以学习研究`cpd.cfg来修正 `6844_profile_4T4R_tdm.cfg`, 让其在官方SDK Visualizer上使用. 有必要时需要研究D:\7.project\TI_Radar_Project\project-code\AWRL6844_InCabin_Demos源代码, 找出原因.

所以,现在关键是找出原因, 未找出真正时不得修改源代码.
