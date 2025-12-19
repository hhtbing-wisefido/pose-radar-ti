#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试Tkinter中正确实现单行进度更新
模拟flash_tool.py的架构
"""

import tkinter as tk
import subprocess
import threading

class ProgressTest:
    def __init__(self, root):
        self.root = root
        self.root.title("Tkinter进度条测试")
        self.root.geometry("900x600")
        
        # 创建日志显示区域
        self.log_text = tk.Text(root, wrap=tk.WORD, height=30, width=100)
        self.log_text.pack(padx=10, pady=10, fill=tk.BOTH, expand=True)
        
        # 创建按钮
        btn_frame = tk.Frame(root)
        btn_frame.pack(pady=5)
        
        tk.Button(btn_frame, text="开始烧录测试", command=self.start_test).pack(side=tk.LEFT, padx=5)
        tk.Button(btn_frame, text="清空日志", command=self.clear_log).pack(side=tk.LEFT, padx=5)
        
        self.log("Tkinter进度条测试工具\n")
        self.log("=" * 60 + "\n")
    
    def log(self, message):
        """添加日志"""
        self.log_text.config(state=tk.NORMAL)
        self.log_text.insert(tk.END, message)
        self.log_text.see(tk.END)
        self.log_text.config(state=tk.DISABLED)
    
    def clear_log(self):
        """清空日志"""
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state=tk.DISABLED)
    
    def get_last_line_start(self):
        """获取最后一行的起始位置"""
        try:
            last_line = int(self.log_text.index('end-1c').split('.')[0])
            return f"{last_line}.0"
        except:
            return None
    
    def update_line_at_mark(self, mark_pos, new_text):
        """更新指定位置的那一行"""
        try:
            if mark_pos:
                self.log_text.config(state=tk.NORMAL)
                line_num = int(mark_pos.split('.')[0])
                self.log_text.delete(f"{line_num}.0", f"{line_num + 1}.0")
                self.log_text.insert(f"{line_num}.0", new_text if new_text.endswith('\n') else new_text + '\n')
                self.log_text.see(tk.END)
                self.log_text.config(state=tk.DISABLED)
                self.log_text.update_idletasks()
        except Exception as e:
            print(f"更新失败: {e}")
    
    def start_test(self):
        """启动烧录测试"""
        thread = threading.Thread(target=self._test_flash, daemon=True)
        thread.start()
    
    def _test_flash(self):
        """烧录测试线程"""
        self.log("\n开始烧录测试...\n")
        self.log("请拔插USB，等待设备识别...\n\n")
        
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
        
        self.log(f"执行命令: {' '.join(cmd)}\n\n")
        
        try:
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=0,
                creationflags=subprocess.CREATE_NO_WINDOW
            )
            
            # 读取输出（二进制模式）
            buffer = b''
            progress_mark = None
            
            while True:
                byte = process.stdout.read(1)
                if not byte:
                    break
                
                buffer += byte
                
                if byte == b'\r':
                    try:
                        line = buffer[:-1].decode('utf-8', errors='ignore').strip()
                        if line:  # 所有\r结尾的非空行都是进度更新
                            if progress_mark is None:
                                # 第一次：插入新行并记录位置
                                self.log_text.config(state=tk.NORMAL)
                                self.log_text.insert(tk.END, line + '\n')
                                self.log_text.see(tk.END)
                                self.log_text.config(state=tk.DISABLED)
                                progress_mark = self.get_last_line_start()
                            else:
                                # 后续：更新现有行
                                self.update_line_at_mark(progress_mark, line + '\n')
                    except Exception as e:
                        print(f"处理\r失败: {e}")
                    buffer = b''
                    
                elif byte == b'\n':
                    try:
                        line = buffer[:-1].decode('utf-8', errors='ignore').strip()
                        if line:
                            progress_mark = None  # 重置mark
                            self.log(line + '\n')
                    except:
                        pass
                    buffer = b''
            
            process.wait()
            
            if process.returncode == 0:
                self.log("\n✅ 烧录成功！\n")
            else:
                self.log("\n❌ 烧录失败！\n")
        
        except Exception as e:
            self.log(f"\n❌ 出错: {str(e)}\n")

def main():
    root = tk.Tk()
    app = ProgressTest(root)
    root.mainloop()

if __name__ == "__main__":
    main()
