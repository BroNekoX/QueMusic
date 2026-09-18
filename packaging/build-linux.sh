#!/usr/bin/env bash
# ============================================================
# QueMusic - Linux (AppImage) 一键打包脚本
# 适用: Arch / Nyarch / Debian / Ubuntu 等主流发行版
# 用法: bash packaging/build-linux.sh
# 产物: QueMusic-<version>-x86_64.AppImage
#
# 说明:
#   - 使用 aqtinstall 安装与 Windows 一致的 Qt 6.10.3,
#     避免与系统 Qt 版本不一致产生兼容性风险
#   - 通过 linuxdeploy + qt 插件打包自包含 AppImage
# ============================================================
set -e

QT_VERSION="6.10.3"
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
    echo "    Arch:  sudo pacman -S --needed base-devel cmake ninja wget python-pip git fuse2 libpulse"
    echo "    Debian: sudo apt install build-essential cmake ninja-build wget python3-pip git fuse libfuse2 libpulse-dev"
    exit 1
fi

# ---------- 1. 安装 Qt 6.10.3 (不存在时才装) ----------
if [ ! -d "${QT_DIR}/${QT_VERSION}/gcc_64" ]; then
    echo "==> [1/5] 未找到 Qt ${QT_VERSION}，正在通过 aqtinstall 安装..."
    pip install --user aqtinstall 2>/dev/null || pip install aqtinstall
    # -m 指定独立 addon 模块(空格分隔多个):
    #   qtmultimedia  -> 音频播放必需
    #   qtshadertools -> qt6_add_shaders 生成 QShader 必需
    #   qt5compat     -> QML 里用了 Qt5Compat.GraphicalEffects
    # 其余所需模块(Qml/Quick/Sql)随基础安装自带
    python3 -m aqt install-qt linux desktop "${QT_VERSION}" "${QT_ARCH}" \
        -O "${QT_DIR}" \
        -m qtmultimedia qtshadertools qt5compat
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
# 输入法插件目录（缺 fcitx5 的 Qt6 插件则无法输入中文）
mkdir -p "${APP_DIR}/usr/plugins/platforminputcontexts"
# 应用图标 (Linux 需要 png, 这里用 resources/icon.png，由 icon.ico 转换，与 Windows 图标一致)
cp resources/icon.png \
    "${APP_DIR}/usr/share/icons/hicolor/256x256/apps/quemusic.png"
cp packaging/QueMusic.desktop "${APP_DIR}/usr/share/applications/"

# ---------- 5. linuxdeploy 打包 ----------
echo "==> [5/5] 打包 AppImage..."
# 关键插件：platforms(xcb+Wayland) / xcbglintegrations(OpenGL) / egldeviceintegrations
# linuxdeploy-plugin-qt 按「依赖命中的模块」部署，平台类插件不保证被带上，这里显式拷贝。
QT_PLUGIN_SRC="${QT_DIR}/${QT_VERSION}/gcc_64/plugins"

mkdir -p "${APP_DIR}/usr/plugins/platforms"
if [ -f "${QT_PLUGIN_SRC}/platforms/libqxcb.so" ]; then
    cp -a "${QT_PLUGIN_SRC}/platforms/libqxcb.so" "${APP_DIR}/usr/plugins/platforms/"
else
    echo "!! 缺少 X11 平台插件 libqxcb.so"
fi
WAYLAND_COUNT=0
for p in "${QT_PLUGIN_SRC}/platforms/"libqwayland*.so; do
    [ -f "$p" ] || continue
    cp -a "$p" "${APP_DIR}/usr/plugins/platforms/"
    WAYLAND_COUNT=$((WAYLAND_COUNT + 1))
done
if [ "${WAYLAND_COUNT}" -gt 0 ]; then
    echo "    附带 Wayland 平台插件: ${WAYLAND_COUNT} 个"
else
    echo "!! 未找到 Wayland 平台插件，产物将只支持 X11（Qt 安装缺少 qtwayland？）"
fi

# X11/XWayland 下创建 OpenGL 上下文的关键：缺了它会出现
# 「QRhi 无法加载 OpenGL / 无法创建 OpenGL 上下文」并闪退（Vulkan 后端不受影响）
if [ -d "${QT_PLUGIN_SRC}/xcbglintegrations" ]; then
    mkdir -p "${APP_DIR}/usr/plugins/xcbglintegrations"
    cp -a "${QT_PLUGIN_SRC}/xcbglintegrations"/. "${APP_DIR}/usr/plugins/xcbglintegrations/"
    echo "    附带 xcbglintegrations（OpenGL 集成）"
else
    echo "!! 未找到 xcbglintegrations，X11 下 OpenGL 可能无法使用"
fi

# EGL 设备/GBM 集成（部分驱动组合需要）
if [ -d "${QT_PLUGIN_SRC}/egldeviceintegrations" ]; then
    mkdir -p "${APP_DIR}/usr/plugins/egldeviceintegrations"
    cp -a "${QT_PLUGIN_SRC}/egldeviceintegrations"/. "${APP_DIR}/usr/plugins/egldeviceintegrations/"
fi

# fcitx5 的 Qt6 输入法插件
# 系统包里的插件（Ubuntu: fcitx5-frontend-qt6 / Arch: fcitx5-qt6）是针对「系统 Qt」编译的，
# 与本项目 aqt 的 Qt 6.10.3 存在 ABI 风险。想严格匹配就先自行编译（与 CI 做法一致）：
#   git clone --depth 1 https://github.com/fcitx/fcitx5-qt.git
#   cmake -S fcitx5-qt -B fcitx5-qt/build -DCMAKE_BUILD_TYPE=Release \
#         -DCMAKE_PREFIX_PATH="${QT_DIR}/${QT_VERSION}/gcc_64" \
#         -DENABLE_QT5=OFF -DBUILD_ONLY_PLUGIN=ON
#   cmake --build fcitx5-qt/build -j"$(nproc)"
FOUND_FCITX="$(find fcitx5-qt/build -name 'libfcitx5platforminputcontextplugin*.so' \
                -type f 2>/dev/null | head -n1 || true)"
if [ -z "${FOUND_FCITX}" ]; then
    for dir in /usr/lib/qt6/plugins/platforminputcontexts \
               /usr/lib/*/qt6/plugins/platforminputcontexts; do
        for f in "${dir}"/*fcitx5*.so; do
            if [ -f "$f" ]; then
                FOUND_FCITX="$f"
                break 2
            fi
        done
    done
fi
if [ -n "${FOUND_FCITX}" ]; then
    cp "${FOUND_FCITX}" "${APP_DIR}/usr/plugins/platforminputcontexts/"
    echo "    附带输入法插件: ${FOUND_FCITX}"
else
    echo "!! 未找到 fcitx5 的 Qt6 输入法插件，产物将无法输入中文"
    echo "   Ubuntu: sudo apt install fcitx5-frontend-qt6 / Arch: sudo pacman -S fcitx5-qt"
fi

# QML 模块: 扫描源码 + 附带 Multimedia/Sql/ShaderTools 插件
export QML_SOURCES_PATHS="$(pwd)"
export EXTRA_QT_MODULES="multimedia;sql;shadertools"
export QT_QPA_PLATFORM=minimal   # 打包阶段无需显示

# 删除无依赖的 Mimer SQL 驱动（libmimerapi.so 通常不存在，且本项目用不到）
rm -f "${QT_DIR}/${QT_VERSION}/gcc_64/plugins/sqldrivers/libqsqlmimer.so"

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