#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试arprog进度条输出机制
目标：研究进度条是单行还是多行
"""

import subprocess
import sys

def test_arprog_output():
    """测试arprog的实际输出"""
    
    print("=" * 70)
    print("测试1: 捕获arprog原始输出")
    print("=" * 70)
    
    # arprog工具路径
    arprog_exe = r"D:\7.项目资料\Ti雷达项目\项目文档\3-固件工具\01-AWRL6844固件系统工具\3-Tools\arprog_cmdline_6844.exe"
    
    # SBL文件（71KB - 会输出307行进度条）
    sbl_file = r"D:\7.项目资料\Ti雷达项目\项目文档\3-固件工具\01-AWRL6844固件系统工具\1-SBL_Bootloader\sbl.release.appimage"
    
    cmd = [
        arprog_exe,
        "-p", "COM3",
        "-f1", sbl_file,
        "-of1", "8192",
        "-s", "SFLASH",
        "-c"
    ]
    
    print(f"\n执行命令: {' '.join(cmd)}\n")
    print("请拔插USB后按Enter...")
    input()
    
    print("\n开始监控输出...\n")
    print("=" * 70)
    
    # 测试1: 逐行读取
    print("\n【测试1: 使用readline()逐行读取】\n")
    
    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )
    
    line_count = 0
    progress_lines = []
    
    for line in process.stdout:
        line_count += 1
        line_stripped = line.rstrip('\r\n')
        
        # 检测进度行
        if '[' in line_stripped or '%' in line_stripped:
            progress_lines.append(line_stripped)
            print(f"[行{line_count:4d}] 进度: {line_stripped}")
        else:
            print(f"[行{line_count:4d}] 普通: {line_stripped}")
    
    process.wait()
    
    print("\n" + "=" * 70)
    print("测试结果汇总:")
    print("=" * 70)
    print(f"总行数: {line_count}")
    print(f"进度行数: {len(progress_lines)}")
    print(f"\n前5行进度:")
    for i, line in enumerate(progress_lines[:5]):
        print(f"  {i+1}. {line}")
    print(f"\n后5行进度:")
    for i, line in enumerate(progress_lines[-5:]):
        print(f"  {i+1}. {line}")
    
    # 分析结论
    print("\n" + "=" * 70)
    print("结论分析:")
    print("=" * 70)
    
    if len(progress_lines) > 100:
        print(f"✅ 进度条是多行输出！({len(progress_lines)}行)")
        print("   每次进度更新都输出新的一行（使用\\n）")
    elif len(progress_lines) == 1:
        print(f"✅ 进度条是单行输出！")
        print("   使用\\r回车符更新同一行")
    else:
        print(f"⚠️  进度行数: {len(progress_lines)}")
        print("   需要进一步分析")
    
    print("\n" + "=" * 70)
    print("原始字节分析")
    print("=" * 70)
    print("重新运行测试，检测\\r和\\n字符...")
    
    # 测试2: 字节级读取
    print("\n【测试2: 字节级读取，检测\\r和\\n】\n")
    print("请再次拔插USB后按Enter...")
    input()
    
    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        bufsize=0  # 无缓冲
    )
    
    output_bytes = b''
    chunk_count = 0
    
    while True:
        chunk = process.stdout.read(1)
        if not chunk:
            break
        
        output_bytes += chunk
        chunk_count += 1
        
        # 显示前1000个字节的特殊字符
        if chunk_count <= 1000:
            if chunk == b'\r':
                print("\\r", end='', flush=True)
            elif chunk == b'\n':
                print("\\n\n", end='', flush=True)
            elif chunk == b'[':
                print("\n[START]", end='', flush=True)
            elif chunk == b']':
                print("[END]", end='', flush=True)
    
    process.wait()
    
    # 统计\r和\n
    r_count = output_bytes.count(b'\r')
    n_count = output_bytes.count(b'\n')
    
    print("\n\n" + "=" * 70)
    print("字节统计:")
    print("=" * 70)
    print(f"\\r (回车符) 数量: {r_count}")
    print(f"\\n (换行符) 数量: {n_count}")
    
    if r_count > n_count:
        print("\n✅ 结论: arprog使用\\r更新进度（单行模式）")
        print("   但Python的readline()会将\\r转换成\\n")
    elif n_count > 100:
        print("\n✅ 结论: arprog使用\\n输出进度（多行模式）")
    else:
        print("\n⚠️  需要更多数据分析")
    
    print("\n测试完成！")

if __name__ == "__main__":
    test_arprog_output()
