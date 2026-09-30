#!/usr/bin/env bash
# ============================================================
# QueMusic Linux 便携包启动脚本
#
#   用法：./run.sh [传给 QueMusic 的参数]
#
# 逻辑就两件事：切到包目录、启动 bin/QueMusic。
# 包内所有库（bin/ 与 _deps/）在打包时已写成 $ORIGIN 相对 RPATH，
# 程序自己就能找到，所以这里不需要拼 LD_LIBRARY_PATH，
# 也不要求解压到固定路径（放哪都行）。
#
# 包外只有 Qt（要求 6.10+）：
#   * 发行版包装的 Qt 在系统标准位置 → 动态加载器直接找得到，下面这段自然跳过；
#   * 官方在线安装器装在 ~/Qt/... → 动态加载器不认识，下面这句补上；
#     也可以用 QT_LIBDIR=/你的/Qt/6.10.x/gcc_64/lib ./run.sh 手动指定。
# ============================================================
set -e

HERE="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
cd "$HERE"

# ---- Qt 库目录：QT_LIBDIR 优先 → qmake6 → 官方在线安装器默认位置 ----
QT_LIB="${QT_LIBDIR:-}"
if [ -z "$QT_LIB" ] && command -v qmake6 >/dev/null 2>&1; then
    QT_LIB="$(qmake6 -query QT_INSTALL_LIBS 2>/dev/null || true)"
fi
if [ -z "$QT_LIB" ]; then
    QT_LIB="$(ls -d "$HOME"/Qt/6.*/gcc_64/lib 2>/dev/null | tail -n1)"
fi
if [ -n "$QT_LIB" ] && [ -d "$QT_LIB" ]; then
    export LD_LIBRARY_PATH="$QT_LIB${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
else
    # 找不到 Qt 时先说一句人话：否则动态加载器只会丢一句
    # "libQt6Core.so.6: cannot open shared object file"，很难看出是 Qt 没找到。
    echo "提示：没找到 Qt 的库目录。本包不含 Qt，需要系统里已经装了 Qt 6.10+。" >&2
    echo "      也可以手动指定：QT_LIBDIR=/你的/Qt/6.10.x/gcc_64/lib ./run.sh" >&2
fi

exec "./bin/QueMusic" "$@"
