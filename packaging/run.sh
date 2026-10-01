#!/usr/bin/env bash
# ============================================================
# QueMusic Linux 便携包启动脚本
#
#   用法：./run.sh [传给 QueMusic 的参数]
#
# 逻辑就两件事：切到包目录、启动 bin/QueMusic。
# 包内所有库（bin/ 与 _deps/）在打包时已写成 $ORIGIN 相对 RPATH，程序自己就能找到；
# 其中自带的 FFmpeg 运行库（libav*/libsw*）就放在 bin/ 里 —— 和 Windows 包自带
# avcodec-61.dll 是一个道理，所以宿主机装不装 ffmpeg 都能跑，也不要求解压到固定位置。
#
# 包外只有 Qt（要求 6.10+）：
#   * 发行版包装的 Qt 在系统标准位置 → 动态加载器直接找得到，下面这段自然跳过；
#   * 官方在线安装器装在 ~/Qt/... 或 /opt/Qt/... → 动态加载器不认识，下面补上；
#     也可以用 QT_LIBDIR=/你的/Qt/6.10.x/gcc_64/lib ./run.sh 手动指定。
# ============================================================
set -e

HERE="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
cd "$HERE"

# 包内 bin/：主程序 + 仓库自己的动态库 + 自带的 FFmpeg 运行库。
# 正常靠 RPATH 就能找到，这里显式加一道当保险（万一 RPATH 被别的工具改坏）。
LIB_PATH="$HERE/bin"

# ---- Qt 库目录：QT_LIBDIR 优先 → qmake6/qtpaths6 → 官方安装器的常见位置 ----
QT_LIB="${QT_LIBDIR:-}"
if [ -z "$QT_LIB" ]; then
    for q in qmake6 qmake-qt6 qtpaths6 qtpaths; do
        if command -v "$q" >/dev/null 2>&1; then
            QT_LIB="$("$q" -query QT_INSTALL_LIBS 2>/dev/null || true)"
            [ -n "$QT_LIB" ] && break
        fi
    done
fi
if [ -z "$QT_LIB" ]; then
    QT_LIB="$(ls -d "$HOME"/Qt/6.*/gcc_64/lib /opt/Qt/6.*/gcc_64/lib /opt/qt6/lib 2>/dev/null | tail -n1)"
fi

# Qt 放在前面：宿主自己那套 ffmpeg（只要是 7.x，SONAME 就是 libavcodec.so.61）
# 会被优先使用；宿主没有时自动落到包内 bin/ 里自带的那份。
if [ -n "$QT_LIB" ] && [ -d "$QT_LIB" ]; then
    LIB_PATH="$QT_LIB:$LIB_PATH"
else
    # 找不到 Qt 时先说一句人话：否则动态加载器只会丢一句
    # "libQt6Core.so.6: cannot open shared object file"，很难看出是 Qt 没找到。
    echo "提示：没找到 Qt 的库目录。本包不含 Qt，需要系统里已经装了 Qt 6.10+。" >&2
    echo "      也可以手动指定：QT_LIBDIR=/你的/Qt/6.10.x/gcc_64/lib ./run.sh" >&2
fi
export LD_LIBRARY_PATH="$LIB_PATH${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

exec "./bin/QueMusic" "$@"
