"""
雷达配置参数测试GUI工具

功能：
1. 选择要测试的命令组合
2. 实时显示雷达数据输出
3. 保存测试日志到分类文件
4. 显示LED状态确认
5. 批量生成测试组合供选择
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox, filedialog
import threading
import time
import json
from datetime import datetime
from pathlib import Path
import queue
import sys
import subprocess
import os


def _set_windows_app_id(app_id: str) -> None:
    """Windows 任务栏/标题栏图标稳定显示所需（必须在 tk.Tk() 之前调用）。"""
    if sys.platform != 'win32':
        return
    try:
        import ctypes
        ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(app_id)
    except Exception:
        pass


def _pip_install(packages: list[str]) -> None:
    subprocess.check_call([sys.executable, '-m', 'pip', 'install', *packages])


try:
    import serial
    import serial.tools.list_ports
except ImportError:
    _pip_install(['pyserial'])
    import serial
    import serial.tools.list_ports


try:
    import psutil
except ImportError:
    _pip_install(['psutil'])
    import psutil


class IconManager:
    def __init__(self, tk_root: tk.Misc, icons_dir: Path | None = None):
        self._root = tk_root
        self._icons_dir = icons_dir or self._resolve_icons_dir()
        self._cache: dict[str, tk.PhotoImage] = {}
        self._spinner_frames: list[tk.PhotoImage] | None = None

    @staticmethod
    def _resolve_icons_dir() -> Path:
        # 脚本模式：相对当前 .py 文件
        if not getattr(sys, 'frozen', False):
            return Path(__file__).parent / 'image' / 'icons'

        # PyInstaller：优先 sys._MEIPASS，其次 exe 同级
        base = Path(getattr(sys, '_MEIPASS', '')) if getattr(sys, '_MEIPASS', '') else Path(sys.executable).parent
        candidates = [
            base / 'image' / 'icons',
            base / 'icons',
            base,
        ]
        for c in candidates:
            if c.exists():
                return c
        return candidates[0]

    def _png_path(self, key: str, size: int) -> Path:
        return self._icons_dir / f"{key}_{size}.png"

    def get_png(self, key: str, size: int) -> tk.PhotoImage | None:
        cache_key = f"{key}:{size}"
        if cache_key in self._cache:
            return self._cache[cache_key]

        path = self._png_path(key, size)
        if not path.exists():
            return None

        img = tk.PhotoImage(master=self._root, file=str(path))
        self._cache[cache_key] = img
        return img

    def get_spinner_frames(self) -> list[tk.PhotoImage]:
        if self._spinner_frames is not None:
            return self._spinner_frames

        gif_path = self._icons_dir / 'spinner_24.gif'
        frames: list[tk.PhotoImage] = []
        if gif_path.exists():
            idx = 0
            while True:
                try:
                    frames.append(tk.PhotoImage(master=self._root, file=str(gif_path), format=f"gif -index {idx}"))
                    idx += 1
                except tk.TclError:
                    break

        self._spinner_frames = frames
        return frames

    def apply_window_icons(self, window: tk.Tk | tk.Toplevel) -> None:
        ico_path = self._icons_dir / 'app_radar.ico'
        if ico_path.exists():
            try:
                window.iconbitmap(ico_path)
            except Exception:
                pass

        # Windows 更稳：iconphoto + 强引用（防 GC 回退到 python 默认图标），优先 256px
        png256 = self.get_png('app_radar', 256)
        png24 = self.get_png('app_radar', 24)
        photo = png256 or png24
        if photo is not None:
            try:
                window.iconphoto(True, photo)
                setattr(window, '_app_icon_256', photo)
            except Exception:
                pass

def check_existing_process():
    """检查是否有同名进程正在运行"""
    current_pid = os.getpid()
    try:
        parent_pid = psutil.Process(current_pid).ppid()
    except Exception:
        parent_pid = None
    current_name = os.path.basename(sys.argv[0])

    existing_processes = []
    for proc in psutil.process_iter(['pid', 'name', 'cmdline']):
        try:
            # 跳过当前进程和父进程（onefile 下父进程同名，不能当成“旧实例”）
            if proc.pid == current_pid or (parent_pid is not None and proc.pid == parent_pid):
                continue

            cmdline = proc.info.get('cmdline', [])
            if not cmdline:
                continue

            if any(current_name in cmd for cmd in cmdline):
                existing_processes.append(proc)
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            pass

    return existing_processes


def ensure_log_directory():
    """确保log目录存在"""
    script_dir = Path(__file__).parent
    log_dir = script_dir / 'log'
    if not log_dir.exists():
        log_dir.mkdir(parents=True, exist_ok=True)
        # Avoid emoji in print to prevent Windows encoding issues
        # print(f"Created log directory: {log_dir}")
    return log_dir

# ============================================================================
# 22命令完整数据结构（基于《雷达配置文件深度分析.md》）
# 包含20种不同命令，实际配置22行（cfarProcCfg×2, cfarFovCfg×2）
# ============================================================================

RADAR_COMMANDS = {
    # ========== 1. 传感器控制命令（2条）==========
    'sensorStop': {
        'cmd': 'sensorStop 0',
        'desc': '停止雷达传感器',
        'category': '传感器控制',
        'order': 1,
        'importance': 5,
        'required': True,
        'params': {
            'subFrameIdx': {'value': 0, 'desc': '子帧索引（0=所有）'}
        }
    },
    'sensorStart': {
        'cmd': 'sensorStart 0 0 0 0',
        'desc': '启动雷达传感器',
        'category': '传感器控制',
        'order': 22,
        'importance': 5,
        'required': True,
        'params': {
            'subFrameIdx': {'value': 0, 'desc': '子帧索引'},
            'reserved1': {'value': 0, 'desc': '预留参数1'},
            'reserved2': {'value': 0, 'desc': '预留参数2'},
            'reserved3': {'value': 0, 'desc': '预留参数3'}
        }
    },

    # ========== 2. 基础硬件配置（4条）==========
    'channelCfg': {
        'cmd': 'channelCfg 153 255 0',
        'desc': '天线通道配置 (4TX4RX MIMO)',
        'category': '基础硬件配置',
        'order': 2,
        'importance': 5,
        'required': True,
        'params': {
            'rxChannelEn': {'value': 153, 'desc': 'RX天线使能（153=0x99=RX0,3,4,7）'},
            'txChannelEn': {'value': 255, 'desc': 'TX天线使能（255=0xFF=全部）'},
            'cascading': {'value': 0, 'desc': '级联模式（0=单芯片）'}
        }
    },
    'chirpComnCfg': {
        'cmd': 'chirpComnCfg 8 0 0 256 1 13.1 3',
        'desc': 'Chirp公共参数配置',
        'category': '基础硬件配置',
        'order': 3,
        'importance': 5,
        'required': True,
        'params': {
            'startIdx': {'value': 8, 'desc': '起始Chirp索引'},
            'endIdx': {'value': 0, 'desc': '结束Chirp索引'},
            'profileId': {'value': 0, 'desc': '配置文件ID'},
            'startFreq': {'value': 256, 'desc': '起始频率（GHz）'},
            'freqSlope': {'value': 1, 'desc': '频率斜率（MHz/μs）'},
            'idleTime': {'value': 13.1, 'desc': '空闲时间（μs）'},
            'adcStartTime': {'value': 3, 'desc': 'ADC采样开始时间（μs）'}
        }
    },
    'chirpTimingCfg': {
        'cmd': 'chirpTimingCfg 6 63 0 160 58',
        'desc': 'Chirp时序配置',
        'category': '基础硬件配置',
        'order': 4,
        'importance': 4,
        'required': True,
        'params': {
            'chirpStartIdx': {'value': 6, 'desc': '起始Chirp索引'},
            'chirpEndIdx': {'value': 63, 'desc': '结束Chirp索引'},
            'loopStartIdx': {'value': 0, 'desc': '起始循环索引'},
            'loopEndIdx': {'value': 160, 'desc': '结束循环索引'},
            'numLoops': {'value': 58, 'desc': '循环次数'}
        }
    },
    'adcDataDitherCfg': {
        'cmd': 'adcDataDitherCfg 1',
        'desc': 'ADC数据抖动（提升信噪比）',
        'category': '基础硬件配置',
        'order': 5,
        'importance': 3,
        'required': False,
        'params': {
            'enable': {'value': 1, 'desc': '使能标志（0=禁用, 1=启用）'}
        }
    },

    # ========== 3. 帧配置（2条）==========
    'frameCfg': {
        'cmd': 'frameCfg 64 0 1358 1 100 0',
        'desc': '帧配置（10Hz帧率）',
        'category': '帧配置',
        'order': 6,
        'importance': 5,
        'required': True,
        'params': {
            'chirpStartIdx': {'value': 64, 'desc': '帧内起始Chirp索引'},
            'chirpEndIdx': {'value': 0, 'desc': '帧内结束Chirp索引'},
            'numLoops': {'value': 1358, 'desc': '每帧循环次数'},
            'numFrames': {'value': 1, 'desc': '连续帧数（1=持续运行）'},
            'framePeriodicity': {'value': 100, 'desc': '帧周期（ms）'},
            'triggerSelect': {'value': 0, 'desc': '触发模式（0=软件触发）'}
        }
    },
    'gpAdcMeasConfig': {
        'cmd': 'gpAdcMeasConfig 0 0',
        'desc': '通用ADC测量配置',
        'category': '帧配置',
        'order': 7,
        'importance': 2,
        'required': False,
        'params': {
            'enable': {'value': 0, 'desc': '使能标志'},
            'numSamples': {'value': 0, 'desc': '采样数量'}
        }
    },

    # ========== 4. 信号处理配置（5条，实际7行）==========
    'cfarProcCfg_Range': {
        'cmd': 'cfarProcCfg 0 2 8 4 3 0 9.0 0',
        'desc': 'CFAR检测器配置（距离维度）',
        'category': '信号处理配置',
        'order': 9,
        'importance': 5,
        'required': True,
        'params': {
            'subFrameIdx': {'value': 0, 'desc': '子帧索引（0=Range）'},
            'procDirection': {'value': 2, 'desc': '处理方向（2=两侧）'},
            'averageMode': {'value': 8, 'desc': 'CASO-CFAR'},
            'winLen': {'value': 4, 'desc': '窗口长度'},
            'guardLen': {'value': 3, 'desc': '保护长度'},
            'div': {'value': 0, 'desc': '除数因子'},
            'threshold': {'value': 9.0, 'desc': '检测阈值（dB）'},
            'cycleLength': {'value': 0, 'desc': '循环长度'}
        }
    },
    'cfarProcCfg_Doppler': {
        'cmd': 'cfarProcCfg 1 2 4 2 2 1 9.0 0',
        'desc': 'CFAR检测器配置（多普勒维度）',
        'category': '信号处理配置',
        'order': 10,
        'importance': 5,
        'required': True,
        'params': {
            'subFrameIdx': {'value': 1, 'desc': '子帧索引（1=Doppler）'},
            'procDirection': {'value': 2, 'desc': '处理方向'},
            'averageMode': {'value': 4, 'desc': 'CAGO-CFAR'},
            'winLen': {'value': 2, 'desc': '窗口长度'},
            'guardLen': {'value': 2, 'desc': '保护长度'},
            'div': {'value': 1, 'desc': '除数因子'},
            'threshold': {'value': 9.0, 'desc': '检测阈值（dB）'},
            'cycleLength': {'value': 0, 'desc': '循环长度'}
        }
    },
    'cfarFovCfg_Range': {
        'cmd': 'cfarFovCfg 0 0.25 9.0',
        'desc': 'CFAR视场配置（距离：0.25-9m）',
        'category': '信号处理配置',
        'order': 11,
        'importance': 4,
        'required': True,
        'params': {
            'subFrameIdx': {'value': 0, 'desc': '子帧索引（0=Range）'},
            'min': {'value': 0.25, 'desc': '最小距离（m）'},
            'max': {'value': 9.0, 'desc': '最大距离（m）'}
        }
    },
    'cfarFovCfg_Doppler': {
        'cmd': 'cfarFovCfg 1 -20.16 20.16',
        'desc': 'CFAR视场配置（速度：±20m/s）',
        'category': '信号处理配置',
        'order': 12,
        'importance': 4,
        'required': True,
        'params': {
            'subFrameIdx': {'value': 1, 'desc': '子帧索引（1=Doppler）'},
            'min': {'value': -20.16, 'desc': '最小速度（m/s）'},
            'max': {'value': 20.16, 'desc': '最大速度（m/s）'}
        }
    },
    'aoaProcCfg': {
        'cmd': 'aoaProcCfg 64 64',
        'desc': '角度处理配置（64点FFT）',
        'category': '信号处理配置',
        'order': 13,
        'importance': 4,
        'required': True,
        'params': {
            'fftSize1': {'value': 64, 'desc': '方位角FFT点数'},
            'fftSize2': {'value': 64, 'desc': '俯仰角FFT点数'}
        }
    },
    'aoaFovCfg': {
        'cmd': 'aoaFovCfg -60 60 -60 60',
        'desc': '角度视场配置（±60°）',
        'category': '信号处理配置',
        'order': 14,
        'importance': 4,
        'required': True,
        'params': {
            'minAzimuth': {'value': -60, 'desc': '最小方位角（度）'},
            'maxAzimuth': {'value': 60, 'desc': '最大方位角（度）'},
            'minElevation': {'value': -60, 'desc': '最小俯仰角（度）'},
            'maxElevation': {'value': 60, 'desc': '最大俯仰角（度）'}
        }
    },
    'clutterRemoval': {
        'cmd': 'clutterRemoval 0',
        'desc': '静态杂波去除',
        'category': '信号处理配置',
        'order': 15,
        'importance': 4,
        'required': False,
        'params': {
            'enable': {'value': 0, 'desc': '使能标志（0=禁用, 1=启用）'}
        }
    },

    # ========== 5. 校准配置（2条）==========
    'factoryCalibCfg': {
        'cmd': 'factoryCalibCfg 1 0 44 2 0x1ff000',
        'desc': '工厂校准配置',
        'category': '校准配置',
        'order': 16,
        'importance': 4,
        'required': True,
        'params': {
            'enable': {'value': 1, 'desc': '使能标志'},
            'reserved1': {'value': 0, 'desc': '预留参数1'},
            'reserved2': {'value': 44, 'desc': '预留参数2'},
            'reserved3': {'value': 2, 'desc': '预留参数3'},
            'flashOffset': {'value': '0x1ff000', 'desc': 'Flash偏移地址'}
        }
    },
    'runtimeCalibCfg': {
        'cmd': 'runtimeCalibCfg 1',
        'desc': '运行时校准配置',
        'category': '校准配置',
        'order': 17,
        'importance': 4,
        'required': True,
        'params': {
            'enable': {'value': 1, 'desc': '使能标志（建议启用）'}
        }
    },

    # ========== 6. 输出与监控（3条）==========
    'guiMonitor': {
        'cmd': 'guiMonitor 1 1 0 0 0 1',
        'desc': 'GUI监控输出配置',
        'category': '输出与监控',
        'order': 8,
        'importance': 4,
        'required': True,
        'params': {
            'subFrameIdx': {'value': 1, 'desc': '子帧索引（1=所有）'},
            'detectObj': {'value': 1, 'desc': '检测目标列表'},
            'logMagRange': {'value': 0, 'desc': '对数幅度-距离图'},
            'noiseProfile': {'value': 0, 'desc': '噪声轮廓'},
            'rangeAzimuthHeatMap': {'value': 0, 'desc': '距离-方位角热图'},
            'rangeDopplerHeatMap': {'value': 1, 'desc': '距离-多普勒热图'}
        }
    },
    'adcDataSource': {
        'cmd': 'adcDataSource 0 adc_test_data_0001.bin',
        'desc': 'ADC数据源配置',
        'category': '输出与监控',
        'order': 19,
        'importance': 2,
        'required': False,
        'params': {
            'mode': {'value': 0, 'desc': '数据源模式（0=实时）'},
            'filename': {'value': 'adc_test_data_0001.bin', 'desc': '文件名'}
        }
    },
    'adcLogging': {
        'cmd': 'adcLogging 0',
        'desc': 'ADC数据记录',
        'category': '输出与监控',
        'order': 20,
        'importance': 2,
        'required': False,
        'params': {
            'enable': {'value': 0, 'desc': '使能标志（0=禁用）'}
        }
    },

    # ========== 7. 系统配置（2条）==========
    'antGeometryBoard': {
        'cmd': 'antGeometryBoard xWRL6844EVM',
        'desc': '板级天线几何配置',
        'category': '系统配置',
        'order': 18,
        'importance': 5,
        'required': True,
        'params': {
            'boardType': {'value': 'xWRL6844EVM', 'desc': '板型号'}
        }
    },
    'lowPowerCfg': {
        'cmd': 'lowPowerCfg 1',
        'desc': '低功耗模式配置',
        'category': '系统配置',
        'order': 21,
        'importance': 3,
        'required': False,
        'params': {
            'enable': {'value': 1, 'desc': '使能标志（1=启用）'}
        }
    }
}

# 命令分类映射
COMMAND_CATEGORIES = {
    '传感器控制': ['sensorStop', 'sensorStart'],
    '基础硬件配置': ['channelCfg', 'chirpComnCfg', 'chirpTimingCfg', 'adcDataDitherCfg'],
    '帧配置': ['frameCfg', 'gpAdcMeasConfig'],
    '信号处理配置': ['cfarProcCfg_Range', 'cfarProcCfg_Doppler', 'cfarFovCfg_Range', 'cfarFovCfg_Doppler',
                      'aoaProcCfg', 'aoaFovCfg', 'clutterRemoval'],
    '校准配置': ['factoryCalibCfg', 'runtimeCalibCfg'],
    '输出与监控': ['guiMonitor', 'adcDataSource', 'adcLogging'],
    '系统配置': ['antGeometryBoard', 'lowPowerCfg']
}

# ============================================================================
# 任务5：预设模板定义（基础配置）
# ============================================================================

# 模板1：TI标准配置（22命令）
TEMPLATE_TI_STANDARD = [
    'sensorStop', 'channelCfg', 'chirpComnCfg', 'chirpTimingCfg',
    'adcDataDitherCfg', 'frameCfg', 'gpAdcMeasConfig', 'guiMonitor',
    'cfarProcCfg_Range', 'cfarProcCfg_Doppler',
    'cfarFovCfg_Range', 'cfarFovCfg_Doppler',
    'aoaProcCfg', 'aoaFovCfg', 'clutterRemoval',
    'factoryCalibCfg', 'runtimeCalibCfg', 'antGeometryBoard',
    'adcDataSource', 'adcLogging', 'lowPowerCfg', 'sensorStart'
]

# 模板2：最小配置（10命令）- 仅包含核心必需命令
TEMPLATE_MINIMAL = [
    'sensorStop',           # 停止传感器
    'channelCfg',          # 天线配置（必需）
    'chirpComnCfg',        # Chirp配置（必需）
    'frameCfg',            # 帧配置（必需）
    'guiMonitor',          # 输出配置（必需）
    'aoaProcCfg',          # 角度处理（必需）
    'factoryCalibCfg',     # 工厂校准（必需）
    'runtimeCalibCfg',     # 运行时校准（必需）
    'antGeometryBoard',    # 天线几何（必需）
    'sensorStart'          # 启动传感器
]

# ============================================================================
# 任务6：预设模板定义（场景配置）
# ============================================================================

# 模板3：人员跌倒检测
TEMPLATE_FALL_DETECTION = [
    'sensorStop',              # 停止传感器
    'channelCfg',             # 天线配置
    'chirpComnCfg',           # Chirp配置
    'chirpTimingCfg',         # Chirp时序（需要更快帧率）
    'frameCfg',               # 帧配置（高帧率）
    'guiMonitor',             # 输出配置
    'cfarProcCfg_Range',      # 距离CFAR（检测人体）
    'cfarProcCfg_Doppler',    # 多普勒CFAR（检测运动）
    'cfarFovCfg_Range',       # 距离FOV（中短距）
    'cfarFovCfg_Doppler',     # 多普勒FOV（捕捉跌倒速度）
    'aoaProcCfg',             # 角度处理（定位人员）
    'aoaFovCfg',              # 角度FOV（室内范围）
    'clutterRemoval',         # 杂波抑制（减少误报）
    'factoryCalibCfg',        # 工厂校准
    'runtimeCalibCfg',        # 运行时校准
    'antGeometryBoard',       # 天线几何
    'sensorStart'             # 启动传感器
]

# 模板4：房间占用检测
TEMPLATE_OCCUPANCY_DETECTION = [
    'sensorStop',              # 停止传感器
    'channelCfg',             # 天线配置
    'chirpComnCfg',           # Chirp配置
    'frameCfg',               # 帧配置（低功耗模式）
    'guiMonitor',             # 输出配置
    'cfarProcCfg_Range',      # 距离CFAR（检测人体）
    'cfarFovCfg_Range',       # 距离FOV（覆盖整个房间）
    'aoaProcCfg',             # 角度处理（人员位置）
    'aoaFovCfg',              # 角度FOV（广角覆盖）
    'clutterRemoval',         # 杂波抑制（忽略静态物体）
    'factoryCalibCfg',        # 工厂校准
    'runtimeCalibCfg',        # 运行时校准
    'antGeometryBoard',       # 天线几何
    'lowPowerCfg',            # 低功耗配置（长期运行）
    'sensorStart'             # 启动传感器
]

# 模板5：手势识别
TEMPLATE_GESTURE_RECOGNITION = [
    'sensorStop',              # 停止传感器
    'channelCfg',             # 天线配置
    'chirpComnCfg',           # Chirp配置
    'chirpTimingCfg',         # Chirp时序（高时间分辨率）
    'frameCfg',               # 帧配置（高帧率）
    'guiMonitor',             # 输出配置
    'cfarProcCfg_Range',      # 距离CFAR（近距检测）
    'cfarProcCfg_Doppler',    # 多普勒CFAR（捕捉手部运动）
    'cfarFovCfg_Range',       # 距离FOV（近距离）
    'cfarFovCfg_Doppler',     # 多普勒FOV（高速运动）
    'aoaProcCfg',             # 角度处理（手势方向）
    'aoaFovCfg',              # 角度FOV（精确角度）
    'clutterRemoval',         # 杂波抑制
    'factoryCalibCfg',        # 工厂校准
    'runtimeCalibCfg',        # 运行时校准
    'antGeometryBoard',       # 天线几何
    'adcDataSource',          # ADC数据源（可能需要原始数据）
    'sensorStart'             # 启动传感器
]

# 模板6：车辆检测
TEMPLATE_VEHICLE_DETECTION = [
    'sensorStop',              # 停止传感器
    'channelCfg',             # 天线配置（4T4R全阵列）
    'chirpComnCfg',           # Chirp配置
    'chirpTimingCfg',         # Chirp时序
    'frameCfg',               # 帧配置（中等帧率）
    'guiMonitor',             # 输出配置
    'cfarProcCfg_Range',      # 距离CFAR（远距检测）
    'cfarProcCfg_Doppler',    # 多普勒CFAR（车辆速度）
    'cfarFovCfg_Range',       # 距离FOV（长距离）
    'cfarFovCfg_Doppler',     # 多普勒FOV（宽速度范围）
    'aoaProcCfg',             # 角度处理（车辆方位）
    'aoaFovCfg',              # 角度FOV（道路覆盖）
    'clutterRemoval',         # 杂波抑制（忽略地面反射）
    'factoryCalibCfg',        # 工厂校准
    'runtimeCalibCfg',        # 运行时校准
    'antGeometryBoard',       # 天线几何
    'sensorStart'             # 启动传感器
]

# 模板字典
CONFIG_TEMPLATES = {
    # 基础配置（任务5）
    'TI标准配置（22命令）': {
        'commands': TEMPLATE_TI_STANDARD,
        'desc': 'TI官方完整标准配置，包含所有22个命令',
        'count': 22,
        'category': '基础配置'
    },
    '最小配置（10命令）': {
        'commands': TEMPLATE_MINIMAL,
        'desc': '最简化配置，仅包含10个核心必需命令',
        'count': 10,
        'category': '基础配置'
    },
    # 场景配置（任务6）
    '人员跌倒检测': {
        'commands': TEMPLATE_FALL_DETECTION,
        'desc': '检测室内人员跌倒事件，高帧率+运动检测',
        'count': 17,
        'category': '场景配置'
    },
    '房间占用检测': {
        'commands': TEMPLATE_OCCUPANCY_DETECTION,
        'desc': '检测房间内是否有人，低功耗长期运行',
        'count': 15,
        'category': '场景配置'
    },
    '手势识别': {
        'commands': TEMPLATE_GESTURE_RECOGNITION,
        'desc': '识别近距离手势动作，高时间分辨率',
        'count': 18,
        'category': '场景配置'
    },
    '车辆检测': {
        'commands': TEMPLATE_VEHICLE_DETECTION,
        'desc': '检测道路车辆及速度，远距离+宽FOV',
        'count': 17,
        'category': '场景配置'
    }
}

# ============================================================================
# 注意：COMMAND_DEFINITIONS已被RADAR_COMMANDS替代
# 后续任务将逐步迁移所有引用到新的RADAR_COMMANDS数据结构
# ============================================================================

def get_commands_by_order():
    """按order字段排序获取命令列表"""
    return sorted(RADAR_COMMANDS.items(), key=lambda x: x[1]['order'])

def get_commands_by_category():
    """按分类组织命令"""
    result = {}
    for category, cmd_names in COMMAND_CATEGORIES.items():
        result[category] = [(name, RADAR_COMMANDS[name]) for name in cmd_names if name in RADAR_COMMANDS]
    return result

def get_required_commands():
    """获取所有必需命令"""
    return {name: info for name, info in RADAR_COMMANDS.items() if info.get('required', False)}

def get_optional_commands():
    """获取所有可选命令"""
    return {name: info for name, info in RADAR_COMMANDS.items() if not info.get('required', False)}

def build_command_string(cmd_name, cmd_info):
    """
    根据命令名称和参数构建完整的命令字符串

    Args:
        cmd_name: 命令名称（如'channelCfg'）
        cmd_info: 命令信息字典（来自RADAR_COMMANDS）

    Returns:
        str: 完整的命令字符串（如'channelCfg 153 255 0'）
    """
    if 'params' in cmd_info and cmd_info['params']:
        # 处理特殊命名（cfarProcCfg_Range -> cfarProcCfg）
        base_cmd_name = cmd_name.replace('_Range', '').replace('_Doppler', '')

        # 构建命令字符串
        cmd_parts = [base_cmd_name]
        for param_name, param_data in cmd_info['params'].items():
            cmd_parts.append(str(param_data['value']))
        return ' '.join(cmd_parts)
    else:
        # 无参数命令，直接返回命令名
        return cmd_name

class RadarTestGUI:
    """
    雷达配置参数测试GUI主类

    功能：
    - 双串口通信管理（CLI端口 + 数据端口）
    - 22个雷达命令的可视化配置
    - 实时数据接收和显示
    - 测试日志记录和保存
    - 预设模板快速加载

    属性：
        root: Tkinter主窗口
        cli_conn: CLI串口连接（发送命令）
        data_conn: 数据串口连接（接收数据）
        command_checkboxes: 命令勾选框字典
        test_results: 测试结果列表
        packet_count: 数据包计数
        total_bytes: 接收字节总数
    """

    def __init__(self, root):
        """
        初始化GUI应用

        Args:
            root: Tkinter根窗口对象

        初始化流程：
            1. 设置窗口标题和大小
            2. 确保log目录存在
            3. 初始化串口连接变量
            4. 初始化数据统计变量
            5. 创建界面组件
            6. 自动扫描可用串口
        """
        self.root = root
        self.root.title("雷达配置参数测试工具 v1.1.5 - 双端口模式")
        self.root.geometry("1200x800")

        self.icons = IconManager(self.root)
        self.icons.apply_window_icons(self.root)

        # 确保log目录存在
        self.log_dir = ensure_log_directory()

        # 串口连接 - 双端口配置
        self.cli_conn = None        # CLI端口(COM3@115200) - 发送命令
        self.data_conn = None       # 数据端口(COM4@1250000) - 接收数据
        self.data_queue = queue.Queue()
        self.cli_thread = None
        self.data_thread = None
        self.is_reading = False

        # 数据统计
        self.packet_count = 0       # 数据包计数
        self.total_bytes = 0        # 总字节数

        # 测试结果
        self.test_results = []
        self.current_test = None
        self.is_testing = False     # 测试运行状态标志

        # 命令勾选状态（任务3新增）
        self.command_checkboxes = {}  # {cmd_name: (BooleanVar, Checkbutton)}
        self.category_collapsed = {}  # {category: BooleanVar} 分类折叠状态

        # 创建界面
        self.create_widgets()

        # 自动扫描串口
        self.scan_ports()

    def create_widgets(self):
        """
        创建所有GUI组件

        界面布局（从上到下）：
        1. 顶部：双串口连接控制（CLI + 数据端口）
        2. 主区域（三栏）：
           - 左侧：命令勾选列表（7个分类，22个命令）
           - 中间：配置显示、测试控制、测试信息
           - 右侧：实时数据输出

        组件关系：
        - 命令勾选触发 on_command_check_changed()
        - 勾选变化自动调用 apply_selected_commands()
        - 配置实时同步到中间配置区域
        """

        # ===== 顶部：连接控制 =====
        conn_frame = self._make_labelframe(self.root, 'connect', '双串口连接配置（已测试验证）', padding=10)
        conn_frame.pack(fill=tk.X, padx=10, pady=5)

        # CLI端口配置
        ttk.Label(conn_frame, text="CLI端口:", font=('Arial', 9, 'bold')).grid(row=0, column=0, padx=5, sticky=tk.W)
        self.cli_port_combo = ttk.Combobox(conn_frame, width=12, state='readonly')
        self.cli_port_combo.grid(row=0, column=1, padx=5)

        ttk.Label(conn_frame, text="@").grid(row=0, column=2)
        self.cli_baudrate_combo = ttk.Combobox(conn_frame, width=10,
                                               values=['115200'],
                                               state='readonly')
        self.cli_baudrate_combo.set('115200')
        self.cli_baudrate_combo.grid(row=0, column=3, padx=5)

        ttk.Label(conn_frame, text="(发送命令)", foreground="gray").grid(row=0, column=4, padx=5)

        # 数据端口配置
        ttk.Label(conn_frame, text="数据端口:", font=('Arial', 9, 'bold')).grid(row=1, column=0, padx=5, sticky=tk.W, pady=5)
        self.data_port_combo = ttk.Combobox(conn_frame, width=12, state='readonly')
        self.data_port_combo.grid(row=1, column=1, padx=5)

        ttk.Label(conn_frame, text="@").grid(row=1, column=2)
        self.data_baudrate_combo = ttk.Combobox(conn_frame, width=10,
                                                values=['1250000'],
                                                state='readonly')
        self.data_baudrate_combo.set('1250000')
        self.data_baudrate_combo.grid(row=1, column=3, padx=5)

        ttk.Label(conn_frame, text="(接收数据)", foreground="gray").grid(row=1, column=4, padx=5)

        # 连接按钮
        self.connect_btn = ttk.Button(
            conn_frame,
            text="连接双端口",
            image=self.icons.get_png('connect', 24),
            compound=tk.LEFT,
            command=self.toggle_connection,
        )
        self.connect_btn.grid(row=0, column=5, rowspan=2, padx=10)

        self.status_label = ttk.Label(conn_frame, text="● 未连接", foreground="red")
        self.status_label.grid(row=0, column=6, rowspan=2, padx=10)

        self.refresh_btn = ttk.Button(
            conn_frame,
            text="刷新端口",
            image=self.icons.get_png('refresh', 24),
            compound=tk.LEFT,
            command=self.scan_ports,
        )
        self.refresh_btn.grid(row=0, column=7, rowspan=2, padx=5)

        # ===== 主面板（使用PanedWindow实现可拖动分割） =====
        main_paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        # ===== 左侧：自定义配置（22命令勾选列表） =====
        left_frame = ttk.Frame(main_paned, width=350)
        main_paned.add(left_frame, weight=1)

        config_frame = self._make_labelframe(left_frame, 'template', '自定义配置（自定义参数）', padding=10)
        config_frame.pack(fill=tk.BOTH, expand=True)

        # 快速操作按钮（第一行）
        quick_btn_frame = ttk.Frame(config_frame)
        quick_btn_frame.pack(fill=tk.X, pady=(0,5))

        ttk.Button(
            quick_btn_frame,
            text="全选",
            width=8,
            image=self.icons.get_png('select_all', 24),
            compound=tk.LEFT,
            command=self.select_all_commands,
        ).pack(side=tk.LEFT, padx=2)
        ttk.Button(
            quick_btn_frame,
            text="全不选",
            width=8,
            image=self.icons.get_png('select_none', 24),
            compound=tk.LEFT,
            command=self.deselect_all_commands,
        ).pack(side=tk.LEFT, padx=2)
        ttk.Button(
            quick_btn_frame,
            text="仅必需",
            width=8,
            image=self.icons.get_png('select_required', 24),
            compound=tk.LEFT,
            command=self.select_required_only,
        ).pack(side=tk.LEFT, padx=2)

        # 预设模板选择（第二行）- 任务5新增
        template_frame = ttk.Frame(config_frame)
        template_frame.pack(fill=tk.X, pady=(0,5))

        self._make_icon_label(template_frame, 'template', "预设模板:").pack(side=tk.LEFT, padx=2)

        # 任务6：扩展模板选项，添加场景配置
        self.template_combo = ttk.Combobox(template_frame, width=25, state='readonly',
                                          values=[
                                              "TI标准配置（22命令）",
                                              "最小配置（10命令）",
                                              "人员跌倒检测",
                                              "房间占用检测",
                                              "手势识别",
                                              "车辆检测"
                                          ])
        self.template_combo.pack(side=tk.LEFT, padx=2)
        self.template_combo.set("TI标准配置（22命令）")

        ttk.Button(
            template_frame,
            text="加载",
            width=8,
            image=self.icons.get_png('load', 24),
            compound=tk.LEFT,
            command=self.load_selected_template,
        ).pack(side=tk.LEFT, padx=2)

        # 可滚动的命令列表区域
        canvas_frame = ttk.Frame(config_frame)
        canvas_frame.pack(fill=tk.BOTH, expand=True)

        # 创建Canvas和Scrollbar
        self.cmd_canvas = tk.Canvas(canvas_frame, bg='white', highlightthickness=0)
        cmd_scrollbar = ttk.Scrollbar(canvas_frame, orient='vertical', command=self.cmd_canvas.yview)

        self.cmd_canvas.configure(yscrollcommand=cmd_scrollbar.set)
        cmd_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.cmd_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        # 在Canvas中创建Frame容纳所有命令勾选框
        self.cmd_container = ttk.Frame(self.cmd_canvas)
        self.cmd_canvas_window = self.cmd_canvas.create_window((0, 0), window=self.cmd_container, anchor='nw')

        # 绑定Canvas大小调整
        self.cmd_container.bind('<Configure>', lambda e: self.cmd_canvas.configure(scrollregion=self.cmd_canvas.bbox('all')))
        self.cmd_canvas.bind('<Configure>', self._on_canvas_configure)

        # 绑定鼠标滚轮
        self.cmd_canvas.bind_all('<MouseWheel>', self._on_mousewheel)

        # 创建22命令的分类勾选框
        self.create_command_checkboxes()

        # 统计信息
        self.cmd_info_label = ttk.Label(config_frame, text="已选: 0/22", foreground='blue')
        self.cmd_info_label.pack(pady=5)

        # ===== 中间：命令编辑 =====
        middle_frame = ttk.Frame(main_paned, width=400)
        main_paned.add(middle_frame, weight=2)

        # 当前测试配置
        config_frame = ttk.LabelFrame(middle_frame, text="当前测试配置", padding=10)
        config_frame.pack(fill=tk.BOTH, expand=True)

        ttk.Label(config_frame, text="测试名称:").pack(anchor=tk.W)
        self.test_name_entry = ttk.Entry(config_frame, width=40)
        self.test_name_entry.pack(fill=tk.X, pady=2)

        ttk.Label(config_frame, text="配置命令:").pack(anchor=tk.W, pady=(10,0))

        # v1.1.4: 创建带行号的文本框
        text_frame = ttk.Frame(config_frame)
        text_frame.pack(fill=tk.BOTH, expand=True)

        # 行号显示区
        self.line_numbers = tk.Text(text_frame, width=4, padx=3, takefocus=0,
                                   border=0, background='lightgray', state='disabled',
                                   font=('Consolas', 9))
        self.line_numbers.pack(side=tk.LEFT, fill=tk.Y)

        # 配置命令文本区（带滚动条）
        self.commands_text = scrolledtext.ScrolledText(text_frame, height=12,
                                                       font=('Consolas', 9),
                                                       wrap=tk.NONE)
        self.commands_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        # 绑定文本变化事件以更新行号
        self.commands_text.bind('<KeyRelease>', self.update_line_numbers)
        self.commands_text.bind('<<Modified>>', self.update_line_numbers)

        # 同步滚动
        self.commands_text.config(yscrollcommand=self._sync_scroll)

        # 初始化行号
        self.update_line_numbers()

        # 控制按钮
        ctrl_frame = ttk.Frame(config_frame)
        ctrl_frame.pack(fill=tk.X, pady=5)

        ttk.Button(
            ctrl_frame,
            text="清空",
            image=self.icons.get_png('clear', 16),
            compound=tk.LEFT,
            command=self.clear_commands,
        ).pack(side=tk.LEFT, padx=2)
        ttk.Button(
            ctrl_frame,
            text="加载配置文件",
            image=self.icons.get_png('open_file', 16),
            compound=tk.LEFT,
            command=self.load_from_file,
        ).pack(side=tk.LEFT, padx=2)
        ttk.Button(
            ctrl_frame,
            text="导出配置文件",
            image=self.icons.get_png('export', 16),
            compound=tk.LEFT,
            command=self.export_config_file,
        ).pack(side=tk.LEFT, padx=2)
        # 注意：应用勾选按钮已删除，现在自动同步配置命令

        # 配置注释框（任务6新增）
        comment_frame = self._make_labelframe(middle_frame, 'edit', '配置注释（可选）', padding=10)
        comment_frame.pack(fill=tk.X, pady=5)

        # v1.1.4: 添加编码提示
        warning_label = ttk.Label(
            comment_frame,
            text="注意：CLI仅支持ASCII编码，中文注释将被跳过，建议使用英文",
            image=self.icons.get_png('warn', 16),
            compound=tk.LEFT,
            foreground='orange',
            font=('Arial', 8),
        )
        warning_label.pack(anchor=tk.W, pady=(0, 2))

        ttk.Label(comment_frame, text="添加注释说明（将作为注释行插入配置文件开头）:").pack(anchor=tk.W)
        self.config_comment_text = scrolledtext.ScrolledText(comment_frame, height=3,
                                                             font=('Consolas', 9),
                                                             wrap=tk.WORD)
        self.config_comment_text.pack(fill=tk.BOTH, expand=True, pady=2)
        self.config_comment_text.insert("1.0", "% Configuration: \n% Created: \n% Scene: ")

        # 测试控制
        test_frame = self._make_labelframe(middle_frame, 'send', '测试控制', padding=10)
        test_frame.pack(fill=tk.X, pady=5)

        # 创建按钮样式
        style = ttk.Style()
        style.configure('Start.TButton', font=('Arial', 11, 'bold'), foreground='green')
        style.configure('Stop.TButton', font=('Arial', 11, 'bold'), foreground='red')
        style.configure('Save.TButton', font=('Arial', 10, 'bold'))

        self.test_btn = ttk.Button(
            test_frame,
            text="发送配置执行",
            image=self.icons.get_png('send', 24),
            compound=tk.LEFT,
            style='Start.TButton',
            command=self.start_test,
        )
        self.test_btn.pack(side=tk.LEFT, padx=5)

        self.stop_btn = ttk.Button(
            test_frame,
            text="停止雷达",
            image=self.icons.get_png('stop', 24),
            compound=tk.LEFT,
            style='Stop.TButton',
            command=self.stop_radar,
        )
        self.stop_btn.pack(side=tk.LEFT, padx=5)

        self.save_btn = ttk.Button(
            test_frame,
            text="保存日志",
            image=self.icons.get_png('save', 24),
            compound=tk.LEFT,
            style='Save.TButton',
            command=self.save_test_log,
        )
        self.save_btn.pack(side=tk.LEFT, padx=5)

        # LED确认（不是控制开关，是确认记录）
        ttk.Label(test_frame, text="│", foreground="gray").pack(side=tk.LEFT, padx=5)
        ttk.Label(test_frame, text="确认:", foreground="gray", font=('Arial', 9)).pack(side=tk.LEFT)
        self.led_var = tk.BooleanVar()
        self.led_check = ttk.Checkbutton(test_frame, text="LED是否闪烁",
                                         variable=self.led_var)
        self.led_check.pack(side=tk.LEFT, padx=5)
        ttk.Label(test_frame, text="(勾选表示你看到LED在闪烁)",
                 foreground="gray", font=('Arial', 8)).pack(side=tk.LEFT, padx=5)

        # 说明标签
        ttk.Label(test_frame, text="│", foreground="gray").pack(side=tk.LEFT, padx=5)
        ttk.Label(
            test_frame,
            text="勾选命令后自动同步，点击'发送配置执行'启动",
            image=self.icons.get_png('info', 16),
            compound=tk.LEFT,
            foreground="blue",
            font=('Arial', 9, 'bold'),
        ).pack(side=tk.LEFT, padx=5)

        # 性能指标显示（任务9新增）
        metrics_frame = self._make_labelframe(middle_frame, 'info', '性能指标', padding=10)
        metrics_frame.pack(fill=tk.X, pady=5)

        self.metrics_text = scrolledtext.ScrolledText(metrics_frame, height=4,
                                                      font=('Consolas', 8),
                                                      bg='#f0f0f0', fg='#333333',
                                                      wrap=tk.WORD)
        self.metrics_text.pack(fill=tk.BOTH, expand=True)
        self.metrics_text.insert("1.0", "配置命令后自动显示性能指标...")
        self.metrics_text.config(state='disabled')

        # 参数调整建议（任务9新增）
        suggestions_frame = self._make_labelframe(middle_frame, 'info', '参数调整建议', padding=10)
        suggestions_frame.pack(fill=tk.X, pady=5)

        self.suggestions_text = scrolledtext.ScrolledText(suggestions_frame, height=4,
                                                          font=('Consolas', 8),
                                                          bg='#fffef0', fg='#555555',
                                                          wrap=tk.WORD)
        self.suggestions_text.pack(fill=tk.BOTH, expand=True)
        self.suggestions_text.insert("1.0", "配置命令后自动显示调整建议...")
        self.suggestions_text.config(state='disabled')

        # ===== 右侧：输出显示 =====
        right_frame = ttk.Frame(main_paned, width=400)
        main_paned.add(right_frame, weight=2)

        # CLI输出
        cli_frame = ttk.LabelFrame(right_frame, text="CLI响应", padding=5)
        cli_frame.pack(fill=tk.BOTH, expand=True)

        self.cli_output = scrolledtext.ScrolledText(cli_frame, height=15,
                                                     font=('Consolas', 9),
                                                     bg='#1e1e1e', fg='#d4d4d4')
        self.cli_output.pack(fill=tk.BOTH, expand=True)

        cli_btn_frame = ttk.Frame(cli_frame)
        cli_btn_frame.pack(fill=tk.X)
        ttk.Button(
            cli_btn_frame,
            text="清空",
            image=self.icons.get_png('clear', 16),
            compound=tk.LEFT,
            command=lambda: self.cli_output.delete(1.0, tk.END),
        ).pack(side=tk.LEFT, padx=2)

        # 雷达数据输出
        data_frame = ttk.LabelFrame(right_frame, text="雷达数据输出", padding=5)
        data_frame.pack(fill=tk.BOTH, expand=True, pady=5)

        # 数据统计
        stats_frame = ttk.Frame(data_frame)
        stats_frame.pack(fill=tk.X)

        ttk.Label(stats_frame, text="数据包:").pack(side=tk.LEFT, padx=5)
        self.packet_count_label = ttk.Label(stats_frame, text="0", foreground="purple")
        self.packet_count_label.pack(side=tk.LEFT, padx=5)

        ttk.Label(stats_frame, text="总量:").pack(side=tk.LEFT, padx=5)
        self.data_count_label = ttk.Label(stats_frame, text="0 bytes", foreground="blue")
        self.data_count_label.pack(side=tk.LEFT, padx=5)

        ttk.Label(stats_frame, text="速率:").pack(side=tk.LEFT, padx=5)
        self.data_rate_label = ttk.Label(stats_frame, text="0 B/s", foreground="green")
        self.data_rate_label.pack(side=tk.LEFT, padx=5)

        # 数据显示（十六进制）
        self.data_output = scrolledtext.ScrolledText(data_frame, height=10,
                                                      font=('Consolas', 8),
                                                      bg='#1e1e1e', fg='#00ff00')
        self.data_output.pack(fill=tk.BOTH, expand=True, pady=5)

        data_btn_frame = ttk.Frame(data_frame)
        data_btn_frame.pack(fill=tk.X)
        ttk.Button(
            data_btn_frame,
            text="清空",
            image=self.icons.get_png('clear', 16),
            compound=tk.LEFT,
            command=lambda: self.data_output.delete(1.0, tk.END),
        ).pack(side=tk.LEFT, padx=2)

        # 状态栏
        status_frame = ttk.Frame(self.root)
        status_frame.pack(fill=tk.X, padx=10, pady=5)

        self.info_label = ttk.Label(status_frame, text="就绪", relief=tk.SUNKEN)
        self.info_label.pack(fill=tk.X)

    def _make_icon_label(self, parent, icon_key: str, text: str) -> ttk.Label:
        return ttk.Label(parent, text=text, image=self.icons.get_png(icon_key, 16), compound=tk.LEFT)

    def _make_labelframe(self, parent, icon_key: str, text: str, **kwargs) -> ttk.LabelFrame:
        icon = self.icons.get_png(icon_key, 16)
        if icon is None:
            return ttk.LabelFrame(parent, text=text, **kwargs)
        label = ttk.Label(parent, text=text, image=icon, compound=tk.LEFT)
        return ttk.LabelFrame(parent, labelwidget=label, **kwargs)

    def _start_refresh_spinner(self) -> None:
        if not hasattr(self, 'refresh_btn'):
            return
        frames = self.icons.get_spinner_frames()
        if not frames:
            return

        self._spinner_frames = frames
        self._spinner_index = 0

        def step():
            if not hasattr(self, '_spinner_frames'):
                return
            self.refresh_btn.config(image=self._spinner_frames[self._spinner_index])
            self._spinner_index = (self._spinner_index + 1) % len(self._spinner_frames)
            self._spinner_job = self.root.after(60, step)

        self._spinner_job = self.root.after(0, step)

    def _stop_refresh_spinner(self) -> None:
        job = getattr(self, '_spinner_job', None)
        if job:
            try:
                self.root.after_cancel(job)
            except Exception:
                pass
        self._spinner_job = None
        if hasattr(self, 'refresh_btn'):
            self.refresh_btn.config(image=self.icons.get_png('refresh', 24))

    def create_tooltip(self, widget, text):
        """为组件创建工具提示"""
        def on_enter(event):
            tooltip = tk.Toplevel()
            tooltip.wm_overrideredirect(True)
            tooltip.wm_geometry(f"+{event.x_root+10}+{event.y_root+10}")
            label = ttk.Label(tooltip, text=text, background="lightyellow",
                            relief=tk.SOLID, borderwidth=1, font=('Arial', 9))
            label.pack()
            widget.tooltip = tooltip

        def on_leave(event):
            if hasattr(widget, 'tooltip'):
                widget.tooltip.destroy()
                del widget.tooltip

        widget.bind('<Enter>', on_enter)
        widget.bind('<Leave>', on_leave)

    def send_single_command(self, command):
        """发送单条命令（通过CLI端口）"""
        if not self.cli_conn or not self.cli_conn.is_open:
            messagebox.showwarning("警告", "请先连接串口")
            return

        try:
            self.cli_conn.write(f"{command}\n".encode())
            time.sleep(0.05)
            self.update_cli(f"> {command}\n")
            self.update_info(f"已发送: {command}")
        except Exception as e:
            messagebox.showerror("错误", f"发送命令失败: {e}")

    # def load_required_commands(self):
    #     """加载必需启动命令 - 已被任务3替代"""
    #     pass

    def scan_ports(self):
        """
        扫描系统可用的串口

        功能：
        - 使用serial.tools.list_ports扫描所有串口
        - 更新CLI端口和数据端口的下拉框
        - 自动选择第一个可用端口

        串口信息包括：
        - device: COM端口号
        - description: 设备描述
        - hwid: 硬件ID
        """
        self._start_refresh_spinner()
        try:
            ports = serial.tools.list_ports.comports()
            port_list = [port.device for port in ports]
        finally:
            # 给 UI 一点时间展示动画（避免扫描过快看不到）
            self.root.after(400, self._stop_refresh_spinner)

        # 更新CLI端口列表
        self.cli_port_combo['values'] = port_list
        if 'COM3' in port_list:
            self.cli_port_combo.set('COM3')
        elif port_list:
            self.cli_port_combo.set(port_list[0])

        # 更新数据端口列表
        self.data_port_combo['values'] = port_list
        if 'COM4' in port_list:
            self.data_port_combo.set('COM4')
        elif len(port_list) > 1:
            self.data_port_combo.set(port_list[1])
        elif port_list:
            self.data_port_combo.set(port_list[0])

    def toggle_connection(self):
        """切换连接状态"""
        if (self.cli_conn and self.cli_conn.is_open) or (self.data_conn and self.data_conn.is_open):
            self.disconnect()
        else:
            self.connect()

    def connect(self):
        """连接双串口"""
        cli_port = self.cli_port_combo.get()
        cli_baudrate = int(self.cli_baudrate_combo.get())
        data_port = self.data_port_combo.get()
        data_baudrate = int(self.data_baudrate_combo.get())

        if not cli_port or not data_port:
            messagebox.showerror("错误", "请选择CLI端口和数据端口")
            return

        if cli_port == data_port:
            messagebox.showwarning("警告", "CLI端口和数据端口相同\n建议使用不同端口以获得最佳性能")

        try:
            # 连接CLI端口
            self.cli_conn = serial.Serial(
                port=cli_port,
                baudrate=cli_baudrate,
                timeout=0.1
            )
            time.sleep(0.3)

            # 连接数据端口
            self.data_conn = serial.Serial(
                port=data_port,
                baudrate=data_baudrate,
                timeout=0.1
            )
            time.sleep(0.3)

            # 启动读取线程
            self.is_reading = True
            self.cli_thread = threading.Thread(target=self.read_cli_data, daemon=True)
            self.cli_thread.start()
            self.data_thread = threading.Thread(target=self.read_radar_data, daemon=True)
            self.data_thread.start()

            # 启动数据队列处理（主线程）
            self.root.after(100, self.process_data_queue)

            self.status_label.config(text=f"● 已连接\nCLI:{cli_port} 数据:{data_port}", foreground="green")
            self.connect_btn.config(
                text="断开",
                image=self.icons.get_png('disconnect', 24),
                compound=tk.LEFT,
            )
            # 连接后检查是否有数据输出，延迟1秒后检查
            self.root.after(1000, self.check_radar_running)
            self.update_info(f"已连接 - CLI:{cli_port}@{cli_baudrate} | 数据:{data_port}@{data_baudrate}")
            # 注意：按钮状态由on_command_check_changed控制，这里不再设置

            # 清空缓冲区（不发送sensorStop，避免干扰正在运行的雷达）

        except Exception as e:
            messagebox.showerror("连接失败", f"无法连接串口:\n{e}")
            self.disconnect()

    def disconnect(self):
        """断开双串口连接"""
        self.is_reading = False

        # 等待线程结束
        if self.cli_thread:
            self.cli_thread.join(timeout=1)
        if self.data_thread:
            self.data_thread.join(timeout=1)

        # 关闭CLI端口
        if self.cli_conn and self.cli_conn.is_open:
            self.cli_conn.close()

        # 关闭数据端口
        if self.data_conn and self.data_conn.is_open:
            self.data_conn.close()

            self.status_label.config(text="● 未连接", foreground="red")
        self.connect_btn.config(
            text="连接双端口",
            image=self.icons.get_png('connect', 24),
            compound=tk.LEFT,
        )
        # v1.1.3: 按钮始终保持可用，点击时检查连接状态
        self.update_info("已断开连接")

    def check_radar_running(self):
        """检查雷达是否正在运行（通过检测数据输出）"""
        if not self.cli_conn or not self.cli_conn.is_open:
            return

        # 检查数据端口是否有数据
        has_data = False
        if self.data_conn and self.data_conn.is_open:
            try:
                has_data = self.data_conn.in_waiting > 0
            except:
                pass

        # 根据数据输出情况更新测试状态（v1.1.3: 不再控制按钮状态）
        if has_data:
            # 有数据输出，说明雷达正在运行
            if not self.is_testing:
                self.is_testing = True
                self.update_info("检测到雷达正在运行")
        else:
            # 无数据输出
            if not self.is_testing:
                pass  # 不做任何操作

        # 每秒检查一次
        if self.is_reading:
            self.root.after(1000, self.check_radar_running)

    def read_cli_data(self):
        """读取CLI响应数据（后台线程）"""
        error_count = 0
        max_errors = 3  # 连续错误3次则断开

        while self.is_reading:
            try:
                if self.cli_conn and self.cli_conn.is_open:
                    try:
                        if self.cli_conn.in_waiting > 0:
                            data = self.cli_conn.read(self.cli_conn.in_waiting)
                            error_count = 0  # 读取成功，重置错误计数
                            try:
                                text = data.decode('utf-8', errors='ignore')
                                if text.strip():
                                    # 使用线程安全的方式更新GUI
                                    self.root.after(0, lambda t=text: self.cli_output.insert(tk.END, t))
                                    self.root.after(0, lambda: self.cli_output.see(tk.END))
                            except:
                                pass
                    except (OSError, serial.SerialException) as e:
                        # 串口错误，可能是设备断开
                        error_count += 1
                        if error_count >= max_errors:
                            self.root.after(0, lambda: self.update_info("WARN: CLI串口已断开"))
                            self.root.after(0, self.disconnect)
                            break
                time.sleep(0.01)
            except Exception as e:
                # 忽略线程退出时的错误
                if self.is_reading:
                    print(f"CLI读取错误: {e}")
                break

    def read_radar_data(self):
        """读取雷达数据（后台线程）"""
        data_count = 0
        total_count = 0  # 总数据量
        last_time = time.time()
        error_count = 0
        max_errors = 3  # 连续错误3次则断开

        while self.is_reading:
            try:
                if self.data_conn and self.data_conn.is_open:
                    try:
                        if self.data_conn.in_waiting > 0:
                            data = self.data_conn.read(self.data_conn.in_waiting)
                            error_count = 0  # 读取成功，重置错误计数
                            data_count += len(data)
                            total_count += len(data)

                            # 放入队列供主线程处理
                            self.data_queue.put(('data', data))

                            # 更新速率（每秒）
                            now = time.time()
                            if now - last_time >= 1.0:
                                rate = data_count / (now - last_time)
                                self.data_queue.put(('stats', (total_count, rate)))
                                data_count = 0
                                last_time = now
                    except (OSError, serial.SerialException) as e:
                        # 串口错误，可能是设备断开
                        error_count += 1
                        if error_count >= max_errors:
                            self.root.after(0, lambda: self.update_info("WARN: 数据串口已断开"))
                            self.root.after(0, self.disconnect)
                            break

                time.sleep(0.01)

            except Exception as e:
                # 忽略线程退出时的错误
                if self.is_reading:
                    print(f"数据读取错误: {e}")
                break

    def process_data_queue(self):
        """处理数据队列（主线程）"""
        try:
            while not self.data_queue.empty():
                msg_type, data = self.data_queue.get_nowait()

                if msg_type == 'data':
                    # 数据包计数
                    self.packet_count += 1
                    packet_size = len(data)

                    # 只显示简洁的数据包信息，不显示全部数据
                    # 检查是否是TI雷达魔术字（0x0201, 0x0304, 0x0506, 0x0708）
                    if packet_size >= 8:
                        magic = data[0:8].hex()
                        if magic.startswith('0201040306050807'):  # TI雷达数据包头
                            self.data_output.insert(tk.END,
                                f"RADAR #{self.packet_count}: {packet_size} bytes [magic: {magic[:16]}...]\n",
                                'radar_packet')
                        else:
                            # 显示前32字节的十六进制
                            preview = data[:32].hex()
                            self.data_output.insert(tk.END,
                                f"DATA #{self.packet_count}: {packet_size} bytes [{preview}...]\n",
                                'data_packet')
                    else:
                        # 小数据包，显示全部
                        self.data_output.insert(tk.END,
                            f"RAW #{self.packet_count}: {packet_size} bytes [{data.hex()}]\n",
                            'small_packet')

                    self.data_output.see(tk.END)

                    # 更新数据包计数显示
                    self.packet_count_label.config(text=f"{self.packet_count}")

                    # 限制显示行数（保留最后500行）
                    lines = int(self.data_output.index('end-1c').split('.')[0])
                    if lines > 500:
                        self.data_output.delete('1.0', f'{lines-500}.0')

                elif msg_type == 'stats':
                    total_bytes, rate = data
                    self.total_bytes = total_bytes
                    self.data_count_label.config(text=f"{total_bytes:,} bytes")
                    self.data_rate_label.config(text=f"{rate:.0f} B/s")

                elif msg_type == 'error':
                    self.cli_output.insert(tk.END, f"[数据读取错误] {data}\n", 'error')

        except queue.Empty:
            pass

        if self.is_reading:
            self.root.after(100, self.process_data_queue)

    def send_command(self, command):
        """
        发送命令（通过CLI端口）

        改进：
        - 减少等待时间（0.3s -> 0.05s）
        - 刷新输出缓冲区确保命令发送
        - 优化响应读取逻辑
        """
        if not self.cli_conn or not self.cli_conn.is_open:
            return None

        try:
            # 清空输入缓冲区
            self.cli_conn.reset_input_buffer()

            # 发送命令并刷新缓冲区 (v1.1.4: 处理中文编码)
            try:
                encoded_cmd = (command + '\n').encode('ascii')
                self.cli_conn.write(encoded_cmd)
                self.cli_conn.flush()  # 强制刷新发送缓冲区
                self.cli_output.insert(tk.END, f"> {command}\n", 'command')
            except UnicodeEncodeError:
                # 包含非ASCII字符（如中文），跳过发送
                self.cli_output.insert(tk.END, f"WARN: Skipped (contains non-ASCII): {command}\n", 'warning')
                self.update_info(f"已跳过包含中文的注释行")
                return None

            # 短暂等待响应（减少延迟）
            time.sleep(0.05)

            response = ""
            retry_count = 0
            max_retries = 5

            # 尝试读取响应（最多5次，每次间隔20ms）
            while retry_count < max_retries:
                if self.cli_conn.in_waiting > 0:
                    response += self.cli_conn.read(self.cli_conn.in_waiting).decode('ascii', errors='ignore')
                    break
                time.sleep(0.02)
                retry_count += 1

            if response:
                self.cli_output.insert(tk.END, response + '\n', 'response')

            self.cli_output.see(tk.END)
            return response

        except Exception as e:
            self.cli_output.insert(tk.END, f"[ERROR] {e}\n", 'error')
            return None

    def check_and_connect_serial(self):
        """
        检查串口连接状态，如果未连接则提示用户连接（v1.1.3新增）

        返回:
            bool: True表示已连接或用户选择连接后成功，False表示用户取消或连接失败
        """
        # 检查CLI端口是否已连接
        if self.cli_conn and self.cli_conn.is_open:
            return True

        # 未连接，弹出提示对话框
        response = messagebox.askyesno(
            "串口未连接",
            "检测到串口未连接！\n\n是否立即连接串口？\n\n点击'是'连接串口\n点击'否'取消操作",
            icon='warning'
        )

        if not response:
            # 用户选择取消
            self.update_info("操作已取消：串口未连接")
            return False

        # 用户选择连接，尝试连接串口
        self.update_info("正在连接串口...")
        self.connect()

        # 短暂等待连接完成
        time.sleep(0.5)

        # 再次检查连接状态
        if self.cli_conn and self.cli_conn.is_open:
            messagebox.showinfo("连接成功", "串口连接成功！\n\n可以继续操作。")
            return True
        else:
            messagebox.showerror("连接失败", "串口连接失败！\n\n请检查：\n1. 串口是否被其他程序占用\n2. 设备是否正确连接\n3. 端口配置是否正确")
            self.update_info("串口连接失败")
            return False

    def start_test(self):
        """
        开始雷达配置测试（v1.1.3更新：添加串口连接检查）

        执行流程：
        1. 检查串口连接（如未连接则提示连接）
        2. 获取测试名称、配置注释和配置命令
        3. 创建测试记录（包含时间戳）
        4. 清空输出区域
        5. 发送注释行到CLI端口（以%开头，雷达会忽略）
        6. 发送配置命令到CLI端口
        7. 每条命令间隔50ms
        8. 更新UI状态
        9. 记录测试日志

        测试记录包含：
        - name: 测试名称
        - timestamp: 时间戳
        - comments: 注释内容
        - commands: 命令列表
        - result: 测试结果
        - output: 输出数据
        - led_status: LED状态（用户手动确认）

        注释处理：
        - 注释行以%开头，雷达CLI会自动忽略
        - 注释和命令一起发送，保持完整性
        - 便于在CLI输出中看到配置说明
        """
        # v1.1.3: 检查串口连接
        if not self.check_and_connect_serial():
            return

        # 获取配置注释
        comment_text = self.config_comment_text.get("1.0", tk.END).strip()

        # 获取配置命令
        commands_text = self.commands_text.get(1.0, tk.END).strip()
        if not commands_text:
            messagebox.showwarning("警告", "请输入测试配置命令")
            return

        # 分离注释行和命令行
        all_lines = commands_text.split('\n')
        commands = []
        comments_in_cmd = []

        for line in all_lines:
            line = line.strip()
            if not line:
                continue
            elif line.startswith('%'):
                comments_in_cmd.append(line)
            else:
                commands.append(line)

        if not commands:
            messagebox.showwarning("警告", "没有有效的配置命令")
            return

        # 记录测试信息
        self.current_test = {
            'name': self.test_name_entry.get() or f"Test_{datetime.now().strftime('%H%M%S')}",
            'comments': comment_text,
            'commands': commands,
            'start_time': datetime.now().isoformat(),
            'led_status': False,
            'data_output': 0
        }

        self.update_info(f"开始测试: {self.current_test['name']}")
        self.cli_output.insert(tk.END, f"\n{'='*60}\n")
        self.cli_output.insert(tk.END, f"测试: {self.current_test['name']}\n")
        self.cli_output.insert(tk.END, f"{'='*60}\n\n")

        # 重置数据计数
        self.packet_count = 0
        self.total_bytes = 0
        self.packet_count_label.config(text="0")
        self.data_count_label.config(text="0 bytes")
        self.data_rate_label.config(text="0 B/s")

        # 设置测试运行状态
        self.is_testing = True

        # v1.1.3: 不再控制按钮状态，始终保持可用

        # 使用线程异步发送命令，避免UI阻塞
        def send_commands_async():
            """异步发送命令的线程函数（任务8：支持注释）v1.1.4: 过滤中文注释"""
            # 先发送注释框中的注释（跳过中文）
            if comment_text:
                self.root.after(0, lambda: self.cli_output.insert(tk.END, f"\nINFO: 发送配置注释...\n"))
                sent_count = 0
                skipped_count = 0
                for comment_line in comment_text.split('\n'):
                    comment_line = comment_line.strip()
                    if comment_line:
                        if not comment_line.startswith('%'):
                            comment_line = '% ' + comment_line
                        # 检查是否包含非ASCII字符
                        try:
                            comment_line.encode('ascii')
                            self.send_command(comment_line)
                            sent_count += 1
                            time.sleep(0.02)
                        except UnicodeEncodeError:
                            skipped_count += 1
                            self.root.after(0, lambda line=comment_line:
                                self.cli_output.insert(tk.END, f"WARN: 跳过非ASCII注释: {line}\n", 'warning'))

                if skipped_count > 0:
                    self.root.after(0, lambda: self.cli_output.insert(tk.END,
                        f"WARN: 已跳过 {skipped_count} 行非ASCII注释（CLI仅支持ASCII）\n", 'warning'))

            # 发送命令区中的注释行（v1.1.4: 跳过中文）
            if comments_in_cmd:
                self.root.after(0, lambda: self.cli_output.insert(tk.END, f"\nINFO: 发送内联注释...\n"))
                for comment in comments_in_cmd:
                    try:
                        comment.encode('ascii')
                        self.send_command(comment)
                        time.sleep(0.02)
                    except UnicodeEncodeError:
                        self.root.after(0, lambda c=comment:
                            self.cli_output.insert(tk.END, f"WARN: 跳过非ASCII注释: {c}\n", 'warning'))

            # 发送配置命令
            self.root.after(0, lambda: self.cli_output.insert(tk.END, f"\nINFO: 发送 {len(commands)} 条命令...\n"))
            for i, cmd in enumerate(commands, 1):
                self.root.after(0, lambda c=cmd, n=i: self.cli_output.insert(tk.END, f"  {n:2d}. {c}\n"))
                self.root.after(0, lambda: self.cli_output.see(tk.END))
                response = self.send_command(cmd)
                time.sleep(0.05)  # 命令间隔50ms

            self.root.after(0, lambda: self.update_info("雷达已启动！观察右侧数据输出窗口..."))

        # 启动发送线程
        send_thread = threading.Thread(target=send_commands_async, daemon=True)
        send_thread.start()

        # 显示提示（不阻塞）
        self.cli_output.insert(tk.END, "\n" + "="*60 + "\n")
        self.cli_output.insert(tk.END, "OK: 雷达已启动！请检查：\n")
        self.cli_output.insert(tk.END, "1. 板子LED是否闪烁（约2Hz频率）\n")
        self.cli_output.insert(tk.END, "2. 右侧'雷达数据输出'窗口是否有数据包\n")
        self.cli_output.insert(tk.END, "3. 数据包计数是否在增加\n")
        self.cli_output.insert(tk.END, "\n如果看到数据，请勾选'LED闪烁'并保存日志\n")
        self.cli_output.insert(tk.END, "="*60 + "\n\n")
        self.cli_output.see(tk.END)

    def stop_radar(self):
        """
        停止雷达传感器（v1.1.3更新：添加串口连接检查）

        功能：
        - 检查串口连接（如未连接则提示连接）
        - 发送sensorStop命令到CLI端口
        - 终止当前测试
        - 记录测试结果
        - 更新UI状态

        停止命令：
        - sensorStop 0: 停止所有子帧

        注意：
        - 停止后需要等待雷达完全停止再发送新命令
        - 多次发送确保命令生效
        """
        # v1.1.3: 检查串口连接
        if not self.check_and_connect_serial():
            return

        # 使用线程异步停止，避免UI阻塞
        def stop_radar_async():
            """异步停止雷达的线程函数"""
            self.cli_output.insert(tk.END, "\nINFO: 发送停止命令...\n")

            # 多次发送停止命令确保生效（雷达可能需要多次确认）
            for i in range(3):
                self.cli_output.insert(tk.END, f"  尝试 {i+1}/3: sensorStop 0\n")
                self.send_command("sensorStop 0")
                time.sleep(0.2)  # 等待更长时间让雷达响应

            # 等待雷达完全停止
            time.sleep(0.5)

            # 清空数据端口缓冲区
            if self.data_conn and self.data_conn.is_open:
                try:
                    # 读取并丢弃所有缓冲数据
                    self.data_conn.reset_input_buffer()
                    self.data_conn.reset_output_buffer()

                    # 再次检查是否还有数据
                    time.sleep(0.3)
                    if self.data_conn.in_waiting > 0:
                        discarded = self.data_conn.read(self.data_conn.in_waiting)
                        self.cli_output.insert(tk.END, f"WARN: 丢弃缓冲数据: {len(discarded)} 字节\n")
                    else:
                        self.cli_output.insert(tk.END, "OK: 数据端口已清空\n")
                except Exception as e:
                    self.cli_output.insert(tk.END, f"WARN: 清空缓冲区失败: {e}\n")

            # 清除测试状态
            self.is_testing = False

            # v1.1.3: 不再控制按钮状态
            self.root.after(0, lambda: self.update_info("OK: 雷达已停止，缓冲区已清空"))
            self.cli_output.insert(tk.END, "OK: 停止命令已发送\n")
            self.cli_output.insert(tk.END, "TIP: 如果仍有数据输出，请检查雷达状态\n\n")
            self.cli_output.see(tk.END)

        # 启动停止线程
        stop_thread = threading.Thread(target=stop_radar_async, daemon=True)
        stop_thread.start()

        if self.current_test:
            self.current_test['led_status'] = self.led_var.get()
            self.current_test['end_time'] = datetime.now().isoformat()

    def save_test_log(self):
        """保存测试日志到log目录"""
        if not self.current_test:
            messagebox.showwarning("警告", "没有测试记录")
            return

        # 更新测试结果
        self.current_test['led_status'] = self.led_var.get()
        self.current_test['cli_output'] = self.cli_output.get(1.0, tk.END)
        self.current_test['data_bytes'] = self.data_count_label.cget('text')

        # 默认保存到log目录
        default_name = f"{self.current_test['name']}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        filepath = filedialog.asksaveasfilename(
            title="保存测试日志",
            defaultextension=".json",
            initialdir=self.log_dir,  # 使用log目录
            initialfile=default_name,
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )

        if filepath:
            # 确保目录存在
            Path(filepath).parent.mkdir(parents=True, exist_ok=True)

            # 保存
            with open(filepath, 'w', encoding='utf-8') as f:
                json.dump(self.current_test, f, indent=2, ensure_ascii=False)

            self.test_results.append(self.current_test)
            self.update_info(f"已保存: {Path(filepath).name}")
            messagebox.showinfo("成功", f"测试日志已保存到:\n{filepath}")

    # ========== 以下函数已被任务3的新UI替代，保留以防需要 ==========

    # def load_preset_combinations(self):
    #     """加载预设组合（只显示精选示例）- 已被任务3替代"""
    #     pass

    # def on_combination_select(self, event):
    #     """组合选择事件 - 已被任务3替代"""
    #     pass

    # def load_selected_combination(self):
    #     """加载选中的组合 - 已被任务3替代"""
    #     pass

    # def export_all_combinations(self):
    #     """导出所有组合到文件 - 已被任务3替代"""
    #     pass

    # ========== 任务3新增：22命令勾选UI功能 ==========

    def create_command_checkboxes(self):
        """创建22命令的分类勾选框"""
        # 获取按分类组织的命令
        categories = get_commands_by_category()

        row = 0
        for category, commands in categories.items():
            if not commands:
                continue

            # 创建分类标题框架（可折叠）
            category_frame = ttk.Frame(self.cmd_container)
            category_frame.grid(row=row, column=0, sticky='ew', padx=5, pady=2)
            row += 1

            # 折叠状态变量 (v1.1.4 修复)
            collapsed_var = tk.BooleanVar(value=False)

            # 分类标题（带折叠按钮）
            header_frame = ttk.Frame(category_frame)
            header_frame.pack(fill=tk.X)

            # 折叠按钮
            collapse_btn = ttk.Button(header_frame, text="▼", width=2,
                                     command=lambda c=category: self.toggle_category(c))
            collapse_btn.pack(side=tk.LEFT)

            # 保存按钮引用 (v1.1.4)
            if category in self.category_collapsed:
                self.category_collapsed[category]['button'] = collapse_btn

            # 分类名称
            ttk.Label(header_frame, text=f"【{category}】",
                     font=('Arial', 9, 'bold'), foreground='darkblue').pack(side=tk.LEFT, padx=5)

            # 移除分类统计（去掉"?必需/?总"）
            # 原代码：ttk.Label(header_frame, text=f"({required_count}必需/{total_count}总)", foreground='gray').pack(side=tk.LEFT)

            # 命令容器（可折叠）
            cmd_list_frame = ttk.Frame(category_frame)
            cmd_list_frame.pack(fill=tk.X, padx=(20, 0))

            # 保存框架引用以便折叠控制 (v1.1.4)
            self.category_collapsed[category] = {'state': collapsed_var, 'frame': cmd_list_frame, 'button': None}

            # 为每个命令创建勾选框
            for cmd_name, cmd_info in commands:
                var = tk.BooleanVar(value=False)

                # 命令行框架
                cb_frame = ttk.Frame(cmd_list_frame)
                cb_frame.pack(fill=tk.X, pady=1)

                # 必需标记（仅图标）
                if cmd_info.get('required', False):
                    ttk.Label(cb_frame, image=self.icons.get_png('ok', 16)).pack(side=tk.LEFT, padx=2)
                else:
                    ttk.Label(cb_frame, image=self.icons.get_png('info', 16)).pack(side=tk.LEFT, padx=2)

                # 命令名称（可点击选择）
                name_label = ttk.Label(cb_frame, text=cmd_name,
                                      font=('Consolas', 8, 'bold'), width=22, anchor='w',
                                      cursor='hand2', foreground='darkblue')
                name_label.pack(side=tk.LEFT, padx=2)

                # 移到名称后面的勾选框
                cb = ttk.Checkbutton(cb_frame, variable=var, text="",
                                    command=self.on_command_check_changed)
                cb.pack(side=tk.LEFT)

                # 描述
                desc_label = ttk.Label(cb_frame, text=cmd_info['desc'],
                                      foreground='gray', font=('Arial', 8))
                desc_label.pack(side=tk.LEFT, padx=5)

                # 保存到字典
                self.command_checkboxes[cmd_name] = (var, cb, cb_frame)

                # 点击名称切换勾选状态
                name_label.bind('<Button-1>', lambda e, v=var: v.set(not v.get()) or self.on_command_check_changed())

                # 绑定双击和右键打开参数编辑窗口（任务4）
                cb_frame.bind('<Double-Button-1>', lambda e, cn=cmd_name: self.show_param_editor(cn))
                cb_frame.bind('<Button-3>', lambda e, cn=cmd_name: self.show_param_editor(cn))
                name_label.bind('<Double-Button-1>', lambda e, cn=cmd_name: self.show_param_editor(cn))
                name_label.bind('<Button-3>', lambda e, cn=cmd_name: self.show_param_editor(cn))
                desc_label.bind('<Double-Button-1>', lambda e, cn=cmd_name: self.show_param_editor(cn))
                desc_label.bind('<Button-3>', lambda e, cn=cmd_name: self.show_param_editor(cn))

    def _on_canvas_configure(self, event):
        """Canvas大小改变时调整窗口宽度"""
        self.cmd_canvas.itemconfig(self.cmd_canvas_window, width=event.width)

    def _on_mousewheel(self, event):
        """鼠标滚轮事件"""
        self.cmd_canvas.yview_scroll(int(-1*(event.delta/120)), "units")

    def update_line_numbers(self, event=None):
        """更新配置命令区的行号 (v1.1.4)"""
        if not hasattr(self, 'line_numbers'):
            return

        # 获取文本总行数
        line_count = int(self.commands_text.index('end-1c').split('.')[0])

        # 生成行号文本
        line_numbers_text = '\n'.join(str(i) for i in range(1, line_count + 1))

        # 更新行号显示
        self.line_numbers.config(state='normal')
        self.line_numbers.delete('1.0', tk.END)
        self.line_numbers.insert('1.0', line_numbers_text)
        self.line_numbers.config(state='disabled')

    def _sync_scroll(self, *args):
        """同步行号和文本的滚动 (v1.1.4)"""
        if hasattr(self, 'line_numbers'):
            self.line_numbers.yview_moveto(args[0])
        # 调用原始的滚动条命令
        if hasattr(self.commands_text, 'vbar'):
            self.commands_text.vbar.set(*args)

    def toggle_category(self, category):
        """折叠/展开分类 (v1.1.4 完整实现)"""
        if category not in self.category_collapsed:
            return

        cat_info = self.category_collapsed[category]
        collapsed = cat_info['state'].get()

        # 切换状态
        cat_info['state'].set(not collapsed)

        # 隐藏/显示命令列表
        if not collapsed:  # 即将折叠
            cat_info['frame'].pack_forget()
            cat_info['button'].config(text="▶")
            self.update_info(f"折叠: {category}")
        else:  # 即将展开
            cat_info['frame'].pack(fill=tk.X, padx=(20, 0))
            cat_info['button'].config(text="▼")
            self.update_info(f"展开: {category}")

    def on_command_check_changed(self):
        """命令勾选状态改变时更新统计并自动同步配置命令"""
        selected_count = sum(1 for var, _, _ in self.command_checkboxes.values() if var.get())
        total_count = len(self.command_checkboxes)
        self.cmd_info_label.config(text=f"已选: {selected_count}/{total_count}")

        # 自动同步配置命令到右侧区域
        self.apply_selected_commands()

    def select_all_commands(self):
        """全选所有命令"""
        for var, _, _ in self.command_checkboxes.values():
            var.set(True)
        self.on_command_check_changed()
        self.update_info("已全选22个命令")

    def deselect_all_commands(self):
        """取消全选"""
        for var, _, _ in self.command_checkboxes.values():
            var.set(False)
        self.on_command_check_changed()
        self.update_info("已取消全选")

    def select_required_only(self):
        """仅选择必需命令"""
        required_cmds = get_required_commands()
        for cmd_name, (var, _, _) in self.command_checkboxes.items():
            var.set(cmd_name in required_cmds)
        self.on_command_check_changed()
        self.update_info(f"已选择{len(required_cmds)}个必需命令")

    def load_ti_standard_config(self):
        """加载TI标准22命令配置（快速按钮触发，保持兼容）"""
        self.load_template_by_name('TI标准配置（22命令）')

    def load_selected_template(self):
        """加载下拉框选中的模板（任务5新增）"""
        template_name = self.template_combo.get()
        self.load_template_by_name(template_name)

    def load_template_by_name(self, template_name):
        """
        根据模板名称加载配置（任务5/6新增）

        支持的模板：
        基础配置（任务5）：
        - TI标准配置（22命令）：完整配置
        - 最小配置（10命令）：核心必需命令

        场景配置（任务6）：
        - 人员跌倒检测：高帧率+运动检测
        - 房间占用检测：低功耗长期运行
        - 手势识别：高时间分辨率
        - 车辆检测：远距离+宽FOV

        加载流程：
        1. 从CONFIG_TEMPLATES获取模板数据
        2. 遍历所有命令勾选框
        3. 根据模板设置勾选状态
        4. 触发on_command_check_changed（自动同步配置）
        5. 更新下拉框选择
        6. 自动填充测试名称（模板名+"测试"）
        7. 显示加载成功消息

        Args:
            template_name: 模板名称（必须存在于CONFIG_TEMPLATES）
        """
        if template_name not in CONFIG_TEMPLATES:
            messagebox.showwarning("警告", f"未找到模板: {template_name}")
            return

        template = CONFIG_TEMPLATES[template_name]
        template_commands = template['commands']

        # 更新勾选状态
        for cmd_name, (var, _, _) in self.command_checkboxes.items():
            var.set(cmd_name in template_commands)

        self.on_command_check_changed()

        # 更新下拉框选择
        if hasattr(self, 'template_combo'):
            self.template_combo.set(template_name)

        # 自动更新测试名称为模板名称
        self.test_name_entry.delete(0, tk.END)
        self.test_name_entry.insert(0, f"{template_name}测试")

        self.update_info(f"已加载模板: {template_name} ({template['count']}命令)")

    def apply_selected_commands(self):
        """
        应用勾选的命令到配置区域（自动触发）

        功能：
        1. 收集所有勾选的命令
        2. 使用build_command_string构建完整命令
        3. 按order字段排序（确保正确顺序）
        4. 清空配置文本框
        5. 填充排序后的命令
        6. 更新状态信息

        触发时机：
        - 勾选/取消勾选命令时
        - 加载预设模板时
        - 快速按钮操作时

        命令顺序至关重要：
        - sensorStop必须在最前
        - 基础配置在前，高级配置在后
        - sensorStart必须在最后
        """
        # 获取所有勾选的命令
        selected_commands = []
        for cmd_name, (var, _, _) in self.command_checkboxes.items():
            if var.get() and cmd_name in RADAR_COMMANDS:
                cmd_info = RADAR_COMMANDS[cmd_name]
                # 使用build_command_string构建完整命令
                cmd_string = build_command_string(cmd_name, cmd_info)
                selected_commands.append((cmd_info['order'], cmd_string, cmd_name))

        if not selected_commands:
            # 没有选中命令，清空显示（v1.1.3: 不再禁用按钮）
            self.commands_text.delete(1.0, tk.END)
            # 清空性能指标和建议
            self.metrics_text.config(state='normal')
            self.metrics_text.delete("1.0", tk.END)
            self.metrics_text.insert("1.0", "配置命令后自动显示性能指标...")
            self.metrics_text.config(state='disabled')
            self.suggestions_text.config(state='normal')
            self.suggestions_text.delete("1.0", tk.END)
            self.suggestions_text.insert("1.0", "配置命令后自动显示调整建议...")
            self.suggestions_text.config(state='disabled')
            # 清空注释
            self.config_comment_text.delete("1.0", tk.END)
            self.config_comment_text.insert("1.0", "% 配置说明：\n% 创建时间：\n% 应用场景：")
            self.update_info("请先勾选要测试的命令")
            return

        # 按order排序
        selected_commands.sort(key=lambda x: x[0])

        # 清空并填充命令
        self.commands_text.delete(1.0, tk.END)
        for _, cmd, _ in selected_commands:
            self.commands_text.insert(tk.END, cmd + '\n')

        # v1.1.3: 不再控制按钮状态

        # 更新性能指标和建议（任务9）
        self.update_performance_metrics(selected_commands)

        # 自动生成注释（v1.1.2新增）
        self.auto_generate_comment(selected_commands)

        self.update_info(f"已应用{len(selected_commands)}个命令到配置区域")

    def auto_generate_comment(self, selected_commands):
        """
        根据选中的命令自动生成配置注释（v1.1.2新增）

        生成内容：
        - 配置说明（根据命令自动识别场景）
        - 创建时间（当前时间）
        - 应用场景（根据命令组合推断）
        - 命令数量统计
        - 关键配置参数

        Args:
            selected_commands: [(order, cmd_string, cmd_name), ...]
        """
        from datetime import datetime

        # 提取命令名称
        cmd_names = [name for _, _, name in selected_commands]

        # 场景识别
        scene = "通用配置"
        if 'clutterRemoval' in cmd_names:
            if len(cmd_names) > 15:
                scene = "高精度检测场景（含杂波抑制）"
            else:
                scene = "室内人员检测场景"
        elif 'lowPowerCfg' in cmd_names:
            scene = "低功耗长期运行场景"
        elif len(cmd_names) >= 20:
            scene = "完整标准配置"
        elif len(cmd_names) <= 10:
            scene = "最小化配置"

        # 检查测试名称是否包含场景关键词
        test_name = self.test_name_entry.get()
        if test_name:
            if '跌倒' in test_name:
                scene = "人员跌倒检测场景"
            elif '占用' in test_name:
                scene = "房间占用检测场景"
            elif '手势' in test_name:
                scene = "手势识别场景"
            elif '车辆' in test_name:
                scene = "车辆检测场景"

        # 生成注释内容
        comment_lines = []
        comment_lines.append(f"% ========================================")
        comment_lines.append(f"% 雷达配置文件 - {test_name or '自定义配置'}")
        comment_lines.append(f"% ========================================")
        comment_lines.append(f"% 创建时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        comment_lines.append(f"% 应用场景: {scene}")
        comment_lines.append(f"% 命令数量: {len(cmd_names)}个")
        comment_lines.append(f"% ========================================")

        # 添加关键配置信息
        key_configs = []
        for order, cmd_str, cmd_name in selected_commands:
            if cmd_name in ['frameCfg', 'cfarFovCfg_Range', 'aoaFovCfg']:
                if cmd_name in RADAR_COMMANDS:
                    desc = RADAR_COMMANDS[cmd_name].get('desc', '')
                    key_configs.append(f"% - {desc}")

        if key_configs:
            comment_lines.append(f"% 关键配置:")
            comment_lines.extend(key_configs)
            comment_lines.append(f"% ========================================")

        # 更新注释框
        self.config_comment_text.delete("1.0", tk.END)
        self.config_comment_text.insert("1.0", "\n".join(comment_lines))

    def update_performance_metrics(self, selected_commands):
        """
        更新性能指标和参数调整建议（任务9新增）

        分析配置命令并计算关键性能指标：
        - 帧率（FPS）
        - 检测距离（m）
        - 速度范围（m/s）
        - 角度范围（°）
        - 功耗估计

        提供调整建议：
        - 提升性能的参数调整
        - 降低功耗的优化建议
        - 平衡性能和功耗的配置
        """
        # 解析命令参数
        config_params = {}
        for item in selected_commands:
            # 兼容两种格式：(order, cmd_str) 或 (order, cmd_str, cmd_name)
            if len(item) == 3:
                order, cmd_str, _ = item
            else:
                order, cmd_str = item

            parts = cmd_str.split()
            if len(parts) < 1:
                continue
            cmd_name = parts[0]
            # 移除后缀（如_Range, _Doppler）
            base_cmd = cmd_name.replace('_Range', '').replace('_Doppler', '')
            if base_cmd not in config_params:
                config_params[base_cmd] = {}
            config_params[base_cmd][cmd_name] = parts[1:] if len(parts) > 1 else []

        # 计算性能指标
        metrics = []
        suggestions = []

        # 帧率分析
        if 'frameCfg' in config_params:
            try:
                frame_params = config_params['frameCfg'].get('frameCfg', [])
                if len(frame_params) >= 5:
                    frame_period_ms = float(frame_params[4])
                    fps = 1000 / frame_period_ms if frame_period_ms > 0 else 0
                    metrics.append(f"帧率: {fps:.1f} FPS ({frame_period_ms}ms周期)")

                    if fps < 5:
                        suggestions.append("WARN: 帧率较低，考虑减小framePeriodicity以提高响应速度")
                    elif fps > 20:
                        suggestions.append("TIP: 帧率较高，可适当增大framePeriodicity以降低功耗")
            except:
                pass

        # 距离分析
        if 'cfarFovCfg' in config_params:
            try:
                range_params = config_params['cfarFovCfg'].get('cfarFovCfg_Range',
                                                               config_params['cfarFovCfg'].get('cfarFovCfg', []))
                if len(range_params) >= 3:
                    min_range = float(range_params[1])
                    max_range = float(range_params[2])
                    metrics.append(f"检测距离: {min_range}m - {max_range}m")

                    if max_range > 10:
                        suggestions.append("TIP: 长距离检测，建议使用高功率模式")
                    elif max_range < 3:
                        suggestions.append("TIP: 短距离检测，可启用低功耗模式节省电力")
            except:
                pass

        # 速度分析
        if 'cfarFovCfg' in config_params:
            try:
                doppler_params = config_params['cfarFovCfg'].get('cfarFovCfg_Doppler', [])
                if len(doppler_params) >= 3:
                    min_vel = float(doppler_params[1])
                    max_vel = float(doppler_params[2])
                    metrics.append(f"速度范围: {min_vel} - {max_vel} m/s")

                    if abs(max_vel) > 15:
                        suggestions.append("TIP: 高速检测，确保chirp配置支持足够的多普勒带宽")
            except:
                pass

        # 角度分析
        if 'aoaFovCfg' in config_params:
            try:
                aoa_params = config_params['aoaFovCfg'].get('aoaFovCfg', [])
                if len(aoa_params) >= 4:
                    min_az = int(aoa_params[0])
                    max_az = int(aoa_params[1])
                    metrics.append(f"方位角: {min_az}° - {max_az}°")

                    if max_az - min_az > 100:
                        suggestions.append("TIP: 广角覆盖，角度分辨率可能降低")
            except:
                pass

        # CFAR阈值分析
        if 'cfarProcCfg' in config_params:
            try:
                range_cfar = config_params['cfarProcCfg'].get('cfarProcCfg_Range', [])
                if len(range_cfar) >= 7:
                    threshold = float(range_cfar[6])
                    metrics.append(f"CFAR阈值: {threshold} dB")

                    if threshold < 8:
                        suggestions.append("WARN: 阈值较低，可能增加误检，建议提高到8-10 dB")
                    elif threshold > 12:
                        suggestions.append("TIP: 阈值较高，漏检风险增加，可适当降低")
            except:
                pass

        # 功耗分析
        low_power_enabled = 'lowPowerCfg' in config_params
        if low_power_enabled:
            metrics.append("低功耗模式: 已启用")
            suggestions.append("OK: 低功耗模式已启用，适合长期运行场景")
        else:
            metrics.append("低功耗模式: 未启用")
            suggestions.append("TIP: 可启用lowPowerCfg以降低功耗（需权衡性能）")

        # 更新显示
        self.metrics_text.config(state='normal')
        self.metrics_text.delete("1.0", tk.END)
        if metrics:
            self.metrics_text.insert("1.0", "\n".join(metrics))
        else:
            self.metrics_text.insert("1.0", "暂无性能指标数据")
        self.metrics_text.config(state='disabled')

        self.suggestions_text.config(state='normal')
        self.suggestions_text.delete("1.0", tk.END)
        if suggestions:
            self.suggestions_text.insert("1.0", "\n".join(suggestions))
        else:
            self.suggestions_text.insert("1.0", "当前配置无明显优化建议")
        self.suggestions_text.config(state='disabled')

    def show_param_editor(self, cmd_name):
        """
        显示参数编辑窗口（任务4新增）

        触发方式：
        - 双击命令勾选框
        - 右键点击命令勾选框

        窗口功能：
        1. 显示命令的所有参数
        2. 每个参数可单独编辑
        3. 实时预览完整命令
        4. 应用修改到RADAR_COMMANDS
        5. 重置为默认值
        6. 复制命令到剪贴板

        窗口布局：
        - 顶部：命令信息（名称、描述、重要性）
        - 中间：参数编辑区域（滚动）
        - 底部：命令预览和操作按钮

        参数编辑：
        - 使用Entry输入框
        - 实时验证输入
        - 修改后自动更新预览

        Args:
            cmd_name: 命令名称（如'channelCfg'）
        """
        if cmd_name not in RADAR_COMMANDS:
            messagebox.showwarning("警告", f"未找到命令定义: {cmd_name}")
            return

        cmd_info = RADAR_COMMANDS[cmd_name]

        # 创建编辑窗口
        editor_window = tk.Toplevel(self.root)
        editor_window.title(f"参数编辑 - {cmd_name}")
        editor_window.geometry("600x500")
        editor_window.transient(self.root)
        editor_window.grab_set()

        # 窗口图标（尝试）
        try:
            self.icons.apply_window_icons(editor_window)
        except Exception:
            pass

        # ===== 顶部：命令信息 =====
        info_frame = ttk.LabelFrame(editor_window, text="命令信息", padding=10)
        info_frame.pack(fill=tk.X, padx=10, pady=5)

        # 命令名称
        ttk.Label(info_frame, text="命令名称:", font=('Arial', 9, 'bold')).grid(row=0, column=0, sticky=tk.W, pady=2)
        ttk.Label(info_frame, text=cmd_name, font=('Consolas', 9), foreground='blue').grid(row=0, column=1, sticky=tk.W, pady=2)

        # 命令描述
        ttk.Label(info_frame, text="描述:", font=('Arial', 9, 'bold')).grid(row=1, column=0, sticky=tk.W, pady=2)
        ttk.Label(info_frame, text=cmd_info['desc'], foreground='gray').grid(row=1, column=1, sticky=tk.W, pady=2)

        # 分类
        ttk.Label(info_frame, text="分类:", font=('Arial', 9, 'bold')).grid(row=2, column=0, sticky=tk.W, pady=2)
        ttk.Label(info_frame, text=cmd_info['category'], foreground='darkgreen').grid(row=2, column=1, sticky=tk.W, pady=2)

        # 必需标记
        required_text = "是" if cmd_info.get('required', False) else "否"
        ttk.Label(info_frame, text="必需:", font=('Arial', 9, 'bold')).grid(row=3, column=0, sticky=tk.W, pady=2)
        ttk.Label(info_frame, text=required_text).grid(row=3, column=1, sticky=tk.W, pady=2)

        # ===== 中间：参数编辑区 =====
        param_frame = ttk.LabelFrame(editor_window, text="参数编辑", padding=10)
        param_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        # 创建Canvas和滚动条（支持大量参数）
        canvas = tk.Canvas(param_frame, bg='white', highlightthickness=0)
        scrollbar = ttk.Scrollbar(param_frame, orient='vertical', command=canvas.yview)

        canvas.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        # 参数容器
        param_container = ttk.Frame(canvas)
        canvas_window = canvas.create_window((0, 0), window=param_container, anchor='nw')

        # 自适应宽度
        def on_param_canvas_configure(event):
            canvas.itemconfig(canvas_window, width=event.width)
        canvas.bind('<Configure>', on_param_canvas_configure)

        # 绑定滚轮
        def on_param_mousewheel(event):
            canvas.yview_scroll(int(-1*(event.delta/120)), "units")
        canvas.bind_all('<MouseWheel>', on_param_mousewheel)

        # 参数输入框字典
        param_entries = {}

        if 'params' in cmd_info and cmd_info['params']:
            # 创建参数输入框
            for i, (param_name, param_info) in enumerate(cmd_info['params'].items()):
                # 参数名标签
                param_label = ttk.Label(param_container, text=f"{param_name}:",
                                       font=('Consolas', 9, 'bold'), width=20, anchor='e')
                param_label.grid(row=i, column=0, padx=5, pady=5, sticky=tk.E)

                # 参数值输入框
                param_var = tk.StringVar(value=str(param_info['value']))
                param_entry = ttk.Entry(param_container, textvariable=param_var,
                                       font=('Consolas', 9), width=20)
                param_entry.grid(row=i, column=1, padx=5, pady=5, sticky=tk.W)

                # 参数描述
                desc_label = ttk.Label(param_container, text=param_info['desc'],
                                      foreground='gray', font=('Arial', 8))
                desc_label.grid(row=i, column=2, padx=5, pady=5, sticky=tk.W)

                # 保存变量
                param_entries[param_name] = param_var
        else:
            # 无参数
            ttk.Label(param_container, text="此命令没有可编辑的参数",
                     foreground='gray', font=('Arial', 10)).pack(pady=20)

        # 更新滚动区域
        param_container.update_idletasks()
        canvas.configure(scrollregion=canvas.bbox('all'))

        # ===== 底部：命令预览和按钮 =====
        preview_frame = ttk.LabelFrame(editor_window, text="命令预览", padding=10)
        preview_frame.pack(fill=tk.X, padx=10, pady=5)

        # 命令预览文本框
        preview_text = tk.Text(preview_frame, height=2, font=('Consolas', 9),
                              wrap=tk.WORD, bg='lightyellow')
        preview_text.pack(fill=tk.X, pady=5)

        def update_preview():
            """更新命令预览"""
            if 'params' in cmd_info and cmd_info['params']:
                # 构建命令字符串
                cmd_parts = [cmd_name.replace('_Range', '').replace('_Doppler', '')]
                for param_name in cmd_info['params'].keys():
                    value = param_entries[param_name].get().strip()
                    cmd_parts.append(value)
                new_cmd = ' '.join(cmd_parts)
            else:
                new_cmd = cmd_info['cmd']

            preview_text.delete(1.0, tk.END)
            preview_text.insert(1.0, new_cmd)

        # 初始预览
        update_preview()

        # 绑定参数变化事件
        for var in param_entries.values():
            var.trace('w', lambda *args: update_preview())

        # 按钮区域
        button_frame = ttk.Frame(editor_window)
        button_frame.pack(fill=tk.X, padx=10, pady=10)

        def apply_changes():
            """应用修改"""
            # 更新命令定义（临时，不保存到文件）
            new_params = {}
            for param_name, param_var in param_entries.items():
                value_str = param_var.get().strip()
                # 尝试转换类型
                try:
                    if '.' in value_str:
                        value = float(value_str)
                    elif value_str.startswith('0x'):
                        value = value_str  # 保持16进制字符串
                    elif value_str.isdigit() or (value_str.startswith('-') and value_str[1:].isdigit()):
                        value = int(value_str)
                    else:
                        value = value_str
                except:
                    value = value_str

                new_params[param_name] = {
                    'value': value,
                    'desc': cmd_info['params'][param_name]['desc']
                }

            # 更新命令字符串
            if 'params' in cmd_info and cmd_info['params']:
                cmd_parts = [cmd_name.replace('_Range', '').replace('_Doppler', '')]
                for param_name in cmd_info['params'].keys():
                    cmd_parts.append(str(param_entries[param_name].get().strip()))
                new_cmd = ' '.join(cmd_parts)
            else:
                new_cmd = cmd_info['cmd']

            # 临时更新（不保存到全局，只用于当前编辑）
            cmd_info['cmd'] = new_cmd
            cmd_info['params'] = new_params

            # 如果命令已勾选，提示用户重新应用
            if cmd_name in self.command_checkboxes:
                var, _, _ = self.command_checkboxes[cmd_name]
                if var.get():
                    response = messagebox.askyesno(
                        "应用到配置",
                        "参数已修改！\n\n是否立即应用勾选的命令到配置区域？",
                        parent=editor_window
                    )
                    if response:
                        self.apply_selected_commands()

            self.update_info(f"已更新命令: {cmd_name}")
            messagebox.showinfo("成功", "参数已保存！", parent=editor_window)

        def reset_defaults():
            """重置为默认值"""
            response = messagebox.askyesno(
                "确认重置",
                "确定要重置所有参数为默认值吗？",
                parent=editor_window
            )
            if response:
                for param_name, param_var in param_entries.items():
                    # 从原始RADAR_COMMANDS获取默认值
                    if cmd_name in RADAR_COMMANDS:
                        default_value = RADAR_COMMANDS[cmd_name]['params'][param_name]['value']
                        param_var.set(str(default_value))
                update_preview()
                self.update_info(f"已重置参数: {cmd_name}")

        def copy_command():
            """复制命令到剪贴板"""
            cmd_text = preview_text.get(1.0, tk.END).strip()
            editor_window.clipboard_clear()
            editor_window.clipboard_append(cmd_text)
            messagebox.showinfo("成功", "命令已复制到剪贴板！", parent=editor_window)

        # 按钮
        ttk.Button(button_frame, text="应用修改", image=self.icons.get_png('ok', 16), compound=tk.LEFT,
              command=apply_changes).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="重置默认", image=self.icons.get_png('refresh', 16), compound=tk.LEFT,
              command=reset_defaults).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="复制命令", image=self.icons.get_png('copy', 16), compound=tk.LEFT,
              command=copy_command).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="关闭", image=self.icons.get_png('close', 16), compound=tk.LEFT,
              command=editor_window.destroy).pack(side=tk.RIGHT, padx=5)

        # 居中显示
        editor_window.update_idletasks()
        x = self.root.winfo_x() + (self.root.winfo_width() - editor_window.winfo_width()) // 2
        y = self.root.winfo_y() + (self.root.winfo_height() - editor_window.winfo_height()) // 2
        editor_window.geometry(f"+{x}+{y}")

        # 清理滚轮绑定
        def on_closing():
            canvas.unbind_all('<MouseWheel>')
            editor_window.destroy()

        editor_window.protocol("WM_DELETE_WINDOW", on_closing)

    # ========== 任务3新增功能结束 ==========

    def clear_commands(self):
        """清空命令"""
        self.commands_text.delete(1.0, tk.END)

    def load_from_file(self):
        """从文件加载配置"""
        filepath = filedialog.askopenfilename(
            title="加载配置文件",
            filetypes=[("Config files", "*.cfg"), ("All files", "*.*")]
        )

        if filepath:
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()

                self.commands_text.delete(1.0, tk.END)
                self.commands_text.insert(1.0, content)

                self.test_name_entry.delete(0, tk.END)
                self.test_name_entry.insert(0, Path(filepath).stem)

                self.update_info(f"已加载: {Path(filepath).name}")
            except Exception as e:
                messagebox.showerror("加载失败", f"无法加载文件:\n{e}")

    def export_config_file(self):
        """
        导出配置文件（任务7新增）

        功能：
        1. 获取配置注释框内容
        2. 获取配置命令文本框内容
        3. 移除行号（如果有）
        4. 合并注释和命令
        5. 保存为UTF-8编码的.cfg文件

        注释格式：
        - 注释行以 % 开头
        - 空行保留
        - 命令行不带 %

        行号处理：
        - GUI中显示行号方便编辑
        - 导出时自动去除行号
        """
        # 默认文件名
        test_name = self.test_name_entry.get() or "radar_config"
        default_filename = f"{test_name}.cfg"

        filepath = filedialog.asksaveasfilename(
            title="导出配置文件",
            defaultextension=".cfg",
            initialfile=default_filename,
            filetypes=[("Config files", "*.cfg"), ("All files", "*.*")]
        )

        if not filepath:
            return

        try:
            # 获取注释内容
            comment_text = self.config_comment_text.get("1.0", tk.END).strip()

            # 获取配置命令内容（去除可能的行号）
            commands_text = self.commands_text.get("1.0", tk.END).strip()

            # 处理行号（如果存在）- 移除每行开头的数字和空格 (v1.1.4 增强)
            import re
            lines = []
            for line in commands_text.split('\n'):
                stripped = line.strip()
                if not stripped:
                    # 空行保留
                    continue

                # 移除行号格式：数字+空格+内容
                # 匹配模式：可选空白 + 数字 + 空白 + 内容
                match = re.match(r'^\s*\d+\s+(.+)$', line)
                if match:
                    # 有行号，提取内容
                    lines.append(match.group(1).strip())
                else:
                    # 没有行号，直接添加
                    lines.append(stripped)

            # 构建最终内容
            final_content = []

            # 添加注释（如果有）
            if comment_text:
                for comment_line in comment_text.split('\n'):
                    comment_line = comment_line.strip()
                    if comment_line:
                        # 确保注释行以 % 开头
                        if not comment_line.startswith('%'):
                            comment_line = '% ' + comment_line
                        final_content.append(comment_line)
                final_content.append('')  # 注释和命令之间空一行

            # 添加命令
            final_content.extend(lines)

            # 写入文件（UTF-8编码）
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write('\n'.join(final_content))

            self.update_info(f"已导出: {Path(filepath).name}")
            messagebox.showinfo("导出成功", f"配置文件已保存到:\n{filepath}")

        except Exception as e:
            messagebox.showerror("导出失败", f"无法导出文件:\n{e}")

    def update_info(self, message):
        """更新状态栏"""
        self.info_label.config(text=message)
        self.root.update()

def main():
    """主函数 - 处理进程检查和后台启动"""
    # Windows：必须在 tk.Tk() 之前设置 AppUserModelID（任务栏分组/固定/图标识别）
    _set_windows_app_id('WiseFido.TI.RadarConfigTool')

    # 启动取证（仅调试使用）：记录 EXE/onefile 下的 sys.argv，帮助定位“为何未进入 --detach 启动器分支”。
    if os.environ.get('RADAR_CONFIG_TOOL_LOG_ARGS') == '1':
        try:
            from pathlib import Path

            forced_log_path = os.environ.get('RADAR_CONFIG_TOOL_LOG_PATH')
            if forced_log_path:
                log_path = Path(forced_log_path)
                log_path.parent.mkdir(parents=True, exist_ok=True)
            else:
                log_path = None

            if getattr(sys, 'frozen', False):
                # onefile 下 sys.executable 可能指向临时副本；用 argv[0] 更接近用户实际启动的 EXE 路径
                log_dir = Path(sys.argv[0]).resolve().parent
                if not log_dir.exists():
                    log_dir = Path(sys.executable).resolve().parent
            else:
                log_dir = Path(__file__).resolve().parent

            if log_path is None:
                log_path = log_dir / 'radar_config_tool_args.log'
            with open(log_path, 'w', encoding='utf-8', newline='\n') as f:
                f.write(f"frozen={getattr(sys, 'frozen', False)}\n")
                f.write(f"executable={sys.executable}\n")
                f.write(f"argv={sys.argv!r}\n")
        except Exception:
            pass

    # 检查命令行参数：--detach 表示后台启动模式
    detach_mode = '--detach' in sys.argv

    if not detach_mode:
        # 首次启动：检查旧进程并后台启动新进程
        existing_procs = check_existing_process()

        if existing_procs:
            # 使用Windows原生消息框，避免创建Tk窗口
            import ctypes
            old_pids = [str(proc.pid) for proc in existing_procs]

            # MB_YESNO = 4, MB_ICONWARNING = 48, IDYES = 6
            response = ctypes.windll.user32.MessageBoxW(
                0,
                f"发现已有 {len(existing_procs)} 个GUI进程正在运行\nPID: {', '.join(old_pids)}\n\n"
                f"是否关闭旧进程并启动新窗口？\n\n"
                f"点击\"是\"：关闭所有旧进程，打开新窗口\n"
                f"点击\"否\"：取消启动",
                "检测到已运行的实例",
                4 | 48  # MB_YESNO | MB_ICONWARNING
            )

            # IDYES = 6, IDNO = 7
            response = (response == 6)

            if response:
                # print(f"Closing {len(existing_procs)} old processes...")
                for proc in existing_procs:
                    try:
                        proc.terminate()
                        proc.wait(timeout=3)
                        # print(f"Closed old process (PID: {proc.pid})")
                    except Exception as e:
                        pass  # Silently handle errors
                time.sleep(0.5)  # 等待进程完全关闭
            else:
                # print("User cancelled, keeping old processes")
                sys.exit(0)

        # 后台启动新进程
        # EXE模式和脚本模式使用不同的命令
        if getattr(sys, 'frozen', False):
            # EXE模式：直接使用exe路径
            cmd = [sys.executable, '--detach']
            cwd = os.path.dirname(os.path.abspath(sys.executable))
        else:
            # 脚本模式：使用python解释器和脚本路径
            script_path = os.path.abspath(__file__)
            cmd = [sys.executable, script_path, '--detach']
            cwd = os.path.dirname(script_path)

        env = os.environ.copy()
        for k in (
            '_MEIPASS2',
            '_PYI_APPLICATION_HOME_DIR',
            '_PYI_PARENT_PROCESS_LEVEL',
            '_PYI_ARCHIVE_FILE',
        ):
            env.pop(k, None)
        env['PYINSTALLER_RESET_ENVIRONMENT'] = '1'

        creationflags = 0
        if sys.platform == 'win32':
            try:
                creationflags = (
                    subprocess.CREATE_NEW_PROCESS_GROUP
                    | subprocess.DETACHED_PROCESS
                    | subprocess.CREATE_NO_WINDOW
                    | getattr(subprocess, 'CREATE_BREAKAWAY_FROM_JOB', 0)
                )
            except Exception:
                creationflags = subprocess.CREATE_NO_WINDOW

        subprocess.Popen(
            cmd,
            cwd=cwd,
            env=env,
            creationflags=creationflags,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            stdin=subprocess.DEVNULL
        )

        # print("GUI started in background")
        # print("You can close this terminal window now")
        time.sleep(1)
        os._exit(0)  # 退出启动进程（onefile 下更可靠）

    else:
        # 后台模式：实际启动GUI
        root = tk.Tk()

        # 任务栏/标题栏图标：尽早设置（root.withdraw 前后都可；这里在 withdraw 前设置也 OK）
        try:
            IconManager(root).apply_window_icons(root)
        except Exception:
            pass

        # 在显示任何内容之前先隐藏窗口，避免闪现
        root.withdraw()

        # 创建应用实例
        app = RadarTestGUI(root)

        # 在一切准备就绪后再显示窗口
        root.deiconify()

        root.mainloop()


if __name__ == "__main__":
    main()
