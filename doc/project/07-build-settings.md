# 第 7 部分 · 构建、第三方依赖、打包、CI 与设置键声明

> 本文件是 [QueMusic 项目全景文档索引](README.md) 的一个分片；任务定位与其它部分请见 [README.md](README.md)。

> 范围：`CMakeLists.txt` / `CMakePresets.json` / `cmake/**` / `justfile` / `packaging/**` /
> `.github/workflows/**` / `tests/**` / `ThirdParty/**`（仅顶层）/ `doc/{updater.txt,qm-dbcheck.bat,macos-install.md}`，
> 以及全仓库（排除 `build/`、`ffmpeg/`、`ThirdParty/`、`api/QCloudMusicApi`）的持久化配置键。
> 未在源码中直接确认的项标「未确认」。

---

## 7.1 构建配置

### 7.1.1 CMake 选项与默认值

| 选项 | 默认 | 定义位置 | 作用 |
|---|---|---|---|
| `QUEMUSIC_ENABLE_LTO` | `ON` | 根 `CMakeLists.txt:12` | Release/RelWithDebInfo 启用 IPO（LTO） |
| `QUEMUSIC_QML_LINT` | `ON` | 根 `CMakeLists.txt:13` | OFF 时给 `qt6_add_qml_module()` 追加 `NO_LINT` |
| `QUEMUSIC_ENABLE_QMLTC` | `OFF` | 根 `CMakeLists.txt:14` | 实验性 qmltc 类型编译总开关（见 1.7） |
| `QUEMUSIC_FFMPEG_ROOT` | `${CMAKE_SOURCE_DIR}/ffmpeg`（CACHE PATH） | `cmake/external/ffmpeg.cmake:12` | FFmpeg 前缀（含 `include/`、`lib/`） |
| `QUEMUSIC_FFMPEG_USE_QT` | `OFF` | `cmake/external/ffmpeg.cmake:65` | Linux 改为优先链接 Qt 自带 FFmpeg；**macOS 恒为启用** |
| `QUEMUSIC_QWK_RUNTIME_COPY` | `ON`（`QWINDOWKIT_BUILD_STATIC=ON` 时置 `OFF`） | 根 `CMakeLists.txt:66-69` | 是否把 QWindowKit 动态库拷到 exe 旁 |
| `QCLOUDMUSICAPI_BUILD_TEST` | `OFF`（`FORCE`） | 根 `CMakeLists.txt:302` | 关掉 QCloudMusicApi 测试 |
| `QCLOUDMUSICAPI_BUILD_SHARED` | `OFF`（`FORCE`） | 根 `CMakeLists.txt:303` | 静态编入主程序 |
| `QWINDOWKIT_BUILD_QUICK / WIDGETS / EXAMPLES / TESTS / STATIC / ENABLE_WINDOWS_SYSTEM_BORDERS` | `ON / OFF / OFF / OFF / ON / OFF`（均 `FORCE`） | `cmake/external/qwindowkit.cmake:17-22` | 只用 Quick + 静态库 |
| `BUILD_BINDINGS` / `BUILD_TESTING`（TagLib） | `OFF` / `OFF`（`FORCE`） | `cmake/external/taglib.cmake:12-13` | 关 TagLib 绑定与测试 |
| `BUILD_SHARED_LIBS`（局部） | 进 TagLib 前临时置 `OFF` | `cmake/external/taglib.cmake:17-21` | TagLib 固定静态；QWindowKit 会改写此全局变量 |
| `ZLIB_ROOT` | 未设 | `cmake/external/zlib.cmake:23-27` | zlib 分级查找提示路径 |

`cmake/CMakeLists.txt` 是一个 `LANGUAGES NONE` 的辅助子工程（`project(cmakehelper)`），只做 `include(external/*.cmake)` + `include(qtruntime.cmake)`，本身不产出 target。

### 7.1.2 工具链要求

| 项 | 值 | 依据 |
|---|---|---|
| CMake 最低版本（根） | `3.16` | 根 `CMakeLists.txt:1`；`cmake/CMakeLists.txt:4` 亦为 3.16 |
| CMakePresets 版本 | `version 6` → 需 **CMake ≥ 3.25** | `CMakePresets.json:2`（README 写「CMake ≥ 3.24」，与文件不一致，按文件为准） |
| 生成器 | Ninja（所有预设固定） | `CMakePresets.json:7` |
| Qt | `find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Network Multimedia Concurrent Sql ShaderTools Widgets)`，**未写最低版本号**；实际开发/CI/打包统一 **6.10.3**，运行期文档要求 Qt 6.10+ | 根 `CMakeLists.txt:21`；各 workflow `QT_VERSION: '6.10.3'` |
| C++ 标准 | `20`（`CMAKE_CXX_STANDARD 20` + `STANDARD_REQUIRED ON`，预设再显式设一次） | 根 `CMakeLists.txt:8-9`、`CMakePresets.json:10` |
| 第三方子工程标准 | QCloudMusicApi `17`；QWindowKit 需 CMake ≥ 3.19；TagLib 需 3.10…3.31；QCloudMusicApi 需 3.14 | 各子模块 `CMakeLists.txt` |
| Qt 策略 | `qt_policy(SET QTP0001 OLD)`（若命令存在） | 根 `CMakeLists.txt:23-25` |
| AUTOMOC/AUTORCC | `CMAKE_AUTOMOC ON`、`CMAKE_AUTOMOC_PATH_PREFIX ON`、`CMAKE_AUTORCC ON` | 根 `CMakeLists.txt:5-7` |

### 7.1.3 输出目录、LTO、PCH、rpath

| 项 | 设置 |
|---|---|
| 可执行文件 / 动态库 | `${CMAKE_BINARY_DIR}/bin` |
| 静态库 | `${CMAKE_BINARY_DIR}/lib` |
| LTO/IPO | `include(CheckIPOSupported)` → `check_ipo_supported()`；成功则对 `QueMusic` 设 `INTERPROCEDURAL_OPTIMIZATION_RELEASE=TRUE`、`INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO=TRUE`；不支持时 `message(WARNING)` 跳过（Debug 不启用） |
| PCH | `target_precompile_headers()` 预编译 `<QtCore/QtCore>`、`<QtGui/QtGui>`、`<QtQml/QtQml>`、`<QtQuick/QtQuick>`、`<QtNetwork/QtNetwork>`、`<QtMultimedia/QtMultimedia>` |
| 链接期裁剪 | 非 MSVC / 非 Apple / 非 Debug：`-ffunction-sections -fdata-sections` + `-Wl,--gc-sections` |
| rpath | `CMAKE_BUILD_RPATH_USE_ORIGIN TRUE`（构建产物用 `$ORIGIN` 相对 RPATH）；Linux 便携包再由 `patchelf` 在打包阶段整体重写 |
| Windows 链接库 | `runtimeobject ole32 shell32 advapi32 crypt32`（SMTC + DPAPI） |

### 7.1.4 目标与平台属性

| 平台 | 属性 |
|---|---|
| 通用 | `qt_add_executable(QueMusic main.cpp $<$<PLATFORM_ID:Windows>:icon.rc>)`；`target_include_directories` 加入 `shaders/`、`cpp/`、`cpp/webdav`、`cpp/plugins`、`api/`、TagLib 的 5 个子目录 |
| Windows | `WIN32_EXECUTABLE TRUE` |
| Apple | `MACOSX_BUNDLE TRUE`、`MACOSX_BUNDLE_BUNDLE_NAME "QueMusic"`、`GUI_IDENTIFIER com.bronekox.quemusic`、`BUNDLE_VERSION`/`SHORT_VERSION_STRING = ${PROJECT_VERSION}`；`resources/app/icon.icns` 存在时额外设 `MACOSX_BUNDLE_ICON_FILE` 与 `RESOURCE`（CI 用 `sips`+`iconutil` 现场生成） |
| Linux/macOS | `QUEMUSIC_QWK_RUNTIME_COPY` 为真时 POST_BUILD 拷 `QWindowKit::Core`、`QWindowKit::Quick` 到 exe 目录 |
| 链接 | PRIVATE：`Qt6::{Core,Sql,Gui,Qml,Quick,Network,Multimedia,Concurrent,Widgets}`、`QCloudMusicApi`、`quemusic::taglib`、`quemusic::zlib`、`quemusic::ffmpeg`、`cryptopp::cryptopp`；PUBLIC：`QWindowKit::Quick` |
| QML 模块 | `qt6_add_qml_module(QueMusic URI QueMusic VERSION 1.0 ...)`，按需追加 `NO_LINT` / `ENABLE_TYPE_COMPILER` |

### 7.1.5 SOURCES / QML_FILES / RESOURCES 清单

**QML_FILES**（`QUE_MUSIC_QML_FILES`）
- 显式：`main.qml`、`SettingsView.qml`、`api/qrcode.js`
- `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` 收集：`components/*.qml`、`layout/*.qml`、`pages/*.qml`、`lyricsui/*.qml`（当前全仓 QML 共 **81** 个，`cmake/qmltc.cmake` 注释里的「78」已过期）

**SOURCES**（节选分组，均为 `shaders/`、`cpp/`、`api/` 下的 .cpp/.h）
| 分组 | 文件 |
|---|---|
| `shaders/` | `MeshGradientItem.cpp/.h`、`LyricsStage.cpp/.h` |
| `cpp/` 顶层 | `AppModels.h`、`MediaTypes.h`、`PlayerDatabase`、`DbService`、`CoverHelper`、`ColorExtractor`、`GetWave`、`FolderModel`、`DownloadManager`、`DownloadedMusicModel`、`Favorites`、`AccountManager`、`LocalLyricsReader`、`CustomFlow`、`QueueModel`、`LocalMusicScanner`、`SearchResultModel`、`WindowsSmtcManager`、`LogManager`、`SystemTrayManager`（各 .cpp+.h） |
| `cpp/webdav/` | `WebDavCache`、`WebDavClient`、`WebDavModel`、`WebDavStore` |
| `cpp/plugins/` | `PluginStore`、`LyricsPluginStore`、`FunctionPluginStore` |
| `cpp/audio/` | `AudioEngine`、`AudioDsp`、`FfmpegDecoder`、`PitchShifter`、`TimeStretch`（.cpp+.h）＋ 纯头 `AudioRing.h`、`AudioSpectrumSink.h` |
| `api/` | `MusicApiService`、`OnlineListModel`、`NeteaseCloudApi`、`KugouApi`、`BilibiliApi`（.cpp+.h）＋ 纯头 `ApiCommon.h` |

**RESOURCES**：`resources/window-bar/*.svg`（close/closed、airplay/airplayd、maximize/maximized、minimize/minimized、restore/restored、return/returnd、setting/settingd）、`resources/icon.ico`、`resources/loader.gif`、`resources/pic/{back1,back2,back3}.jpg`、`resources/pic/cloudRainbow.png`、`resources/app/{header.png,rainbowMusicIcon.png,musicpic.png,cd.png,cdpoint.png}`、`resources/app/icons/{bronekox.jpg,qticon.png,qwk.png}`、`resources/fonts/{feather.ttf,poppins.ttf}`

**QML 单例声明**（`set_source_files_properties(... QT_QML_SINGLETON_TYPE TRUE)`，共 10 个）：
`components/{Style,Theme,Config,ShortCut,Options,Playback,PluginSlots,Fonts}.qml`、`lyricsui/{LyricsFreeConfig,Lyrics3DConfig}.qml`

### 7.1.6 着色器编译（`qt6_add_shaders`，两次调用，均 `BATCHABLE`）

| 目标名 | PREFIX | 文件 |
|---|---|---|
| `effectshaders` | `/shaders` | `lyricfade.frag`、`cardblur.frag`、`cardblur_hq.frag`、`bg_cubes.frag`、`bg_grid.frag`、`bg_beams.frag`、`bg_stars.frag`、`bg_stage.frag`、`bg_void.frag`、`lyricplane.frag` |
| `bgshaders` | `/` | `background.vert`、`fluid.frag`、`classic.frag`、`stage.vert`、`stage.frag` |

QML 侧引用前缀必须区分：`/shaders/...` 与 `/...`（两套 PREFIX 不同，不能混用）。

### 7.1.7 qmltc 机制与跳过清单逻辑（`cmake/qmltc.cmake`，默认关闭）

1. `QUEMUSIC_ENABLE_QMLTC=OFF` → 直接 `return()`，`QUE_MUSIC_QMLTC_ACTIVE=FALSE`。
2. 打开时先检查白名单 `QUE_MUSIC_QMLTC_ALLOWED`（19 项）。若其中任何文件位于模块**子目录**（`get_filename_component(... DIRECTORY)` 非空），打印 `WARNING` 并 `return()`（保持关闭，**不会让构建失败**）。
3. 当前白名单里有 **4 个文件已不存在**：`components/StyleThemes.qml`、`StyleSettings.qml`、`OptionsSettings.qml`、`OptionsShortCuts.qml`；其余 15 个全部位于 `components/` 子目录 → 因此即使 `QUEMUSIC_ENABLE_QMLTC=ON` 也必然停在降级分支。
4. 只有白名单**全部位于模块根目录**时：`QUE_MUSIC_QMLTC_ACTIVE=TRUE`、`find_package(Qt6 REQUIRED COMPONENTS QmlPrivate QuickPrivate)` 并链接 `Qt6::QmlPrivate`/`Qt6::QuickPrivate`；随后遍历 `QUE_MUSIC_QML_FILES`，对 `.qml` 中不在白名单内的文件设 `QT_QML_SKIP_TYPE_COMPILER TRUE`（跳过清单），最后打印启用信息。
5. 文件顶部记录的原因：`qmltc` 无法编译引用了其它 QML 模块（QtQuick.Controls.*、QtQuick.Dialogs、Qt5Compat.GraphicalEffects）的文档；且被引用文件必须构成闭包；且只能编译模块根目录下的文档。

### 7.1.8 `qt_deploy_runtime()` 与部署裁剪（`cmake/qtruntime.cmake`）

- 调用点：根 `CMakeLists.txt:358-368`，条件 `NOT APPLE AND NOT CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$"` → **macOS 分支在该函数内实际是死代码**（macOS 由 `build-macos.yml` 自己调 `macdeployqt`）。
- Windows：`windeployqt --qmldir ${CMAKE_CURRENT_SOURCE_DIR} --no-opengl-sw <exe>`；找不到就 `WARNING` 跳过。
- Windows 裁剪（POST_BUILD `cmake -E rm -f`）：`opengl32sw.dll`（旧部署残留）、`sqldrivers/` 下 `qsqlmysql/qsqloci/qsqlodbc/qsqlpsql/qsqlmimer/qsqlibase.dll` 与 `libmysql.dll`、`libssl-3-x64.dll`、`libcrypto-3-x64.dll`（只保留 `qsqlite`）。
- 根 `CMakeLists.txt` 追加的第二段裁剪：删除 `multimedia/ffmpegmediaplugin.dll` 与 `swscale-8.dll`（全项目不用 `MediaPlayer`，约省 1.7 MB；解码层用的 `avcodec/avformat/avutil/swresample` 保留）。
- Linux：仅当 PATH 里有 `linuxdeployqt` 才调用（CI 与打包脚本都不用它，改用 linuxdeploy AppImage）。

---

## 7.2 子模块与第三方

`ThirdParty/` 顶层只有两个目录：`qwindowkit/`、`taglib/`（均为 git submodule）。`.gitmodules` 共 3 条。

| 依赖 | 引入方式 | 版本 | 用途 | 许可证 |
|---|---|---|---|---|
| QWindowKit | git submodule `ThirdParty/qwindowkit`（`shallow`，stdware/qwindowkit）；优先 `find_package(QWindowKit QUIET)`，未装才用子模块 | 1.5.1.0（子模块 `CMakeLists.txt` `project(VERSION)`） | 无边框窗口/系统标题栏，导出 `QWindowKit::Quick` | 见其仓库 LICENSE（未在本仓库复述） |
| TagLib | git submodule `ThirdParty/taglib`（`shallow`，taglib/taglib）＋ `add_subdirectory`（构建目录 `build/_deps/taglib-build`，`EXCLUDE_FROM_ALL`） | 2.3.1（`taglib/toolkit/taglib.h`；`THIRD_PARTY_NOTICES.md` 同） | 本地音频元数据 / ID3v2 歌词读取；导出 `quemusic::taglib`（接口库 + `tag`） | LGPL-2.1-or-later / MPL-1.1 双许可 |
| zlib | 系统包（`find_package(ZLIB QUIET)`）→ 回退 `find_library(z/zlib/zlib1)`（含 `-DZLIB_ROOT`、编译器 sysroot、Qt Tools 下 MinGW 的 `libz.a`）；Windows 用 `WIN32` 判断而非 `MINGW` | 随系统/工具链（未固定版本） | `api/KugouApi.cpp` 解 KRC 歌词 `uncompress()`；导出 `quemusic::zlib` | zlib license |
| Crypto++ | `api/QCloudMusicApi/3rdparty/cryptopp`（submodule，weidai11/cryptopp）＋ `cryptopp-cmake`（随源码目录）`add_subdirectory(3rdparty/cryptopp-cmake/)`；`CRYPTOPP_SOURCES` 指向本地目录以避免联网 | 8.9（`3rdparty/cryptopp/config_ver.h`：`CRYPTOPP_MAJOR 8` / `CRYPTOPP_MINOR 9`） | 网易云 API 的加密（weapi/eapi 等）；导出 `cryptopp::cryptopp` | Crypto++ 自身许可（Boost-like，见其 License.txt） |
| libqrencode | **随仓库源码存放但未接入构建**：`api/QCloudMusicApi/3rdparty/libqrencode`，无 `add_subdirectory`、无链接；仓库内仅在自身目录被引用 | 4.1.1（其 `CMakeLists.txt` `project(QRencode VERSION 4.1.1)`） | 无（当前未使用；二维码由 `api/qrcode.js` 实现） | LGPL-2.1（其 `COPYING`） |
| QCloudMusicApi | `add_subdirectory(api/QCloudMusicApi)`（源码随仓库，非 submodule），强制 `BUILD_TEST/SHARED=OFF`；`set(PREFIX "")` | 工程 `VERSION 1.0`（无上游版本号，未确认） | 网易云音乐在线接口；导出 `QCloudMusicApi` | MIT（`api/QCloudMusicApi/LICENSE`，© 2023 s12mmm3） |
| FFmpeg | Windows：仓库自带 `ffmpeg/{include,lib,bin}`（`find_library ... NO_DEFAULT_PATH`，缺任一导入库即 `FATAL_ERROR`）；Linux/macOS：`QUEMUSIC_FFMPEG_USE_QT`/macOS 默认用 Qt 自带 `libav*`，否则 `pkg_check_modules` 找系统包 | 7.1.3（libavcodec 61 / libavformat 61 / libavutil 59 / libswresample 5；`include/libavutil/version.h`：`LIBAVUTIL_VERSION_MAJOR 59`） | 音频解码/解封装/重采样（`cpp/audio/FfmpegDecoder`） | LGPL-2.1-or-later（Qt 6.10.3 自带分发；构建参数不含 `--enable-gpl/--enable-nonfree`，见 `THIRD_PARTY_NOTICES.md`） |
| Qt 6.10.3 | 系统/官方安装包（aqtinstall）；`find_package(Qt6 ...)` | 6.10.3 | 界面、音频输出、网络、SQL、着色器工具 | LGPLv3（动态链接） |
| 图标字体 `feather.ttf` / `poppins.ttf` | 仓库资源 | 未固定 | 图标与正文字体 | 归原作者（见 `THIRD_PARTY_NOTICES.md`） |

---

## 7.3 构建与运行命令

### 7.3.1 CMakePresets 预设（`CMakePresets.json`）

公共父预设 `base`（hidden）：Ninja、`binaryDir = ${sourceDir}/build/${presetName}`、`CMAKE_CXX_STANDARD=20`、`CMAKE_EXPORT_COMPILE_COMMANDS=ON`。

| configure 预设 | 条件 | 编译器 | 构建类型 | 其它 cache / 环境 |
|---|---|---|---|---|
| `mac-clang-release` | host = Darwin | `clang++` | `Release` | `CMAKE_OSX_DEPLOYMENT_TARGET=11.0` |
| `mac-clang-beta` | host = Darwin | `clang++` | `RelWithDebInfo` | — |
| `linux-gcc-release` | host = Linux | `g++` | `Release` | — |
| `win-mingw-release` | host = Windows | `g++` | `Release` | 需 Qt `mingw_64` 套件 |
| `win-llvm-mingw-release` | host = Windows | `clang` / `clang++` | `Release` | `CMAKE_PREFIX_PATH=C:/Qt/6.10.3/llvm-mingw_64`；`environment.PATH` 预置 `C:/Qt/Tools/llvm-mingw1706_64/bin` 与 `C:/Qt/Tools/Ninja` |

| build 预设 | configurePreset |
|---|---|
| `mac-release` / `mac-beta` | `mac-clang-release` / `mac-clang-beta` |
| `linux-release` | `linux-gcc-release` |
| `win-release` | `win-mingw-release` |
| `win-llvm-release` | `win-llvm-mingw-release` |

### 7.3.2 just 目标（`justfile`）

`os := \`uname -s\``；预设自动选择：Darwin→`mac-clang-release`，Linux→`linux-gcc-release`，其它→`win-llvm-mingw-release`。

| 目标 | 命令 |
|---|---|
| `just setup` | `git submodule update --init --recursive` |
| `just ffmpeg` | `powershell -ExecutionPolicy Bypass -File cmake/fetch_ffmpeg.ps1` |
| `just b`（默认） | `cmake --preset {{preset}}` + `cmake --build build/{{preset}} -j 8` |
| `just r` | 依赖 `b`，随后执行 `./build/{{preset}}/bin/QueMusic.exe`（路径带 `.exe`，在 Linux/macOS 上该命令不成立） |
| `just mac-bundle` | 依赖 `b`，`macdeployqt ./build/{{preset}}/QueMusic.app -dmg`（实际 bundle 落在 `build/bin/QueMusic.app`，路径可能失效，未确认） |
| `just clean` | `rm -rf build/` |

### 7.3.3 手动 CMake 调用（各平台实测写法）

```bash
# Windows（MinGW，Qt mingw_64）
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.10.3/mingw_64
cmake --build build --parallel

# Windows（LLVM-MinGW，Qt 6.10.3 llvm-mingw_64）
cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.10.3/llvm-mingw_64
cmake --build build --parallel

# Linux（AppImage 流程：用系统 libav*-dev，不开 USE_QT）
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=~/Qt/6.10.3/gcc_64
cmake --build build -j"$(nproc)"

# Linux（tar 便携包流程：绑到 Qt 自带 FFmpeg 7.1.3）
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=~/Qt/6.10.3/gcc_64 \
  -DQUEMUSIC_FFMPEG_USE_QT=ON
cmake --build build -j"$(nproc)"

# macOS
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=~/Qt/6.10.3/clang_64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build -j 8
```

CI（`build-linux.yml`）另传 `-DQT_DIR`、`-DQt6_DIR`、`-DQt6Multimedia_DIR`（macOS 流程用）。
构建产物统一在 `build/<preset 或 build>/bin/QueMusic[.exe]`，运行：`build/bin/QueMusic`（Windows `build\bin\QueMusic.exe`）。

### 7.3.4 FFmpeg 获取（`cmake/fetch_ffmpeg.ps1`，仅维护用）

- 正常构建**不需要联网**：`ffmpeg/{include,lib,bin,LICENSE.txt}` 随仓库提供。
- 脚本本身不下载：① 从本机 Qt 安装目录拷 `avcodec-61/avformat-61/avutil-59/swresample-5.dll` 到 `ffmpeg/bin`；② 用 `gendef.exe` + `llvm-dlltool.exe` 生成 `ffmpeg/lib/lib{avcodec,avformat,avutil,swresample}.dll.a`；③ 传 `-SourceDir <解压的 ffmpeg-7.1.3 源码>` 时才刷新 `include/`，并补 `avconfig.h`、`ffversion.h`、`LICENSE.txt`。
- 参数：`-QtBin`、`-Toolchain`（llvm-mingw 的 bin）、`-SourceDir`；不传时自动在 `C:\Qt`（深度 4）/`C:\Qt\Tools`（深度 3）里搜。
- 用法：`powershell -ExecutionPolicy Bypass -File cmake/fetch_ffmpeg.ps1`

### 7.3.5 各平台系统依赖安装

```bash
# Arch（AppImage 打包脚本 / PKGBUILD）
sudo pacman -S --needed base-devel cmake ninja wget python-pip git fuse2 libpulse ffmpeg pkgconf
# Debian/Ubuntu（CI ci.yml 的完整列表）
sudo apt install -y libxcb1-dev libxcb-icccm4-dev libxcb-keysyms1-dev libxcb-image0-dev \
  libxcb-shm0-dev libxcb-util-dev libxcb-cursor0 libxcb-cursor-dev libxcb-randr0-dev \
  libxcb-shape0-dev libxcb-xinerama0-dev libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev \
  libgl1-mesa-dev libegl1-mesa-dev libpulse-dev libx11-dev extra-cmake-modules ninja-build wget file \
  libavcodec-dev libavformat-dev libavutil-dev libswresample-dev
# Linux tar 包流程额外：patchelf cmake file；不打 AppImage，故不装 libfuse2、不装 libav*-dev
# AppImage 流程额外：libfuse2 libwayland-dev libwayland-bin wayland-protocols libvulkan1 libvulkan-dev \
#   libfcitx5core-dev libfcitx5config-dev libfcitx5utils-dev
# macOS：官方 Qt 包自带 libav*.dylib；自制 Qt 才需 brew install ffmpeg pkgconf
python3 -m aqt install-qt linux desktop 6.10.3 linux_gcc_64 -O ~/Qt -m qtmultimedia qtshadertools qt5compat
```

---

## 7.4 打包

| 平台 | 脚本/方式 | 产物 | 关键步骤 | 对应 CI |
|---|---|---|---|---|
| Windows | **仓库内无打包脚本**；仅 `build-windows.yml` 上传裸 exe | `build/bin/QueMusic.exe`（artifact `QueMusic-windows-build`） | configure → build → upload；无 windeployqt 收集、无安装器 | `.github/workflows/build-windows.yml` |
| Windows 安装包 | 文档口径为「windeployqt + Inno Setup（可选）」，`packaging/README.md` 提到 `QueMusic-setup.exe` | `QueMusic-setup.exe` | **仓库内无 `.iss` 脚本**，需本地手工做（未确认） | 无 |
| Linux x86_64 | `packaging/build-linux.sh` | `QueMusic-x86_64.AppImage` | 依赖检查 → aqt 装 Qt 6.10.3（`qtmultimedia qtshadertools qt5compat`）→ 下 linuxdeploy + qt 插件到 `packaging/tools/` → `cmake -B build-linux` 构建 → 组 `AppDir/usr/{bin,lib,plugins,share}`（显式拷 xcb/Wayland 平台插件、xcbglintegrations、egldeviceintegrations、fcitx5 输入法插件）→ `linuxdeploy --appdir --executable build/bin/QueMusic --plugin qt --output appimage`（失败回退 `--appimage-extract-and-run`） | `build-linux.yml`（更详细：裁剪 Qt 插件、依赖补扫、校验门禁） |
| Linux arm64 | `packaging/build-linux-arm.sh` | `QueMusic-aarch64.AppImage` | 与 x86 脚本一一对应：`linux_gcc_arm64`、`linuxdeploy-aarch64.AppImage`；先做架构自检（必须 aarch64 本机运行） | `build-linux-arm.yml`（`ubuntu-24.04-arm`） |
| Linux 便携 tar | `packaging/build-linux-tar.sh` | `QueMusic-linux-<arch>.tar.xz`（内含 `bin/`、`_deps/`、`run.sh`、`使用注意事项.txt`） | `-DQUEMUSIC_FFMPEG_USE_QT=ON` 构建 → 拷 `build/bin`+`build/_deps` → 剔除 `CMakeFiles`、`*.o/*.obj/*.a` → 加 `run.sh`/说明 → `patchelf --set-rpath '$ORIGIN'` 逐个规整 → `tar -cJf` | `build-linux-tar.yml` |
| Linux Arch 原生包 | `packaging/PKGBUILD` | `quemusic` 包（`/usr/bin/QueMusic` 等） | `pkgname=quemusic`、`pkgver=0.1.0`、`arch=x86_64`；`prepare()` 拉子模块；`build()` 用**系统 Qt（6.11+）**；`package()` 装 binary + `QueMusic.desktop` + `resources/app/rainbowMusicIcon.png` → `/usr/share/pixmaps/quemusic.png` + LICENSE | 无 |
| macOS | `build-macos.yml`（云端，无需 Mac） | `QueMusic-macOS-{arm64,x86_64}.dmg` | 生成 `icon.icns` → 构建 → 只留 sqlite 驱动 → `macdeployqt <app> -qmldir=$PWD -always-overwrite` → 校验（4 个 `libav*` 在 `Contents/Frameworks`、无 `/opt/homebrew|/usr/local|/Users` 绝对路径、QML 模块目录存在）→ 由内向外 `codesign`（有证书时 `--options runtime --entitlements packaging/entitlements-macos.plist`，否则 ad-hoc `-`）→ `codesign --verify --deep --strict` → `hdiutil create -format UDZO` → 有证书时签 DMG + `notarytool submit --wait` + `stapler staple` | `build-macos.yml` |
| 一键发布 | `release.yml` | 把 4 份产物挂到一个 Release | 复用 `build-linux.yml`/`build-linux-arm.yml`/`build-macos.yml`/`build-linux-tar.yml`（`workflow_call`），`publish` job 下载全部 artifacts，`gh release create/upload`，并在 Step Summary 生成国内加速链接 | `release.yml` |

其它打包文件：
- `packaging/QueMusic.desktop`：`Exec=QueMusic`、`Icon=quemusic`、`Categories=AudioVideo;Audio;Player;Qt;`、`StartupWMClass=QueMusic`。
- `packaging/entitlements-macos.plist`：`allow-jit`、`allow-unsigned-executable-memory`、`disable-library-validation`（公证 + hardened runtime 下 QML V4 JIT 必需）。
- `packaging/run.sh`：`cd` 到包目录 → 依次用 `QT_LIBDIR` / `qmake6` / `qtpaths6` / `~/Qt/6.*/gcc_64/lib` 找 Qt 库目录 → 组 `LD_LIBRARY_PATH`（Qt 在前、包内 `bin/` 在后）→ `exec ./bin/QueMusic "$@"`。
- `packaging/使用注意事项.txt`：Linux 便携包用户说明（要求 Qt 6.10+，包内自带 FFmpeg 7.1.3；`run.sh` 用法；`QSG_RHI_BACKEND`/`QT_QPA_PLATFORM`/`QT_LIBDIR` 排障变量；常见报错对照）。
- `packaging/README.md`：三平台打包方式一览、macOS 云端出包步骤、AppImage 试运行与 FUSE 依赖、Arch 包注意（系统 Qt 编译，兼容问题优先用 AppImage）、发布前必测项、目录结构。

---

## 7.5 CI（`.github/workflows/`）

| 工作流 | 触发 | Runner / 矩阵 | 关键步骤 | 验证内容 |
|---|---|---|---|---|
| `ci.yml`「CI（PR 门禁）」 | `push`/`pull_request` → `main`；`workflow_dispatch`；`concurrency: cancel-in-progress` | `ubuntu-24.04`，单 job（`timeout-minutes: 45`） | 拉子模块 → apt 依赖（含 `libav*-dev`）→ aqt 装 Qt 6.10.3（`qtmultimedia qtshadertools qt5compat`）→ `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`（**不带** `QUEMUSIC_FFMPEG_USE_QT`，走 pkg-config 系统 FFmpeg）→ 构建 → `qmllint -I build` 扫全部 QML（排除 `build/`、`ThirdParty/`、`api/QCloudMusicApi/`），拦 `Error:` 与「自研类型（`*Store|Model|Service|Manager|Scanner|Engine|Extractor|Reader|Writer|Helper|Database|Cache`）上的 `missing-property`」→ `tests/smoke-stderr.sh build/bin/QueMusic 12` | **真构建 + QML 静态检查 + 启动冒烟**（唯一同时做三者且无发布行为的门禁） |
| `build-windows.yml` | 仅 `workflow_dispatch` | `windows-latest` | aqt 装 `win64_mingw` + `tools_mingw1310` + `tools_ninja` → `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` → build → `upload-artifact` | **真构建**；只上传裸 `QueMusic.exe`，无冒烟、无打包 |
| `build-linux.yml` | `workflow_dispatch`（`release_tag` 输入）、`push` tags `v*`、`workflow_call` | `ubuntu-24.04` | 依赖 → Qt 6.10.3 → **Qt 能力 sanity**（Wayland QPA/`xdg-shell`/wayland-egl 集成/`libQt6WaylandClient`、`libQt6OpenGL`、GLX 集成、QtGui 内 Vulkan 字符串）→ `cmake ... -DQUEMUSIC_FFMPEG_USE_QT=ON` → build → 自编 fcitx5-qt 插件 → 下 linuxdeploy(x86_64) + qt 插件 → **裁剪 Qt 安装**（sqldrivers 只留 sqlite、删 `libgstreamermediaplugin.so`、删非 Basic 的 Controls 风格与 `libQt6QuickControls2*` 库）→ 手工组 `AppDir`（xcb+全部 wayland 平台插件、`wayland-{shell,decoration,graphics}-*`、xcbglintegrations、egldeviceintegrations、fcitx5/ibus/compose 输入法、Wayland 客户端库、`qt.conf`）→ 依赖补扫（白名单：wayland-cursor、xkbcommon、Fcitx5、libav*/libsw*）→ 自定义 `AppRun.sh`（`QT_QUICK_CONTROLS_STYLE=Basic`、`QT_QPA_PLATFORM=wayland;xcb`、`QT_IM_MODULE=fcitx5`、`--vulkan/--opengl/--software/--x11/--wayland/--debug-plugins`）→ linuxdeploy 打包（`--exclude-library libwayland-client/libwayland-egl/libvulkan`）→ **校验门禁**（必需插件/库存在；禁打包 `libGL/libEGL/libGLX/libOpenGL/libgbm/libwayland-egl/libwayland-client`；禁非 Basic 风格；AppImage 内 ffmpeg 只能有一套；ldd 缺 `libQt6*/libFcitx5*/libwayland*/libxkbcommon*/libav*` 即失败）→ 体积构成报告写入 Step Summary → artifact → tag 或给了 `release_tag` 时 `gh release create/upload` | **真构建 + 打包 + 产物校验**（最强门禁） |
| `build-linux-arm.yml` | `workflow_dispatch`、`push` tags `v*`、`workflow_call` | `ubuntu-24.04-arm`（原生 arm64，非交叉） | 与 x86 同结构：apt 依赖（另加编译 fcitx5-qt 所需 ECM/libwayland，**不装 qt6-wayland/fcitx5-frontend-qt6**）→ `aqt install-qt linux_arm64 desktop 6.10.3 linux_gcc_arm64 -m qtmultimedia qtshadertools qt5compat` → sanity（Wayland/OpenGL/FFmpeg：优先 pkg-config `libav*`，否则提示需 `-DQUEMUSIC_FFMPEG_USE_QT=ON`）→ build → fcitx5-qt（`ENABLE_QT6=ON`）→ linuxdeploy-aarch64 → 裁剪 + 组 AppDir + AppRun → 打包 → 校验 → artifact `QueMusic-aarch64.AppImage` → tag 时 `softprops/action-gh-release@v2` | **真构建 + 打包 + 产物校验（arm64）** |
| `build-linux-tar.yml` | `workflow_dispatch`、`push` tags `v*`、`workflow_call` | `ubuntu-24.04` | apt 依赖（含 `patchelf`，**不含 libav*-dev、不含 libfuse2**）→ Qt 6.10.3 → sanity（`libQt6ShaderToolsConfig.cmake`、`libavcodec.so.61`、`libavformat.so.61`、`libavutil.so.59`、`libavcodec.so` 链接名）→ `-DQUEMUSIC_FFMPEG_USE_QT=ON` build → 组 `QueMusic-linux-<arch>/{bin,_deps,run.sh,使用注意事项.txt}` + 拷 Qt 自带 `libav*.so.*`/`libsw*.so.*` 进 `bin/` → 删中间产物 → **内联 Python + patchelf** 给全包 ELF 写 `$ORIGIN`（含各含库目录）相对 RPATH → `tar -cJf` → 校验（结构、`bin/QueMusic` 为 64 位 ELF、包内自带 4 个 `libav*`、无绝对 RPATH、链的是 `libavcodec.so.61`、依赖可解析、包内不含 Qt 本体）→ artifact `QueMusic-Linux-tar` → tag 时 `softprops` 附到 Release | **真构建 + 便携包 + 强校验** |
| `build-macos.yml` | `workflow_dispatch`、`push` tags `v*`、`workflow_call`（4 个 Apple secrets 可选） | 矩阵 `macos-14`（arm64）+ `macos-15-intel`（x86_64），`fail-fast: false` | aqt `mac desktop 6.10.3 clang_64 -m all` → 定位 `QT_PREFIX`（并强制要求 Qt6Multimedia）→ 确保 FFmpeg（Qt 自带 `libav*.dylib` 优先，否则 brew）→ `sips`/`iconutil` 生成 `resources/app/icon.icns` → `cmake -B build -DCMAKE_BUILD_TYPE=Release`（+`QT_DIR`/`Qt6_DIR`/`Qt6Multimedia_DIR`）→ build → 删非 sqlite 驱动 → `macdeployqt -qmldir -always-overwrite` → 校验（同上）→ `codesign` 分层签名 → `codesign --verify --deep --strict` + `spctl -a -vvv`（拒绝属预期）→ `hdiutil create` → 有证书才签 DMG + 公证 + stapler → artifact + tag 时 `softprops` | **真构建 + 打包 + 签名校验** |
| `release.yml`「Release（一键打包并发 Release）」 | 仅 `workflow_dispatch`（输入 `tag` 必填、`title`、`notes`、`download_proxy` 默认 `https://gh-proxy.com`、`draft=true`、`prerelease=false`） | `publish` job 在 `ubuntu-24.04`，`needs: [linux-x86, linux-arm64, macos, linux-tar]` | 4 个子流程以 `workflow_call` 复用（`release_tag: ''`，子流程不自建 Release）→ `download-artifact` 到 `dist/` → 只挑 `*.AppImage`/`*.dmg`/`*.tar.xz`/`*.tar.gz` → `gh release create/upload` → 生成加速下载表格进 Step Summary | **编排与发布**（不新增校验；构建正确性由被复用的子流程负责） |

要点：真正**构建**的是 `ci.yml`、`build-linux*.yml`、`build-macos.yml`、`build-windows.yml`；只有 `ci.yml` 做 **qmllint 静态门禁 + 启动冒烟**；`release.yml` 只编排发布；Windows 全程**没有**装机包与产物完整性校验。

---

## 7.6 测试

- 唯一测试资产：`tests/smoke-stderr.sh`（90 行 bash）。
- 用法：`tests/smoke-stderr.sh <可执行文件绝对/相对路径> [运行秒数]`，例如 `tests/smoke-stderr.sh build/bin/QueMusic 12`。
- 行为：强制 `QT_QPA_PLATFORM=offscreen`、`QT_LOGGING_RULES="qt.qpa.*=false"`，把 stderr 收进临时文件，后台启动应用最多 N 秒（默认 12），期间若进程消失则记录退出码。
- 失败判据（正则命中即失败）：`QQmlApplicationEngine failed to load component`、`is not a type`、`Cannot read property`、`TypeError`、`Unable to assign`、`Cannot assign to non-existent property`、`ShaderEffect: Failed`、`Failed to deserialize QShader`、`Binding loop detected`、`qrc:.*: .* is not a`。
- 退出码：提前退出为 0 且无错误模式 → 只警告（CI 无音频设备常见）；`126/127` → 运行库/PATH 问题判失败；其它非零 → 崩溃/初始化失败判失败。
- 环境噪声（无声卡/无 GPU/无字体）只提示，不计失败。
- **无单元测试框架**：全仓库（排除 `build/`、`ThirdParty/`、`ffmpeg/`、`api/QCloudMusicApi/`）无 `enable_testing()`、无 `add_test(`、无 `QTEST_*`、无 `Qt6::Test`、无 `ctest` 调用。静态质量保障实际由 `ci.yml` 的 `qmllint` 步骤承担。

---

## 7.7 设置键总表

### 7.7.1 ini 文件位置与 QSettings 配置方式

| 项 | 值 |
|---|---|
| 主配置目录 | `QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)` → `main.cpp:118` 的 `configPath`，并以 `engine.rootContext()->setContextProperty("configDir", configPath)` 暴露给 QML |
| 主 ini | **`<ConfigLocation>/BroNekoX/QueMusic.ini`**（Windows 通常 `%APPDATA%\BroNekoX\QueMusic.ini`；Linux `~/.config/BroNekoX/QueMusic.ini`） |
| 配置方式 | `main.cpp:147-148`：`QSettings::setDefaultFormat(QSettings::IniFormat)` + `QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, configPath)`；`main.cpp:141-145` 设 `organizationName="BroNekoX"`、`organizationDomain="com.bronekox.quemusic"`、`applicationName="QueMusic"` |
| 显式构造同一文件的代码 | `main.cpp:119`、`cpp/LogManager.cpp:35,226`、`api/MusicApiService.cpp:164`（均硬编码 `configPath + "/BroNekoX/QueMusic.ini"`） |
| QML 侧 | 各 `Settings {}` 单例只设 `category`，不设 `fileName`/`format`/`location` → 走 `QSettings` 默认（IniFormat + org/app）落到同一 ini（推断，未逐项验证 QML `location` 默认值） |
| 无分组键 | `cpp/audio/AudioEngine.cpp`、`cpp/webdav/WebDavStore.cpp`、`cpp/plugins/*` 用**默认构造** `QSettings settings;` → 同样落到上述 ini |
| 独立 ini | **`<exe 所在目录>/Account.ini`**（`cpp/AccountManager.cpp:46`：`QCoreApplication::applicationDirPath() + "/Account.ini"`）——注意它跟随安装目录，不随用户配置目录走 |
| 相关数据文件 | SQLite：`<AppDataLocation>/player_data.db`（`cpp/DbService.cpp:64,178-205`，含迁移逻辑；`doc/qm-dbcheck.bat` 按 `%APPDATA%\BroNekoX\QueMusic\player_data.db` 检查）；缓存：`<AppDataLocation>/cache`（`DbService::cacheDir()`）、WebDAV 缓存 `<cacheDir>/webdav/<key>`（`WebDavCache.cpp:37`）；日志：`<AppDataLocation>/logs/QueMusic_YYYY-MM-DD.log`（`LogManager.cpp:29,197`） |

### 7.7.2 `Options/`（QML 单例 `components/Config.qml`，`category: "Options"`，共 45 键）

| 键名 | 类型 | 默认值 | 写入方 | 读取方 | 作用 |
|---|---|---|---|---|---|
| `Options/musicVolume` | real | `0.6` | Playback、PlayerControl、SettingsView | Playback、PlayerControl、main、SettingsView | 主音量 0–1 |
| `Options/closeToManage` | bool | `false` | SettingsView | WindowControls、SettingsView | 关窗改为最小化到托盘 |
| `Options/rememberWindow` | bool | `false` | SettingsView | WindowControls、main、SettingsView | 记住窗口位置与大小 |
| `Options/winX` | int | `0` | WindowControls | WindowControls、main | 上次窗口 X |
| `Options/winY` | int | `0` | WindowControls | WindowControls、main | 上次窗口 Y |
| `Options/winW` | int | `0` | WindowControls | WindowControls、main | 上次窗口宽 |
| `Options/winH` | int | `0` | WindowControls | WindowControls、main | 上次窗口高 |
| `Options/autoUpdate` | bool | `false` | SettingsView | main、SettingsView | 自动更新开关 |
| `Options/searchList` | list<string> | `[]` | main、SearchCard | main、SearchCard | 搜索历史 |
| `Options/openShortCut` | bool | `true` | SettingsView | main、SettingsView | 全局快捷键总开关（录制时临时置 false） |
| `Options/cycleIndex` | int | `0` | PlayerControl、DesktopPlayerWindow | Playback、PlayerControl、DesktopPlayerWindow、main | 循环模式 0 列表/1 单曲/2 随机/3 暂停操作（读取时钳到 0–3） |
| `Options/picCache` | bool | `false` | SettingsView | QPicture、SettingsView | 图片缓存开关 |
| `Options/soundQuality` | int | `1` | PlayerOptions、SettingsView | PlayerOptions、SettingsView、`api/MusicApiService.cpp:167`（启动兜底读） | 在线音质档位 |
| `Options/useDefaultDevice` | bool | `true` | PlayerOptions、SettingsView | PlayerOptions、main、SettingsView | 使用系统默认音频输出 |
| `Options/audioDevice` | int | `0` | PlayerOptions、SettingsView | PlayerOptions、main、SettingsView | 输出设备索引（越界时回退默认设备） |
| `Options/autoPlay` | bool | `true` | PlayerOptions、SettingsView | PlayerOptions、SettingsView | 自动播放 |
| `Options/playerRateIndex` | int | `2` | PlayerOptions | PlayerOptions | 倍速预设索引（6 = 自定义） |
| `Options/volumeStep` | int | `5` | SettingsView | Playback、SettingsView | 音量步长(%) |
| `Options/seekStep` | int | `5` | PlayerOptions、SettingsView | Playback、PlayerOptions、SettingsView | 精确跳转步长(秒) |
| `Options/fadeEnabled` | bool | `true` | PlayerOptions、SettingsView | Playback、PlayerOptions、SettingsView | 淡入淡出防爆音 |
| `Options/fadeMs` | int | `200` | PlayerOptions、SettingsView | Playback、PlayerOptions、SettingsView | 淡变时长(ms) |
| `Options/sleepMinutes` | int | `30` | PlayerOptions、SettingsView | Playback、PlayerOptions、SettingsView | 睡眠定时默认分钟 |
| `Options/historyLimit` | int | `200` | SettingsView | Playback、SettingsView | 播放历史上限（读取时 `max(20, …)`） |
| `Options/shuffleAvoid` | int | `2` | PlayerOptions、SettingsView | Playback、PlayerOptions、SettingsView | 随机播放避开最近 N 首 |
| `Options/autoRestoreQueue` | bool | `true` | SettingsView | Playback、SettingsView | 启动恢复上次队列 |
| `Options/playHistory` | string(JSON) | `"[]"` | Playback | Playback | 播放历史（2s 防抖落盘） |
| `Options/lastQueue` | string(JSON) | `"[]"` | Playback | Playback | 上次播放列表 |
| `Options/lastQueueIndex` | int | `-1` | Playback | Playback | 上次播放索引 |
| `Options/downloadFolder` | string | `""` | SettingsView | main、SettingsView | 下载目录 |
| `Options/cacheUrl` | string | `""` | SettingsView | main（`coverHelper.setCacheDir`） | 封面缓存目录 |
| `Options/mainMusicSource` | int | `0` | SettingsView | main、SettingsView | 默认歌曲源 |
| `Options/cacheSize` | int | `500` | SettingsView | main、SettingsView | 缓存上限(MB) |
| `Options/noWindowKit` | bool | `false` | SettingsView | main、SettingsView | 改用系统原生标题栏 |
| `Options/gpuRenderMode` | int | `0` | SettingsView | `main.cpp:120`（QApplication 前设 `QSG_RHI_BACKEND`/`QT_QUICK_BACKEND`）、SettingsView | 渲染后端 0 系统偏好/1 OpenGL/2 Vulkan/3 Software |
| `Options/displayFps` | bool | `false` | SettingsView | main、SettingsView | 右上角显示帧率 |
| `Options/debug` | bool | `false` | SettingsView | main、SettingsView | 调试面板 |
| `Options/timerAnimator` | bool | `false` | SettingsView | `main.cpp:125`（`QSG_NO_VSYNC=1`）、SettingsView | 用 QTimer 驱动动画替代 Vsync |
| `Options/qmlAnimator` | bool | `false` | SettingsView | `main.cpp:127`（为 false 时设 `QSG_USE_SIMPLE_ANIMATION_DRIVER=1`）、SettingsView | QML 动画驱动方式 |
| `Options/logEnabled` | bool | `true` | `LogManager::persistSettings()`、SettingsView（经 `LogManager.enabled`） | `LogManager` 构造、SettingsView | 日志写文件开关 |
| `Options/logLevel` | int | `1`（Info） | `LogManager::persistSettings()`、SettingsView | `LogManager` 构造、SettingsView | 最低日志等级 0 调试…4 致命 |
| `Options/scanTime` | int | `15` | 无 | 无 | **声明未使用（死键）**：自动扫描间隔 |
| `Options/dataUrl` | string | `""` | 无 | 无 | **声明未使用（死键）** |
| `Options/metaDataSource` | int | `0` | 无 | 无 | **声明未使用（死键）**：0 默认/1 Metadata/2 NetWork |
| `Options/serverAgency` | int | `0` | 无 | 无 | **声明未使用（死键）** |
| `Options/editSource` | string | `""` | 无 | 无 | **声明未使用（死键）** |
| `Options/softwareRender` | bool | `false` | 无 | 无 | **声明未使用（死键）**：已被 `gpuRenderMode=3` 取代 |
| `Options/displayDebugControl` | bool | `false`（无默认表达式） | 无 | 无 | **声明未使用（死键）** |

### 7.7.3 `Style/`（`components/Style.qml`，`category: "Style"`，共 43 键）

除注明外，**写入方均为 `SettingsView.qml`**（主题/字号/圆角/侧栏/歌词等设置页），读取方散布于几乎全部 QML 组件。

| 键名 | 类型 | 默认值 | 主要读取方 | 作用 |
|---|---|---|---|---|
| `Style/theme` | int | `0` | Options（`darkis`）、SettingsView | 主题样式 0 跟随系统/1 深/2 浅（依 `Options.darkis` 判定） |
| `Style/color` | int | `0` | Options、SettingsView | 主题色索引 |
| `Style/colorList` | list<color> | `["#3481fa","#34fa4a","#faad34","#ad34fa"]` | Options、SettingsView | 主题色列表（设置页可增删，2–8 个） |
| `Style/backmode` | int | `0` | LeftSideBar、main、SettingsView | 背景模式 |
| `Style/backpic` | int | `0` | main、SettingsView | 背景图片选择 |
| `Style/backgroundImage` | string | `"qrc:/QueMusic/resources/pic/back2.jpg"` | main、SettingsView | 自定义背景图 |
| `Style/shadowBlur` | real | `1.0` | SettingsView | 阴影模糊系数（无其它读取方） |
| `Style/shadowXOffset` | int | `5` | SettingsView | 阴影 X 偏移（无其它读取方） |
| `Style/shadowYOffset` | int | `5` | SettingsView | 阴影 Y 偏移（无其它读取方） |
| `Style/shadowSize` | int | `16` | SettingsView | 阴影尺寸 |
| `Style/blurSize` | int | `48` | QBlurCard、QBlurTapBar、SettingsView | 模糊半径 |
| `Style/highQualityBlur` | bool | `false` | QBlurCard、SettingsView | 高质量模糊 |
| `Style/textH1` | int | `22` | AnimatorWindow、PlayListWindow、HomePage、SettingsView | 一级标题字号 |
| `Style/textH2` | int | `17` | AccountCard、DesktopPlayer、MusicInfo、PlayList、QHead、main 等 | 二级标题字号 |
| `Style/textmain` | int | `13` | 多数组件/页面（85 处） | 正文字号 |
| `Style/text` | int | `12` | 43 处（含 SettingsView） | 小号正文字号 |
| `Style/textTip` | int | `11` | 30 处 | 提示文字字号 |
| `Style/texticon` | int | `16` | 41 处 | 图标字号 |
| `Style/texticonH` | int | `22` | PlayerControl、PlayerMaxCenter | 大图标字号 |
| `Style/pageTitle` | int | `26` | 6 个页面 + SettingsView | 页头字号 |
| `Style/labelRadius` | int | `12` | 104 处（几乎全部控件） | 控件圆角 |
| `Style/cubeRadius` | int | `16` | 50 处 | 卡片/面板圆角 |
| `Style/noControlRadius` | bool | `false` | QBlurTapBar、QButton、QWKButton、SButton、SettingsView | 关闭控件圆角 |
| `Style/borderDepth` | real | `0.08` | SettingsView（无其它读取方） | 边框透明度 |
| `Style/layerEnabled` | bool | `true` | SettingsView（无其它读取方） | 分层背景开关 |
| `Style/premiumAnime` | bool | `false` | FilePage、SettingsView | 高级动画 |
| `Style/animeSpeed` | int | `1` | SettingsView（无其它读取方） | 动画速度 |
| `Style/homeLayout` | int | `0` | HomePage、SettingsView | 首页布局 0 默认/1 竖向/2 混合 |
| `Style/noOpacityControl` | bool | `false` | PlayerControl、SettingsView | 透明底部播放栏 |
| `Style/sidebarColor` | bool | `false` | LeftSideBar、SettingsView | 侧栏着色 |
| `Style/sidebarStyle` | int | `1` | LeftSideBar、SettingsView | 侧栏样式 |
| `Style/animeType` | int | `0` | AnimatorWindow、PlayListWindow、SettingsView | 弹窗动画类型 |
| `Style/menutheme` | int | `1` | SettingsView（无其它读取方） | 菜单主题 1 material/2 fluent |
| `Style/glmode` | int | `0` | SettingsView（无其它读取方） | 毛玻璃模式 |
| `Style/uilevel` | int | `0` | SettingsView（无其它读取方） | UI 密度级别 |
| `Style/lyricSize` | int | `10` | PlayerMaxCenter、SettingsView（两处均可写） | 歌词字号 |
| `Style/flowStyle` | int | `1` | MainLyric、LyricsFree、SettingsView | 流体背景算法 0 Fluid/1 Classic/2 静态渐变 |
| `Style/waveDisplay` | bool | `true` | MainLyric、LyricsFree、main、SettingsView | 显示音波效果 |
| `Style/premiumLyricAnime` | bool | `true` | MainLyric、LyricsFree、SettingsView | 逐行弹簧动画 |
| `Style/textWidth` | int | `600` | MainLyric、LyricsFree、SettingsView | 歌词最大宽度 |
| `Style/maskBlur` | bool | `true` | MainLyric、LyricsFree、SettingsView | 歌词遮罩模糊 |
| `Style/lyricHideGui` | bool | `true` | PlayerMaxCenter、SettingsView（两处均可写） | 歌词界面隐藏控件 |
| `Style/fontFamily` | string | 无默认（空） | MainLyric、Lyrics3D、LyricsFree、SettingsView | 全局字体族 |

### 7.7.4 `ShortCuts/`（`components/ShortCut.qml`，`category: "ShortCuts"`，共 26 键）

读写均为**动态下标**：`ShortCut[modelData.k]` / `ShortCut[flag]`（`main.qml:195-197` 注册全局 `Shortcut`）、`ShortCut[recordingAction] = sequence` 与 `ShortCut[prop] = …`（`SettingsView.qml:1842,2030,2058` 录制与恢复默认）。

| 键名 | 类型 | 默认值 | 键名 | 类型 | 默认值 |
|---|---|---|---|---|---|
| `ShortCuts/play` | string | `"Space"` | `ShortCuts/globalShortcutPlay` | bool | `true` |
| `ShortCuts/back` | string | `"Left"` | `ShortCuts/globalShortcutBack` | bool | `true` |
| `ShortCuts/forward` | string | `"Right"` | `ShortCuts/globalShortcutForward` | bool | `true` |
| `ShortCuts/playList` | string | `"Alt"` | `ShortCuts/globalShortcutPlayList` | bool | `true` |
| `ShortCuts/musicControl` | string | `"Up"` | `ShortCuts/globalShortcutMusicControl` | bool | `true` |
| `ShortCuts/volumeUp` | string | `"Ctrl+Up"` | `ShortCuts/globalShortcutVolumeUp` | bool | `true` |
| `ShortCuts/volumeDown` | string | `"Ctrl+Down"` | `ShortCuts/globalShortcutVolumeDown` | bool | `true` |
| `ShortCuts/seekBack` | string | `"Ctrl+Left"` | `ShortCuts/globalShortcutSeekBack` | bool | `true` |
| `ShortCuts/seekForward` | string | `"Ctrl+Right"` | `ShortCuts/globalShortcutSeekForward` | bool | `true` |
| `ShortCuts/mute` | string | `"Ctrl+M"` | `ShortCuts/globalShortcutMute` | bool | `true` |
| `ShortCuts/abLoop` | string | `"Ctrl+B"` | `ShortCuts/globalShortcutAbLoop` | bool | `true` |
| `ShortCuts/favorite` | string | `"Ctrl+D"` | `ShortCuts/globalShortcutFavorite` | bool | `true` |
| `ShortCuts/playerOptions` | string | `"Ctrl+T"` | `ShortCuts/globalShortcutPlayerOptions` | bool | `true` |

作用：各功能键位 + 每个功能**独立**的全局快捷键开关（`Config.openShortCut` 为总开关，见 `main.qml:197`）。

### 7.7.5 `LastMedia/`（`components/OptionsLastSongs.qml`，`category: "LastMedia"`，经 `Options.lastSongs` 访问）

| 键名 | 类型 | 默认值 | 写入方 | 读取方 | 作用 |
|---|---|---|---|---|---|
| `LastMedia/name` | string | `""` | WindowControls（会话结束） | HomePage | 上次播放曲名 |
| `LastMedia/artist` | string | `""` | WindowControls | HomePage | 上次播放歌手 |
| `LastMedia/cover` | string | `""` | WindowControls | HomePage | 上次封面（空时用内置占位图） |
| `LastMedia/hash` | string | `""` | WindowControls | HomePage | 上次曲目标识（在线为 hash，本地为 path） |
| `LastMedia/source` | int | `-1` | WindowControls | HomePage | 音源（<0 本地 / 3 WebDAV / 其它在线） |

### 7.7.6 `LyricsFree/`（`lyricsui/LyricsFreeConfig.qml`，`category: "LyricsFree"`，12 键；读写均在 `lyricsui/LyricsFree.qml`）

| 键名 | 类型 | 默认值 | 作用 |
|---|---|---|---|
| `LyricsFree/bgStyle` | int | `0` | 背景 0 流体/1 图片/2 视频/3 平面星空 |
| `LyricsFree/bgImage` | string | `""` | 背景图片路径 |
| `LyricsFree/bgVideo` | string | `""` | 背景视频路径 |
| `LyricsFree/showTitle` | bool | `true` | 封面下方显示歌曲名 |
| `LyricsFree/showArtist` | bool | `true` | 封面下方显示歌手名 |
| `LyricsFree/coverReflection` | bool | `false` | 封面倒影 |
| `LyricsFree/lyricSpacingScale` | real | `1.0` | 行距缩放 |
| `LyricsFree/lyricCardAngle` | int | `0` | 歌词卡片倾角 |
| `LyricsFree/lyricCurrentScale` | real | `1.02` | 当前行放大倍数 |
| `LyricsFree/lyricIdleOpacity` | real | `0.5` | 非当前行透明度 |
| `LyricsFree/lyricSungColor` | string | `""` | 已唱颜色（空 = 跟随主题） |
| `LyricsFree/lyricLineColor` | string | `""` | 当前行颜色（空 = 跟随主题） |

（共 12 个键：`bgStyle`、`bgImage`、`bgVideo`、`showTitle`、`showArtist`、`coverReflection`、`lyricSpacingScale`、`lyricCardAngle`、`lyricCurrentScale`、`lyricIdleOpacity`、`lyricSungColor`、`lyricLineColor`；另有一个 non-property 成员 `Settings` 分类本身不落键）

### 7.7.7 `Lyrics3D/`（`lyricsui/Lyrics3DConfig.qml`，`category: "Lyrics3D"`，18 键；读写均在 `lyricsui/Lyrics3D.qml` 的滑杆表与取色器）

| 键名 | 类型 | 默认值 | 作用 |
|---|---|---|---|
| `Lyrics3D/preset` | int | `0` | 预设索引 |
| `Lyrics3D/bright` | real | `0.85` | 亮度 |
| `Lyrics3D/saturate` | real | `1.45` | 饱和度 |
| `Lyrics3D/audio` | real | `1.0` | 音频驱动强度 |
| `Lyrics3D/density` | real | `1.0` | 粒子密度 |
| `Lyrics3D/fieldScale` | real | `1.0` | 场缩放 |
| `Lyrics3D/camDist` | real | `9.0` | 相机距离 |
| `Lyrics3D/camSens` | real | `1.0` | 相机灵敏度 |
| `Lyrics3D/tilt` | real | `0.5` | 倾斜 |
| `Lyrics3D/lyricScale` | real | `1.0` | 歌词缩放 |
| `Lyrics3D/lyricY` | real | `0.0` | 歌词 Y 偏移 |
| `Lyrics3D/lyricZ` | real | `0.0` | 歌词 Z 偏移 |
| `Lyrics3D/glow` | real | `0.55` | 辉光 |
| `Lyrics3D/sweep` | real | `0.7` | 扫光 |
| `Lyrics3D/beatGlow` | real | `0.6` | 节拍辉光 |
| `Lyrics3D/floatAmt` | real | `0.4` | 漂浮幅度 |
| `Lyrics3D/textColorCustom` | string | `""` | 自定义文字色（空 = 跟随主题） |
| `Lyrics3D/glowColorCustom` | string | `""` | 自定义辉光色（空 = 跟随主题） |

### 7.7.8 `AudioDsp/`（`cpp/audio/AudioEngine.cpp`，`settings.beginGroup("AudioDsp")`，20 键）

写入：`AudioEngine::saveSettings()`（`AudioEngine.cpp:1314-1336`，全组落盘）；读取：`AudioEngine::loadSettings()`（`:1265-1296`，全部带 `qBound` 钳位）。

| 键名 | 类型 | 默认值 | 取值范围 / 作用 |
|---|---|---|---|
| `AudioDsp/dspEnabled` | bool | `true` | 总开关（未设时视为 `eqEnabled`） |
| `AudioDsp/eqEnabled` | bool | `false` | 均衡器开关 |
| `AudioDsp/eqGains` | QByteArray | 空 | 10 段增益（字节数组） |
| `AudioDsp/eqQ` | double | `1.41` | 钳到 0.2–8.0 |
| `AudioDsp/preampDb` | double | `0.0` | 钳到 −24…24 |
| `AudioDsp/autoHeadroom` | bool | `true` | 自动余量 |
| `AudioDsp/balance` | double | `0.0` | 钳到 −1…1 |
| `AudioDsp/stereoWidth` | double | `1.0` | 钳到 0…2 |
| `AudioDsp/mono` | bool | `false` | 单声道 |
| `AudioDsp/swapChannels` | bool | `false` | 左右互换 |
| `AudioDsp/gainLeft` | double | `0.0` | 钳到 −24…24 |
| `AudioDsp/gainRight` | double | `0.0` | 钳到 −24…24 |
| `AudioDsp/rgMode` | int | `0` | 钳到 0…2（响度归一模式） |
| `AudioDsp/rgPreamp` | double | `0.0` | 钳到 −24…24 |
| `AudioDsp/rgPreventClip` | bool | `true` | 防削波 |
| `AudioDsp/limiter` | bool | `false` | 限幅器 |
| `AudioDsp/limiterDb` | double | `-0.5` | 钳到 −24…0 |
| `AudioDsp/outputSampleRate` | int | `0` | 钳到 0…384000（0 = 跟随源） |
| `AudioDsp/outputBufferMs` | int | `0`（读取时回落到内部默认） | 输出缓冲毫秒 |
| `AudioDsp/pitchSemitones` | double | `0.0` | 钳到 −12…12 |

### 7.7.9 `Plugins/*` 与 `webdav/*`（C++ 写入，均落在主 ini）

| 键名 | 类型 | 默认值 | 写入方 | 读取方 | 作用 |
|---|---|---|---|---|---|
| `Plugins/lyrics` | string | `"builtin.default"` | `LyricsPluginStore::setSelectedId()` | `LyricsPluginStore` 构造 | 选中的歌词界面插件 id（内置：`builtin.default` / `builtin.free` / `builtin.3d(Beta)`；选中项被卸载则回退默认） |
| `Plugins/function/<pluginId>` | bool | `true` | `FunctionPluginStore::setEnabled()` | `FunctionPluginStore::loadEnabled()` | 每个功能插件的启用开关；插件卸载后键保留 |
| `Plugins/settings/<pluginId>/<任意键>` | 任意 | 无 | 插件自身（`PluginApi.settings`，`Settings { category: "Plugins/settings/" + pluginId }`） | 插件自身 | 插件私有设置命名空间（键名由插件决定，未确认） |
| `webdav/servers` | list<map> | 空 | `WebDavStore::save()`（add/update/remove 后自动） | `WebDavStore::load()`（构造时） | WebDAV 服务器列表；元素字段：`id`(UUID 无花括号)、`name`、`url`(规范化 `scheme://host/path/`)、`user`、`password`(Windows 用 DPAPI 后 Base64；其它平台仅 Base64，**非加密**)；`servers()` 对外会剔除 `password` |

### 7.7.10 `Account.ini`（`cpp/AccountManager.cpp`，文件 `<exe 目录>/Account.ini`，非主 ini）

| 键名 | 类型 | 默认值 | 写入方 | 读取方 | 作用 |
|---|---|---|---|---|---|
| `Netease/deviceId` | string | 首次生成 52 位大写 hex | `ensureNeteaseDeviceId()` | `ensureNeteaseDeviceId()` / `loadNetease()` | 稳定设备号（避免被网易云判环境异常） |
| `Netease/anonymousToken` | string | 空 | `ensureNeteaseAnonymousToken()` | `loadNetease()` | 匿名注册得到的 `MUSIC_A` |
| `Netease/cookie` | string | 空 | `persistNetease()` | `loadNetease()`；退出登录时 `clearLoginSettings("Netease")` 删除 | 登录态 cookie（读取时补 `deviceId`、`os=pc`） |
| `Netease/nickname` | string | 空 | `persistNetease()` | `loadNetease()` | 昵称 |
| `Netease/avatar` | string | 空 | `persistNetease()` | `loadNetease()` | 头像 URL |
| `Kugou/cookie` | string | 空 | `persistKugou()` | `loadKugou()`；`clearLoginSettings("Kugou")` 删除 | 酷狗登录态 |
| `Kugou/nickname` | string | 空 | `persistKugou()` | `loadKugou()` | 昵称 |
| `Kugou/avatar` | string | 空 | `persistKugou()` | `loadKugou()` | 头像 |
| `Kugou/guid` | string | 空 | `persistKugou()`（亦在 `:630` 写入） | `loadKugou()`（亦在 `:625` 读取） | 酷狗设备 guid |
| `Kugou/dfid` | string | 空 | `persistKugou()`（亦在 `:643` 写入） | `loadKugou()`（亦在 `:626` 读取） | 酷狗 dfid |

### 7.7.11 未持久化 / 非 ini 的「类设置」说明

- `components/Theme.qml` 是**纯内存** `QtObject`（无 `Settings`），颜色由 `Options.changeTheme()` 依 `Style.theme` 与 `Style.color` 计算，不落盘。
- `components/Fonts.qml`、`components/Options.qml`、`components/Playback.qml`、`components/PluginSlots.qml` 为运行时单例，本身不持久化（`Playback` 通过 `Config.playHistory`/`lastQueue` 间接落盘）。
- `api/MusicApiService` 另有 `loadQualityCache()`/`saveQualityCache()`（`m_altsSaveTimer`），存的是音质探测缓存而非配置键（未确认文件位置）。
- `components/Config.qml` 中声明但全仓库无读写的 7 个键见 7.2 表末。

---

## 7.8 资源与部署注意事项

| 项 | 说明 |
|---|---|
| FFmpeg 运行库 | Windows：根 `CMakeLists.txt:345-354` 先 `FATAL_ERROR` 校验 `${QUEMUSIC_FFMPEG_ROOT}/bin` 存在，再 POST_BUILD `copy_directory` 到 exe 目录（`avcodec-61.dll`、`avformat-61.dll`、`avutil-59.dll`、`swresample-5.dll`）。缺目录**不静默跳过**（否则能启动但无法播放）。Linux tar 包在打包步骤单独拷 Qt 自带 `libav*.so.*`/`libsw*.so.*` 进 `bin/` |
| QWindowKit DLL | `QUEMUSIC_QWK_RUNTIME_COPY` 为真时（即非静态 QWK）POST_BUILD 把 `QWindowKit::Core` + `QWindowKit::Quick` 拷到 `$<TARGET_FILE_DIR>`；DLL 实际生成在 `build/_deps/qwindowkit-build` 下。用仓库子模块时 `QWINDOWKIT_BUILD_STATIC=ON` → 该拷贝关闭（静态库已链进 exe）；用系统共享版 QWindowKit 时才需要 |
| 多媒体 ffmpeg 插件裁剪 | `qt_deploy_runtime` 之后删 `multimedia/ffmpegmediaplugin.dll` 与 `swscale-8.dll`（全项目不使用 `MediaPlayer`，约省 1.7 MB）；解码层所需的 `avcodec/avformat/avutil/swresample` 保留 |
| SQL 驱动裁剪 | Windows 部署后删非 `qsqlite` 的驱动与 `libmysql/libssl/libcrypto`；Linux CI 在 Qt 安装目录直接删非 sqlite 驱动；macOS CI 同策略。项目只用 `QSQLITE` |
| `icon.rc` | 根目录，`IDI_ICON1 ICON "resources/icon.ico"` + `VS_VERSION_INFO`（FileVersion/ProductVersion `0.1.0.0`，CompanyName `QueMusic Contributors`，Translation `0x0804,1200`）；仅在 Windows 作为源文件加入目标（`$<$<PLATFORM_ID:Windows>:icon.rc>`） |
| 应用图标其它形态 | Windows 运行时 `QIcon(":/QueMusic/resources/icon.ico")`（`main.cpp:144`，必须 `:/` 前缀）；macOS 由 CI `sips`+`iconutil` 从 `resources/app/rainbowMusicIcon.png` 生成 `resources/app/icon.icns`；Linux 打包用 `resources/icon.png` |
| Windows SMTC 身份 | `main.cpp:31-109`：`SetCurrentProcessExplicitAppUserModelID("BroNekoX.QueMusic")`、写 `HKCU\Software\Classes\AppUserModelId\BroNekoX.QueMusic` 的 `DisplayName`/`IconUri`、在 `%APPDATA%\Microsoft\Windows\Start Menu\Programs\QueMusic.lnk` 写 `PKEY_AppUserModel_ID` |
| `.gitignore` 相关 | 忽略 `build*/`、`cmake-build-*/`、`out/`、`ThirdParty/*/build/`、`AppDir/`、`squashfs-root/`、`fcitx5-qt/`、`*.exe/*.dll/*.so/*.dylib/*.lib/*.a/*.obj/*.o`、`.qmlc/*.qmlc/*.jsc`、`*.log/temp/logs`、`.idea/.vscode/.qtcreator/*.user`。**`ffmpeg/` 并未整体忽略**：为让 include/lib/bin 与许可证入库，另有否定规则 `!ffmpeg/bin/*.dll`、`!ffmpeg/lib/*.dll.a`（必须排在 `*.dll`/`*.a` 之后）。`packaging/tools/`（linuxdeploy AppImage 下载目录）**未列入** `.gitignore`（未确认是否有意） |
| `.gitattributes` | `* text=auto eol=lf`；`*.cpp/*.h/*.hpp` 用 `diff=cpp`；`*.qml/*.mjs/*.js`、`CMakeLists.txt/*.cmake` 标记为 text；`*.png/*.jpg/*.jpeg/*.ico/*.ttf/*.svg/*.gif/*.qsb` 标记 binary |
| 运行期 RPATH | 主程序 `CMAKE_BUILD_RPATH_USE_ORIGIN=TRUE`；Linux 便携包由 `patchelf` 把每个 ELF 的 RPATH 重写为 `$ORIGIN`（含包内各含库目录），CI 会门禁「出现任何绝对 RPATH 即失败」 |
| 渲染后端环境变量 | 由 `Options/gpuRenderMode`、`Options/timerAnimator`、`Options/qmlAnimator` 在 `QApplication` 构造前 `qputenv`：`QSG_RHI_BACKEND=opengl|vulkan`、`QT_QUICK_BACKEND=software`、`QSG_NO_VSYNC=1`、`QSG_USE_SIMPLE_ANIMATION_DRIVER=1` |
| 高 DPI | `QApplication::setHighDpiScaleFactorRoundingPolicy(PassThrough)`（`main.cpp:130-131`）；AppImage 的 `AppRun.sh` 刻意**不**设 `QT_SCALE_FALLING_ROUNDING_POLICY=PassThrough`（XWayland 下会放大取整偏差） |
| 托盘 | 使用 `QApplication`（QtWidgets）而非 `QGuiApplication`，并 `setQuitOnLastWindowClosed(false)`；退出统一走 QML `Options.exit()` |
| 辅助文档 | `doc/updater.txt`：0.6.0 更新日志文本（WebDAV、插件系统、评论、音频后端重构等），供更新提示展示。`doc/qm-dbcheck.bat`：Windows 收藏写库诊断批处理（查 `%APPDATA%\BroNekoX\QueMusic\player_data.db`、清理 `-wal/-shm`、试写目录、`sqlite3` 直插测试、查进程占用），输出到桌面 `qm-dbcheck.txt`。`doc/macos-install.md`：macOS DMG 被 Gatekeeper 拦截（「已损坏」）时的三种放行办法与下载选包指引 |
---

← 返回 [索引 README.md](README.md)
