#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
最终方案：使用Label显示进度条，避免Text widget的渲染问题
"""

import tkinter as tk
import subprocess
import threading

class ProgressTestFinal:
    def __init__(self, root):
        self.root = root
        self.root.title("最终进度条测试")
        self.root.geometry("900x600")
        
        # 顶部区域：静态日志
        log_frame = tk.Frame(root)
        log_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        self.log_text = tk.Text(log_frame, wrap=tk.WORD, height=25, width=100)
        self.log_text.pack(fill=tk.BOTH, expand=True)
        
        # 底部区域：动态进度条（使用Label）
        progress_frame = tk.Frame(root, bg="#2c3e50", height=40)
        progress_frame.pack(fill=tk.X, padx=10, pady=(0, 10))
        progress_frame.pack_propagate(False)
        
        self.progress_label = tk.Label(
            progress_frame,
            text="",
            font=("Consolas", 10),
            bg="#2c3e50",
            fg="#27ae60",
            anchor="w",
            justify=tk.LEFT
        )
        self.progress_label.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # 按钮
        btn_frame = tk.Frame(root)
        btn_frame.pack(pady=5)
        
        tk.Button(btn_frame, text="开始烧录测试", command=self.start_test).pack(side=tk.LEFT, padx=5)
        tk.Button(btn_frame, text="清空日志", command=self.clear_log).pack(side=tk.LEFT, padx=5)
        
        self.log("最终进度条测试 - 使用Label显示进度\n")
        self.log("=" * 60 + "\n\n")
    
    def log(self, message):
        """添加静态日志"""
        self.log_text.config(state=tk.NORMAL)
        self.log_text.insert(tk.END, message)
        self.log_text.see(tk.END)
        self.log_text.config(state=tk.DISABLED)
    
    def update_progress(self, text):
        """更新进度条（Label直接替换文本）"""
        self.progress_label.config(text=text)
        self.progress_label.update()  # 强制刷新
    
    def clear_log(self):
        """清空日志"""
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state=tk.DISABLED)
        self.progress_label.config(text="")
    
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
            progress_count = 0
            
            while True:
                byte = process.stdout.read(1)
                if not byte:
                    break
                
                buffer += byte
                
                if byte == b'\r':
                    try:
                        line = buffer[:-1].decode('utf-8', errors='ignore').strip()
                        if line:  # 所有\r结尾的非空行都是进度更新
                            progress_count += 1
                            # 直接更新Label，简单粗暴！
                            self.update_progress(line)
                    except Exception as e:
                        print(f"处理\r失败: {e}")
                    buffer = b''
                    
                elif byte == b'\n':
                    try:
                        line = buffer[:-1].decode('utf-8', errors='ignore').strip()
                        if line:
                            # 换行消息添加到静态日志
                            self.log(line + '\n')
                            # 清空进度条
                            self.update_progress("")
                    except:
                        pass
                    buffer = b''
            
            process.wait()
            
            print(f"\n烧录结束，共{progress_count}次进度更新")
            
            if process.returncode == 0:
                self.log("\n✅ 烧录成功！\n")
                self.update_progress("✅ 烧录成功！")
            else:
                self.log("\n❌ 烧录失败！\n")
                self.update_progress("❌ 烧录失败！")
        
        except Exception as e:
            self.log(f"\n❌ 出错: {str(e)}\n")
            print(f"异常: {e}")

def main():
    root = tk.Tk()
    app = ProgressTestFinal(root)
    root.mainloop()

if __name__ == "__main__":
    main()
