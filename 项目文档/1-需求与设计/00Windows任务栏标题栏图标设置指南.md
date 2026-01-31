# 🪟 Windows 任务栏/标题栏图标设置指南（Tkinter / PyInstaller）

> 🎯 目标：让 Windows 任务栏/标题栏显示应用图标，而不是默认 `python.exe` 图标。
>
> ✅ 本指南基于本项目已验证的实现与踩坑经验整理，可直接复用到其他 Tkinter 项目。

---

## 🎯 典型现象与结论

### 🔥 现象

- 资源管理器里 EXE 图标正常，但运行后任务栏仍显示“默认 Python 图标”。
- 或者标题栏/任务栏图标偶尔正常，过一会儿又“回退”。
- 或者脚本模式正常，PyInstaller onefile EXE 异常。

### ✅ 结论（最稳组合）

在 Windows 上，Tkinter GUI 想稳定显示任务栏/标题栏图标，推荐组合：

1. ✅ **设置 AppUserModelID**（影响任务栏分组/固定/图标识别）
2. ✅ **`root.iconphoto(True, photo)`**（更可靠的运行期窗口图标）
3. ✅ **强引用 `PhotoImage`**（防止被 GC 后回退）
4. ✅ **PyInstaller 构建时 `--icon` +（onefile 下）`--add-data`**（EXE 外观图标 + 运行期可读 `.ico` 兜底）

---

## 🧠 背景原理（你需要知道的最少知识）

### 🧩 1) 为什么“EXE 有图标”不代表“任务栏也有图标”？

- `--icon xxx.ico` 主要影响 **Explorer/文件图标**。
- **任务栏图标**更依赖“运行时窗口图标（`iconphoto/iconbitmap`）+ AppUserModelID（分组/固定/识别）”。

### 🧩 2) 为什么 `iconbitmap(.ico)` 经常不稳定？

- Tk/Tkinter 在 Windows 上对 `.ico` 的支持存在差异（环境/位数/尺寸/调用方式）。
- 运行时 `.ico` 还需要“真实文件路径”，onefile 模式下如果没有把 `.ico` 打进 `_MEIPASS`，就必然失败。

### 🧩 3) 为什么会“回退成默认 Python 图标”？

- Tkinter 的 `PhotoImage` 如果**没有强引用**，可能被垃圾回收。
- 图标对象被回收后，窗口就会回退到默认图标（常见就是 python 图标）。

---

## ✅ 成功经验（本项目验证有效）

### ✅ A) AppUserModelID：一定要设置，而且要尽早

- ✅ 在 **创建 `tk.Tk()` 之前**调用：

```python
import ctypes

APP_ID = "YourCompany.YourProduct"  # 建议稳定且唯一（不要带版本号）
ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(APP_ID)
```

- ✅ 经验：把它放到 Windows 平台初始化的最前面（模块加载/`main()` 开头都可），避免 Tk 初始化后再设导致不生效。

### ✅ B) `iconphoto` 优先：任务栏更可靠

- ✅ 推荐用 `iconphoto(True, photo)` 设置 256×256 的 PNG/PhotoImage。
- ✅ 并且**必须**保存强引用：

```python
root.iconphoto(True, photo)
root._app_icon_256 = photo  # 关键：强引用，防 GC 回退
```

### ✅ C) `.ico` 仅做兜底：脚本/EXE 分别处理路径

- ✅ 脚本模式：从 `scripts/` 目录读取 `.ico`。
- ✅ PyInstaller onefile：从 `sys._MEIPASS/app_icon.ico` 读取（前提是你把它 `--add-data` 打包进去）。

### ✅ D) onefile EXE：`--icon` 之外，还要 `--add-data`

- ✅ `--icon scripts/app_icon.ico`：让 EXE 文件自身图标正确。
- ✅ `--add-data scripts/app_icon.ico;.`：让运行时可从 `_MEIPASS` 找到 `.ico`（用于 `iconbitmap` 兜底）。

---

## ❌ 失败经验 / 踩坑清单（本项目遇到过或已验证无效/不稳）

### ❌ 1) 只设置 `iconbitmap`，不设 `iconphoto`

- 常见结果：标题栏可能变了，但任务栏仍不稳定，或在不同机器上表现不一致。
- ✅ 结论：**`iconphoto` 才是主力**，`.ico` 只能当兜底。

### ❌ 2) 设置了 `iconphoto`，但没保留引用

- 表现：启动时正常，过一会儿回退到默认 python 图标。
- 根因：`PhotoImage` 被 GC。

### ❌ 3) onefile EXE 里用 `sys._MEIPASS/app_icon.ico`，但构建没 `--add-data`

- 表现：代码逻辑看起来对，但永远找不到 `.ico`，兜底永远不会生效。

### ❌ 4) AppUserModelID 设置太晚 / 或只在“启动器进程”里设置

- 表现：任务栏分组/图标仍像 python。
- 关键点：**AppUserModelID 是“进程级”的**，真正显示窗口的 GUI 进程必须设置。

### ⚠️ 5) AppUserModelID 带版本号

- 影响：每个版本可能被 Windows 当成“不同应用”，出现固定项重复/分组变化。
- ✅ 建议：对外发布的 AppID 用稳定值（如 `Company.Product`），版本号放到 UI 文案即可。

### ⚠️ 6) 任务栏“固定/缓存”导致你以为代码没生效

- 表现：你换了图标、换了 AppID、重打包，但任务栏仍然显示旧图标。
- 原因：Windows 任务栏对“已固定的快捷方式/分组”有缓存策略。
- ✅ 建议排查顺序：
    - 先确认你运行的确实是新 EXE（看版本号/窗口标题/文件时间）
    - 临时取消固定旧图标 → 再运行新 EXE → 再重新固定

### ⚠️ 7) 有“启动器/后台进程”时：必须在真正 GUI 进程里设置 AppID

- 表现：启动器设置了 AppUserModelID，但任务栏还是 python 图标。
- 根因：**AppUserModelID 是进程级的**；任务栏展示跟随“拥有窗口的那个进程”。
- ✅ 经验：
    - 启动器可以存在，但 GUI 子进程必须也执行 `SetCurrentProcessExplicitAppUserModelID()`。
    - 如果你用 `subprocess.Popen()` 启动 GUI，确保 GUI 入口最早执行 AppID 设置。

### ⚠️ 8) 多窗口（`Toplevel`）场景：别只给主窗口设图标

- 现象：主窗口图标正确，但弹出的设置窗口/对话窗口图标还是默认。
- ✅ 建议：
    - 主窗口：用 `root.iconphoto(True, photo)`。
    - 其他 `tk.Toplevel`：也调用 `top.iconphoto(True, photo)`（并保留引用）。
    - 如果你的项目会动态创建很多窗口，把 `set_window_icons()` 封装成工具函数。

### ⚠️ 9) `.ico` 文件本身不合格（尺寸/格式）

- 现象：Explorer 图标有时清晰，有时模糊；任务栏/开始菜单显示不一致。
- ✅ 建议：使用包含多尺寸的 `.ico`（至少含 `16/24/32/48/256`），并确保 256×256 为 PNG 压缩格式（常见兼容最好）。

---

## 🧪 推荐实现（可直接复制）

### ✅ 方案：`AppUserModelID + iconphoto + 强引用 + iconbitmap 兜底`

```python
import os
import sys
import tkinter as tk


def set_windows_app_id(app_id: str) -> None:
    if sys.platform != "win32":
        return
    try:
        import ctypes
        ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(app_id)
    except Exception:
        pass


def set_window_icons(root: tk.Tk, photo_256: "tk.PhotoImage | None" = None) -> None:
    # 1) PNG/PhotoImage 主方案
    if photo_256 is not None:
        try:
            root.iconphoto(True, photo_256)
            root._app_icon_256 = photo_256  # 强引用
        except Exception:
            pass

    # 2) .ico 兜底（需要真实文件路径）
    try:
        if getattr(sys, "frozen", False):
            base = getattr(sys, "_MEIPASS", "")
            ico_path = os.path.join(base, "app_icon.ico") if base else ""
        else:
            ico_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "app_icon.ico")

        if ico_path and os.path.exists(ico_path):
            try:
                root.iconbitmap(ico_path)
            except Exception:
                pass
            try:
                root.iconbitmap(default=ico_path)
            except Exception:
                pass
    except Exception:
        pass


def main():
    set_windows_app_id("YourCompany.YourProduct")

    root = tk.Tk()

    # photo_256 建议用 Pillow 或你的图标渲染器生成
    photo_256 = None
    set_window_icons(root, photo_256=photo_256)

    root.title("Demo")
    root.mainloop()


if __name__ == "__main__":
    main()
```

---

## 📦 PyInstaller 打包要点（onefile）

### ✅ 关键参数

- `--icon scripts/app_icon.ico`：设置 EXE 文件图标
- `--add-data scripts/app_icon.ico;.`：onefile 运行时把 `.ico` 解到 `_MEIPASS`，供 `iconbitmap` 兜底

> 📌 注意：分隔符 Windows 用 `;`，macOS/Linux 用 `:`。

### ✅ onefile 运行期 `.ico` 校验（你应该能看到它）

- 你使用了 `--add-data scripts/app_icon.ico;.` 后，运行时通常会被解压到：`sys._MEIPASS\app_icon.ico`
- 如果该路径不存在：说明 `--add-data` 没生效或代码取路径不对

---

## 🧰 快速自检片段（复制即用）

### 🧪 A) 检查 onefile 环境与 `.ico` 是否可见

```python
import os
import sys

print("frozen:", bool(getattr(sys, "frozen", False)))
print("sys.executable:", sys.executable)
print("_MEIPASS:", getattr(sys, "_MEIPASS", None))

meipass = getattr(sys, "_MEIPASS", None)
if meipass:
    ico = os.path.join(meipass, "app_icon.ico")
    print("ico exists:", os.path.exists(ico), ico)
```

### 🧪 B) 检查 `PhotoImage` 是否被强引用

```python
# 设置后立即检查
root.iconphoto(True, photo)
root._app_icon_256 = photo

# 关键：别把 photo 作为局部变量用完就丢
print("has strong ref:", hasattr(root, "_app_icon_256"))
```

---

---

## 🔍 排查清单（从快到慢）

1. ✅ 你的 GUI 进程是否在 `tk.Tk()` 之前设置了 `SetCurrentProcessExplicitAppUserModelID`？
2. ✅ 是否调用了 `root.iconphoto(True, photo)`？
3. ✅ 是否保留了 `PhotoImage` 强引用（例如 `root._app_icon_256 = photo`）？
4. ✅ onefile 模式下是否把 `.ico` `--add-data` 进包，并且运行时确实存在 `sys._MEIPASS/app_icon.ico`？
5. ✅ `.ico` 是否包含 256×256 多尺寸（有些环境对单尺寸 ico 兼容性差）？
6. ✅ 如果有启动器/后台子进程：真正创建窗口的 GUI 进程是否也设置了 AppUserModelID？
7. ✅ 是否存在 `Toplevel` 子窗口没设置 iconphoto 导致“看起来没修复”？
8. ⚠️ 如果你在 Windows 任务栏“固定”过旧版本：可能需要取消固定后重新固定（属于系统行为差异）。

---

## 🧷 本项目的落地位置（可参考对照）

- ✅ 运行时窗口图标：`scripts/icon_downloader.py` 的 `_set_window_icons(root)`
  - `root.iconphoto(True, icon_256)` + `root._app_icon_256 = icon_256`
  - onefile `.ico` 兜底：从 `sys._MEIPASS/app_icon.ico` 读取
- ✅ AppUserModelID：`scripts/icon_downloader.py` 的 Windows 初始化段
- ✅ 构建脚本：`scripts/build_release.py`
  - `--icon scripts/app_icon.ico`
  - `--add-data scripts/app_icon.ico;.`

---

## ✅ 最小结论（给未来项目的“一句话标准”）

> 🏁 Windows Tkinter 要稳定任务栏图标：**先设 AppUserModelID**，再用 **`iconphoto` + 强引用**，`.ico` 只做兜底，并确保 onefile 下 `--add-data` 让运行期拿得到 `.ico`。
