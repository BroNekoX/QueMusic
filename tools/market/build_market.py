#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""把 QuePlugins 仓库打包成静态插件市场目录。

产物结构（可直接用 nginx / 任意静态服务器托管）:

    <out>/
      index.json
      packages/<id>.zip
      preview/<id>.<ext>

用法:
    python tools/market/build_market.py \
        --plugins C:/Users/Administrator/Desktop/echoaxmusic/QuePlugins/QuePlugins \
        --out build/market
"""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import shutil
import zipfile
from pathlib import Path

SKIP_DIRS = {".git", "__pycache__", "node_modules", ".idea", ".vscode"}
SKIP_FILES = {".DS_Store", "Thumbs.db"}


def iter_files(root: Path):
    for path in sorted(root.rglob("*")):
        if any(part in SKIP_DIRS for part in path.relative_to(root).parts):
            continue
        if path.is_file() and path.name not in SKIP_FILES:
            yield path


def build_zip(plugin_dir: Path, target: Path) -> tuple[str, int]:
    with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for file in iter_files(plugin_dir):
            arcname = f"{plugin_dir.name}/{file.relative_to(plugin_dir).as_posix()}"
            zf.write(file, arcname)
    data = target.read_bytes()
    return hashlib.sha256(data).hexdigest(), len(data)


def main() -> int:
    parser = argparse.ArgumentParser(description="构建插件市场静态目录")
    parser.add_argument("--plugins", required=True, help="QuePlugins 仓库根目录")
    parser.add_argument("--out", required=True, help="输出目录")
    parser.add_argument("--base-url", default="", help="下载/预览 URL 前缀（默认相对路径）")
    args = parser.parse_args()

    repo = Path(args.plugins)
    out = Path(args.out)
    if not repo.is_dir():
        parser.error(f"插件仓库不存在: {repo}")

    packages = out / "packages"
    previews = out / "preview"
    for folder in (packages, previews):
        folder.mkdir(parents=True, exist_ok=True)

    entries = []
    for group, default_type in (("lyrics", "lyrics"), ("Tools", "function"), ("Sources", "source")):
        group_dir = repo / group
        if not group_dir.is_dir():
            continue
        for plugin_dir in sorted(p for p in group_dir.iterdir() if p.is_dir()):
            info_path = plugin_dir / "info.json"
            if not info_path.is_file():
                print(f"skip {plugin_dir.name}: 缺少 info.json")
                continue
            info = json.loads(info_path.read_text(encoding="utf-8"))
            plugin_type = str(info.get("type", default_type)).lower()
            if plugin_type in ("music", "sources"):
                plugin_type = "source"
            if plugin_type not in ("lyrics", "function", "source"):
                print(f"skip {plugin_dir.name}: type={plugin_type}")
                continue

            plugin_id = plugin_dir.name
            archive = packages / f"{plugin_id}.zip"
            digest, size = build_zip(plugin_dir, archive)

            preview_name = info.get("preview", "info.png")
            preview_src = plugin_dir / preview_name
            preview_url = ""
            if preview_src.is_file():
                preview_file = previews / f"{plugin_id}{preview_src.suffix.lower()}"
                shutil.copyfile(preview_src, preview_file)
                preview_url = f"preview/{preview_file.name}"

            newest = max((f.stat().st_mtime for f in iter_files(plugin_dir)), default=0)
            updated = datetime.datetime.fromtimestamp(newest).strftime("%Y-%m-%d %H:%M") if newest else ""

            entries.append({
                "id": plugin_id,
                "name": info.get("name", plugin_id),
                "type": plugin_type,
                "author": info.get("author", ""),
                "version": info.get("version", ""),
                "description": info.get("description", ""),
                "updated": updated,
                "preview": preview_url,
                "download": f"packages/{archive.name}",
                "sha256": digest,
                "size": size,
            })
            print(f"packed {plugin_id:16s} {size:8d} B  {plugin_type:8s} {updated}")

    base = args.base_url.rstrip("/")
    index = {"version": 1, "plugins": entries}
    text = json.dumps(index, ensure_ascii=False, indent=2) + "\n"
    (out / "index.json").write_text(text, encoding="utf-8")

    print(f"\n{len(entries)} 个插件 -> {out}")
    if base:
        print(f"索引地址: {base}/index.json")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())