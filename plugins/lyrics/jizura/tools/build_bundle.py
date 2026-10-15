#!/usr/bin/env python3
"""Bundle the JIZURA runtime + the QML shim into one QML JavaScript library.

    python tools/build_bundle.py [--tier core|recommended|full] [--jizura PATH] [--out FILE]

The JIZURA checkout is only ever *read*: the generated bundle is what the plugin
ships.  Re-run this after updating JIZURA, or after editing src/runtime/*.js.

Why one file: a QML library script (`.pragma library`) is evaluated once and
shares a single scope, so JIZURA's `window.J` singleton, its top-level
declarations and our browser shim all see each other without patching a single
JIZURA source file.
"""
import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DEFAULT_JIZURA = r"C:\Users\Administrator\Desktop\echoaxmusic\JIZURA-main"

# JIZURA file order (its own build.py concatenates sorted(glob('src/*.js'))).
# 30_fonts.js is spliced in right after 01_util.js because it replaces 02_fonts.js.
CORE_BEFORE_FONTS = ["01_util.js"]
RUNTIME_FONTS = "src/runtime/30_fonts.js"
CORE_AFTER_FONTS = [
    "02b_lang.js", "03_text.js", "04_styles.js", "05_anim.js", "05b_registry.js",
    "06_layouts.js", "07_decor.js", "08_planner.js", "08b_omakase.js", "09_render.js",
]
CORE_TAIL = ["11q_sets.js"]            # must come after every pack
RUNTIME_HEAD = ["src/runtime/00_prelude.js", "src/runtime/10_recorder.js", "src/runtime/20_dom.js"]
RUNTIME_TAIL = ["src/runtime/40_bridge.js"]


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


# JIZURA keeps per-cut / per-pass caches in module-level WeakMaps.  The bundle is
# a QML library script that outlives every plugin instance, so each time the host
# destroys the plugin item those weakly-held keys die in one batch - and Qt
# 6.10.3's weak-table collection then corrupts the V4 heap (the process dies at a
# random later property lookup, inside QV4::Value::sameValueZero, or inside
# runGC).  Measured: 3 of 3 load/unload runs crashed with them; 0 of 20+ survived
# when all seven were replaced.  src/runtime/00_prelude.js defines JZW(), which
# stores the entry as a hidden own property on the key: same lifetime semantics,
# no weak table, and invisible to the Object.assign({}, env, ...) /
# Object.assign({}, cut, ...) copies those objects go through.
# `evOwner` is left alone: it is plan-time only and provably not involved.
WEAKMAP_CACHES = ["bgRun", "snapCache", "BBC", "clockCache", "stepCache",
                  "QUOTED", "SPOT"]


def swap_weakmaps(text):
    n = 0
    for name in WEAKMAP_CACHES:
        old = "const %s = new WeakMap();" % name
        if old in text:
            n += text.count(old)
            text = text.replace(old, "const %s = JZW();" % name)
    return text, n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tier", default=None)
    ap.add_argument("--jizura", default=DEFAULT_JIZURA)
    ap.add_argument("--out", default=os.path.join(ROOT, "js", "jizura.bundle.js"))
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()

    cfg = json.loads(read(os.path.join(ROOT, "src", "config", "packs.json")))
    tier = args.tier or cfg["default_tier"]
    if tier not in cfg["tiers"]:
        sys.exit("unknown tier %r (have %s)" % (tier, ", ".join(cfg["tiers"])))
    patterns = cfg["tiers"][tier]

    src_dir = os.path.join(args.jizura, "src")
    if not os.path.isdir(src_dir):
        sys.exit("JIZURA src not found at %s (pass --jizura)" % src_dir)

    available = sorted(f for f in os.listdir(src_dir) if f.endswith(".js"))
    excluded = set(cfg["core_excluded"])

    # ---- resolve the pack list ------------------------------------------------
    packs = []
    if patterns == ["ALL"] or patterns == ["11p_*.js"]:
        packs = [f for f in available if f.startswith("11p_")]
    else:
        for p in patterns:
            if p.endswith("*"):
                prefix = p[:-1]
                packs += [f for f in available if f.startswith(prefix)]
            else:
                if p not in available:
                    sys.exit("pack %r is not in %s" % (p, src_dir))
                packs.append(p)
        packs = sorted(set(packs))

    # ---- resolve the core list ------------------------------------------------
    core = []
    missing = []
    for name in CORE_BEFORE_FONTS + CORE_AFTER_FONTS + CORE_TAIL:
        if name not in available:
            missing.append(name)
        else:
            core.append(name)
    if missing:
        sys.exit("JIZURA src is missing %s" % ", ".join(missing))

    plan = ([(os.path.join(ROOT, p), p) for p in RUNTIME_HEAD]
            + [(os.path.join(src_dir, "01_util.js"), "01_util.js")]
            + [(os.path.join(ROOT, RUNTIME_FONTS), RUNTIME_FONTS)]
            + [(os.path.join(src_dir, n), n) for n in CORE_AFTER_FONTS]
            + [(os.path.join(src_dir, n), n) for n in packs]
            + [(os.path.join(src_dir, n), n) for n in CORE_TAIL]
            + [(os.path.join(ROOT, p), p) for p in RUNTIME_TAIL])

    version = "?"
    vpath = os.path.join(args.jizura, "VERSION")
    if os.path.exists(vpath):
        version = read(vpath).strip()

    parts = []
    total = 0
    manifest = []
    for path, label in plan:
        text = read(path)
        total += len(text.encode("utf-8"))
        manifest.append((label, len(text.encode("utf-8"))))
        parts.append("\n/* ===================== %s ===================== */\n" % label + text)

    banner = (
        ".pragma library\n"
        "/*! JIZURA for QueMusic - generated bundle, do not edit.\n"
        " *\n"
        " * JIZURA - lyric motion video engine, sources included verbatim.\n"
        " * https://github.com/hakoniwa/JIZURA   version %s   MIT licensed.\n"
        " *\n"
        " * MIT License\n"
        " *\n"
        " * Copyright (c) 2026 hakoniwa\n"
        " *\n"
        " * Permission is hereby granted, free of charge, to any person obtaining a copy\n"
        " * of this software and associated documentation files (the \"Software\"), to deal\n"
        " * in the Software without restriction, including without limitation the rights\n"
        " * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
        " * copies of the Software, and to permit persons to whom the Software is\n"
        " * furnished to do so, subject to the following conditions:\n"
        " *\n"
        " * The above copyright notice and this permission notice shall be included in all\n"
        " * copies or substantial portions of the Software.\n"
        " *\n"
        " * THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
        " * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
        " * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
        " * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
        " * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
        " * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
        " * SOFTWARE.\n"
        " *\n"
        " * See THIRD_PARTY_NOTICES.md for the full list of notices.\n"
        " *\n"
        " * tier: %s   (core %d files + %d expression packs)\n"
        " * generated by tools/build_bundle.py\n"
        " *\n"
        " * The QML port lives in src/runtime/*.js:\n"
        " *   00_prelude.js   layer model + the shared JIZURA namespace\n"
        " *   10_recorder.js  Canvas2D -> display-list recorder (the render core)\n"
        " *   20_dom.js       window / document / ES2019 polyfills\n"
        " *   30_fonts.js     Qt font catalogue + metrics, replaces src/02_fonts.js\n"
        " *   40_bridge.js    renderer layer routing + the JZ API used by the QML stage\n"
        " */\n" % (version, tier, len(core), len(packs))
    )
    out = banner + "".join(parts)
    out, nsub = swap_weakmaps(out)

    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write(out)

    if not args.quiet:
        print("weakmaps    : %d replaced by JZW()" % nsub)

    if not args.quiet:
        print("tier        : %s" % tier)
        print("jizura      : %s (v%s)" % (args.jizura, version))
        print("core        : %d files" % len(core))
        print("packs       : %d files -> %s" % (len(packs), ", ".join(p[:-3] for p in packs) or "-"))
        print("excluded    : %s" % ", ".join(sorted(excluded)))
        print("output      : %s" % args.out)
        print("size        : %.1f KB source" % (len(out.encode("utf-8")) / 1024))
        print("largest     :")
        for label, size in sorted(manifest, key=lambda x: -x[1])[:6]:
            print("              %8.1f KB  %s" % (size / 1024, label))


if __name__ == "__main__":
    main()
