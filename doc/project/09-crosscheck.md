# 第 9 部分 · 交叉核对与勘误（独立验证）

> 本文件是 [QueMusic 项目全景文档索引](README.md) 的一个分片；任务定位与其它部分请见 [README.md](README.md)。

> 本部分由文档生成时的**独立验证**得到（grep 实际引用位置 + 读取运行时产物），用于校正第 1–8 部分里可能存在的偏差，以及纠正仓库文档（README）里的过期描述。

## 9.1 QML 中实例化 C++ 非单例类型的精确位置

| C++ 类型 | QML 实例化位置 | 备注 |
|---|---|---|
| `ColorExtractor` | `main.qml:537` | 全局唯一实例 |
| `CoverHelper` | `main.qml:689` | 全局唯一实例（`Options.coverHelper` 也指向它） |
| `GetWave` | `main.qml:788` | 频谱数据源，`Options.getWave` 指向它 |
| `AudioEngine` | `main.qml:796` | **整个应用唯一的播放引擎实例**，由 `Playback.player` 持有 |
| `SystemTrayManager` | `main.qml:875` | 托盘 |
| `QueueModel` | `main.qml:905` | 播放队列，`Options.queue` / `Playback.queue` 指向它 |
| `MeshGradientItem` | `layout/MainLyric.qml:94`、`lyricsui/LyricsFree.qml:100` | 两处各一个（歌词背景） |
| `CustomFlow` | `layout/MainLyric.qml:496`、`lyricsui/LyricsFree.qml:571` | 同上 |
| `LyricsStage` | `lyricsui/Lyrics3D.qml:153` | 仅 3D 歌词界面 |
| `DownloadedMusicModel` | `pages/DownloadPage.qml:14` | 已下载列表 |
| `WebDavModel` | `pages/WebDavPage.qml:20`（`id: browser`） | WebDAV 目录浏览 |
| `LocalMusicScanner` | `pages/FilePage.qml:530` | 本地扫描 |
| `WindowsSmtcManager` | `layout/SmtcBridge.qml:69`（`property WindowsSmtcManager mgr`） | 仅 Windows 生效 |

**不通过 QML 实例化的类型**：

| 类型 | 实际创建者 | 依据 |
|---|---|---|
| `SearchResultModel` | `cpp/FolderModel.cpp:260` 与 `cpp/LocalMusicScanner.cpp:193` 各自 `new`，**role 子集不同**（前者 `SongIdRole…`，后者 `FileNameRole…`） | `QSortModel.qml` 注释：`SearchResultModel` 用 `getRow(row)`，而 `Favorites`/`SongModel` 用 `get(row)` |
| `DownloadManager` | `MusicApiService` 的成员 `m_downloader`，以 `MusicApi.downloader`（`Q_PROPERTY(... CONSTANT)`）暴露；`pages/DownloadPage.qml` 里 `MusicApi.downloader` 出现 9 次 | `api/MusicApiService.h` |
| `DbService`、`PlayerDatabase` | 纯 C++ 内部使用 | 全仓 QML 里出现次数为 **0** |

## 9.2 QML → C++ 单例引用热力表

> 数字是该名字在对应 QML 文件里出现的次数；用于判断"改这个 C++ 类会波及哪些界面"。

| C++ 单例 / 类型 | 被哪些 QML 引用（次数） |
|---|---|
| `MusicApi` | `PlaylistPage(80)`、`HomePage(61)`、`SearchPage(43)`、`main.qml(18)`、`FavouritePage(11)`、`DownloadPage(10)`、`PlayerMaxCenter(8)`、`MusicInfo(7)`、`FilePage(7)`、`Playback(6)`、`SettingsView(6)`、`PlayerControl(5)`、`PlayListWindow(5)`、`DesktopLyrics(3)`、`QListView(3)`、`LeftSideBar(2)`、`AnimatorWindow(1)`、`Config(1)`、`QLoadSign(1)`、`QLocalView(1)` |
| `AudioEngine` | `main.qml(5)`、`SmtcBridge(4)`、`DesktopLyrics(1)`、`DesktopPlayerWindow(1)`、`EqualizerPanel(1)`、`MusicInfo(1)`、`Playback(1)`、`PlayerOptions(1)`、`PlayerControl(1)`、`PlayerMaxCenter(1)`、`FilePage(1)`、`WebDavPage(1)`、`SettingsView(1)` |
| `AccountManager` | 仅 `SettingsView(38)` —— **登录态只在设置页操作** |
| `LogManager` | 仅 `SettingsView(6)` |
| `WebDav`（`WebDavStore`） | `WebDavPage(14)`、`Playback(5)`、`FilePage(4)`、`main.qml(1)`、`PlayList(1)` |
| `WebDavCache` | `Playback(2)`、`main.qml(1)` |
| `LyricsPlugins` | `PlayerMaxCenter(5)`、`SettingsView(1)` |
| `FunctionPlugins` | `PluginHost(3)`、`SettingsView(1)` |
| `Songs` | `FilePage(22)`、`FavouritePage(2)`、`SettingsView(1)` |
| `MyFolders` / `LocalFolders` | 各仅 `FilePage(5)` |
| `FavoriteSongs` | `PlayerControl(7)`、`FavouritePage(7)`、`PlayList(3)`、`QListView(3)` |
| `FavoritePlaylists` | `FavouritePage(5)`、`PlayListWindow(4)`、`HomePage(1)`、`QListView(1)` |
| `FavoriteArtists` | **无任何 QML 引用**（已注册但未接线，属未完成/预留） |
| `PluginSlots` | `PluginApi(2)`、`PluginSlot(2)` |
| `SearchResultModel` | 仅 `QSortModel(1)`（作为注释/类型判断出现） |

## 9.3 勘误：README / 旧文档与代码树不一致的地方

| 说法 | 实际情况 |
|---|---|
| README 项目结构里的 `centers/` 目录 | **不存在**；沉浸中心实现是 `layout/PlayerMaxCenter.qml` |
| README 里的 `components/MusicApi.qml`（"在线音乐整合单例"） | **不存在**；`MusicApi` 是 C++ 单例 `api/MusicApiService.h`（`QML_NAMED_ELEMENT(MusicApi)`） |
| README 里的 `FullCenterView.qml` | **不存在**（仓库根目录没有该文件）；旧版本遗留描述 |
| README 说"歌词视图 `layout/MainLyric.qml`" | 正确 |
| README 未提及 `cpp/webdav/`、`cpp/plugins/`、`pages/WebDavPage.qml`、`lyricsui/` 与插件系统 | 这些是新功能，README 的目录树尚未更新；**以第 2、5、6 部分为准** |
| `CMakeLists.txt` 的 `project(QueMusic VERSION 0.1)` | QML 侧实际版本是 `0.6.0`；不要据 CMake 版本号判断应用版本 |
| 插件 `info.json` 的 `apiVersion: 1` | 与宿主 `Options.pluginApi = 10` 不是同一套编号；判断宿主 API 版本要看 `Options.pluginApi` |
| README 说"`build/<preset>`" | 本机实际构建目录是 `build/Desktop_Qt_6_10_3_llvm_mingw_64_bit_Release`（Qt Creator 生成）；preset 构建会用 `build/<presetName>` |

## 9.4 死代码 / 未完成项 / 文档与实现不一致清单（节省后续排查时间）

> 这些是各部分独立分析时共同发现的可疑点。**在它们身上花时间之前先看这里**。标「未确认」的需要再验证一次。

| 项目 | 位置 | 现象 | 影响 |
|---|---|---|---|
| `musicControl` 快捷键分支 | `main.qml:182-190` | 引用不存在的 id `controlMaxLoader` / `minedAnimation` 与未定义的 `window.playermined()` | 该快捷键分支实际不可用（未确认） |
| `Config.displayDebugControl` | `components/Config.qml:63` | 无初值，全项目无读写 | 死键（但已出现在 INI 里） |
| `Config.scanTime` / `dataUrl` / `metaDataSource` / `serverAgency` / `editSource` / `softwareRender` | `components/Config.qml` | 声明但全仓库无读写（`softwareRender` 与 `Options/gpuRenderMode` 功能重叠） | 死键 |
| `FavoriteArtists` | `cpp/AppModels.h` | 已注册 `QML_SINGLETON`，QML 中**零引用** | 歌手收藏未接线；`FavouritePage.qml` 的「歌手」子页未使用它 |
| `QButton.tipText` | `components/QButton.qml` | 属性声明了但没有接 `QTip` | 设置无效 |
| `components/IconData.js` | 资源目录 | MDI 图标名单，全库零引用 | 死资源 |
| `IconData`/字体码位 | `resources/fonts/feather.ttf` | 文件名叫 feather，实际是 **FontAwesome 码位**（U+F000–U+F120，292 字形） | **不可直接替换字体文件**，否则所有图标消失 |
| `LyricsStage` 的 NOTIFY 信号 | `shaders/LyricsStage.h` | 9 个属性的 `<name>Changed()` 永不 emit（只有 `setPreset`/`setCount`/`setColor1`/`setColor2` 真 emit） | QML 里用 `onXxxChanged` 驱动逻辑会失效 |
| `AudioEngine::m_chunkFrames` | `cpp/audio/AudioEngine.h` | 死字段，只在初始化处赋值 | 无功能影响 |
| `FfmpegDecoder::readFrames` | `cpp/audio/FfmpegDecoder.cpp` | 头注释说会返回 `-1` 表示出错，实现从不返回 | 调用方若判断 `-1` 则该分支永不触发 |
| `libqrencode` | `api/QCloudMusicApi/3rdparty/libqrencode` | 目录存在，但 CMake 未构建、源码未引用（二维码实际走 `api/qrcode.js`） | 不要去找它的调用点 |
| qmltc | `cmake/qmltc.cmake` | 白名单 19 项里有 4 个文件已删除（`StyleThemes/StyleSettings/OptionsSettings/OptionsShortCuts.qml`），其余 15 个都在 `components/` 子目录 | `QUEMUSIC_ENABLE_QMLTC=ON` **实际永远不会启用**，只会 WARNING 后降级 |
| `qt_deploy_runtime()` 的 APPLE 分支 | `cmake/qtruntime.cmake` | 只在非 Apple 非 Debug 调用；macOS 部署由 `build-macos.yml` 自己做 | APPLE 分支是死代码 |
| Windows 打包脚本 | 仓库内 | **没有** Windows 打包脚本，也**没有任何 Inno Setup `.iss` 文件**（`packaging/README` 与 README 提到的 `QueMusic-setup.exe` 属未实现） | 想在本地复现安装包是做不到的 |
| Windows 单元测试 | 全仓库 | 无 CTest / QTest / `add_test` | 「跑测试」只有 `tests/smoke-stderr.sh`（12s offscreen 启动冒烟） |
| 行数口径 | 文档生成过程中出现过 `Get-Content` 统计 | 会漏空行 | 全文已统一为 `ReadAllLines`，见 §9.6 |
| `Options.exitIndex` 注释 | `components/Options.qml:30` | 注释写「要求主界面切到的页」，实际是**返回层级计数器** | 按注释理解会写出错误逻辑，见第 5 部分 §5.3.4 |
| `pages/WebDavPage.qml` | 不在 `layout/MainContent.qml` 的 `pages` 列表里 | 由 `pages/FilePage.qml:292` 作为子页内嵌 | 侧栏到不了 WebDAV，要经本地文件页 |
| `windowsConfigPath` / 多份 INI | 文件系统 | `%APPDATA%\BroNekoX\QueMusic.ini` 与 `%LOCALAPPDATA%\BroNekoX\QueMusic\BroNekoX\QueMusic.ini` 是遗留 | 只有 `%LOCALAPPDATA%\BroNekoX\QueMusic.ini` 生效，见第 8 部分 |
| `Account.ini` | `<exe 目录>\Account.ini` | 账号 Cookie 与**主配置分离**，不受 `QSettings::setPath` 影响 | 排查登录态要看这个文件，不在 `QueMusic.ini` 里 |

## 9.5 本部分相对第 1–8 部分的勘误汇总

| 第 1–8 部分可能的说法 | 更正 |
|---|---|
| 「`Options.exitIndex` 是页面下标」 | 是返回层级计数器；页面下标是 `MainContent.pageIndex`（0–5） |
| 「WebDAV 是一个导航页」 | 是 `FilePage` 的第 3 个子页 |
| 「`DownloadManager` 由 QML 实例化」 | 由 `MusicApiService` 持有，QML 经 `MusicApi.downloader` 访问 |
| 「`SearchResultModel` 由 QML 使用」 | 由 `FolderModel` / `LocalMusicScanner` 各自 `new`，role 子集不同 |
| 「`FavoriteArtists` 用于歌手收藏页」 | QML 中零引用，未接线 |
| 「`components/MusicApi.qml` 存在」 | 不存在；`MusicApi` 是 C++ 单例 |
| 「版本号 0.1」 | CMake 里的 `VERSION 0.1` 是遗留；应用版本是 `0.6.0` / `versionCode 60` |

## 9.6 统计口径说明（避免后续复核时对不上）

- 文档中所有行数用 `[System.IO.File]::ReadAllLines(path).Length` 统计，与编辑器和 `read` 工具显示的行号一致。
- **不要用** `(Get-Content path).Count`（会漏掉空行，`main.cpp` 会得出 157 而不是 160）或 `Measure-Object -Line`（同样漏空行）。
- 行数快照见附录 A；本仓库当前：C++（`cpp/` + `api/` + `shaders/` 的 `.cpp/.h`）≈ 28 200 行，QML（`components/` + `layout/` + `pages/` + `lyricsui/` + `main.qml` + `SettingsView.qml`）≈ 21 100 行，合计（含脚本与 CI）≈ 53 900 行。
- 第三方 vendored 代码（`api/QCloudMusicApi/`、`ThirdParty/`）不计入上述规模，也不在本文档的接口梳理范围内；其中 `api/QCloudMusicApi/QCloudMusicApi/module.cpp` 单文件就有 5157 行，**不要整读**。
---

← 返回 [索引 README.md](README.md)
