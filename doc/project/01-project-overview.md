# 第 1 部分 · 项目身份与全局地图

> 本文件是 [QueMusic 项目全景文档索引](README.md) 的一个分片；任务定位与其它部分请见 [README.md](README.md)。

## 1.1 项目身份卡

| 项 | 值 |
|---|---|
| 名称 | QueMusic（project / executable / target 名均为 `QueMusic`） |
| 定位 | C++20 + Qt 6 + QML 原生跨平台桌面音乐播放器（非 Web 套壳） |
| 版本 | `0.6.0`（`versionCode 60`），QML 侧见 `components/Options.qml` |
| 插件 API 版本 | `10`（`Options.pluginApi`）；插件清单 `info.json` 里写的是 `apiVersion: 1`（两者语义不同，见第 2 部分） |
| 许可证 | Apache-2.0（`LICENSE` + `NOTICE` + `THIRD_PARTY_NOTICES.md`） |
| 组织 / 应用标识 | `BroNekoX` / `com.bronekox.quemusic`（`main.cpp` 设置） |
| Qt 版本 | 6.10.3（`CMakePresets.json` 硬编码 `C:/Qt/6.10.3/llvm-mingw_64`） |
| C++ 标准 | C++20（`CMAKE_CXX_STANDARD 20`） |
| 最低 CMake | 根 `CMakeLists.txt` 写 3.16，`CMakePresets.json`（version 6）实际需要 ≥3.24 |
| 平台 | Windows（主力）/ Linux / macOS |
| QML 模块 URI | `QueMusic`，版本 `1.0`（`qt6_add_qml_module`） |
| 本地构建目录 | `build/Desktop_Qt_6_10_3_llvm_mingw_64_bit_Release`（Qt Creator 生成，不是 preset 目录 `build/<preset>`） |
| 当前 git 分支 | `main` |

### 顶层目录 → 职责速查

| 路径 | 职责 | 详见 |
|---|---|---|
| `main.cpp` | C++ 入口：环境变量、QApplication、注册 QML 类型与单例 | §1.2 |
| `main.qml` | QML 主入口：窗口、全局对象装配、页面导航 | 第 5 部分 |
| `SettingsView.qml` | 设置页（全项目最大的单个 QML 文件） | 第 5 部分 |
| `cpp/` | C++ 后端：模型、数据库、下载、扫描、歌词读取、账号、日志、托盘、SMTC | 第 2 部分 |
| `cpp/audio/` | 自研音频引擎：FFmpeg 解码 + 无锁环形缓冲 + DSP + QAudioSink | 第 3 部分 |
| `cpp/webdav/` | WebDAV 在线曲库（目录浏览 / 加密凭据 / 后台缓存） | 第 2 部分 |
| `cpp/plugins/` | 插件扫描与注册（歌词界面插件 + 功能插件） | 第 2 部分 |
| `api/` | 在线媒体层：网易云 / 酷狗 / B站 统一接入 + 调度中枢 `MusicApi` | 第 4 部分 |
| `components/` | 自研 QML 组件库 + 全局单例（Style/Theme/Options/Config/Playback/ShortCut/PluginSlots/Fonts） | 第 6 部分 |
| `layout/` | 外壳布局：侧栏、内容容器、播放控制栏、歌词视图、窗口按钮、SMTC 桥 | 第 5 部分 |
| `pages/` | 普通模式页面（首页/搜索/歌单/收藏/本地文件/下载/WebDAV） | 第 5 部分 |
| `lyricsui/` | 可整体替换的歌词界面预设（Free / 3D），同时是插件契约的参考实现 | 第 6 部分 |
| `shaders/` | GLSL 着色器 + 2 个自定义 QQuickItem（`MeshGradientItem`、`LyricsStage`） | 第 3 部分 |
| `resources/` | 图标、字体、背景图、窗口按钮 SVG（在 `CMakeLists.txt` 中逐条列为 `RESOURCES`） | 第 7 部分 |
| `cmake/` | 第三方依赖集成（`external/*.cmake`）、qmltc、运行时部署、FFmpeg 拉取脚本 | 第 7 部分 |
| `packaging/` | 各平台打包脚本（AppImage / tar / PKGBUILD / entitlements / .desktop） | 第 7 部分 |
| `tests/` | `smoke-stderr.sh`（启动冒烟测试，**没有**单元测试框架） | 第 7 部分 |
| `.github/workflows/` | 7 个 workflow（CI + 三平台构建 + release） | 第 7 部分 |
| `ThirdParty/` | git 子模块：`qwindowkit`、`taglib` | 第 7 部分 |
| `api/QCloudMusicApi/` | 内嵌第三方库：网易云接口实现（含 `3rdparty/cryptopp`、`libqrencode`） | 第 4、7 部分 |
| `ffmpeg/` | 本地 FFmpeg 开发库（**被 .gitignore**，Windows 用脚本拉取；缺失时 CMake 直接 `FATAL_ERROR`） | 第 7 部分 |
| `doc/` | 本文档 + macOS 安装说明 + 数据库诊断脚本 | — |
| `build/` | 构建产物 | 第 7 部分 |

---

## 1.2 启动链路（`main.cpp` → `main.qml`）

`main.cpp` 的执行顺序本身有约束，改错会**静默失效**：

1. **Windows 专属** `registerSmtcAppIdentity()`：`SetCurrentProcessExplicitAppUserModelID("BroNekoX.QueMusic")` → 写注册表 `HKCU\Software\Classes\AppUserModelId\BroNekoX.QueMusic` → 建开始菜单 `QueMusic.lnk` 并写入 `PKEY_AppUserModel_ID`。**必须早于任何 UI**。
2. 读 `ConfigLocation/BroNekoX/QueMusic.ini`，按 `Options/gpuRenderMode`（0 默认 / 1 opengl / 2 vulkan / 3 software）、`Options/timerAnimator`、`Options/qmlAnimator` **在 `QApplication` 构造前** `qputenv`。
   → 这几个渲染开关**改完必须重启进程**（不是重启窗口）。
3. `QApplication`（**不是** `QGuiApplication`，因为托盘用了 QtWidgets）；`setQuitOnLastWindowClosed(false)` → 常驻托盘，退出统一走 QML 的 `Qt.quit()`。
4. `QQuickWindow::setDefaultAlphaBuffer(true)`；构造 `QQmlApplicationEngine`；调用 `qml_register_types_QueMusic()`（由 `qt6_add_qml_module` 生成，注册所有 `QML_ELEMENT`）；`engine.addImportPath("qrc:/")`。
5. 元信息：organization `BroNekoX`、domain `com.bronekox.quemusic`、applicationName `QueMusic`、windowIcon `:/QueMusic/resources/icon.ico`（**必须 `:/` 前缀，`qrc:/` 会加载失败**）。
6. `QSettings::setDefaultFormat(IniFormat)` + `setPath(IniFormat, UserScope, configPath + "/BroNekoX/QueMusic.ini")`。

   **实测落盘位置（2026-10-11 本机核对）**：

   | 文件 | 大小 / mtime | 状态 |
   |---|---|---|
   | `%LOCALAPPDATA%\BroNekoX\QueMusic.ini` | 55 KB / 2026-10-10 | ✅ **真正生效的主配置**：`[Options] [Style] [ShortCuts] [Player] [AudioDsp] [Lyrics3D] [LyricsFree] [Plugins] [webdav] [LastMedia]` |
   | `%APPDATA%\BroNekoX\QueMusic.ini` | 1.4 KB / 2026-09-20 | ⚠️ 旧版本遗留，只有 `[LastMedia]` |
   | `%LOCALAPPDATA%\BroNekoX\QueMusic\BroNekoX\QueMusic.ini` | 41 B | ⚠️ 早期构建遗留（`[Options] logEnabled/logLevel`） |

   运行时数据在 **Roaming**，与配置**不在同一目录**：

   | 路径 | 内容 |
   |---|---|
   | `%APPDATA%\BroNekoX\QueMusic\player_data.db` | SQLite 曲库 / 收藏（含 `-wal` / `-shm`） |
   | `%APPDATA%\BroNekoX\QueMusic\cache\` | 封面缓存 `cover-<sha1>.jpg`、`cover-<sha1>-64.jpg` |
   | `%APPDATA%\BroNekoX\QueMusic\logs\QueMusic_YYYYMMDD.log` | 按天日志 |
   | `%APPDATA%\BroNekoX\QueMusic\plugins\{lyrics,function}\<插件名>\` | 已安装插件 |

   > 排查"设置不生效 / 收藏丢失"时优先看 `%LOCALAPPDATA%\...\QueMusic.ini`（配置）与 `%APPDATA%\BroNekoX\QueMusic\player_data.db`（数据）。注意 `DbService` 在 `AppDataLocation` 不可写时会**回退到 `<exe目录>/data` 并整库迁移**，此时配置与数据会分家。

7. `LogManager::create()`、`AccountManager::create()`（QML 单例工厂，签名 `create(QQmlEngine*, QJSEngine*)`）。
8. `MusicApiService::setSharedAccountManager(accountManager)` —— 在线 API 单例拿到账号单例。
9. `engine.rootContext()->setContextProperty("configDir", configPath)` —— **全项目唯一一个 context property**。
10. `QWK::registerTypes(&engine)`；`engine.load("qrc:/QueMusic/main.qml")`。

> 约定：除 `configDir` 外**一律用 QML 单例而非 context property**。原因写在 `cpp/AppModels.h` 顶部注释里：context property 无法被 `qmlcachegen` 编译期解析，只有按类型名访问才能编译；也不能把多个模型塞进"一个单例的多个属性"，那样会拒绝继续属性查找。
>
> ⚠️ 单例工厂写法陷阱（`cpp/AppModels.h`）：构造函数**绝不能带默认参数**。`singletonConstructionMode()` 里 `is_default_constructible` 优先于 `HasSingletonFactory`，写成 `X(QObject *parent = nullptr)` 会走默认构造、`create()` 永不执行，表现为过滤类型失效、收藏列表列出全部数据。

---

## 1.3 分层与依赖方向

```
QML 层        main.qml / SettingsView.qml / layout/ / pages/ / components/ / lyricsui/
              │ 只做呈现与交互；通过 QML 单例名访问后端（编译期可解析）
              ▼
编排层        api/MusicApiService (MusicApi) · cpp/audio/AudioEngine · cpp/plugins/*
              │ 统一动作签名、分发、降级、线程调度
              ▼
能力层        api/{NeteaseCloudApi,KugouApi,BilibiliApi} · cpp/{FolderModel,Favorites,
              DownloadManager,LocalMusicScanner,LocalLyricsReader,CoverHelper,
              ColorExtractor,AccountManager,WebDav*,WindowsSmtcManager,LogManager}
              ▼
基础设施      cpp/DbService(SQLite) · cpp/audio/FfmpegDecoder(FFmpeg) · TagLib · QNetwork
```

依赖方向**单向向下**，没有反向 include；QML 永远不直接碰 `DbService` / `FfmpegDecoder`（实测 QML 里 `DbService`、`PlayerDatabase` 出现 0 次）。

### 三条主数据流

| 流程 | 链路 |
|---|---|
| **本地播放** | `LocalMusicScanner` 扫描 → `DbService`/`PlayerDatabase` 入库 → `FolderModel`/`MyFolders`/`LocalFolders` 暴露给 QML → 选中后 `AudioEngine.source` → 解码线程读 TagLib/FFmpeg → 环形缓冲 → DSP → QAudioSink |
| **在线播放** | QML 调 `MusicApi.<action>()` → `MusicApiService` 按 `source` 分派给平台类 → 平台返回 → `ApiCommon` 归一化字段 → 灌入 `OnlineListModel` → `resultReady(action, data, source)` 信号 → QML 取播放地址 → 交给 `AudioEngine`（网络 URL）或 `MusicApi.downloader` |
| **外设同步** | `AudioEngine` 状态 → `layout/SmtcBridge.qml` → `WindowsSmtcManager`（Windows 系统媒体控件）/ `SystemTrayManager`（托盘）/ `components/DesktopLyrics.qml`（桌面歌词） |

---

## 1.4 全局 QML 单例速查（改代码时最常需要的那几个）

| 单例 | 文件 | 类别 / 存储 | 作用 |
|---|---|---|---|
| `Config` | `components/Config.qml` | `Settings { category: "Options" }` | **所有用户可调配置项**（INI `Options/*`） |
| `Style` | `components/Style.qml` | `Settings { category: "Style" }` | 主题 / 字号 / 圆角 / 模糊 / 歌词界面选项（INI `Style/*`） |
| `Theme` | `components/Theme.qml` | `QtObject`（**不持久化**） | 由 `Options.changeTheme()` 实时算出的色板 |
| `Options` | `components/Options.qml` | `QtObject` | 运行时共享状态 + 跨文件对象注入点 + 信号总线 |
| `Playback` | `components/Playback.qml` | `QtObject` 单例 | 播放状态机 / 队列 / 切歌 / 淡变 / AB 循环 / 睡眠定时 / 播放历史 |
| `ShortCut` | `components/ShortCut.qml` | `Settings { category: "ShortCuts" }` | 快捷键定义与"是否全局快捷键"开关 |
| `PluginSlots` | `components/PluginSlots.qml` | `QtObject` 单例 | 扩展点注册表（`titlebar` / `player` / `sidebar.bottom` / `window.overlay`） |
| `Fonts` | `components/Fonts.qml` | 单例 | 加载 `resources/fonts/*.ttf` |
| `LyricsFreeConfig` | `lyricsui/LyricsFreeConfig.qml` | `Settings { category: "LyricsFree" }` | 自由歌词界面的自有设置 |
| `Lyrics3DConfig` | `lyricsui/Lyrics3DConfig.qml` | `Settings { category: "Lyrics3D" }` | 3D 歌词界面的自有设置 |

C++ 侧单例（详见第 2、3、4 部分）：`MusicApi`、`AccountManager`、`LogManager`、`WebDav`、`WebDavCache`、`LyricsPlugins`、`FunctionPlugins`、`MyFolders`、`LocalFolders`、`Songs`、`FavoriteSongs`、`FavoritePlaylists`、`FavoriteArtists`。

> `Config` 存"用户设置"，`Style` 存"外观设置"，`Theme` 是**派生只读**结果 —— 不要往 `Theme` 写值。
> `Options` 同时是**注入容器**：`main.qml` 启动时把 `queue` / `dialog` / `desktop` / `smtc` / `getWave` / `coverHelper` / 各种 Loader 与 Animation 赋给它，其它 QML 通过 `Options.xxx` 反查。**新增全局对象时必须在这里登记**（完整清单见第 5 部分）。

---

## 1.5 版本与发布事实

- `Options.version = "0.6.0"`，`Options.versionCode = 60`；`doc/updater.txt` 第一行是 `060 Beta-0.6.0` + 更新日志（供更新器读取，**GBK 编码**）。
- `CMakeLists.txt` 里 `project(QueMusic VERSION 0.1 ...)` —— 与 QML 侧 0.6.0 **不一致**；发布相关逻辑以 QML 侧为准。
- 更新日志正文在 `CHANGELOG.md`；README 有功能矩阵、性能实测（i5-12400 + UHD 730，1080P/60FPS 歌词页：CPU ≈4%、GPU ≈15%）与免责声明。
- README 里的 `centers/` 目录、`components/MusicApi.qml`、`FullCenterView.qml` **在当前代码树中并不存在**（README 已过期，实际沉浸中心是 `layout/PlayerMaxCenter.qml`）。以代码树为准。

---

## 1.6 本文档的使用方式

1. 先看 §0 的任务索引，用关键词定位到"任务 → 文件"，再只读那一部分/那些文件。
2. 改 QML 前先看第 6 部分的组件目录与约定；改 C++ 前先看第 2 / 3 / 4 部分的**接口表**，不要先读 `.cpp` 实现。
3. 各部分末尾的「坑与约定」是从源码注释里提取的硬约束。**违反它们通常不会编译报错，而是运行期静默失效**。
4. 行数标注用于判断成本：>500 行的文件（`SettingsView.qml` 2934、`FilePage.qml` 1985、`main.qml` 1068、`LyricsFree.qml` 1006、`HomePage.qml` 944 等）应使用 grep 定位而不是整读，具体上限清单见 §0.9。
---

← 返回 [索引 README.md](README.md)
