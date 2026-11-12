#!/usr/bin/env python3
"""从一方形设计稿生成各平台的应用图标，并校验仓库里的产物没和设计稿脱节。

子命令：

    macos   合成 1024×1024 的 macOS 画布（824 主体 + 100px 透明留白 + 185.4 圆角），
            交给 CI 的 sips / iconutil 切 .icns。
    ico     生成多尺寸 Windows .ico（16→256，≤128 用 32bpp BMP，256 用 PNG 压缩）。
    linux   生成 freedesktop hicolor 用的 PNG（16→512）。
    check   校验 resources/icon.ico 与 resources/icons/ 是否完整、是否落后于设计稿。
    all     macos + ico + linux。

    python3 packaging/make-icons.py all
    python3 packaging/make-icons.py check

为什么要单独合成 macOS 画布：
    macOS 不会给 .icns 自动加圆角/留白，系统把 icns 图片直接铺满 Dock / Finder
    给它的那个方格。Apple 的网格是 1024×1024 画布 + 居中 824×824 的圆角主体
    （四周 100px 透明留白），主体圆角半径 185.4px。直接把「铺满整张画布」的
    设计稿喂给 iconutil，Dock 里会比系统图标大约 24%（1024/824），四角也更方。

源图默认取 resources/originicon.png（设计原图），没有就用 resources/icon.png。
只有 Pillow 依赖；源图建议 >= 512×512，脚本只缩小不放大。
"""

from __future__ import annotations

import argparse
import io
import os
import struct
import sys
from pathlib import Path

from PIL import Image, ImageChops, ImageStat

ROOT = Path(__file__).resolve().parent.parent
RESOURCES = ROOT / "resources"

# Apple 的 macOS 应用图标模板（Big Sur 及以后）
CANVAS = 1024
BODY = 824
RADIUS = 185.4  # 相对 824 主体；约为主体的 22.5%
EXPONENT = 5.0  # 圆角的「连续曲率」程度，5 接近 Apple 的 squircle

ICO_SIZES = (16, 20, 24, 32, 40, 48, 64, 128, 256)
LINUX_SIZES = (16, 24, 32, 48, 64, 128, 256, 512)

DEFAULT_SOURCES = (RESOURCES / "originicon.png", RESOURCES / "icon.png")

# 设计稿改了但忘记重跑 icon 生成时，差异一般远大于这个值（0-255 的通道均值）
STALE_THRESHOLD = 4.0


# --------------------------------------------------------------------------
# 公共
# --------------------------------------------------------------------------
def pick_source(explicit: str | None) -> tuple[Image.Image, Path]:
    candidates = (Path(explicit).resolve(),) if explicit else DEFAULT_SOURCES
    for path in candidates:
        if path.is_file():
            src = Image.open(path).convert("RGBA")
            if src.width != src.height:
                raise SystemExit(f"错误：源图标必须是正方形，当前 {path} 是 {src.width}×{src.height}")
            print(f"源图标：{_rel(path)} ({src.width}×{src.height})")
            return src, path
    if explicit:
        raise SystemExit(f"错误：找不到源图标 {explicit}")
    raise SystemExit("错误：找不到 resources/originicon.png 或 resources/icon.png")


def _rel(path: Path) -> str:
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def _flatten(im: Image.Image, bg: tuple[int, int, int] = (128, 128, 128)) -> Image.Image:
    """把带 alpha 的图压到中性底色上，方便做像素比较。"""
    base = Image.new("RGBA", im.size, (*bg, 255))
    base.alpha_composite(im.convert("RGBA"))
    return base.convert("RGB")


def _mean_diff(a: Image.Image, b: Image.Image) -> float:
    diff = ImageChops.difference(_flatten(a), _flatten(b))
    means = ImageStat.Stat(diff).mean
    return float(sum(means) / len(means))


# --------------------------------------------------------------------------
# macOS
# --------------------------------------------------------------------------
def body_mask(size: int, radius: float, exponent: float, supersample: int = 4) -> Image.Image:
    """返回 size×size 的灰度遮罩：中间是半径为 radius 的连续圆角方形。"""
    import numpy as np

    n = size * supersample
    coord = (np.arange(n, dtype=np.float64) + 0.5) / supersample
    x, y = np.meshgrid(coord, coord)

    # 到最近一条边的距离；直边区域不用算圆角
    cx = np.minimum(x, size - x)
    cy = np.minimum(y, size - y)

    straight = (cx >= radius) | (cy >= radius)
    tx = np.clip((radius - cx) / radius, 0.0, 1.0)
    ty = np.clip((radius - cy) / radius, 0.0, 1.0)
    corner = (tx**exponent + ty**exponent) <= 1.0
    inside = (straight | corner).astype(np.float64)

    # 4×4 超采样降回目标分辨率，得到抗锯齿的 alpha
    mask = inside.reshape(size, supersample, size, supersample).mean(axis=(1, 3))
    return Image.fromarray((mask * 255.0 + 0.5).astype("uint8"), mode="L")


def build_macos_canvas(src: Image.Image, canvas: int, body: int, radius: float, exponent: float) -> Image.Image:
    if src.width < body:
        print(
            f"警告：源图 {src.width}px 小于主体 {body}px，会放大 {body / src.width:.2f}×，"
            "大尺寸下会偏糊；建议换成 >= 1024px 的导出图。",
            file=sys.stderr,
        )
    art = src.resize((body, body), Image.LANCZOS)
    if radius > 0:
        mask = body_mask(body, min(radius, body / 2), exponent)
        art.putalpha(Image.composite(art.getchannel("A"), Image.new("L", art.size, 0), mask))

    out = Image.new("RGBA", (canvas, canvas), (0, 0, 0, 0))
    offset = (canvas - body) // 2
    out.alpha_composite(art, (offset, offset))
    return out


# --------------------------------------------------------------------------
# Windows .ico
# --------------------------------------------------------------------------
def _ico_dib(img: Image.Image) -> bytes:
    """32bpp BMP（含 AND 掩码），Windows XP 以来所有尺寸都能读。"""
    w, h = img.size
    px = img.load()
    header = struct.pack("<IiiHHIIiiII", 40, w, h * 2, 1, 32, 0, w * h * 4, 0, 0, 0, 0)
    xor = bytearray()
    for y in range(h - 1, -1, -1):  # BMP 是自下而上
        for x in range(w):
            r, g, b, a = px[x, y]
            xor += bytes((b, g, r, a))
    stride = ((w + 31) // 32) * 4
    mask = bytearray()
    for y in range(h - 1, -1, -1):
        row = bytearray(stride)
        for x in range(w):
            if px[x, y][3] < 128:
                row[x >> 3] |= 0x80 >> (x & 7)
        mask += row
    return header + bytes(xor) + bytes(mask)


def write_ico(img: Image.Image, out_path: Path, sizes) -> list[tuple[int, str, int]]:
    """写多尺寸 ICO：<=128 用 BMP（兼容性最好），256 用 PNG（体积小）。"""
    payloads: list[tuple[int, bytes, str]] = []
    for size in sorted(set(sizes)):
        if size > img.width:
            print(f"跳过 {size}px：源图只有 {img.width}px", file=sys.stderr)
            continue
        frame = img.resize((size, size), Image.LANCZOS)
        if size >= 256:
            buf = io.BytesIO()
            frame.save(buf, "PNG")
            payloads.append((size, buf.getvalue(), "png"))
        else:
            payloads.append((size, _ico_dib(frame), "bmp"))

    out = bytearray(struct.pack("<HHH", 0, 1, len(payloads)))
    offset = 6 + 16 * len(payloads)
    for size, data, _kind in payloads:
        out += struct.pack(
            "<BBBBHHII",
            0 if size >= 256 else size,  # 0 表示 256
            0 if size >= 256 else size,
            0, 0, 1, 32, len(data), offset,
        )
        offset += len(data)
    for _size, data, _kind in payloads:
        out += data

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(bytes(out))
    return [(size, kind, len(data)) for size, data, kind in payloads]


# --------------------------------------------------------------------------
# 生成入口
# --------------------------------------------------------------------------
def generate_macos(src: Image.Image, out: Path, canvas: int, body: int, radius: float, exponent: float) -> str:
    canvas_img = build_macos_canvas(src, canvas, body, radius, exponent)
    out.parent.mkdir(parents=True, exist_ok=True)
    canvas_img.save(out, "PNG")
    return (
        f"{out.name}: {canvas}×{canvas}，主体 {body}px（{body / canvas:.1%}，"
        f"留白 {(canvas - body) // 2}px），圆角 {radius:.1f}px（主体的 {radius / body:.1%}）"
    )


def generate_ico(src: Image.Image, out: Path, sizes) -> str:
    info = write_ico(src, out, sizes)
    return f"{_rel(out)}: {len(info)} 档 -> " + ", ".join(f"{s}px/{k}" for s, k, _ in info)


def generate_linux(src: Image.Image, outdir: Path) -> str:
    outdir.mkdir(parents=True, exist_ok=True)
    done = []
    for size in LINUX_SIZES:
        if size > src.width:
            print(f"跳过 {size}px：源图只有 {src.width}px", file=sys.stderr)
            continue
        src.resize((size, size), Image.LANCZOS).save(outdir / f"{size}.png", "PNG", optimize=True)
        done.append(size)
    return f"{_rel(outdir)}/{{{','.join(str(s) for s in done)}}}.png"


# --------------------------------------------------------------------------
# check
# --------------------------------------------------------------------------
def cmd_check(args: argparse.Namespace) -> int:
    src, src_path = pick_source(args.source)
    problems: list[str] = []
    warnings: list[str] = []

    ico_path = RESOURCES / "icon.ico"
    if not ico_path.is_file():
        problems.append("缺少 resources/icon.ico")
    else:
        ico = Image.open(ico_path)
        have = {s[0] for s in ico.info.get("sizes", [])}
        want = {s for s in ICO_SIZES if s <= src.width}
        if missing := sorted(want - have):
            problems.append(f"resources/icon.ico 缺少 {missing} 这几档")
        for size in sorted(want & have):
            ico.size = (size, size)
            ico.load()
            diff = _mean_diff(ico.convert("RGBA"), src.resize((size, size), Image.LANCZOS))
            if diff > STALE_THRESHOLD:
                warnings.append(f"icon.ico {size}px 与设计稿差异 {diff:.1f}")

    for size in (s for s in LINUX_SIZES if s <= src.width):
        png = RESOURCES / "icons" / f"{size}.png"
        if not png.is_file():
            problems.append(f"缺少 resources/icons/{size}.png")
            continue
        im = Image.open(png).convert("RGBA")
        if im.size != (size, size):
            problems.append(f"resources/icons/{size}.png 尺寸是 {im.size[0]}×{im.size[1]}")
            continue
        diff = _mean_diff(im, src.resize((size, size), Image.LANCZOS))
        if diff > STALE_THRESHOLD:
            warnings.append(f"resources/icons/{size}.png 与设计稿差异 {diff:.1f}")

    for line in warnings:
        msg = f"{line}（设计稿改过了？请重跑 make-icons.py）"
        if os.environ.get("GITHUB_ACTIONS") == "true":
            print(f"::warning::{msg}")
        else:
            print(f"警告：{msg}", file=sys.stderr)
    for line in problems:
        print(f"错误：{line}", file=sys.stderr)
    if problems:
        return 1
    print(f"图标资源检查通过（源图 {_rel(src_path)}，ico {len(ICO_SIZES)} 档，hicolor {len(LINUX_SIZES)} 档）")
    return 0


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="生成 / 校验 macOS、Windows、Linux 应用图标")
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("macos", help="合成 1024×1024 macOS 画布")
    p.add_argument("--source", help="方形源图标（默认 resources/originicon.png → icon.png）")
    p.add_argument("--out", default=str(ROOT / "icon-macos-1024.png"))
    p.add_argument("--canvas", type=int, default=CANVAS)
    p.add_argument("--body", type=int, default=BODY)
    p.add_argument("--radius", type=float, default=RADIUS, help="主体圆角半径，0 = 保留源图自带圆角")
    p.add_argument("--exponent", type=float, default=EXPONENT)

    p = sub.add_parser("ico", help="生成多尺寸 Windows .ico")
    p.add_argument("--source")
    p.add_argument("--out", default=str(RESOURCES / "icon.ico"))
    p.add_argument("--sizes", default=",".join(str(s) for s in ICO_SIZES))

    p = sub.add_parser("linux", help="生成 freedesktop hicolor PNG")
    p.add_argument("--source")
    p.add_argument("--outdir", default=str(RESOURCES / "icons"))

    p = sub.add_parser("check", help="校验产物是否完整、是否落后于设计稿")
    p.add_argument("--source")

    p = sub.add_parser("all", help="macos + ico + linux")
    p.add_argument("--source")

    args = ap.parse_args(argv)

    if args.cmd == "check":
        return cmd_check(args)

    src, _ = pick_source(args.source)
    if args.cmd == "macos":
        print("已生成 " + generate_macos(src, Path(args.out), args.canvas, args.body, args.radius, args.exponent))
    elif args.cmd == "ico":
        sizes = [int(s) for s in args.sizes.split(",") if s.strip()]
        print("已生成 " + generate_ico(src, Path(args.out), sizes))
    elif args.cmd == "linux":
        print("已生成 " + generate_linux(src, Path(args.outdir)))
    else:  # all
        print("已生成 " + generate_macos(src, ROOT / "icon-macos-1024.png", CANVAS, BODY, RADIUS, EXPONENT))
        print("已生成 " + generate_ico(src, RESOURCES / "icon.ico", ICO_SIZES))
        print("已生成 " + generate_linux(src, RESOURCES / "icons"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
