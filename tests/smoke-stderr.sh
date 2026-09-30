#!/usr/bin/env bash
# 启动冒烟测试：离屏跑一次应用，只要出现 QML / 渲染层错误就判失败。
# 用途：CI 门禁（.github/workflows/ci.yml）与本地改完代码后的快速自检。
#
# 用法：tests/smoke-stderr.sh <可执行文件路径> [运行秒数]
#   例：tests/smoke-stderr.sh build/bin/QueMusic 12
#
# 只把「代码问题」当失败；环境类噪声（无声卡 / 无 GPU / 无字体）只提示。

set -uo pipefail

BIN="${1:-}"
SECONDS_RUN="${2:-12}"

if [ -z "$BIN" ] || [ ! -x "$BIN" ]; then
    echo "用法: $0 <可执行文件路径> [运行秒数]（找不到可执行文件：${BIN:-未给出}）"
    exit 2
fi

LOG="$(mktemp)"
export QT_QPA_PLATFORM=offscreen          # 不需要显示器/X11
export QT_LOGGING_RULES="qt.qpa.*=false"  # 平台层噪声压掉

echo "── 启动冒烟：$BIN（${SECONDS_RUN}s，offscreen）──"
# 用绝对路径启动：CI 里相对路径 + bash 的 PATH 重置容易踩空
BIN_ABS="$(cd "$(dirname "$BIN")" && pwd)/$(basename "$BIN")"
"$BIN_ABS" >/dev/null 2>"$LOG" &
APP_PID=$!

# 期间死掉也要把日志留下（崩溃本身就是问题）
for _ in $(seq 1 "$SECONDS_RUN"); do
    sleep 1
    kill -0 "$APP_PID" 2>/dev/null || break
done
EXIT_CODE=0
if kill -0 "$APP_PID" 2>/dev/null; then
    kill "$APP_PID" 2>/dev/null
    wait "$APP_PID" 2>/dev/null
    EXITED_EARLY=0
else
    wait "$APP_PID" 2>/dev/null
    EXIT_CODE=$?
    EXITED_EARLY=1
fi

echo "── stderr（$(wc -l < "$LOG") 行）──"
cat "$LOG"

# 判定为失败的错误模式：QML 加载/类型/绑定/着色器问题
FAIL_PATTERNS=(
    'QQmlApplicationEngine failed to load component'
    'is not a type'
    'Cannot read property'
    'TypeError'
    'Unable to assign'
    'Cannot assign to non-existent property'
    'ShaderEffect: Failed'
    'Failed to deserialize QShader'
    'Binding loop detected'
    'qrc:.*: .* is not a'
)

FAILED=0
for pattern in "${FAIL_PATTERNS[@]}"; do
    if grep -Eq "$pattern" "$LOG"; then
        echo "❌ 命中错误模式: $pattern"
        grep -En "$pattern" "$LOG" | head -5
        FAILED=1
    fi
done

if [ "$EXITED_EARLY" -eq 1 ]; then
    case "$EXIT_CODE" in
        0)
            [ "$FAILED" -eq 0 ] && echo "⚠️  应用在 ${SECONDS_RUN}s 内正常退出（CI 无音频设备时常见，不计失败）" ;;
        126|127)
            echo "❌ 可执行文件无法运行（退出码 $EXIT_CODE）：检查 Qt 运行库 / PATH 是否就绪"
            FAILED=1 ;;
        *)
            echo "❌ 应用异常退出（退出码 $EXIT_CODE）：崩溃（>=128 为信号）、初始化失败或提前报错"
            FAILED=1 ;;
    esac
fi

rm -f "$LOG"
if [ "$FAILED" -ne 0 ]; then
    echo "❌ 冒烟测试失败：上面的 QML/渲染错误必须修掉（这类问题会直接影响用户）"
    exit 1
fi
echo "✅ 冒烟测试通过：没有 QML / 渲染层错误"
