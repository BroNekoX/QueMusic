# 🎵 QueMusic Project

<p align="center"><img alt="Logo" src="doc/example/logo.png"></p>

<p align="center"><b>基于 C++/Qt/QML 构建的现代跨平台音乐播放器（Beta）</b></p>

<p align="center">
  <img alt="License" src="https://img.shields.io/badge/License-Apache--2.0-blue?style=flat-square">
  <img alt="Qt" src="https://img.shields.io/badge/Qt-6.10.3-41CD52?style=flat-square">
  <img alt="Language" src="https://img.shields.io/badge/Language-C%2B%2B17%20%7C%20QML-orange?style=flat-square">
  <img alt="Platform" src="https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey?style=flat-square">
  <img alt="Stars" src="https://img.shields.io/github/stars/BroNekoX/QueMusic?style=flat-square">
  <img alt="Release" src="https://img.shields.io/github/v/release/BroNekoX/QueMusic?style=flat-square">
</p>

> **基于 Qt 6.10 / QML 与 GPU 加速 RHI 渲染的开源跨平台音乐播放器，支持接入网易云、酷狗等平台的公开音乐服务接口。**
> 动效精致，开发者坚持 **永久免费 & 开源**。
> 
> 🚧 项目正处于 **开发/预览阶段**，部分功能尚未完善, 仍存在部分问题，有一些功能无法使用，会持续更新，欢迎 Star & Fork 一起参与！

## 📋 功能状态

| 状态 | 功能 |
|------|------|
| ✅ 已完善 | 本地播放、本地歌词（.lrc / 内嵌 SYLT & USLT）、沉浸式歌词页、主题与界面自定义、收藏、播放历史、A-B 循环、睡眠定时、Windows SMTC 媒体控件 |
| 🚧 开发中 | 在线搜索与播放（酷狗 / 网易云公开接口）、歌单管理、下载管理、桌面歌词、桌面小窗播放器、均衡器与播放器选项 |
| ❌ 暂不支持 | 投屏（AirPlay 类功能）、账号登录与云同步、移动端 |

国内快速下载本应用及历史版本：

永久下载链接：[下载](https://pan.baidu.com/s/1Q0ptzhJaFljabU8FHNrYSw) 提取码：iqda

加入QQ群获取最新消息：1105114511

---

> [!WARNING]
> 
> 1.本项目仅供用户学习与研究使用，禁止将本项目用于任何商业用途与非法用途。
> 
> 2.本项目开发者不接受任何形式的赞助，打赏，捐赠行为，禁止任何用户向本开发者赞助，打赏，捐赠。
> 
> 3.本项目的使用者出现的任何侵权、盗用、版权问题等违规情况，与本项目无关。
> 
> 4.本项目并不提供公共云端曲库与媒体分发服务，在线音频获取的能力均使用第三方平台个人账号授权获取，付费内容，会员内容，受限制的内容请遵循第三方平台版权。
> 
> 5.如果音乐平台发现本项目包含侵权或有问题的行为，可联系开发者进行更改或移除。
> 
> 6.本项目使用了一些第三方模块，如果你认为本项目违反了部分协议，可联系开发者进行更改或移除。

---

<p align="center">
  <a href="#-核心特性">特性</a> •
  <a href="#%EF%B8%8F-技术栈">技术栈</a> •
  <a href="#-安装与运行">安装</a> •
  <a href="#-截图预览">截图</a> •
  <a href="#-项目结构">结构</a> •
  <a href="#-未来计划">计划</a> •
  <a href="#-贡献指南">贡献</a> •
  <a href="#-许可证">许可证</a> •
  <a href="#免责声明">免责声明</a>
</p>

---

## ✨ 核心特性

| 维度 | 亮点 |
|------|------|
| **🎶 多平台音乐** | 支持登录网易云、酷狗等平台公开接口接入（仅访问公开内容，详见[免责声明](#免责声明)） |
| **⚡ 性能** | C++ 核心模块 + QML RHI 场景渲染（Qt 6 默认 GPU 渲染管线），核显 / 老旧 CPU 可流畅运行 |
| **🎨 精美 UI** | 高级毛玻璃圆角卡片、可自定义主题色 & 界面样式，自研 Theme / 配色系统 |
| **🔄 流畅动画** | 自定义贝塞尔曲线动画，歌词界面丝滑 |
| **📦 功能丰富** | 歌词滚动 / 桌面歌词、均衡器、歌单管理、搜索推荐、收藏同步 |
| **🛡️ 可靠性** | 自制 JS API 管理层，统一错误处理，持续优化 |
| **💻 跨平台** | 支持 **Windows / macOS / Linux** 桌面端（Beta 阶段以 Windows / Linux 为主） |

### 本地歌词加载

本地歌曲按以下顺序读取歌词：同目录同名 `.lrc` → 音频内嵌歌词 → 在线匹配 → “纯音乐，请欣赏”占位。
内嵌歌词支持 ID3v2 的同步歌词（SYLT）与非同步歌词（USLT），以及 TagLib 能识别的
`LYRICS` 文本元数据（常见于 FLAC、Ogg/Opus、MP4/M4A 等格式）。

---

## 🖼️ 截图预览

> 项目正处于 **开发/预览阶段**，下面直接放效果（演示的歌曲仅供参考）

<table>
  <tr>
    <td align="center"><img src="doc/example/home.jpg" width="480"><br><sub>🏠 主界面</sub></td>
    <td align="center"><img src="doc/example/lyric.jpg" width="480"><br><sub>🎤 歌词页 · 沉浸式</sub></td>
  </tr>
  <tr>
    <td align="center"><img src="doc/example/lyric2.jpg" width="480"><br><sub>🎶 歌词页 · 常规</sub></td>
    <td align="center"><img src="doc/example/settings.jpg" width="480"><br><sub>⚙️ 设置页</sub></td>
  </tr>
</table>

---

## 🛠️ 技术栈

| 类别 | 技术 |
|------|------|
| **框架** | Qt 6.10.3 Community |
| **构建** | CMake ≥ 3.24 / Ninja |
| **语言** | C++17 / JavaScript / QML |
| **音频** | Qt Multimedia (FFmpeg7.1.1 后端) |
| **渲染** | QtRHI — 基于平台原生GPU渲染器 |
| **数据库** | Qt SQL / SQLite |
| **工具链** | MSVC 2022 / GCC 13+ / MinGW 13+ / LLVM-MinGW 17+ |

---

## 📥 安装与运行

### 前置条件

- Qt **6.10+**（含 Qt Multimedia, Qt SQL, Qt ShaderTools等基础Qt库）
- CMake ≥ **3.24**
- 编译器：GCC 13+ / MinGW 13+ / LLVM-MinGW 17+ / Clang
- （可选）**Ninja** 构建系统（推荐，已内置在预设中）

### 克隆（含子模块）

```bash
git clone --recurse-submodules https://github.com/BroNekoX/QueMusic.git
cd QueMusic
```

> ⚠️ **重要**：本项目使用 QWindowKit、TagLib 作为 git 子模块，务必加上 `--recurse-submodules`。
> 如果已经 clone 但忘记拉子模块，运行：
> 
> ```bash
> git submodule update --init --recursive
> ```

### 快速构建

本项目内置`CmakePresets.json`,支持快速构建：

#### 1.使用just构建与运行

! 该操作需提前安装just ! 然后执行：

```bash
just setup  # 首次使用：拉取子模块
just b      # 构建
just r      # 运行
```

#### 2.使用Cmake构建

```bash
# 配置（自动选择预设）
cmake --preset <preset-name>

# 编译
cmake --build build/<preset-name> -j 8

# 运行
./build/<preset-name>/bin/QueMusic   # Linux/macOS
./build/<preset-name>/bin/QueMusic.exe  # Windows
```

预设如下：

| 平台 | 预设名 | 编译器 |
|------|------|------|
| Windows | win-llvm-mingw-release | LLVM-MinGW（Qt 6.10 的 `llvm-mingw_64` 套件） |
| Windows | win-mingw-release | MinGW（Qt 的 `mingw_64` 套件） |
| Linux | linux-gcc-release | GCC |
| MacOS | mac-clang-release | Clang |

> 💡 如果 CMake 找不到 Qt，请先设置环境变量 `CMAKE_PREFIX_PATH` 指向你的 Qt 安装目录（例如 `~/Qt/6.10.3/llvm-mingw_64` 或 `~/Qt/6.10.3/gcc_64`）。

### 普通构建

#### Windows（LLVM-MinGW / MinGW）

```bash
# LLVM-MinGW（Qt 6.10 推荐）
cmake -B build -G Ninja \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.10.3/llvm-mingw_64
cmake --build build --parallel
./build/bin/QueMusic

# MinGW
cmake -B build -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.10.3/mingw_64
```

#### Linux 构建

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=~/Qt/6.10.3/gcc_64
cmake --build build -j"$(nproc)"
./build/bin/QueMusic
```

#### MacOS 构建

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=~/Qt/6.10.3/clang_64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build -j 8
./build/bin/QueMusic.app/Contents/MacOS/QueMusic
```

> 📦 三种平台的可执行安装包都会随 [Release](https://github.com/BroNekoX/QueMusic/releases) 发布。

### 打包分发

- **Windows**：使用Windeployqt + Inno Setup(可选)进行分步打包。
- **Linux**：运行 `bash packaging/build-linux.sh` 生成 AppImage。
- **MacOS**：运行 `just mac-bundle` 生成 `.dmg`（需安装 `macdeployqt`）。

---

## 📁 项目结构

```
QueMusic/
├── CMakeLists.txt              # 顶层构建配置
├── cmake/                      # CMake 模块
│   ├── external                # 子模块cmake快速集成
│   └── qtruntime.cmake
├── main.cpp                    # C++ 程序入口
├── main.qml                    # QML 主入口
├── SettingsView.qml            # 设置布局页面
├── cpp/                        # C++ 后端模块
│   ├── CoverHelper.cpp/h       # 封面图片处理
│   ├── ColorExtractor.cpp/h    # 颜色提取（自适应主题色）
│   ├── GetWave.cpp/h           # 音频波形数据
│   ├── FolderModel.cpp/h       # 本地文件夹模型
│   ├── DownloadManager.cpp/h   # 下载管理器
│   ├── LocalLyricsReader.cpp/h # .lrc 与音频内嵌歌词读取
│   ├── Favorites.cpp/h         # 收藏管理
│   └── ...                     # 等其他相关C++模块
├── api/                        # JavaScript API 层
│   ├── QCloudMusicApi/         # 存放QCloudMusicApi第三方项目
│   ├── MusicApiService.cpp/h   # 在线音乐 API总部
│   ├── KugouApi.cpp/h          # 酷狗音乐 API
│   ├── NeteaseApi.cpp/h        # 网易云音乐 API
│   └── OnlinelistModel.cpp/h   # 在线api的列表模型自定义组件
├── components/                 # QML 组件库（自研 UI 库）
│   ├── Q***.qml                # 各自控件，QueMusic由它们组成
│   ├── MusicApi.qml            # 在线音乐整合单例
│   ├── Style.qml               # 全局单例样式
│   ├── Options.qml             # 设置文件
│   └── ...
├── layout/                     # 页面布局
│   ├── LeftSideBar.qml         # 左侧导航栏
│   ├── MainContent.qml         # 主内容区
│   ├── PlayerControl.qml       # 播放控制栏
│   └── PlayerMaxCenter.qml     # 全屏 / 最大化歌词中心
├── pages/                      # 页面
│   ├── HomePage.qml            # 首页 / 推荐
│   ├── SearchPage.qml          # 搜索
│   ├── PlaylistPage.qml        # 歌单详情
│   ├── FavouritePage.qml       # 收藏
│   ├── FilePage.qml            # 本地文件
│   └── DownloadPage.qml        # 下载管理
├── resources/                  # 资源文件
│   ├── app/                    # 应用图标、图片
│   ├── fonts/                  # 字体（Poppins, Feather Icons）
│   ├── window-bar/             # 窗口按钮图标
│   ├── pic/                    # 背景图片
│   └── app/shaders/            # GLSL 着色器
├── shaders/                    # 着色器文件
├── ThirdParty/
│   └── qwindowkit/             # Git Submodule — 无边框窗口框架
├── centers/                    # 存放沉浸中心相关QML组件文件
├── .gitignore
├── .gitattributes
├── .gitmodules
├── LICENSE                     # Apache License 2.0
└── README.md
```

---

## 🎮 未来计划

- **首要-功能完善**：补全设置、编辑、歌单管理等功能，增强稳定性
- **音源插件化**：核心播放器与在线音源解耦，音源作为可插拔模块由使用者自行配置与负责
- **品牌统一性**: 在名称以及宣传上计划使用一个新的名称或定义，统一形象
- **推动发展**: 后面计划推出QueMusic网站，建立QQ群，与社区共建生态
- **优化性能**：持续优化内存 & GPU 占用，解决性能瓶颈
- **加入沉浸播放**：参考 Folia / MineRadio 概念，引入 3D 可视化与高度自定义歌词
- **UI 强化**：继续打磨自研 QML 组件库，统一设计语言
- **国际化**：可选计划，由于使用国内音乐平台，不一定更新
- **更多**：自定义插件系统,自定义主题UI插件系统

即使不断更新，QueMusic开发者始终保持开源，免费，没有付费内容，保持完全的免费，但是对于歌曲版权方面，请自费购买平台VIP或付费歌曲，登录平台账号进行收听（即将更新）。

---

## 🤝 贡献指南

欢迎任何形式的贡献！

| 方式 | 说明 |
|------|------|
| 🐛 **报告 Bug** | 提交 [Issue](https://github.com/bronekox/quemusic/issues)，附上复现步骤和环境 |
| 💡 **提出新功能** | 在 [Discussion](https://github.com/bronekox/quemusic/discussions) 中发起讨论 |
| ⭐ **Star** | 点亮 GitHub Star，支持持续开发 |
| 🧪 **测试** | 构建并试用，反馈兼容性问题 |
| 🔧 **Pull Request** | 修复 Bug、优化代码、完善功能 —— **欢迎任何人** |
| 📖 **利用** | 基于Apache-2.0 协议，开发者欢迎任何项目使用本项目的代码 |

### 开发流程

1. Fork 本仓库
2. 创建功能分支：`git checkout -b feat/your-feature`
3. 提交修改：`git commit -m "feat: add xxx"`
4. 推送：`git push origin feat/your-feature`
5. 发起 Pull Request

> 代码风格请参考现有文件，遵循 **C++17 / Qt6 / QML best practices**。

---

## 📄 许可证

本项目主体遵循 **Apache License 2.0** —— 欢迎自由使用、修改、分发，甚至商用（需保留版权声明与许可证副本）。

```
Apache License
Version 2.0, January 2004
Copyright (c) 2025-2026 QueMusic Contributors
```

> 💡 **Apache-2.0 要点**：允许商用、修改、分发；需在衍生作品中保留原始版权声明与 NOTICE；对专利授权有明确条款，为用户提供额外保护。

---

## 🎨 背景着色器变更

在 **Beta 0.5.0** 之前的版本使用了 [AMLL-Core(Apple Music Like Lyrics)](https://github.com/amll-dev/applemusic-like-lyrics) 项目的背景着色器并移植到Qt当中

开发者深知该做法可能触碰到了许可证边界，虽然相对妥协做出较合规的操作，但为了进一步优秀发展，让QueMusic变得更好，遂做出更改。

**Beta 0.5.0** 之后的版本将使用基于自主框架的背景着色器，并新增多个背景着色器选择，用于代替之前使用的AMLL背景着色器：

- **Fluid**： 使用了 [Paper-design/shaders](https://github.com/paper-design/shaders) 的着色器，并移植到Qt当中，该项目使用Apache License 2.0协议，因此本项目使用了该项目，这完全合规。
- **Classic**： 该着色器为自主设置，尽可能靠拢AMLL的效果。参考了AMLL的实现方法，但算法、代码实现、数值均未直接抄袭于照搬AMLL，算法均为公开的算法。

> **Classic** 的着色器算法大致思路：
> 切换封面：封面>高斯模糊>后处理（亮度/饱和）>做为背景贴图源；
> 封面渲染：贴图源（source）>多个重复取样>旋转=>3d变形背景>后处理(暗角，抖动，噪声等)>组件显示区域

由于着色器变更，效果可能不如 **0.5.0** 往前的版本，QueMusic将会持续优化背景效果，使得效果再进一步更华丽。

---


## 📢 免责声明

### 1. 音乐版权

本项目中的所有音乐内容（歌曲、歌词、封面等）版权均归其原始权利人所有。本项目**不提供、不存储、不缓存**任何音乐文件，所有播放内容均来自用户自行选择的第三方公开网络服务。

### 2. 在线服务接口

- 本项目仅调用各音乐平台对外**公开**的接口，**不包含任何破解、绕过付费、解锁 VIP、盗取音源等行为**；
- 不提供任何付费内容的非法获取途径，也无法播放需要单独授权的加密内容；
- 各平台接口可能随时调整或失效，本项目不对接口的可用性与稳定性作任何保证。

### 3. 商标与品牌

本项目中出现的所有商标、产品名称、服务名称均为其各自所有者的财产，仅用于描述兼容性，不代表任何官方授权、认可或关联。

### 4. 使用者责任

使用者应遵守所在地法律法规以及各第三方平台的服务条款。因使用本项目而产生的任何直接或间接后果，由使用者自行承担，项目开发者不承担任何责任。

### 5. 无担保

本项目按 **"现状"（AS-IS）** 提供，不附带任何明示或暗示的担保。详细免责条款请参阅 [LICENSE](LICENSE) 文件。

---

## 🙏 致谢

- [Qt Project](https://www.qt.io/) — 提供强大的跨平台框架
- [QWindowKit](https://github.com/stdware/qwindowkit) — 无边框窗口解决方案
- [qiuliw/Qt6_QWindowKit_QML_demo](https://github.com/qiuliw/Qt6_QWindowKit_QML_demo) — 项目框架参考
- [QCloudMusicApi](https://github.com/s12mmm3/QCloudMusicApi) — 使用了本项目api服务，以实现在线音乐网易云音乐平台部分
- [Cryptopp](https://github.com/weidai11/cryptopp) — 用于QCloudMusicApi解析
- [libqrencode](https://github.com/weidai11/cryptopp) — 用于QCloudMusicApi
- [Taglib](https://github.com/taglib/taglib) — 用于解析本地音乐部分数据
- [SMTC-Bridge-Cpp](https://github.com/Cainongw/SMTC-Bridge-Cpp) — Windows 系统媒体控件(SMTC)桥接参考实现(C++)
- [smtc_bridge_rust](https://github.com/Cainongw/smtc_bridge_rust) — Windows 系统媒体控件(SMTC)桥接参考实现(Rust)
- [Paper-design/shaders](https://github.com/paper-design/shaders) — 背景着色器"Fluid"的算法移植

#### 其他对于本项目有帮助的

- [EvolveUI](https://evolveui.top/) — 部分组件设计参考
- [AMLL-Core(Apple Music Like Lyrics)](https://github.com/amll-dev/applemusic-like-lyrics) — 背景着色器的实现方法参考
- [ShaderToy](https://www.shadertoy.com/) — 着色器灵感来源
- 所有贡献者与测试者

---

## QueMusic Pro 高级版

### 不可能存在的喵！！！

QueMusic 官方版本始终保持开源与永久免费，没有任何Pro、Ultra、高级版、捐献版等版本，QueMusic也没有任何付费、会员、捐献、充值、赞助内容，如果你发现你的QueMusic需要钱或者QueMusic内存在付费项目，请立即向开发者告知。

---

## 联系开发者

- QQ：241422517
- 邮箱：uihugd@outlook.com
- Bilibili: 695207057
- QQ群：1105114511

---

<p align="center">
  <sub>Written for QueMusic Project</sub><br/>
  <sub>最后更新：2026-9-12</sub>
</p>

