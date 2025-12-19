#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
正确处理arprog进度条的方法
关键：不能用text=True，必须用二进制模式读取，自己处理\r
"""

import subprocess
import sys

def test_correct_method():
    """测试正确的进度条处理方法"""
    
    print("=" * 70)
    print("测试：正确处理\\r的进度条")
    print("=" * 70)
    
    arprog_exe = r"D:\7.项目资料\Ti雷达项目\项目文档\3-固件工具\01-AWRL6844固件系统工具\3-Tools\arprog_cmdline_6844.exe"
    sbl_file = r"D:\7.项目资料\Ti雷达项目\项目文档\3-固件工具\01-AWRL6844固件系统工具\1-SBL_Bootloader\sbl.release.appimage"
    
    cmd = [
        arprog_exe,
        "-p", "COM3",
        "-f1", sbl_file,
        "-of1", "8192",
        "-s", "SFLASH",
        "-c"
    ]
    
    print("\n请拔插USB后按Enter...")
    input()
    
    print("\n开始烧录（单行进度）...\n")
    
    # 关键：不用text=True，用二进制模式
    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        bufsize=0  # 无缓冲
    )
    
    buffer = b''
    line_count = 0
    progress_count = 0
    
    while True:
        # 逐字节读取
        byte = process.stdout.read(1)
        if not byte:
            break
        
        buffer += byte
        
        # 遇到\r或\n时处理
        if byte == b'\r':
            # 回车符 - 单行更新进度
            try:
                line = buffer[:-1].decode('utf-8', errors='ignore')
                if '[' in line:
                    # 进度行 - 在同一行更新
                    print(f"\r{line}", end='', flush=True)
                    progress_count += 1
                else:
                    # 普通行
                    print(f"\n{line}")
                    line_count += 1
            except:
                pass
            buffer = b''
            
        elif byte == b'\n':
            # 换行符 - 新行
            try:
                line = buffer[:-1].decode('utf-8', errors='ignore')
                if line.strip():
                    print(f"\n{line}")
                    line_count += 1
            except:
                pass
            buffer = b''
    
    process.wait()
    
    print("\n\n" + "=" * 70)
    print(f"结果：")
    print("=" * 70)
    print(f"普通行数: {line_count}")
    print(f"进度更新次数: {progress_count}")
    print(f"\n✅ 进度条在单行更新！")

if __name__ == "__main__":
    test_correct_method()
