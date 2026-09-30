#!/usr/bin/env bash
# ============================================================
# QueMusic - Linux tar 便携包 一键打包（本地用）
#
# 与 CI 的 .github/workflows/build-linux-tar.yml 逻辑一致：
#   1) 用 Qt 6.10.3（aqt，自带 FFmpeg 7.1.3）配置并构建全部目标
#   2) 把 build/bin 与 build/_deps 抠出来，加上 run.sh 与使用注意事项.txt
#   3) 把构建机绝对 RPATH 规整成 $ORIGIN，打成 tar.xz
#
# 用法:  bash packaging/build-linux-tar.sh
# 产物:  QueMusic-linux-<架构>.tar.xz
#        解压后进目录 ./run.sh 即可运行（目标机器需 Qt 6.10+）
# ============================================================
set -euo pipefail

QT_VERSION="${QT_VERSION:-6.10.3}"
QT_DIR="${QT_DIR:-${HOME}/Qt}"
QT_PREFIX="${QT_DIR}/${QT_VERSION}/gcc_64"
BUILD_DIR="${BUILD_DIR:-build}"
ARCH="$(uname -m)"
NAME="QueMusic-linux-${ARCH}"
PACKAGE="${NAME}.tar.xz"

echo "============================================="
echo " QueMusic Linux tar 便携包"
echo " Qt: ${QT_VERSION}  架构: ${ARCH}"
echo "============================================="

# ---------- 0. 环境检查 ----------
MISSING=""
for c in cmake patchelf tar file; do
    command -v "$c" >/dev/null 2>&1 || MISSING="$MISSING $c"
done
if [ -n "$MISSING" ]; then
    echo "!! 缺少命令:${MISSING}"
    echo "   Arch:  sudo pacman -S --needed cmake ninja patchelf tar file"
    echo "   Debian: sudo apt install cmake ninja-build patchelf tar file"
    exit 1
fi
[ -e "${QT_PREFIX}/lib/libavcodec.so.61" ] || {
    echo "!! 没在 ${QT_PREFIX} 找到 Qt 自带的 FFmpeg（libavcodec.so.61）"
    echo "   这个包要求 Qt 6.10+ 且带 FFmpeg 7.1.3；用 aqt 装一个："
    echo "     python3 -m aqt install-qt linux desktop ${QT_VERSION} linux_gcc_64 \\"
    echo "       -O ${QT_DIR} -m qtmultimedia qtshadertools qt5compat"
    exit 1
}

# ---------- 1. 配置 + 构建 ----------
echo "==> [1/4] 构建（全部目标）..."
# -DQUEMUSIC_FFMPEG_USE_QT=ON 让产物绑到 Qt 自带的 FFmpeg（libavcodec.so.61 = 7.1.3），
# 与 CI 的 build-linux-tar.yml 以及「使用注意事项.txt」里写的要求一致；
# 不加这个开关时会退回系统 ffmpeg（要求就变成系统那个版本了）。
cmake -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="${QT_PREFIX}" \
    -DQUEMUSIC_FFMPEG_USE_QT=ON
cmake --build "${BUILD_DIR}" -j"$(nproc)"

# ---------- 2. 组装 ----------
echo "==> [2/4] 组装 ${NAME}/ ..."
rm -rf "${NAME}"
mkdir -p "${NAME}"
cp -a "${BUILD_DIR}/bin"   "${NAME}/bin"
cp -a "${BUILD_DIR}/_deps" "${NAME}/_deps"

# 剔掉纯编译中间产物（运行期用不到）
find "${NAME}/_deps" -type d -name CMakeFiles -prune -exec rm -rf {} + 2>/dev/null || true
find "${NAME}" -type f \( -name '*.o' -o -name '*.obj' -o -name '*.a' \) -delete 2>/dev/null || true

cp packaging/run.sh "${NAME}/run.sh"
chmod +x "${NAME}/run.sh"
cp packaging/使用注意事项.txt "${NAME}/使用注意事项.txt"

# ---------- 3. 清掉构建机绝对路径 ----------
echo "==> [3/4] 规整 RPATH → \$ORIGIN ..."
PATCHED=0
while IFS= read -r f; do
    [ -f "$f" ] || continue
    if file -b "$f" 2>/dev/null | grep -q '^ELF'; then
        patchelf --set-rpath '$ORIGIN' "$f" 2>/dev/null && PATCHED=$((PATCHED + 1)) || true
    fi
done < <(find "${NAME}" -type f \( -name '*.so' -o -name '*.so.*' -o -name 'QueMusic' \) 2>/dev/null)
echo "    已处理 ${PATCHED} 个 ELF"

# ---------- 4. 打包 ----------
echo "==> [4/4] 打 tar.xz ..."
tar -cJf "${PACKAGE}" "${NAME}"
ls -lh "${PACKAGE}"
du -sh "${NAME}"

echo "============================================="
echo " ✅ 完成: ${PACKAGE}"
echo "    试运行: tar -xf ${PACKAGE} && cd ${NAME} && ./run.sh"
echo "============================================="
