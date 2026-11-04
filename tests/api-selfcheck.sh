#!/usr/bin/env bash
# 离屏 API 自检：不加载 UI、不联网，验证音源注册、模型、路由、歌词时间轴等不变量。
# 用法：tests/api-selfcheck.sh <可执行文件路径>
set -uo pipefail

BIN="${1:-}"
if [ -z "$BIN" ] || [ ! -x "$BIN" ]; then
    echo "用法: $0 <可执行文件路径>（找不到可执行文件：${BIN:-未给出}）"
    exit 2
fi

export QT_LOGGING_RULES="qt.qpa.*=false"
export QT_ASSUME_STDERR_HAS_CONSOLE=1

echo "── API 自检：$BIN（无 GUI）──"
"$BIN" --self-check
status=$?
if [ "$status" -eq 0 ]; then
    echo "✅ API 自检通过"
else
    echo "❌ API 自检失败（退出码 $status）"
fi
exit "$status"
