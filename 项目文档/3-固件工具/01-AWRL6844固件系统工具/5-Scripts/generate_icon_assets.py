#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""generate_icon_assets.py

从外部图标库 (D:\\7.project\\Icon-data\\icons) 生成本项目Tkinter可用的PNG/ICO资源。

输出目录：
- PNG: 5-Scripts/image/icons/
- 应用图标: 5-Scripts/flash_tool_icon.png + 5-Scripts/flash_tool_icon.ico

依赖：cairosvg, pillow
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Iterable
import re


@dataclass(frozen=True)
class IconSpec:
    key: str
    svg_path: Path
    color: str


PROJECT_SCRIPTS_DIR = Path(__file__).resolve().parent
PROJECT_ICON_DIR = PROJECT_SCRIPTS_DIR / "image" / "icons"

ICON_DATA_DIR = Path(r"D:\7.project\Icon-data\icons")


def _ensure_deps() -> tuple[object, object, object]:
    try:
        import cairosvg  # type: ignore
        from PIL import Image, ImageDraw  # type: ignore

        return cairosvg, Image, ImageDraw
    except Exception:
        raise SystemExit(
            "缺少依赖：请先安装 cairosvg 和 pillow\n"
            "建议命令：pip install cairosvg pillow"
        )


def _render_svg_to_png_bytes(cairosvg: object, svg_bytes: bytes, size: int, color: str) -> bytes:
    # cairosvg.svg2png 返回 bytes
    # 注意：CairoSVG 2.8.x 的 svg2png 不再提供 css 参数，改为直接在SVG里注入<style>做着色。
    svg_text = svg_bytes.decode("utf-8")
    style_tag = (
        "<style type=\"text/css\">"
        f"*{{fill:{color} !important; stroke:{color} !important;}}"
        "</style>"
    )

    # 将 style 注入到 <svg ...> 的开头
    m = re.search(r"<svg[^>]*>", svg_text, flags=re.IGNORECASE)
    if not m:
        raise ValueError("不是有效的SVG内容：未找到 <svg ...> 标签")

    insert_pos = m.end()
    svg_text = svg_text[:insert_pos] + style_tag + svg_text[insert_pos:]

    return cairosvg.svg2png(  # type: ignore[attr-defined]
        bytestring=svg_text.encode("utf-8"),
        output_width=size,
        output_height=size,
    )


def _write_bytes(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def _validate_svg_exists(specs: Iterable[IconSpec]) -> None:
    missing = [s.svg_path for s in specs if not s.svg_path.exists()]
    if missing:
        msg = "\n".join(str(p) for p in missing)
        raise SystemExit(f"以下SVG不存在，请检查路径：\n{msg}")


def _make_app_icon(Image: object, base_png: Path, out_ico: Path) -> None:
    img = Image.open(base_png).convert("RGBA")  # type: ignore[attr-defined]
    sizes = [16, 20, 24, 32, 48, 64, 128, 256]
    icon_images = [img.resize((s, s), resample=Image.Resampling.LANCZOS) for s in sizes]  # type: ignore[attr-defined]
    icon_images[0].save(out_ico, format="ICO", sizes=[(s, s) for s in sizes])  # type: ignore[attr-defined]


def _hex_to_rgb(value: str) -> tuple[int, int, int]:
    v = value.strip().lstrip("#")
    if len(v) != 6:
        raise ValueError(f"Invalid hex color: {value}")
    return int(v[0:2], 16), int(v[2:4], 16), int(v[4:6], 16)


def main() -> None:
    cairosvg, Image, ImageDraw = _ensure_deps()

    PROJECT_ICON_DIR.mkdir(parents=True, exist_ok=True)

    specs = [
        IconSpec(
            key="radar",
            svg_path=ICON_DATA_DIR / "material-icons" / "filled" / "radar.svg",
            color="#3498db",
        ),
        IconSpec(
            key="microchip",
            svg_path=ICON_DATA_DIR / "fontawesome" / "svgs" / "solid" / "microchip.svg",
            color="#2c3e50",
        ),
        IconSpec(
            key="upload",
            svg_path=ICON_DATA_DIR / "material-icons" / "filled" / "upload.svg",
            color="#3498db",
        ),
        IconSpec(
            key="folder_open",
            svg_path=ICON_DATA_DIR / "heroicons" / "optimized" / "24" / "solid" / "folder-open.svg",
            color="#3498db",
        ),
        IconSpec(
            key="search",
            svg_path=ICON_DATA_DIR / "heroicons" / "optimized" / "24" / "solid" / "magnifying-glass.svg",
            color="#2c3e50",
        ),
        IconSpec(
            key="trash",
            svg_path=ICON_DATA_DIR / "fontawesome" / "svgs" / "solid" / "trash-can.svg",
            color="#e74c3c",
        ),
        IconSpec(
            key="refresh",
            svg_path=ICON_DATA_DIR / "fontawesome" / "svgs" / "solid" / "arrows-rotate.svg",
            color="#2c3e50",
        ),
        IconSpec(
            key="plus",
            svg_path=ICON_DATA_DIR / "heroicons" / "optimized" / "24" / "solid" / "plus.svg",
            color="#27ae60",
        ),
        IconSpec(
            key="minus",
            svg_path=ICON_DATA_DIR / "heroicons" / "optimized" / "24" / "solid" / "minus.svg",
            color="#e74c3c",
        ),
        IconSpec(
            key="info",
            svg_path=ICON_DATA_DIR / "fontawesome" / "svgs" / "solid" / "circle-info.svg",
            color="#3498db",
        ),
        IconSpec(
            key="warning",
            svg_path=ICON_DATA_DIR / "fontawesome" / "svgs" / "solid" / "triangle-exclamation.svg",
            color="#e74c3c",
        ),
        IconSpec(
            key="ok",
            svg_path=ICON_DATA_DIR / "fontawesome" / "svgs" / "solid" / "circle-check.svg",
            color="#27ae60",
        ),
        IconSpec(
            key="error",
            svg_path=ICON_DATA_DIR / "fontawesome" / "svgs" / "solid" / "circle-xmark.svg",
            color="#e74c3c",
        ),
        IconSpec(
            key="settings",
            svg_path=ICON_DATA_DIR / "fontawesome" / "svgs" / "solid" / "gear.svg",
            color="#2c3e50",
        ),
        IconSpec(
            key="stop",
            svg_path=ICON_DATA_DIR / "heroicons" / "optimized" / "24" / "solid" / "stop.svg",
            color="#e74c3c",
        ),
        IconSpec(
            key="clock",
            svg_path=ICON_DATA_DIR / "heroicons" / "optimized" / "24" / "solid" / "clock.svg",
            color="#f39c12",
        ),
        IconSpec(
            key="clipboard",
            svg_path=ICON_DATA_DIR / "heroicons" / "optimized" / "24" / "solid" / "clipboard.svg",
            color="#2c3e50",
        ),
        IconSpec(
            key="rocket",
            svg_path=ICON_DATA_DIR / "material-icons" / "filled" / "rocket.svg",
            color="#27ae60",
        ),
        IconSpec(
            key="fire",
            svg_path=ICON_DATA_DIR / "heroicons" / "optimized" / "24" / "solid" / "fire.svg",
            color="#e67e22",
        ),
        IconSpec(
            key="plug",
            svg_path=ICON_DATA_DIR / "bootstrap-icons" / "icons-main" / "icons" / "plug.svg",
            color="#2c3e50",
        ),
    ]

    _validate_svg_exists(specs)

    sizes = [16, 20, 64]

    for spec in specs:
        svg_bytes = spec.svg_path.read_bytes()

        for size in sizes:
            out_path = PROJECT_ICON_DIR / f"{spec.key}_{size}.png"
            png_bytes = _render_svg_to_png_bytes(cairosvg, svg_bytes, size=size, color=spec.color)
            _write_bytes(out_path, png_bytes)

    # 生成应用图标（使用 radar_256 作为源）
    radar_svg = (ICON_DATA_DIR / "material-icons" / "filled" / "radar.svg").read_bytes()
    app_png_path = PROJECT_SCRIPTS_DIR / "flash_tool_icon.png"
    app_ico_path = PROJECT_SCRIPTS_DIR / "flash_tool_icon.ico"

    # 更鲜艳：渐变圆角底 + 白色前景
    import io
    size = 256
    bg = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    c1 = _hex_to_rgb("#00D2FF")
    c2 = _hex_to_rgb("#3A7BD5")
    px = bg.load()
    for y in range(size):
        t = y / (size - 1)
        r = int(c1[0] * (1 - t) + c2[0] * t)
        g = int(c1[1] * (1 - t) + c2[1] * t)
        b = int(c1[2] * (1 - t) + c2[2] * t)
        for x in range(size):
            px[x, y] = (r, g, b, 255)
    mask = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(mask)
    draw.rounded_rectangle((0, 0, size - 1, size - 1), radius=56, fill=255)
    bg.putalpha(mask)

    fg_bytes = _render_svg_to_png_bytes(cairosvg, radar_svg, size=160, color="#ffffff")
    fg = Image.open(io.BytesIO(fg_bytes)).convert("RGBA")
    bg.alpha_composite(fg, ((size - fg.size[0]) // 2, (size - fg.size[1]) // 2))

    app_png_path.parent.mkdir(parents=True, exist_ok=True)
    bg.save(app_png_path, format="PNG")
    _make_app_icon(Image, base_png=app_png_path, out_ico=app_ico_path)

    print("[OK] 图标资源已生成")
    print(f"- PNG目录: {PROJECT_ICON_DIR}")
    print(f"- 应用图标: {app_ico_path.name}, {app_png_path.name}")


if __name__ == "__main__":
    main()
