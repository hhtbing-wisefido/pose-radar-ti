#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import annotations

import tkinter as tk
from pathlib import Path


class IconManager:
    def __init__(self, master: tk.Misc, icon_dir: Path | None = None):
        self._master = master
        self._icon_dir = icon_dir or (Path(__file__).resolve().parent / "image" / "icons")
        self._cache: dict[tuple[str, int], tk.PhotoImage] = {}

    @property
    def icon_dir(self) -> Path:
        return self._icon_dir

    def get(self, key: str, size: int) -> tk.PhotoImage:
        cache_key = (key, size)
        cached = self._cache.get(cache_key)
        if cached is not None:
            return cached

        path = self._icon_dir / f"{key}_{size}.png"
        if not path.exists():
            raise FileNotFoundError(f"图标不存在: {path}")

        img = tk.PhotoImage(master=self._master, file=str(path))
        self._cache[cache_key] = img
        return img

    def apply(self, widget: tk.Widget, *, key: str, size: int, text: str | None = None) -> None:
        img = self.get(key, size)
        cfg: dict[str, object] = {"image": img, "compound": "left"}
        if text is not None:
            cfg["text"] = text
        widget.configure(**cfg)

    def make_labelframe_labelwidget(
        self,
        parent: tk.Misc,
        *,
        key: str | None = None,
        icon_key: str | None = None,
        size: int,
        text: str,
        bg: str | None = None,
        fg: str | None = None,
        font: object | None = None,
    ) -> tk.Frame:
        resolved_key = key or icon_key
        if not resolved_key:
            raise TypeError("make_labelframe_labelwidget() missing required argument: 'key'")
        container = tk.Frame(parent, bg=bg)
        tk.Label(container, image=self.get(resolved_key, size), bg=bg).pack(side=tk.LEFT)
        tk.Label(container, text=text, bg=bg, fg=fg, font=font).pack(side=tk.LEFT, padx=(6, 0))
        return container
