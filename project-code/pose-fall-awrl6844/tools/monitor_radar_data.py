"""
简易雷达数据监控脚本
直接从 COM4 读取数据并显示基本信息
"""

import serial
import struct
import time
import sys

# 配置参数
DATA_PORT = 'COM4'
BAUD_RATE = 921600  # 数据端口通常使用更高的波特率
MAGIC_WORD = [0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0x08, 0x07]

def read_and_parse_frame(port):
    """读取并解析一帧数据"""
    # 查找魔术字
    magic_found = False
    for _ in range(1000):
        byte = port.read(1)
        if len(byte) == 0:
            continue
        if byte[0] == MAGIC_WORD[0]:
            # 尝试读取完整魔术字
            magic = [byte[0]] + list(port.read(7))
            if magic == MAGIC_WORD:
                magic_found = True
                break
    
    if not magic_found:
        return None
    
    # 读取帧头
    header = port.read(32)  # 标准帧头32字节
    if len(header) < 32:
        return None
    
    # 解析版本和长度
    version = struct.unpack('<I', header[0:4])[0]
    total_packet_len = struct.unpack('<I', header[4:8])[0]
    platform = struct.unpack('<I', header[8:12])[0]
    frame_number = struct.unpack('<I', header[12:16])[0]
    
    return {
        'version': version,
        'length': total_packet_len,
        'platform': platform,
        'frame': frame_number
    }

def main():
    print("="*60)
    print("  AWRL6844 雷达数据监控")
    print("="*60)
    print(f"数据端口: {DATA_PORT}")
    print(f"波特率: {BAUD_RATE}")
    print()
    
    try:
        # 打开串口
        print(f"正在打开 {DATA_PORT}...", end=' ')
        port = serial.Serial(
            port=DATA_PORT,
            baudrate=BAUD_RATE,
            timeout=1
        )
        print("✅ 成功")
        print()
        print("等待数据帧... (按 Ctrl+C 停止)")
        print("-"*60)
        
        frame_count = 0
        byte_count = 0
        start_time = time.time()
        last_data_time = time.time()
        TIMEOUT = 15  # 15秒超时
        data_received = False
        
        while True:
            # 检查超时
            elapsed_no_data = time.time() - last_data_time
            if not data_received and elapsed_no_data > TIMEOUT:
                print(f"\n❌ 超时! {TIMEOUT}秒内未收到任何数据")
                print("\n可能的问题:")
                print("  1. SOP 开关设置不正确 (功能模式: S7=下, S8=上)")
                print("  2. 固件未正确烧录或启动")
                print("  3. 需要发送配置命令启动雷达")
                print("  4. COM4 端口错误")
                return 1
            
            # 显示倒计时
            if not data_received:
                remaining = TIMEOUT - int(elapsed_no_data)
                print(f"\r等待数据中... 剩余 {remaining:2d} 秒 ", end='', flush=True)
            
            # 简化逻辑：直接读取可用数据
            if port.in_waiting > 0:
                data = port.read(port.in_waiting)
                byte_count += len(data)
                frame_count += 1
                last_data_time = time.time()
                
                if not data_received:
                    data_received = True
                    print(f"\n✅ 收到数据!")
                    print("-"*60)
                
                elapsed = time.time() - start_time
                fps = frame_count / elapsed if elapsed > 0 else 0
                
                print(f"数据包 #{frame_count:4d} | "
                      f"大小: {len(data):5d} 字节 | "
                      f"累计: {byte_count:8d} 字节 | "
                      f"速率: {fps:.2f} 包/秒")
                
                # 每10包显示一次统计
                if frame_count % 10 == 0:
                    avg_size = byte_count / frame_count
                    print(f"  → 统计: {frame_count} 包, "
                          f"平均 {avg_size:.1f} 字节/包, "
                          f"{fps:.2f} 包/秒")
            else:
                time.sleep(0.1)  # 短暂休眠避免CPU占用过高
        
    except serial.SerialException as e:
        print(f"\n❌ 串口错误: {e}")
        print("\n可能的原因:")
        print("  1. COM4 被其他程序占用 (关闭 Visualizer)")
        print("  2. 雷达未启动")
        print("  3. 数据端口配置错误")
        return 1
        
    except KeyboardInterrupt:
        print("\n\n用户中断")
        print("="*60)
        print(f"总共接收: {frame_count} 帧")
        if frame_count > 0:
            print(f"平均帧率: {frame_count / (time.time() - start_time):.2f} fps")
        print("="*60)
        return 0
        
    except Exception as e:
        print(f"\n❌ 错误: {e}")
        import traceback
        traceback.print_exc()
        return 1
        
    finally:
        if 'port' in locals() and port.is_open:
            port.close()
            print("串口已关闭")

if __name__ == "__main__":
    sys.exit(main())
