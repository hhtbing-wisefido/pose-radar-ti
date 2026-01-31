r"""生成 GUI 图标资源（从外部 SVG 图标库导出 PNG/ICO）。

- 图标索引：D:\7.project\Icon-data\icons\icons-data.json
- SVG 根目录：D:\7.project\Icon-data\icons
- 输出目录：本脚本同目录下的 image/icons/

说明：Tkinter 无法直接渲染 SVG；运行时使用导出的 PNG/ICO。
"""

from __future__ import annotations

import io
import json
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

def _pip_install(packages: list[str]) -> None:
    subprocess.check_call([sys.executable, '-m', 'pip', 'install', *packages])


try:
    import cairosvg
except ImportError:
    _pip_install(['CairoSVG'])
    import cairosvg


try:
    from PIL import Image, ImageDraw
except ImportError:
    _pip_install(['Pillow'])
    from PIL import Image, ImageDraw


ICONS_ROOT = Path(r"D:\7.project\Icon-data\icons")
ICONS_INDEX = ICONS_ROOT / "icons-data.json"

OUTPUT_DIR = Path(__file__).resolve().parent / "image" / "icons"
PREVIEW_DIR = OUTPUT_DIR / "preview"


@dataclass(frozen=True)
class IconSpec:
    key: str
    library: str
    name: str
    fg: str      # hex, e.g. "#ffffff"
    bg_from: str # hex, gradient start
    bg_to: str   # hex, gradient end


# 本 APP 的图标集合（语义优先；尽量统一 feather-icons，少量 bootstrap-icons）
# 设计目标：所有 UI 图标都带“高饱和渐变圆角底”，前景统一白色，保证更鲜艳且在浅/深色背景都清晰。
ICON_SPECS: list[IconSpec] = [
    # App icon
    IconSpec("app_radar", "bootstrap-icons", "radar", "#ffffff", "#00c6ff", "#0072ff"),

    # Connection / ports
    IconSpec("connect", "bootstrap-icons", "plug-fill", "#ffffff", "#00c853", "#64dd17"),
    IconSpec("disconnect", "bootstrap-icons", "plug-fill", "#ffffff", "#ff1744", "#ff5252"),
    IconSpec("refresh", "feather-icons", "refresh-cw", "#ffffff", "#2979ff", "#00b0ff"),

    # Selection / templates
    IconSpec("select_all", "feather-icons", "check-square", "#ffffff", "#00c853", "#64dd17"),
    IconSpec("select_none", "feather-icons", "square", "#ffffff", "#546e7a", "#90a4ae"),
    IconSpec("select_required", "feather-icons", "star", "#ffffff", "#ff6d00", "#ffab40"),
    IconSpec("template", "feather-icons", "package", "#ffffff", "#651fff", "#d500f9"),
    IconSpec("load", "feather-icons", "download", "#ffffff", "#2962ff", "#448aff"),

    # Actions
    IconSpec("send", "feather-icons", "send", "#ffffff", "#00bfa5", "#1de9b6"),
    IconSpec("stop", "feather-icons", "stop-circle", "#ffffff", "#d50000", "#ff1744"),
    IconSpec("save", "feather-icons", "save", "#ffffff", "#00c6ff", "#0072ff"),
    IconSpec("open_file", "feather-icons", "folder", "#ffffff", "#2962ff", "#448aff"),
    IconSpec("export", "feather-icons", "upload", "#ffffff", "#2962ff", "#448aff"),
    IconSpec("clear", "feather-icons", "trash-2", "#ffffff", "#546e7a", "#90a4ae"),

    # Editor
    IconSpec("edit", "feather-icons", "edit-3", "#ffffff", "#2962ff", "#448aff"),
    IconSpec("copy", "feather-icons", "copy", "#ffffff", "#2962ff", "#448aff"),
    IconSpec("close", "feather-icons", "x", "#ffffff", "#546e7a", "#90a4ae"),

    # Status
    IconSpec("info", "feather-icons", "info", "#ffffff", "#00b0ff", "#18ffff"),
    IconSpec("warn", "feather-icons", "alert-triangle", "#ffffff", "#ff6f00", "#ffd740"),
    IconSpec("ok", "feather-icons", "check-circle", "#ffffff", "#00c853", "#64dd17"),
]


def _load_index() -> dict:
    if not ICONS_INDEX.exists():
        raise FileNotFoundError(f"找不到图标索引: {ICONS_INDEX}")
    return json.loads(ICONS_INDEX.read_text(encoding="utf-8"))


def _find_svg_rel_path(index: dict, library: str, name: str) -> str:
    lib = index["libraries"][library]
    for item in lib["icons"]:
        if item["name"] == name:
            return item["path"]
    raise KeyError(f"在 {library} 未找到图标: {name}")


def _colorize_svg(svg_text: str, color: str) -> str:
    # 常见约定：currentColor；feather 以 stroke 为主；bootstrap 多为 fill。
    # 这里做最小侵入式替换：只替换 currentColor。
    return svg_text.replace("currentColor", color)


def _hex_to_rgb(color: str) -> tuple[int, int, int]:
    c = color.strip().lstrip('#')
    if len(c) != 6:
        raise ValueError(f"非法颜色值: {color}")
    return int(c[0:2], 16), int(c[2:4], 16), int(c[4:6], 16)


def _lerp(a: int, b: int, t: float) -> int:
    return int(round(a + (b - a) * t))


def _render_svg_icon_image(svg_path: Path, size: int, color: str) -> Image.Image:
    svg_text = svg_path.read_text(encoding="utf-8")
    svg_text = _colorize_svg(svg_text, color)
    png_bytes = cairosvg.svg2png(
        bytestring=svg_text.encode("utf-8"),
        output_width=size,
        output_height=size,
    )
    return Image.open(io.BytesIO(png_bytes)).convert("RGBA")


def _make_rounded_gradient_bg(size: int, bg_from: str, bg_to: str) -> Image.Image:
    r1, g1, b1 = _hex_to_rgb(bg_from)
    r2, g2, b2 = _hex_to_rgb(bg_to)

    grad = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    px = grad.load()
    for y in range(size):
        t = 0.0 if size <= 1 else y / (size - 1)
        r = _lerp(r1, r2, t)
        g = _lerp(g1, g2, t)
        b = _lerp(b1, b2, t)
        for x in range(size):
            px[x, y] = (r, g, b, 255)

    radius = max(2, int(round(size * 0.28)))
    mask = Image.new("L", (size, size), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, size - 1, size - 1), radius=radius, fill=255)

    out = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    out.paste(grad, (0, 0), mask)
    return out


def _render_svg_to_colored_png(svg_path: Path, out_png: Path, size: int, fg: str, bg_from: str, bg_to: str) -> None:
    icon_size = max(8, int(round(size * 0.62)))
    icon = _render_svg_icon_image(svg_path, icon_size, fg)

    bg = _make_rounded_gradient_bg(size, bg_from, bg_to)
    layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    x = (size - icon_size) // 2
    y = (size - icon_size) // 2
    layer.paste(icon, (x, y), icon)

    out = Image.alpha_composite(bg, layer)
    out_png.parent.mkdir(parents=True, exist_ok=True)
    out.save(out_png, format="PNG")


def _render_app_ico(png_256: Path, out_ico: Path) -> None:
    img = Image.open(png_256)
    out_ico.parent.mkdir(parents=True, exist_ok=True)
    img.save(
        out_ico,
        sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)],
    )


def _make_spinner_gif(base: Image.Image, out_gif: Path, size: int) -> None:
    base = base.convert("RGBA")
    frames: list[Image.Image] = []
    for angle in range(0, 360, 30):
        frame = base.rotate(-angle, resample=Image.Resampling.BICUBIC, expand=True)
        frame = frame.resize((size, size), resample=Image.Resampling.LANCZOS)
        frames.append(frame)

    out_gif.parent.mkdir(parents=True, exist_ok=True)
    frames[0].save(
        out_gif,
        save_all=True,
        append_images=frames[1:],
        duration=60,
        loop=0,
        disposal=2,
        transparency=0,
    )


def main() -> int:
    index = _load_index()

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    PREVIEW_DIR.mkdir(parents=True, exist_ok=True)

    manifest: dict[str, dict] = {}

    for spec in ICON_SPECS:
        rel = _find_svg_rel_path(index, spec.library, spec.name)
        svg_path = ICONS_ROOT / rel
        if not svg_path.exists():
            raise FileNotFoundError(f"SVG 文件不存在: {svg_path}")

        out16 = OUTPUT_DIR / f"{spec.key}_16.png"
        out24 = OUTPUT_DIR / f"{spec.key}_24.png"
        out32 = PREVIEW_DIR / f"{spec.key}_32.png"

        _render_svg_to_colored_png(svg_path, out16, 16, spec.fg, spec.bg_from, spec.bg_to)
        _render_svg_to_colored_png(svg_path, out24, 24, spec.fg, spec.bg_from, spec.bg_to)
        _render_svg_to_colored_png(svg_path, out32, 32, spec.fg, spec.bg_from, spec.bg_to)

        manifest[spec.key] = {
            "library": spec.library,
            "name": spec.name,
            "svg": str(rel).replace("\\", "/"),
            "fg": spec.fg,
            "bg_from": spec.bg_from,
            "bg_to": spec.bg_to,
            "png16": str(out16.relative_to(Path(__file__).resolve().parent)).replace("\\", "/"),
            "png24": str(out24.relative_to(Path(__file__).resolve().parent)).replace("\\", "/"),
            "preview32": str(out32.relative_to(Path(__file__).resolve().parent)).replace("\\", "/"),
        }

    # App ICO
    app_png_256 = OUTPUT_DIR / "app_radar_256.png"
    rel = _find_svg_rel_path(index, "bootstrap-icons", "radar")
    svg_path = ICONS_ROOT / rel
    _render_svg_to_colored_png(svg_path, app_png_256, 256, "#ffffff", "#00c6ff", "#0072ff")
    _render_app_ico(app_png_256, OUTPUT_DIR / "app_radar.ico")

    # 动画：旋转“纯前景”刷新图标（用于“扫描/刷新端口”按钮的可选动画效果）
    try:
        refresh_svg_rel = _find_svg_rel_path(index, "feather-icons", "refresh-cw")
        refresh_svg_path = ICONS_ROOT / refresh_svg_rel
        spinner_icon = _render_svg_icon_image(refresh_svg_path, 24, "#00b0ff")
        _make_spinner_gif(spinner_icon, OUTPUT_DIR / "spinner_24.gif", 24)
    except Exception as e:
        print(f"[WARN] spinner gif 生成失败: {e}")

    (OUTPUT_DIR / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )

    print(f"已生成 {len(ICON_SPECS)} 个图标资源 -> {OUTPUT_DIR}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
