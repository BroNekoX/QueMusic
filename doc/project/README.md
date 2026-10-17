# QueMusic 项目全景文档（AI 快速解析用）

> 本文档的**唯一目的**：让后续任何一次对话在不重新通读源码的情况下，快速定位到需要改的文件、需要遵守的约定、以及容易踩的坑，从而节省 token 与时间。
>
> - 生成时间：2026-10-11；对应工作区 `quemusicproject`（git 分支 `main`）。
> - 全文分 9 个部分 + 附录 A。**章节编号是全局唯一的**（`§3.2` 只在第 3 部分出现），跨部分引用写「第 N 部分」。
> - 事实优先：凡是从代码/运行产物直接读出的事实都写精确标识符；不确定的标为「未确认」。
> - 第 9 部分是**对第 1–8 部分的交叉核对与勘误**（含 README 过期项、QML→C++ 引用热力表）。如果某处描述与代码冲突，以第 9 部分和代码为准。
> - 更新方式：改动架构性内容（新增单例、新增顶层目录、改变启动链路、改配置分组）后请同步更新对应部分。
> - ⚠️ **本文件约 390 KB / 4100 行，不要整读**。正确用法：先读 §0（任务索引，约 12 KB），定位到「第 N 部分」后再只读那一部分（通常 1–2 个部分就够）。

# 第 0 部分 · 阅读指南与任务索引

## 阅读顺序建议

| 你的目标 | 读哪些部分 |
|---|---|
| 第一次接触项目，建立整体认知 | 第 1 部分（全部）→ 第 5 部分（只看导航模型） |
| 确认某段描述是否与代码一致 / 想知道谁引用了某个类 | 第 9 部分（交叉核对与勘误） |
| 改 UI / 加界面 | 第 6 部分（组件库）→ 第 5 部分（外壳与页面） |
| 改播放行为 / 音质 / DSP | 第 3 部分 → 第 4 部分（取流地址与音质降级） |
| 加/修在线平台接口 | 第 4 部分 |
| 改本地曲库 / 收藏 / 下载 / 歌词读取 | 第 2 部分 |
| 改配置项 / 排查"设置不生效" | 第 8 部分（实测）→ 第 7 部分（源码声明） |
| 改构建 / 打包 / CI | 第 7 部分 |
| 加插件 / 改插件契约 | 第 2 部分（插件子系统）+ 第 6 部分（歌词界面契约） |

---

## 0. 任务 → 文件 速查索引

> 用法：找到最接近的任务行，只打开列出的文件，不要全仓库搜索。

### 0.1 启动 / 全局装配 / 环境开关

| 任务 | 主要文件 |
|---|---|
| 改启动流程、注册单例、改渲染后端环境变量 | `main.cpp` |
| 改主窗口、全局对象装配、页面导航、托盘/桌面歌词接线 | `main.qml` |
| 新增"全局可见对象"（新对话框、新 Loader、新动画） | `components/Options.qml` + `main.qml`（在 main.qml 里赋值给 `Options.xxx`） |
| 新增 C++ 类型给 QML 用 | 目标类的 `.h`（加 `QML_ELEMENT` / `QML_SINGLETON`）→ 若不在 `CMakeLists.txt` 的 `SOURCES` 里则补上 |
| 新增数据模型单例 | `cpp/AppModels.h`（用 `QUEMUSIC_DECLARE_MODEL_SINGLETON` 宏） |
| 改应用图标 / 名称 / 组织名 / AppUserModelID | `main.cpp`、`icon.rc`、`resources/icon.ico` |

### 0.2 播放链路

| 任务 | 主要文件 |
|---|---|
| 播放/暂停/切歌/淡变/循环/随机/AB 循环/睡眠定时/播放历史 | `components/Playback.qml`（QML 侧中枢） |
| 解码、缓冲、输出设备、EQ/变速/变调/响度/限幅、seek、位置上报 | `cpp/audio/AudioEngine.{h,cpp}` |
| 具体 DSP 算法（10 段 EQ、混音、限幅） | `cpp/audio/AudioDsp.{h,cpp}` |
| 变速 / 变调算法 | `cpp/audio/TimeStretch.*`、`cpp/audio/PitchShifter.*` |
| FFmpeg 解码与元数据 | `cpp/audio/FfmpegDecoder.{h,cpp}` |
| 无锁环形缓冲 | `cpp/audio/AudioRing.h` |
| 频谱可视化数据源 | `cpp/audio/AudioSpectrumSink.h` + `cpp/GetWave.*` + `components/EqualizerPanel.qml` |
| 播放队列数据与持久化 | `cpp/QueueModel.{h,cpp}` + `Config.lastQueue/lastQueueIndex` |
| 播放控制栏 UI | `layout/PlayerControl.qml` |
| 播放器选项弹窗（音质/设备/倍速等） | `components/PlayerOptions.qml`、`components/EqualizerPanel.qml` |
| 桌面歌词窗口 | `components/DesktopLyrics.qml`、`components/DesktopPlayerWindow.qml` |
| Windows 系统媒体控件（SMTC） | `cpp/WindowsSmtcManager.{h,cpp}` + `layout/SmtcBridge.qml` |
| 系统托盘 | `cpp/SystemTrayManager.{h,cpp}` |

### 0.3 在线媒体

| 任务 | 主要文件 |
|---|---|
| QML 侧调用的在线接口入口（单例 `MusicApi`） | `api/MusicApiService.{h,cpp}` |
| 加新平台 / 改某平台实现 | `api/NeteaseCloudApi.*`、`api/KugouApi.*`、`api/BilibiliApi.*` |
| 统一字段映射、音质档位与降级 | `api/ApiCommon.h` + `api/MusicApiService.cpp` |
| 在线列表模型（搜索结果/歌单/排行榜渲染） | `api/OnlineListModel.{h,cpp}` + `components/QListView.qml` |
| HTTP 请求封装 | `api/ApiHttp.h` |
| 登录 / 二维码登录 / 账号态 | `cpp/AccountManager.{h,cpp}` + `components/QRCodeView.qml` + `api/qrcode.js` |
| 网易云底层（第三方，勿改） | `api/QCloudMusicApi/` |
| 在线音乐页面 | `pages/HomePage.qml`、`pages/SearchPage.qml`、`pages/PlaylistPage.qml` |

### 0.4 本地曲库与文件

| 任务 | 主要文件 |
|---|---|
| 本地扫描入库 | `cpp/LocalMusicScanner.{h,cpp}` |
| 曲库/歌单/收藏的数据库访问（线程化） | `cpp/DbService.{h,cpp}`、`cpp/PlayerDatabase.{h,cpp}` |
| 文件夹/歌单模型（`MyFolders`/`LocalFolders`/`Songs`） | `cpp/FolderModel.{h,cpp}` + `cpp/AppModels.h` |
| 收藏（`FavoriteSongs`/`FavoritePlaylists`/`FavoriteArtists`） | `cpp/Favorites.{h,cpp}` + `cpp/AppModels.h` |
| 本地歌词（`.lrc` / 内嵌 SYLT/USLT） | `cpp/LocalLyricsReader.{h,cpp}` |
| 封面抓取与磁盘缓存 | `cpp/CoverHelper.{h,cpp}` |
| 封面主色提取（自适应主题） | `cpp/ColorExtractor.{h,cpp}` |
| 下载管理 | `cpp/DownloadManager.{h,cpp}`、`cpp/DownloadedMusicModel.{h,cpp}` |
| 本地文件页 UI（约 1900 行，只 grep 定位） | `pages/FilePage.qml` |
| 收藏页 / 下载页 | `pages/FavouritePage.qml`、`pages/DownloadPage.qml` |

### 0.5 WebDAV

| 任务 | 主要文件 |
|---|---|
| WebDAV 服务器配置与凭据 | `cpp/webdav/WebDavStore.{h,cpp}`（`[webdav] servers`，DPAPI 加密） |
| WebDAV 目录浏览模型 | `cpp/webdav/WebDavModel.{h,cpp}` |
| 播放缓存（音频/歌词/封面，LRU） | `cpp/webdav/WebDavCache.{h,cpp}` |
| 低层 HTTP/WebDAV 协议 | `cpp/webdav/WebDavClient.{h,cpp}` |
| WebDAV 页面 UI | `pages/WebDavPage.qml` |

### 0.6 界面与视觉

| 任务 | 主要文件 |
|---|---|
| 全局配色（改主题色/明暗） | `components/Style.qml`（持久化）+ `components/Options.qml` 的 `changeTheme()` + `components/Theme.qml`（派生） |
| 加/改通用控件 | `components/Q*.qml`（见第 6 部分目录） |
| 主布局（侧栏/内容区） | `layout/LeftSideBar.qml`、`layout/MainContent.qml` |
| 歌词视图（常规模式） | `layout/MainLyric.qml` |
| 沉浸播放中心 | `layout/PlayerMaxCenter.qml` |
| 自由歌词界面 | `lyricsui/LyricsFree.qml` + `lyricsui/LyricsFreeConfig.qml` |
| 3D 歌词界面 | `lyricsui/Lyrics3D.qml` + `lyricsui/Lyrics3DConfig.qml` |
| 背景特效 / 模糊 / 歌词着色器 | `shaders/*.frag`、`shaders/*.vert` + `shaders/MeshGradientItem.*`、`shaders/LyricsStage.*` |
| 设置页（约 2900 行，只 grep 定位） | `SettingsView.qml` |
| 窗口无边框/按钮 | `ThirdParty/qwindowkit`、`layout/WindowControls.qml`、`components/QWKButton.qml` |

### 0.7 插件

| 任务 | 主要文件 |
|---|---|
| 插件扫描/安装/启停（宿主侧） | `cpp/plugins/PluginStore.{h,cpp}` |
| 歌词界面插件（单例 `LyricsPlugins`） | `cpp/plugins/LyricsPluginStore.{h,cpp}` |
| 功能插件（单例 `FunctionPlugins`） | `cpp/plugins/FunctionPluginStore.{h,cpp}` |
| 插件挂载 API / 宿主容器 | `components/PluginApi.qml`、`components/PluginHost.qml`、`components/PluginSlot.qml`、`components/PluginSlots.qml` |
| 插件管理界面 | `components/PluginManager.qml`（设置页内） |
| 插件目录与实例参考 | `%APPDATA%\BroNekoX\QueMusic\plugins\{lyrics,function}\<名字>\`（`info.json` + 入口 QML） |

### 0.8 构建 / 发布

| 任务 | 主要文件 |
|---|---|
| 加源文件 / 加 QML / 加资源 / 加着色器 | `CMakeLists.txt`（QML 按目录 glob，新增 QML 通常**不用改**；C++ 源文件**必须**加进 `SOURCES`；资源**必须**加进 `RESOURCES`） |
| 改第三方依赖集成 | `cmake/external/*.cmake` |
| 加构建预设 | `CMakePresets.json` |
| 改打包 | `packaging/*`（Linux）、`justfile` 的 `mac-bundle`、Inno Setup 在 Windows CI |
| 改 CI | `.github/workflows/*.yml` |
| 冒烟测试 | `tests/smoke-stderr.sh` |

---

### 0.9 全项目文件规模（用于判断"能不能整读"）

> 完整逐文件行数见**附录 A**；下列是必须**只 grep 不整读**的大文件。

（行数用 `[System.IO.File]::ReadAllLines()` 统计，与编辑器显示一致；`Get-Content .Count` 会漏掉空行，不要用。）

| 文件 | 行数 | 建议 |
|---|---|---|
| `api/QCloudMusicApi/QCloudMusicApi/module.cpp` | 5157 | **第三方，勿读勿改** |
| `SettingsView.qml` | 2934 | 只 grep 设置项名字 |
| `pages/FilePage.qml` | 1985 | 只 grep 控件 id |
| `api/KugouApi.cpp` | 1497 | 按 action 名 grep |
| `cpp/audio/AudioEngine.cpp` | 1338 | 先看第 3 部分 |
| `api/MusicApiService.cpp` | 1325 | 按 `action` 字符串 grep |
| `main.qml` | 1068 | 先看第 5 部分的导航表，再定点读 |
| `lyricsui/LyricsFree.qml` | 1006 | 只改具体特效时定点读 |
| `cpp/WindowsSmtcManager.cpp` | 977 | Windows COM，通常不用动 |
| `pages/HomePage.qml` | 944 | 同上 |
| `cpp/AccountManager.cpp` | 924 | 先看 `.h` 接口表 |
| `api/BilibiliApi.cpp` | 773 | — |
| `api/NeteaseCloudApi.cpp` | 720 | — |
| `layout/MainLyric.qml` | 718 | — |
| `cpp/FolderModel.cpp` | 713 | — |
| `pages/PlaylistPage.qml` | 690 | — |
| `layout/PlayerControl.qml` | 647 | — |
| `components/EqualizerPanel.qml` | 597 | — |
| `cpp/LocalMusicScanner.cpp` | 579 | — |
| `api/qrcode.js` | 514 | 二维码生成库，勿动 |
| `cpp/LocalLyricsReader.cpp` | 493 | — |
| `shaders/MeshGradientItem.cpp` | 493 | — |
| `components/QListView.qml` | 492 | 在线列表的核心控件 |

全项目源码规模（不含第三方 vendored 之外的部分）：

| 类别 | 行数 |
|---|---|
| C++（`cpp/` + `api/` + `shaders/` 的 `.cpp/.h`） | ≈ 28 200 |
| QML（`components/` + `pages/` + `layout/` + `lyricsui/` + `main.qml` + `SettingsView.qml`） | ≈ 21 100 |
| 合计（含脚本/CI） | ≈ 53 900 |

---

## 分片文件清单（按需只读一个文件即可）

| 文件 | 行数 | KB | 内容 |
|---|---:|---:|---|
| [`01-project-overview.md`](01-project-overview.md) | 162 | 12.9 | 第 1 部分 · 项目身份卡、顶层目录速查、启动链路、分层与三条数据流、全局单例速查、版本事实 |
| [`02-cpp-backend.md`](02-cpp-backend.md) | 499 | 47.4 | 第 2 部分 · `cpp/`（不含 audio）：数据库表与线程模型、15 个类的 QML 接口表、模型 roles、WebDAV、插件、坑 |
| [`03-audio-shaders.md`](03-audio-shaders.md) | 502 | 46.1 | 第 3 部分 · 音频引擎线程模型与 DSP 严格顺序、`AudioEngine` 44 个属性、解码层、着色器清单与 2 个自定义 Item、坑 |
| [`04-api-layer.md`](04-api-layer.md) | 622 | 61.6 | 第 4 部分 · `MusicApi` 28 个 Q_INVOKABLE + 30 个属性、action 字典、`ApiCommon` 字段与音质降级、三平台对比、坑 |
| [`05-qml-shell.md`](05-qml-shell.md) | 723 | 63 | 第 5 部分 · `main.qml` / `SettingsView.qml` / `layout/` / `pages/`：装配、导航模型、布局树、快捷键、QML↔C++ 耦合、坑 |
| [`06-components.md`](06-components.md) | 476 | 55.5 | 第 6 部分 · `components/` 61 个组件总表、10 个单例、可复用模式、歌词界面插件契约、字体资源、坑 |
| [`07-build-settings.md`](07-build-settings.md) | 547 | 56.8 | 第 7 部分 · CMake 选项、第三方依赖、构建命令、打包、CI、测试、设置键总表（约 185 键） |
| [`08-config-evidence.md`](08-config-evidence.md) | 63 | 6 | 第 8 部分 · 实际落盘 INI 的分组与键（运行时真值）+ 遗留键分析 |
| [`09-crosscheck.md`](09-crosscheck.md) | 115 | 10.9 | 第 9 部分 · QML→C++ 实例化位置与引用热力表、README 勘误、死代码/未完成项清单、统计口径 |
| [`10-favorites-folders.md`](10-favorites-folders.md) | — | — | 第 10 部分 · 歌曲收藏夹（多文件夹收藏）：folders(type='fav')、favorites.folder_id、默认夹迁移、FavoriteFoldersModel、收藏夹弹窗与调用点 |
| [`A-appendix-linecounts.md`](A-appendix-linecounts.md) | 227 | 10.5 | 附录 A · 216 个文件的行数与字节快照 |

> 分片共 11 个文件。各部分内部的章节编号是**全局唯一**的（§3.2 只在第 3 部分出现），跨文件引用一律写「第 N 部分」，因此拆分后原有交叉引用依然有效。

> ⚠️ **不要一次读完整套分片**（约 390 KB）。标准用法：读本文件（约 13 KB）→ 用 §0 的任务索引定位 → 只打开命中的那 1–2 个分片。

> 更新方式：改动架构性内容（新增单例、新增顶层目录、改变启动链路、改配置分组）后，请直接编辑对应分片文件。
