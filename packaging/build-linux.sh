#!/usr/bin/env bash
# ============================================================
# QueMusic - Linux (AppImage) 一键打包脚本
# 适用: Arch / Nyarch / Debian / Ubuntu 等主流发行版
# 用法: bash packaging/build-linux.sh
# 产物: QueMusic-<version>-x86_64.AppImage
#
# 说明:
#   - 使用 aqtinstall 安装与 Windows 一致的 Qt 6.9.3,
#     避免与系统 Qt 6.11 产生兼容性风险
#   - 通过 linuxdeploy + qt 插件打包自包含 AppImage
# ============================================================
set -e

QT_VERSION="6.9.3"
QT_ARCH="linux_gcc_64"
QT_DIR="${HOME}/Qt"
BUILD_DIR="build-linux"
APP_DIR="AppDir"
OUTPUT_APPIMAGE="QueMusic-x86_64.AppImage"

echo "============================================="
echo " QueMusic Linux AppImage 打包"
echo " Qt: ${QT_VERSION}  Arch: ${QT_ARCH}"
echo "============================================="

# ---------- 0. 检查系统依赖 ----------
echo "==> [0/5] 检查系统依赖..."
MISSING=""
for c in cmake ninja wget python3 pip pip3 git; do
    command -v "$c" >/dev/null 2>&1 || MISSING="$MISSING $c"
done
if [ -n "$MISSING" ]; then
    echo "!! 缺少依赖:${MISSING}"
    echo "    Arch:  sudo pacman -S --needed base-devel cmake ninja wget python-pip git fuse2"
    echo "    Debian: sudo apt install build-essential cmake ninja-build wget python3-pip git fuse libfuse2"
    exit 1
fi

# ---------- 1. 安装 Qt 6.9.3 (不存在时才装) ----------
if [ ! -d "${QT_DIR}/${QT_VERSION}/gcc_64" ]; then
    echo "==> [1/5] 未找到 Qt ${QT_VERSION}，正在通过 aqtinstall 安装..."
    pip install --user aqtinstall 2>/dev/null || pip install aqtinstall
    python3 -m aqt install-qt linux desktop "${QT_VERSION}" "${QT_ARCH}" -O "${QT_DIR}"
else
    echo "==> [1/5] 检测到 Qt ${QT_VERSION}，跳过安装"
fi
export QMAKE="${QT_DIR}/${QT_VERSION}/gcc_64/bin/qmake"

# ---------- 2. 下载 linuxdeploy 工具 ----------
echo "==> [2/5] 准备 linuxdeploy 工具..."
mkdir -p packaging/tools
cd packaging/tools
[ -f linuxdeploy-x86_64.AppImage ] || \
    wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
[ -f linuxdeploy-plugin-qt-x86_64.AppImage ] || \
    wget -q https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage linuxdeploy-plugin-qt-x86_64.AppImage
cd ../..

# ---------- 3. CMake 配置与构建 ----------
echo "==> [3/5] 配置并构建 (Qt ${QT_VERSION})..."
cmake -B "${BUILD_DIR}" \
    -DCMAKE_PREFIX_PATH="${QT_DIR}/${QT_VERSION}/gcc_64" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build "${BUILD_DIR}" -j"$(nproc)"

# ---------- 4. 准备 AppDir ----------
echo "==> [4/5] 准备 AppDir 目录..."
rm -rf "${APP_DIR}"
mkdir -p "${APP_DIR}/usr/share/applications"
mkdir -p "${APP_DIR}/usr/share/icons/hicolor/256x256/apps"
# 应用图标 (Linux 需要 png, 这里用 rainbowMusicIcon.png)
cp resources/app/rainbowMusicIcon.png \
    "${APP_DIR}/usr/share/icons/hicolor/256x256/apps/quemusic.png"
cp packaging/QueMusic.desktop "${APP_DIR}/usr/share/applications/"

# ---------- 5. linuxdeploy 打包 ----------
echo "==> [5/5] 打包 AppImage..."
# QML 模块: 扫描源码 + 附带 Multimedia/Sql/ShaderTools 插件
export QML_SOURCES_PATHS="$(pwd)"
export EXTRA_QT_MODULES="multimedia;sql;shadertools"
export QT_QPA_PLATFORM=minimal   # 打包阶段无需显示

./packaging/tools/linuxdeploy-x86_64.AppImage \
    --appdir "${APP_DIR}" \
    --executable "${BUILD_DIR}/bin/QueMusic" \
    --desktop-file "${APP_DIR}/usr/share/applications/QueMusic.desktop" \
    --icon-file "${APP_DIR}/usr/share/icons/hicolor/256x256/apps/quemusic.png" \
    --plugin qt \
    --output appimage \
    || { echo "!! AppImage 打包失败，尝试兼容模式..."; \
         ./packaging/tools/linuxdeploy-x86_64.AppImage --appimage-extract-and-run \
         --appdir "${APP_DIR}" \
         --executable "${BUILD_DIR}/bin/QueMusic" \
         --desktop-file "${APP_DIR}/usr/share/applications/QueMusic.desktop" \
         --icon-file "${APP_DIR}/usr/share/icons/hicolor/256x256/apps/quemusic.png" \
         --plugin qt --output appimage; }

echo "============================================="
echo " ✅ 打包完成: ${OUTPUT_APPIMAGE}"
echo "    试运行:  chmod +x ${OUTPUT_APPIMAGE} && ./${OUTPUT_APPIMAGE}"
echo "============================================="
