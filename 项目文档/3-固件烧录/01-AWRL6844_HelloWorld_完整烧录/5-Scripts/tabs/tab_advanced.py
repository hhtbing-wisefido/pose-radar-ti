#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tab_advanced.py - 高级功能标签页
版本: v1.0.8
作者: Benson@Wisefido

⚠️ 此模块不能单独运行，必须从 flash_tool.py 主入口启动！
"""

import tkinter as tk
from tkinter import ttk, scrolledtext

class AdvancedTab:
    """高级功能标签页类"""
    
    def __init__(self, parent_frame, app):
        """
        初始化高级功能标签页
        
        Args:
            parent_frame: 父容器（tk.Frame）
            app: 主应用实例（FlashToolGUI）
        """
        self.frame = parent_frame
        self.app = app
        
        # 检查是否是通过主入口启动
        if not hasattr(app, 'VERSION'):
            self._show_error_and_exit()
        
        # 创建界面
        self.create_ui()
    
    def _show_error_and_exit(self):
        """显示错误并退出"""
        import sys
        print("=" * 70)
        print("⚠️  错误：tab_advanced 模块不能单独运行！")
        print("=" * 70)
        print()
        print("请从主入口启动烧录工具：")
        print()
        print("  cd 5-Scripts")
        print("  python flash_tool.py")
        print()
        print("=" * 70)
        sys.exit(1)
    
    def create_ui(self):
        """创建标签页UI"""
        self.frame.configure(bg="#ecf0f1")
        
        # 主容器 - 两列布局
        left_col = tk.Frame(self.frame, bg="#ecf0f1")
        left_col.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        right_col = tk.Frame(self.frame, bg="#ecf0f1")
        right_col.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # ============= 左列：高级烧录功能 =============
        
        # 标题
        tk.Label(
            left_col,
            text="🔧 高级烧录选项",
            font=("Microsoft YaHei UI", 14, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50"
        ).pack(pady=(0, 15))
        
        # SBL单独烧录
        sbl_frame = tk.LabelFrame(
            left_col,
            text="📦 仅烧录SBL Bootloader",
            font=("Microsoft YaHei UI", 11, "bold"),
            bg="#ecf0f1",
            fg="#e67e22",
            relief=tk.GROOVE,
            bd=2
        )
        sbl_frame.pack(fill=tk.X, pady=(0, 15))
        
        tk.Label(
            sbl_frame,
            text="⚠️ 仅在以下情况使用：\n• SBL损坏需要修复\n• 升级SBL版本\n• 首次烧录（配合App一起）",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1",
            fg="#7f8c8d",
            justify=tk.LEFT
        ).pack(padx=10, pady=5)
        
        tk.Button(
            sbl_frame,
            text="🔥 烧录SBL到 0x2000",
            font=("Microsoft YaHei UI", 11, "bold"),
            command=self.app.flash_sbl_only,
            bg="#e67e22",
            fg="white",
            relief=tk.FLAT,
            padx=20,
            pady=10,
            cursor="hand2"
        ).pack(pady=10)
        
        # App单独烧录
        app_frame = tk.LabelFrame(
            left_col,
            text="📱 仅烧录Application",
            font=("Microsoft YaHei UI", 11, "bold"),
            bg="#ecf0f1",
            fg="#3498db",
            relief=tk.GROOVE,
            bd=2
        )
        app_frame.pack(fill=tk.X, pady=(0, 15))
        
        tk.Label(
            app_frame,
            text="✅ 适用场景：\n• 板子已有SBL且正常工作\n• 只修改了App代码\n• COM3可以连接（说明SBL正常）",
            font=("Microsoft YaHei UI", 9),
            bg="#ecf0f1",
            fg="#7f8c8d",
            justify=tk.LEFT
        ).pack(padx=10, pady=5)
        
        tk.Button(
            app_frame,
            text="🔥 烧录App到 0x42000",
            font=("Microsoft YaHei UI", 11, "bold"),
            command=self.app.flash_app_only,
            bg="#3498db",
            fg="white",
            relief=tk.FLAT,
            padx=20,
            pady=10,
            cursor="hand2"
        ).pack(pady=10)
        
        # 超时设置
        timeout_frame = tk.LabelFrame(
            left_col,
            text="⏱️ 超时设置",
            font=("Microsoft YaHei UI", 11, "bold"),
            bg="#ecf0f1",
            fg="#9b59b6",
            relief=tk.GROOVE,
            bd=2
        )
        timeout_frame.pack(fill=tk.X)
        
        timeout_info = tk.Frame(timeout_frame, bg="#ecf0f1")
        timeout_info.pack(fill=tk.X, padx=10, pady=10)
        
        tk.Label(
            timeout_info,
            text="烧录超时：",
            font=("Microsoft YaHei UI", 10),
            bg="#ecf0f1",
            fg="#2c3e50"
        ).pack(side=tk.LEFT, padx=(0, 10))
        
        timeout_options = ["120秒（标准）", "180秒（推荐）", "300秒（大文件）"]
        self.app.timeout_combo = ttk.Combobox(
            timeout_info,
            values=timeout_options,
            state="readonly",
            width=18,
            font=("Microsoft YaHei UI", 9)
        )
        self.app.timeout_combo.set(timeout_options[1])  # 默认180秒
        self.app.timeout_combo.pack(side=tk.LEFT)
        
        tk.Label(
            timeout_frame,
            text="💡 大固件建议300秒，避免超时失败",
            font=("Microsoft YaHei UI", 8),
            bg="#ecf0f1",
            fg="#7f8c8d"
        ).pack(padx=10, pady=(0, 10))
        
        # ============= 右列：指南和说明 =============
        
        # 标题
        tk.Label(
            right_col,
            text="📖 烧录决策指南",
            font=("Microsoft YaHei UI", 14, "bold"),
            bg="#ecf0f1",
            fg="#2c3e50"
        ).pack(pady=(0, 15))
        
        # 决策指南文本框
        guide_text = scrolledtext.ScrolledText(
            right_col,
            font=("Microsoft YaHei UI", 9),
            bg="#ffffff",
            fg="#2c3e50",
            wrap=tk.WORD,
            relief=tk.GROOVE,
            bd=2,
            padx=10,
            pady=10,
            height=15
        )
        guide_text.pack(fill=tk.BOTH, expand=True)
        
        guide_content = """🤔 如何判断是否需要烧录SBL？

【快速测试】
1️⃣ 点击"基本烧录"标签页的"测试COM3"按钮
2️⃣ 如果显示"端口可用" → SBL正常，可以只烧App
3️⃣ 如果显示"端口不可用" → SBL损坏，需要完整烧录

【详细判断流程】

✅ 只烧录App的情况：
• 板子之前烧录过SBL
• COM3端口可以连接
• 只修改了Application代码
• SBL没有升级需求
→ 使用"仅烧录Application"

⚠️ 需要烧录SBL的情况：
• 全新板子（首次烧录）
• SBL损坏或板子无法启动
• 升级SBL版本
• COM3端口无法连接
→ 使用"完整烧录"或"仅烧录SBL"

【Flash内存布局】

0x0000    ┌─────────────────┐
          │ ROM Bootloader  │ ← 芯片内置
0x2000    ├─────────────────┤
          │ SBL (256KB)     │ ← 可烧录
0x42000   ├─────────────────┤
          │ Application     │ ← 可烧录
          │ (最大3.75MB)    │
0x3FFFFF  └─────────────────┘

【烧录地址说明】
• SBL地址：0x2000（固定，芯片ROM要求）
• App地址：0x42000（固定，SBL编译时确定）
• 地址完全自动，无需手动设置

【最佳实践】

💚 推荐：日常开发使用"仅烧录App"
• 节省时间（跳过SBL烧录）
• 安全性高（不影响SBL）
• 适合频繁测试

🔴 注意：首次烧录必须用"完整烧录"
• SBL是启动的关键
• 没有SBL无法运行App
• 完整烧录最安全
"""
        
        guide_text.insert(tk.END, guide_content)
        guide_text.config(state=tk.DISABLED)


# 如果直接运行此文件，显示错误提示
if __name__ == "__main__":
    import sys
    print("=" * 70)
    print("⚠️  错误：tab_advanced.py 不能单独运行！")
    print("=" * 70)
    print()
    print("请从主入口启动烧录工具：")
    print()
    print("  cd 5-Scripts")
    print("  python flash_tool.py")
    print()
    print("=" * 70)
    sys.exit(1)
