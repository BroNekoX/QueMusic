# 第 5 部分 · QML 外壳、布局与页面（main.qml、SettingsView.qml、layout/、pages/）

> 本文件是 [QueMusic 项目全景文档索引](README.md) 的一个分片；任务定位与其它部分请见 [README.md](README.md)。

> 覆盖范围：`main.cpp` → `main.qml`、`SettingsView.qml`、`layout/*.qml`（7 个）、`pages/*.qml`（7 个）。
> 行号以**当前工作区文件**为准（LF 计数）。本部分撰写过程中曾出现行数口径差异，现已统一：
> `main.qml` = **1068**、`SettingsView.qml` = **2934**、`pages/FilePage.qml` = **1985**（全量对照见附录 A）。

---

## 5.1 启动链路

### 5.1.1 `main.cpp`（160 行）执行顺序

| # | 动作 | 关键约束 |
|---|---|---|
| 1 | `registerSmtcAppIdentity()`（`#if defined(Q_OS_WIN)`） | 设置进程 AppUserModelID `BroNekoX.QueMusic`、写 `HKCU\Software\Classes\AppUserModelId\...`、建开始菜单 `.lnk` 并写 `PKEY_AppUserModel_ID`。**必须早于任何 UI** |
| 2 | 读 `ConfigLocation/BroNekoX/QueMusic.ini`，按 `Options/gpuRenderMode`(0默认/1 opengl/2 vulkan/3 software)、`Options/timerAnimator`、`Options/qmlAnimator` 调用 `qputenv` | 必须在 `QApplication` 构造前；改动需**重启进程**才生效 |
| 3 | `QApplication`（非 `QGuiApplication`，托盘用 QtWidgets）；`setQuitOnLastWindowClosed(false)` | 退出统一走 QML 侧 `Qt.quit()` |
| 4 | `QQuickWindow::setDefaultAlphaBuffer(true)`；`QQmlApplicationEngine engine` | — |
| 5 | `qml_register_types_QueMusic()`；`engine.addImportPath("qrc:/")` | 该函数由 `qt6_add_qml_module` 生成，注册全部 `QML_ELEMENT` |
| 6 | `setOrganizationName("BroNekoX")`、`setOrganizationDomain(...)`、`setWindowIcon(QIcon(":/QueMusic/resources/icon.ico"))`、`setApplicationName("QueMusic")` | QIcon 只认 `:/` 前缀，`qrc:/` 会失败 |
| 7 | `QSettings::setDefaultFormat(IniFormat)` + `setPath(UserScope, configPath)` | — |
| 8 | `LogManager::create(&engine,&engine)`；`AccountManager *accountManager = AccountManager::create(&engine,&engine)` | 两个 QML 单例工厂 |
| 9 | `MusicApiService::setSharedAccountManager(accountManager)` | 在线 API 单例拿账号单例 |
| 10 | `engine.rootContext()->setContextProperty("configDir", configPath)` | **全项目唯一 context property** |
| 11 | `QWK::registerTypes(&engine)`；`engine.load("qrc:/QueMusic/main.qml")` | — |

### 5.1.2 `main.qml` 根对象

```qml
pragma ComponentBehavior: Bound
import QtQuick / QueMusic 1.0 / QtCore / QtMultimedia / QWindowKit 1.0 / QtQuick.Controls.Basic
Window {                 // 不是 ApplicationWindow
    id: window
    width: 1140; height: 760; minimumWidth: 810; minimumHeight: 540
    color: Theme.primaryColor; title: "QueMusic"
}
```

- **窗口类型**：`Window`（QtQuick 原生），无 `ApplicationWindow`。
- **WindowKit**：`WindowAgent { id: windowAgent }`（`QWindowKit 1.0`）挂在 `Window` 直接子级；
  `titleBar` 在自身 `Component.onCompleted` 里 `windowAgent.setTitleBar(titleBar)`；
  每个需要点击的标题栏子项都要 `windowAgent.setHitTestVisible(item, true)`（插件挂进来的对象由 `window.syncTitleBarHitTest()` 补标）。
- **系统按钮**：`minButton`/`maxButton`/`closeButton` 各自 `windowAgent.setSystemButton(WindowAgent.Minimize/Maximize/Close, ...)`；macOS 下三者 `visible = false`。
- **属性**：macOS 跳过 `dark-mode`；`Config.noWindowKit` 时额外设 `extra-margins=3`、`title-bar-height=40`；`Style.backmode===4` 时动态设 `dwm-blur=true`。

### 5.1.3 `Component.onCompleted` 装配顺序（`main.qml:20-77`）

1. **注入 15 个全局对象到 `Options`**（见 §5.2.3）。
2. `windowAgent.setup(window)`；`if(!Options.isMacOS) windowAgent.setWindowAttribute("dark-mode", false)`；`if(Config.noWindowKit)` 设 `extra-margins` / `title-bar-height`。
3. `MusicApi.songSource = Config.mainMusicSource`、`MusicApi.downloadPath = Config.downloadFolder`、`MusicApi.soundQuality = Config.soundQuality`。
4. 恢复窗口几何：`if(Config.rememberWindow && Config.winW > 0)` → `window.x/y/width/height = Config.winX/winY/winW/winH`。
5. `window.visible = true`（在此之前窗口不可见，避免白屏/尺寸跳变）。
6. `Playback.player = mainMedia`；`Playback.queue = playListModel`（播放中枢的两根指针）。
7. `Options.changeUi()`、`Options.changeTheme()`（主题与 UI 初始化）。
8. `Qt.callLater(...)` 把磁盘/网络 I/O 移出首帧：`Playback.loadHistory()`、`Playback.restoreSession()`、`coverHelper.setCacheDir(Config.cacheUrl)`、`coverHelper.pruneCache(Config.cacheSize)`、`if(Config.autoUpdate) autoUpdateTimer.start()`。

另有两条常驻 `Binding`：`MusicApi.soundQuality ← Config.soundQuality`、`MusicApi.downloadPath ← Config.downloadFolder`（`main.qml:79-88`）。

### 5.1.4 全局有意义的 `id`（逐项作用见 §5.5）

| 分组 | id |
|---|---|
| 窗口/外壳 | `window`（根）、`windowAgent`、`titleBar`、`win`（`WindowControls`）、`mainLayout` |
| 布局/页面 | `sidebar`（`LeftSideBar`）、`mainContent`（`MainContent`）、`settingsView`（`Loader`） |
| 播放 | `musicControlMin`（`PlayerControl`）、`musicControlMax`（`PlayerMaxCenter`）、`mainMedia`（`AudioEngine`）、`playListModel`（`QueueModel`）、`getWave`、`musicDevices` |
| 系统集成 | `systemTray`、`smtc`、`coverHelper`、`colorExtractor` |
| 弹窗/提示 | `globalDialog`、`mainWarn`、`mainMessage`、`picWatch`、`songInfo`、`searchCard`、`desktopPlayer` |
| 异步 Loader | `desktopPlayerLoader`、`desktopLyricsLoader`、`textWatch` |
| 动画 | `openMaxLyric`、`closeMaxLyric`、`settingAnime`、`settingOutAnime`、`autoUpdateTimer` |
| 扩展点 | `pluginOverlay`（+ 无 id 的 `PluginHost`） |

---

## 5.2 全局单例装配

### 5.2.1 QML 单例（`pragma Singleton` + CMake 标记）

`CMakeLists.txt:116-127` 用 `set_source_files_properties(... PROPERTIES QT_QML_SINGLETON_TYPE TRUE)` 显式声明 10 个单例文件：

| 单例 | 文件 | 基类型/存储 | 作用 |
|---|---|---|---|
| `Style` | `components/Style.qml` (70) | `Settings { category:"Style" }` | 主题/字号/圆角/模糊/歌词界面选项 → INI `Style/*` |
| `Theme` | `components/Theme.qml` (30) | `QtObject` | **派生只读色板**，由 `Options.changeTheme()` 实时改写；不持久化 |
| `Config` | `components/Config.qml` (64) | `Settings { category:"Options" }` | **用户可调配置项** → INI `Options/*` |
| `Options` | `components/Options.qml` (75) | `QtObject` | 运行时共享状态 + 跨文件对象注入点 + 信号总线 |
| `Playback` | `components/Playback.qml` (410) | `QtObject`（同时 `ComponentBehavior: Bound`） | 播放中枢：切歌/淡变/音量/跳转/A-B/睡眠/历史/真随机 |
| `ShortCut` | `components/ShortCut.qml` (40) | `Settings { category:"ShortCuts" }` | 键位 + 每功能全局开关 |
| `PluginSlots` | `components/PluginSlots.qml` (24) | `QtObject` | 扩展点注册表 `{名字 → 容器}` |
| `Fonts` | `components/Fonts.qml` (17) | `QtObject` | `FontLoader` 载入 `feather.ttf`(icon) / `poppins.ttf`(text) |
| `LyricsFreeConfig` | `lyricsui/LyricsFreeConfig.qml` (24) | `Settings { category:"LyricsFree" }` | 「自由」歌词界面自有设置 |
| `Lyrics3DConfig` | `lyricsui/Lyrics3DConfig.qml` (29) | `Settings { category:"Lyrics3D" }` | 「3D」歌词界面自有设置 |

注册路径：全部在 `qt6_add_qml_module(... URI QueMusic VERSION 1.0 QML_FILES ...)` 的**同一个模块**里，QML 侧一句 `import QueMusic 1.0` 即可见（无需 import 子目录）。

### 5.2.2 C++ 单例（`QML_ELEMENT` + `QML_SINGLETON` → `qml_register_types_QueMusic()`）

| QML 名 | C++ 类 | 头文件 | 备注 |
|---|---|---|---|
| `MusicApi` | `MusicApiService` | `api/MusicApiService.h:31` | `QML_NAMED_ELEMENT(MusicApi)` + `QML_SINGLETON`，工厂里取 `setSharedAccountManager` 注入的账号单例 |
| `AccountManager` | `AccountManager` | `cpp/AccountManager.h:31` | `create(QQmlEngine*, QJSEngine*)` |
| `LogManager` | `LogManager` | `cpp/LogManager.h:29` | 同上 |
| `LyricsPlugins` | `LyricsPluginStore` | `cpp/plugins/LyricsPluginStore.h:14` | `QML_NAMED_ELEMENT(LyricsPlugins)` |
| `FunctionPlugins` | `FunctionPluginStore` | `cpp/plugins/FunctionPluginStore.h:16` | `QML_NAMED_ELEMENT(FunctionPlugins)` |
| `WebDav` | `WebDavStore` | `cpp/webdav/WebDavStore.h:17` | `QML_NAMED_ELEMENT(WebDav)` |
| `WebDavCache` | `WebDavCache` | `cpp/webdav/WebDavCache.h:19` | 单例 |
| `MyFolders` / `LocalFolders` | `FolderModel` 子类 | `cpp/AppModels.h:38-39` | 宏 `QUEMUSIC_DECLARE_MODEL_SINGLETON`，过滤类型 `"my"` / `"local"` |
| `Songs` | `SongModel` 子类 | `cpp/AppModels.h:42` | — |
| `FavoriteSongs` / `FavoritePlaylists` / `FavoriteArtists` | `FavoritesModel` 子类 | `cpp/AppModels.h:56-58` | 过滤类型 `"song"` / `"playlist"` / `"artist"` |

**非单例、按需实例化的 C++ 类型**（`QML_ELEMENT`，QML 里直接 `Xxx { }`）：
`AudioEngine`、`QueueModel`、`GetWave`、`CoverHelper`、`ColorExtractor`、`CustomFlow`、`LocalMusicScanner`、`DownloadedMusicModel`、`DownloadManager`、`SystemTrayManager`、`WindowsSmtcManager`、`WebDavModel`、`MeshGradientItem`、`LyricsStage`。

**QML 不可见（纯 C++）**：`DbService`、`PlayerDatabase`、`FfmpegDecoder`、`AudioDsp`、`WebDavClient`、`PluginStore`（基类）、`SearchResultModel`、`FavoritesModel`/`FolderModel`/`SongModel`（抽象基类，实际只用上面的单例子类）。

### 5.2.3 `main.qml` → `Options` 注入清单（**全部 15 条，无遗漏**）

`main.qml:21-35` 逐条赋值：

| # | 语句 | 被注入对象 | `Options` 里的声明（`Options.qml`） |
|---|---|---|---|
| 1 | `Options.queue = playListModel` | `QueueModel` | `property QueueModel queue` (L11) |
| 2 | `Options.dialog = globalDialog` | `QAlertDialog` | `property QAlertDialog dialog` (L12) |
| 3 | `Options.desktop = desktopPlayer` | `DesktopPlayer`（Popup） | `property DesktopPlayer desktop` (L13) |
| 4 | `Options.smtc = smtc` | `SmtcBridge` | `property SmtcBridge smtc` (L14) |
| 5 | `Options.mainLayout = mainLayout` | `Item` | `property Item mainLayout` (L17) |
| 6 | `Options.getWave = getWave` | `GetWave` | `property GetWave getWave` (L18) |
| 7 | `Options.textWatch = textWatch` | `Loader` | `property Loader textWatch` (L19) |
| 8 | `Options.picWatch = picWatch` | `QOptionDialog` | `property QOptionDialog picWatch` (L20) |
| 9 | `Options.songInfo = songInfo` | `SongInfoDialog` | `property SongInfoDialog songInfo` (L21) |
| 10 | `Options.coverHelper = coverHelper` | `CoverHelper` | `property CoverHelper coverHelper` (L22) |
| 11 | `Options.desktopPlayerLoader = desktopPlayerLoader` | `Loader` | `property Loader desktopPlayerLoader` (L23) |
| 12 | `Options.desktopLyricsLoader = desktopLyricsLoader` | `Loader` | `property Loader desktopLyricsLoader` (L24) |
| 13 | `Options.openMaxLyric = openMaxLyric` | `NumberAnimation` | `property NumberAnimation openMaxLyric` (L25) |
| 14 | `Options.settingOutAnime = settingOutAnime` | `NumberAnimation` | `property NumberAnimation settingOutAnime` (L26) |
| 15 | `Options.mainSearchInput = mainSearchInput` | `TextField` | `property TextField mainSearchInput` (L27) |

**不通过 `main.qml` 注入、而在单例内部自建的两个成员**：
`Options.lastSongs`（`Options.qml:40`，`property OptionsLastSongs lastSongs: OptionsLastSongs {}`）、`Options.isMacOS`（`readonly`）。

**另有两条不属于 `Options` 的指针注入**：`Playback.player = mainMedia`、`Playback.queue = playListModel`（`main.qml:59-60`）。
`Playback.qml` 顶部注释明确：「宿主注入：播放引擎不再靠上下文继承访问宿主的局部 id」——`layout/PlayerControl.qml:19`、`layout/PlayerMaxCenter.qml:13`、`SettingsView.qml:15`、`components/DesktopLyrics.qml:23`、`components/DesktopPlayerWindow.qml:23`、`components/MusicInfo.qml:21`、`components/PlayerOptions.qml:15` 都各自声明 `readonly property AudioEngine player: Playback.player`，**不再直接引用 `main.qml` 的局部 id**。

---

## 5.3 导航模型

### 5.3.1 机制

页面切换是**两段式**，都在 `layout/` 内完成，`main.qml` 不直接切页：

1. **侧栏 → 页面下标**：`LeftSideBar` 的 `navModel`（5 项，`layout/LeftSideBar.qml:177-184`）→ delegate `MouseArea.onClicked` 调
   `sidebar.indexed(index)`（选择条动画 + 首次拉数据）**和** `sidebar.mainCont.contentIndexed(index)`（真正切页）。
2. **`MainContent` 持有 6 个异步 `Loader` 与 `pages` 列表**：`property list<Item> pages: [homePage, playlistPage, favouritePage, filePage, downloadPage, searchPage]`，
   `contentIndexed(choice)` 做「旧的 `visible=false; active=false` → 新的 `active=true` → `pageIndex=choice`」；
   每个 Loader `onLoaded` 回调 `mainContent.finishedLoaderPage(n)` 触发 `pageAnine`（`ParallelAnimation`：`opacity 0→1` + `shiftY 180→0`，320ms）。
3. **搜索页是唯一没有侧栏入口的页面**（下标 5），由 `main.qml:121`、`layout/PlayerControl.qml:63` 调 `mainContent.contentIndexed(5)` 进入。

### 5.3.2 整数 → 页面 QML 映射（**权威**，源自 `layout/MainContent.qml:14-15`）

| 下标 | 页面 QML | Loader `id` | `active` 初值 | 对应侧栏项 | 备注 |
|---|---|---|---|---|---|
| 0 | `pages/HomePage.qml` | `homePage` | **`true`** | 推荐 | 首屏常驻加载 |
| 1 | `pages/PlaylistPage.qml` | `playlistPage` | `false` | 分类 | — |
| 2 | `pages/FavouritePage.qml` | `favouritePage` | `false` | 收藏 | — |
| 3 | `pages/FilePage.qml` | `filePage` | `false` | 本地 | 内含 WebDAV 子页（见 §5.8） |
| 4 | `pages/DownloadPage.qml` | `downloadPage` | `false` | 下载 | — |
| 5 | `pages/SearchPage.qml` | `searchPage` | `false` | **无** | 由标题栏搜索 / 播放条右键搜索进入；Loader 额外标了 `asynchronous: true` |

`navModel` 只有 5 项（0-4），因此 `contentIndexed(5)` **不可能**从侧栏触发；`MainContent.qml:12` 的 `pageHeight: height - 60` 与每个 Loader 的 `y: 60` 对应「页头 60px 常驻区」。
侧栏选中态是 `sidebar.choiceIndex`，与 `mainCont.pageIndex` **是两个独立变量** —— 离开搜索页时靠 `LeftSideBar` 的 `onExit` 把 `choiceIndex` 写回。

### 5.3.3 懒加载标志（`components/Options.qml:31-32`）

| 标志 | 写入点 | 作用 |
|---|---|---|
| `Options.homeLoaded` | `pages/HomePage.qml:15,31`（`Component.onCompleted` 内 `if(!Options.homeLoaded)`） | 首页首帧数据（`MusicApi.getHotlistMenu` / `getHotPlaylistMenu(3)` / `getHotPlaylists(1)` + 按时段问候语）只拉一次 |
| `Options.playlistLoaded` | `pages/PlaylistPage.qml:74,78` | 分类页首帧数据（`MusicApi.getPlaylistMenu(3)` / `getNewSongs(1,1,20)` / `getAllToplist()`）只拉一次 |

两者都是 `Options` 上的**运行期**标志，不持久化（重启后重新拉）。
`LeftSideBar.indexed()` 里还有一条独立的一次性拉取：`case 1: if(MusicApi.recommendSongs.count === 0) MusicApi.getRecommendSongs(1,24)`。

### 5.3.4 `Options.exitIndex` 语义 —— **返回层级计数器，不是页面下标**

`Options.qml:30` 注释写的是「要求主界面切到的页」，但**实际实现是导航深度/返回栈层数**。取值与含义：

| 值 | 含义 | 谁写入 | 谁消费 |
|---|---|---|---|
| `0` | 顶层侧栏页，无子窗口/子页面 | 初始值；`Esc` / 返回按钮递减可回到 0 | `Layout/LeftSideBar.qml:63` 的 `onExit` |
| `1` | 打开**一层**子界面：一层子窗口（`winIndex: 1`）或搜索页 | `main.qml:126`（`doSearch`）、`layout/PlayerControl.qml:65`（右键搜索）、`pages/HomePage.qml`（206/225/507/557/616/699）、`pages/FavouritePage.qml:229`、`pages/SearchPage.qml:66`、`pages/FilePage.qml:445,689` | `AnimatorWindow`/`PlayListWindow` 的 `onExit`：`if(Options.exitIndex <= root.winIndex)` 自行关闭 |
| `2` | 打开**二层**子窗口（`winIndex: 2`）：首页热门歌单详情、分类页歌单详情、专辑弹层 | `pages/HomePage.qml:899`、`pages/PlaylistPage.qml:352,454,600`、`pages/SearchPage.qml:144` | 同上（`2 <= 2` 才关） |

**递减协议**（三处，必须是「先 `Options.exit()`，再 `-= 1`」）：

- `main.qml:151-163`：`Shortcut { sequence:"Esc"; context: Qt.ApplicationShortcut; enabled: !Options.recordingShortCut }` → `Options.exit()`；`if(Options.exitIndex > 0) Options.exitIndex -= 1`；`mainLayout.forceActiveFocus()`。
- `main.qml:239-250`：标题栏 `returnButton` → 同样的 `Options.exit()` + `-= 1`（**注意它不重置焦点**）。
- `components/AnimatorWindow.qml:37,43` 与 `components/PlayListWindow.qml:59,65`：`closed()` 里 `Options.exitIndex -= 1`；`Connections { target: Options; enabled: root.visible; function onExit() { if(Options.exitIndex <= root.winIndex) {...关闭动画...} } }`。

**搜索页特例**（`layout/LeftSideBar.qml:60-68`）：

```qml
Connections { target: Options
    function onExit(): void {
        if(sidebar.mainCont.pageIndex === 5 && Options.exitIndex <= 1) {
            sidebar.mainCont.contentIndexed(sidebar.choiceIndex)   // 从搜索页回到侧栏上次选中的页
            MusicApi.searchSongsResults.clear()
        }
    }
}
```

`winIndex` 分布（`<= exitIndex` 判据的另一半）：`FilePage` 765/1197 与 `FavouritePage` 447 为 `1`；`HomePage` 906、`PlaylistPage` 612/656、`SearchPage` 207 为 `2`；组件默认 `1`（`PlayListWindow.qml:30`、`AnimatorWindow.qml:13`）。

---

## 5.4 布局层级

```
Window { id: window }
├─ [非可视] Binding ×2 (MusicApi.soundQuality / downloadPath)
├─ [非可视] Timer autoUpdateTimer (3s → silentUpdateCheck) · WindowControls win · Shortcut Esc
├─ [非可视] Instantiator → 13 个全局 Shortcut · WindowAgent windowAgent
├─ Rectangle titleBar        (z=10, 60px, 透明; onCompleted → windowAgent.setTitleBar(titleBar))
│   ├─ Item appTitleBlock    (x10,y10,180×40; setHitTestVisible)
│   ├─ Row barLeftWidgets    (anchors.left = sidebar.width+16)
│   │    QWKButton returnButton → Options.exit + exitIndex-- · TextField mainSearchInput · SButton searchButton
│   └─ Row barRightWidgets   (anchors.right; y = 10 - musicControlMax.hideHeight)
│        PluginSlot "titlebar" · QWKButton fullScreenButton / settingButton / minButton / maxButton / closeButton
│        （min/max/close：macOS 隐藏，否则 windowAgent.setSystemButton(WindowAgent.*, btn)）
├─ Item mainLayout           (anchors.fill, z=5; property int maxLyricType:0)
│   ├─ Connections Options.onChangeTheme  (按 Style.backmode 0/1/2/3/4 切背景与配色 + dwm-blur)
│   ├─ Image backGround                   (z=0, 全屏, asynchronous, PreserveAspectCrop)
│   ├─ LeftSideBar sidebar                (z=2, 0,0, 210×(h-78); required property MainContent mainCont)
│   │    choicebar + downBar/upBar · Logo+"Beta" · ListModel navModel(5 项) · Repeater navDelegate(42px)
│   │    Column sidebarBottomSlot → PluginSlot "sidebar.bottom"
│   ├─ MainContent mainContent            (z=1, x=sidebar.width, w=parent.width-x, h-78)
│   │    ParallelAnimation pageAnine (opacity + shiftY)
│   │    Loader ×6  y=60, w=mainContent.width, h=pageHeight, transform: Translate{y: shiftY}
│   │      homePage(0,active) playlistPage(1) favouritePage(2) filePage(3) downloadPage(4) searchPage(5,async)
│   ├─ PlayerControl musicControlMin      (z=4, y=h-78+musicControlMax.hideHeight, w=100%, 78px)
│   │    progressSlider · musicinfo(封面 MultiEffect + 标题/歌手 + 收藏/下载) · 中间控制 Row · playerRow→PluginSlot "player"
│   │    ToolTip volumeControl(QBlurCard+QSlider→Config.musicVolume) · MouseArea→centerChange
│   │    PlayList playList · PlayerOptions · MusicInfo · QOptionDialog optionsEQ(EqualizerPanel)
│   ├─ PlayerMaxCenter musicControlMax    (z=3, y=100, 100%×100%, visible=false)
│   │    MouseArea(悬浮→hideDelay 3s→hideHeight=76) · ParallelAnimation rectcolorAnime(三色)
│   │    Loader lyricLoader(source = LyricsPlugins.source; onLoaded → inject(item))
│   │    Binding ×3 → MusicApi.lyricFollowActive/lyricPositionMs/lyricOffsetMs · Timer 320ms → funConnect()
│   │    SButton playerminedButton/centerThemeButton/centerStyleButton/翻译
│   │    QOptionDialog maxLyricsThemeDialog(LyricsPlugins.plugins) · maxLyricsDialog(常驻 3 项 + item.styleOptions)
│   ├─ NumberAnimation openMaxLyric / closeMaxLyric  (z 位移 420ms BezierSpline; 控制 barLeftWidgets 与 hideHeight)
│   └─ ColorExtractor colorExtractor      (onColorsExtracted → musicControlMax.changeColor)
├─ Loader settingsView       (anchors.fill, active:false, asynchronous, z=6, opacity Behavior)
│    sourceComponent: SettingsView { backSource; sidebarColor; mainContentColor }
├─ Loader fpsCounter (active: Config.displayFps, z=99) · Loader debugHud (active: Config.debug, z=99)
├─ NumberAnimation settingAnime / settingOutAnime  (设置页淡入淡出; 切换 mainLayout.visible)
├─ CoverHelper coverHelper   (onLocalCoverReady → mainMedia.urlStr / colorExtractor)
├─ Connections WebDavCache.onCached · Connections MusicApi (onUrlplay/onWarned/onLocalLyricsReady/onLocalLyricsFailed/onLocalMetadataReady)
├─ MediaDevices musicDevices · GetWave getWave(engine:mainMedia, renderWindow:window, bands:128)
├─ AudioEngine mainMedia     ★真正的播放器：volume←Playback.outVolume、deviceId←Config、onMediaStatusChanged 分派 4 种循环模式
├─ SystemTrayManager systemTray (iconSource/title/playing/nowPlaying + 播放/上一首/下一首/退出/激活)
├─ SmtcBridge smtc (player:mainMedia, queue:playListModel, targetWindow:window) · QueueModel playListModel(playListIndex:-1)
├─ SearchCard searchCard · DesktopPlayer desktopPlayer(Popup) · Loader desktopPlayerLoader · Loader desktopLyricsLoader
├─ QAlertDialog globalDialog · QWarn mainWarn · QMessage mainMessage · QOptionDialog picWatch · SongInfoDialog songInfo
├─ Loader textWatch (source QTextWindow.qml)
├─ Item pluginOverlay (anchors.fill, z=20) → PluginSlot "window.overlay"
└─ PluginHost (onPluginLoaded → window.syncTitleBarHitTest())
```

### 响应式 / 最大化行为

| 行为 | 实现位置 | 说明 |
|---|---|---|
| 最小尺寸 | `main.qml:16-17` | `minimumWidth:810`、`minimumHeight:540`；默认 1140×760 |
| 侧栏固定宽 | `LeftSideBar` `x:0,w:210`（`main.qml:461-469` 再显式写一次） | 不随窗口宽度变化 |
| 内容区自适应 | `MainContent` `x: sidebar.width`、`width: parent.width - x` | 唯一横向伸缩者 |
| 底部播放条 | `y: mainLayout.height - 78 + musicControlMax.hideHeight`、`height:78` | 沉浸模式下随 `hideHeight` 下移 |
| 主区高度 | `sidebar`/`mainContent` 均 `height: parent.height - 78` | 为播放条留出 78px |
| 沉浸层切换 | `openMaxLyric` / `closeMaxLyric`（`PlayerControl.onCenterChange` 驱动） | 420ms BezierSpline 位移；`musicControlMax.hideHeight` 由 `hideDelay`(3s) 置 76 做自动隐藏 UI |
| 标题栏随沉浸下沉 | `barRightWidgets.y = 10 - musicControlMax.hideHeight`；`barLeftWidgets.y` 由动画置 -48/12 | — |
| 全屏 | `fullScreenButton` + `property int prevVisibility` | 退出全屏时按记忆恢复 `showMaximized()` 或 `showNormal()` |
| 无边框/系统标题栏 | `Config.noWindowKit` → `extra-margins=3`、`title-bar-height=40`；macOS 隐藏三大按钮 | — |
| 设置页宽度自适应 | `SettingsView.settingStack`：`standWidth = parent.width > 1410 ? 1148 : parent.width-262`；`containWidth = >1410 ? 1200 : parent.width-210`；`containX = >1410 ? parent.width/2-705 : 0` | 宽屏居中、窄屏铺满 |
| 歌词界面响应式 | `MainLyric.piclong = width < 1280 ? height/3 + width/8 - 100 : height/3 + 60`；`standHeight = lyricSize + height/32 + width/56` | — |

---

## 5.5 `main.qml` 重要对象表

### 对话框 / 弹窗类

| id | 类型 | 作用 |
|---|---|---|
| `globalDialog` | `QAlertDialog` | 全局确认/输入弹窗；`openSimpleDialog(title,text,callback)`；`blurSource: mainLayout.visible ? mainLayout : settingsView` |
| `mainMessage` | `QMessage` | 消息弹窗，`openSimpleDialog(title,text,cb)` → `dialog(title,text,"\uf11a")` |
| `mainWarn` | `QWarn` | 轻提示；`tiped(text,type)`；订阅 `Options.onWarned` |
| `picWatch` | `QOptionDialog` | 图片查看/保存到系统图片目录/拖拽出图；`dialog(source,title)` |
| `songInfo` | `SongInfoDialog` | 全应用共用，各列表调 `Options.songInfo.show(item,isList)` |
| `searchCard` | `SearchCard` | 搜索历史下拉，`onSearchIndex(index)` → `window.doSearch` |
| `desktopPlayer` | `DesktopPlayer`（`Popup`） | 桌面部件模式（0 无 / 1 灵动岛 / 2 小窗播放器 / 3 歌词栏） |

### 模型 / 数据类

| id | 类型 | 作用 |
|---|---|---|
| `playListModel` | `QueueModel` | 播放队列（C++ O(1) 路径查找、批量操作、角色化访问）；`playListIndex:-1` |
| `mainMedia` | `AudioEngine` | 自研播放引擎；`urlStr/noTitle/album` 由 QML 侧回填；`onMediaStatusChanged` 按 `Config.cycleIndex` 0/1/2/3 分派；`onErrorOccurred` 带 `Playback.autoSkipCount>=2` 熔断 |
| `musicDevices` | `MediaDevices` | `audioOutputs` 供 `deviceId` 绑定 |
| `getWave` | `GetWave` | `bands:128`，`enabled: Style.waveDisplay && musicControlMax.visible` |
| `coverHelper` | `CoverHelper` | `onLocalCoverReady` / `findLocalCover` / `findEmbeddedCoverAsync` / `jsonCover` / `setCacheDir` / `pruneCache` |
| `colorExtractor` | `ColorExtractor` | `extractColorsFromUrl` → `onColorsExtracted` → `musicControlMax.changeColor` |
| `systemTray` | `SystemTrayManager` | 托盘图标/菜单/消息；`Trigger`/`DoubleClick` 恢复窗口 |
| `smtc` | `SmtcBridge` | SMTC（见下） |

### Loader / 动画 / 桥接类

| id | 类型 | 作用 |
|---|---|---|
| `settingsView` | `Loader` | 异步加载 `SettingsView.qml`；`opacity` 由 `settingAnime`/`settingOutAnime` 驱动 |
| `textWatch` | `Loader` | 异步加载 `QTextWindow.qml`（长文本查看） |
| `desktopPlayerLoader` | `Loader` | 异步加载 `DesktopPlayerWindow.qml` |
| `desktopLyricsLoader` | `Loader` | 异步加载 `DesktopLyrics.qml`；`win.onSessionClosing` 时两者 `active=false` |
| `openMaxLyric` / `closeMaxLyric` | `NumberAnimation` | 沉浸层上滑/下滑（`openMaxLyric` 经 `Options.openMaxLyric` 暴露给 `PlayerMaxCenter`） |
| `settingAnime` / `settingOutAnime` | `NumberAnimation` | 设置页淡入/淡出；`settingOutAnime` 经 `Options.settingOutAnime` 暴露给 `SettingsView` 返回按钮 |
| `autoUpdateTimer` | `Timer` | 3s 后触发 `silentUpdateCheck()`（XHR 拉 `doc/updater.txt`，比 `versionCode`） |
| `win` | `WindowControls` | `closeToTray/toClosing/hideToTray/restoreWindow/quitApp`；`sessionClosing` 信号 |
| `smtc` | `SmtcBridge` | `QtObject`；内嵌 `WindowsSmtcManager`（`initialize(targetWindow)`）+ 3 组 `Connections`（player / Playback / queue）；`push()`/`pushInfo()` 全部经 `Qt.callLater` 防重入 |
| `windowAgent` | `WindowAgent` | QWindowKit；`setup/setTitleBar/setHitTestVisible/setSystemButton/setWindowAttribute` |
| `pluginOverlay` | `Item` | `PluginSlot { slotName:"window.overlay" }` |
| （无 id） | `PluginHost` | 功能插件宿主，`onPluginLoaded → syncTitleBarHitTest()` |

### 键盘 / 快捷键入口

| 对象 | 说明 |
|---|---|
| `Shortcut { sequence:"Esc" }` | 全局返回：`Options.exit()` + `exitIndex--` + `mainLayout.forceActiveFocus()`；`enabled: !Options.recordingShortCut` |
| `Instantiator`（13 项 model） | 每项 `{k,a}` → `delegate: Shortcut`；`sequence: ShortCut[k]`、`enabled: Config.openShortCut && ShortCut["globalShortcut"+K]` |
| `TextField mainSearchInput` | `onAccepted` 回车 → `window.doSearch(text)` |

---

## 5.6 快捷键

### 5.6.1 `components/ShortCut.qml`（`Settings { category:"ShortCuts" }`，40 行）

| 功能键（属性名） | 默认键位 | 全局开关属性 | 开关默认 |
|---|---|---|---|
| `play` 播放/暂停 | `Space` | `globalShortcutPlay` | `true` |
| `back` 上一首 | `Left` | `globalShortcutBack` | `true` |
| `forward` 下一首 | `Right` | `globalShortcutForward` | `true` |
| `playList` 打开/关闭播放列表 | `Alt` | `globalShortcutPlayList` | `true` |
| `musicControl` 音乐控制面板 | `Up` | `globalShortcutMusicControl` | `true` |
| `volumeUp` 音量增大 | `Ctrl+Up` | `globalShortcutVolumeUp` | `true` |
| `volumeDown` 音量减小 | `Ctrl+Down` | `globalShortcutVolumeDown` | `true` |
| `seekBack` 快退 | `Ctrl+Left` | `globalShortcutSeekBack` | `true` |
| `seekForward` 快进 | `Ctrl+Right` | `globalShortcutSeekForward` | `true` |
| `mute` 静音切换 | `Ctrl+M` | `globalShortcutMute` | `true` |
| `abLoop` A-B 片段循环 | `Ctrl+B` | `globalShortcutAbLoop` | `true` |
| `favorite` 收藏当前曲目 | `Ctrl+D` | `globalShortcutFavorite` | `true` |
| `playerOptions` 播放器选项 | `Ctrl+T` | `globalShortcutPlayerOptions` | `true` |

注释明确（`ShortCut.qml:26`）：「每个功能单独控制是否为全局快捷键（#44：不要用总开关控制所有功能）」。
`Config.openShortCut` 仍是**总开关**（`main.qml:197` 里 `enabled: Config.openShortCut && ShortCut[flag]`）。

### 5.6.2 `main.qml` 的 `Shortcut` 声明

**A. 硬编码 `Esc`**（`main.qml:151-163`，`context: Qt.ApplicationShortcut`）→ `Options.exit()`；`exitIndex--`；`mainLayout.forceActiveFocus()`。

**B. `Instantiator` + `delegate: Shortcut`**（`main.qml:165-200`，全部 `context: Qt.ApplicationShortcut`）：

| `k` | 动作 |
|---|---|
| `volumeUp` | `Playback.stepVolume(+Playback.volumeStep)` + 提示音量 |
| `volumeDown` | `Playback.stepVolume(-Playback.volumeStep)` + 提示音量 |
| `seekBack` | `Playback.seekBack()` |
| `seekForward` | `Playback.seekForward()` |
| `mute` | `Playback.toggleMute()` + 提示 |
| `abLoop` | `Playback.setAbPoint(Playback.abA < 0 \|\| Playback.abArmed ? 0 : 1)` + 提示 |
| `favorite` | `musicControlMin.toggleFavorite()` |
| `playerOptions` | `musicControlMin.openPlayerOptions()` |
| `play` | `Playback.togglePlay()` |
| `back` | `Playback.previous()` |
| `forward` | `Playback.next(false)` |
| `playList` | `playList.visible ? playList.close() : playList.open()` |
| `musicControl` | `mainLayout.state === ""` → `controlMaxLoader.active = true`；否则 `window.playermined()` + `minedAnimation.start()` + `mainLayout.state = ""` |

> ⚠️ `musicControl` 分支引用了 `controlMaxLoader` 与 `minedAnimation`，**这两个 id 在 `main.qml` 里不存在**（沉浸层改由 `musicControlMax` + `openMaxLyric/closeMaxLyric` 接管）—— 疑似重构遗留。`window.playermined()` 同样在 `main.qml` 里没有定义。**标记为「未确认 / 疑似死代码」**。

其余 `Shortcut` / `Keys.onPressed` 只在别处出现：`SettingsView.qml` 的 `keyCapture`（`Keys.onPressed` 捕获录制键位，非 `Shortcut`）、`components/ColorPickerDialog.qml`（未确认）。

---

## 5.7 `SettingsView.qml` 结构（2934 行）

### 5.7.1 外层结构

```qml
pragma ComponentBehavior: Bound
import QtQuick / QtQuick.Controls.Basic / QtQuick.Dialogs / QtCore / QueMusic 1.0

Item {                       // 不是 Page/Dialog；由 main.qml 的 Loader 承载
    id: settingsView
    readonly property AudioEngine player: Playback.player
    property url backSource; property color sidebarColor; property color mainContentColor

    ├─ Image settingsBackGround        (visible: Style.backmode===2 || 3)
    ├─ Connections  AccountManager     (登录成功自动收起面板)
    ├─ QOptionDialog neteaseLoginDialog (网易云扫码登录 + QRCodeView + 手机号/Cookie 备选)
    ├─ QOptionDialog kugouLoginDialog   (酷狗扫码登录 + QRCodeView)
    ├─ FileDialog   下载目录选择         (title "选择默认下载目录" → Config.downloadFolder)
    ├─ Rectangle leftSidebarSettings   (x0,y0,w210,h100%,z5)
    │   ├─ Connections  Options.onChangeTheme (按 Style.sidebarStyle 调 choicebar1)
    │   ├─ function index1ed(choice)   选择条动画 + 切换 settingStack.setPages
    │   ├─ Rectangle choicebar1 + downBarS/upBarS
    │   ├─ Item 返回按钮 QWKButton + Label "应用设置"  (→ Options.settingOutAnime.running = true)
    │   └─ ListModel navModelSettings + Column navListViewSettings(Repeater delegate)
    ├─ Rectangle settingStack           (x210, w = parent.width-210, h100%, z2)
    │   ├─ property list<Item> setPages: [themeset,uiset,toolset,playerset,shortcutset,modset,aboutset,debugset]
    │   ├─ property int setPageIndex: 0
    │   ├─ ParallelAnimation setPageAnine (opacity 0→1 + y 240→60, 320ms)
    │   ├─ standWidth / containWidth / containX  (见 §5.4 响应式表)
    │   └─ 8 个页面容器（QScrollView ×6 / Item ×1 / QScrollView ×1）
    └─ Item updater                     (XHR 检查更新 + QAlertDialog updateDialog)
```

**分类（侧栏）↔ 页面容器映射**（`navModelSettings` 与 `settingStack.setPages` **必须同序**）：

| 下标 | 侧栏显示 | 页面容器 `id` | 容器类型 | 内容高度 |
|---|---|---|---|---|
| 0 | 通用 | `themeset` | `QScrollView` | `settingStack.height - 60` |
| 1 | 界面 | `uiset` | `QScrollView` | 同上 |
| 2 | 播放器 | `toolset` | `QScrollView` | 同上 |
| 3 | 音频 | `playerset` | `QScrollView` | 同上 |
| 4 | 快捷键 | `shortcutset` | `QScrollView` | 同上 |
| 5 | 插件 | `modset` | `Item`（内含 `QBlurTapBar` + `QPages`） | `height - 60` |
| 6 | 关于 | `aboutset` | `QScrollView` | 同上 |
| 7 | Debug | `debugset` | `QScrollView` | 同上 |

侧栏项图标字符（`navModelSettings`）：通用 `\uf038` / 界面 `\uf0a7` / 播放器 `\uf010` / 音频 `\uf044` / 快捷键 `\uf0c7` / 插件 `\uf0ff` / 关于 `\uf0b6` / Debug `\uf060`。

### 5.7.2 设置项 → 配置键（系统化清单）

**页面 0「通用」`themeset`**

| 分组 `QHead` | 设置项 | 绑定键 / 调用 | 作用 |
|---|---|---|---|
| 全局主题 (693) | 全局主题 | `choice: Style.theme` → `Style.theme = choiced` | 浅色/深色/跟随系统（`Options.darkis`） |
| | 全局主题色 | `Repeater model: Style.colorList` → `Style.color = index` + `Options.changeTheme()` | 内置 4 色，可增删（`Style.colorList.length` 4→8，SButton ±）；`ColorPickerDialog themeColorChoose.onAccepted` → `Style.colorList.push(...)` |
| | 应用背景 | `choice: Style.backmode` (0默认/1浅主题色/2云母/3图片/4模糊窗口) | — |
| | 选择背景图片 | `choice: Style.backpic`；`FileDialog`(822) → `Style.backgroundImage = selectedFile` | 仅 backmode=3 生效 |
| 在线平台与账户 (838) | 酷狗音乐 / 网易云音乐 / 哔哩哔哩 | `PlatformCard`：`MusicApi.songSource = 0/1/2`；`isLogin/header/name ← AccountManager.kugou*/netease*`；点击 → 登录弹窗 / `logoutKugou()` / `logoutNetease()` | 左侧三项是**音乐源**，点右侧头像才登录 |
| 应用程序 (949) | 全局音量 | `Config.musicVolume`（`value: Math.floor(Config.musicVolume*100)`） | — |
| | 下载目录 | `Config.downloadFolder`（`FileDialog` 359） | — |
| | 最小化托盘 | `Config.closeToManage`（**注意键名是 "Manage" 不是 "Tray"**） | — |
| | 记住窗口 | `Config.rememberWindow` | 配合 `win.toClosing()` 写 `Config.winX/Y/W/H` |
| | 自动检查更新 | `Config.autoUpdate` | 决定 `autoUpdateTimer` 是否启动 |
| | 默认缓存位置 | `Config.cacheUrl`（`FileDialog` 1037） | — |
| | 自动缓存图片 | `Config.picCache` | — |
| | 清除图片缓存 | `Options.coverHelper.clearCache()` + **`Songs.reload()`** | 清缓存后 DB 引用作废，必须重载列表丢弃死链 |

**页面 1「界面」`uiset`**

| 分组 | 设置项 | 绑定键 |
|---|---|---|
| 界面样式 (1109) | 左栏沉浸 | `Style.sidebarColor` |
| | 左栏样式 | `Style.sidebarStyle` (0 hover / 1 themeColor) |
| | 不透明底栏 | `Style.noOpacityControl` |
| | 卡片圆角大小 | `Style.cubeRadius` |
| | 控件圆角大小 | `Style.labelRadius` |
| | 不使用控件大圆角 | `Style.noControlRadius` |
| 界面效果 (1205) | 高级材质 | `Style.highQualityBlur` |
| | 卡片阴影模糊度 | `Style.shadowSize` |
| | 卡片背景模糊度 | `Style.blurSize` |
| | 子页面切入切出动画 | `Style.animeType`（索引进 `AnimatorWindow.animeOnList` / `PlayListWindow.animeOnList`） |
| | 高级动画效果 | `Style.premiumAnime` |
| 页面布局 (1289) | 首页默认布局 | `Style.homeLayout`（0 默认 / 1 竖向 / 2 混合） |
| 播放器歌词模式 (1315) | 标准歌词大小 | `Style.lyricSize` |
| | 歌词字重 | `Style.textWidth` |
| | 歌词字体 | `Style.fontFamily`（`FontFamilyDialog` 1373） |
| | 背景着色器样式 | `Style.flowStyle`（0 Fluid / 1 Classic / 2 静态渐变） |
| | 显示音波效果 | `Style.waveDisplay` |
| | 歌词渐进模糊 | `Style.maskBlur` |
| | 高级逐行弹簧动画 | `Style.premiumLyricAnime` |
| | 自动进入沉浸模式 | `Style.lyricHideGui` |

**页面 2「播放器」`toolset`**

| 分组 | 设置项 | 绑定键 |
|---|---|---|
| 在线功能 (1470) | 默认音质 | `Config.soundQuality` |
| | 默认音乐源 | `Config.mainMusicSource` |
| | 缓存大小/MB | `Config.cacheSize` |
| 通用 (1528) | 使用默认输出设备 | `Config.useDefaultDevice` |
| | 音频输出设备 | `Config.audioDevice`（`MediaDevices.audioOutputs` 下标） |
| | 自动播放 | `Config.autoPlay` |
| | 音量步长 | `Config.volumeStep` |
| 播放控制 (1596) | 精确跳转步长 | `Config.seekStep`（选项 `[3,5,10,15,30]`） |
| | 播放淡入淡出 | `Config.fadeEnabled` |
| | 淡变时长 | `Config.fadeMs` |
| | 睡眠定时默认 | `Config.sleepMinutes` |
| | 随机避免最近 | `Config.shuffleAvoid` |
| | 恢复上次播放列表 | `Config.autoRestoreQueue` |
| | 播放历史上限 | `Config.historyLimit` |
| | 清空播放历史 | `Options.dialog.openSimpleDialog` → `Playback.clearHistory()` |

**页面 3「音频」`playerset`**：无 `SettingItemCard`，整块是 `EqualizerPanel { engine: settingsView.player; cardColor: settingStack.cardColor }`（`SettingsView.qml:1750`）。EQ / 声道 / 增益由 `EqualizerPanel` 直接读写 `AudioEngine` 的 AudioDsp 接口。

**页面 4「快捷键」`shortcutset`**

| 分组 | 设置项 | 绑定键 / 调用 |
|---|---|---|
| 全局快捷键 (1888) | 启用全局快捷键 | `Config.openShortCut` |
| | 隐藏按键捕获器 `Rectangle keyCapture` | `Keys.onPressed` → `stopRecording`；录制期间 `Config.openShortCut=false` + `Options.recordingShortCut=true`，结束后还原 |
| 快捷键列表 (1970) | 13 行 `Repeater model: actionDefs` | 键位显示 `ShortCut[name] \|\| defau`；`QSwitch.switchTrue: ShortCut["globalShortcut"+Name]`；「设置」→ `shortcutset.startRecording(name)`；「重置」→ `ShortCut[name] = defau` |

`actionDefs`（`SettingsView.qml:1852-1867`）与 `ShortCut.qml` 的 13 个属性一一对应，`defau` 字段仅用于展示默认值。
`globalPropertyName(actionName)` = `"globalShortcut" + actionName[0].toUpperCase() + actionName.slice(1)`（`SettingsView.qml:1772`）—— 与 `main.qml:194` 拼 `flag` 的逻辑完全同构，**两边必须同步改**。

**页面 5「插件」`modset`**：`property int tabIndex`（0 歌词界面 / 1 功能 / 2 音乐源）+ `QBlurTapBar model: ["歌词界面","功能","音乐源"]` + `QPages pageList: [uiMod, toolMod, musicMod]`。

| 子页 | 内容 |
|---|---|
| `uiMod` | `PluginManager { store: LyricsPlugins; pluginKind: "歌词界面插件" }` |
| `toolMod` | `PluginManager { store: FunctionPlugins; pluginKind: "功能插件"; multi: true }` |
| `musicMod` | 占位提示「音乐源插件还未开发完成，等待开发者更新喵」 |

**页面 6「关于」`aboutset`**

| 分组 | 内容 |
|---|---|
| 应用信息 | `SettingItem label: "QueMusic Beta (测试版本)"` + 「检查更新」→ `updater.checkForUpdate()` |
| 软件信息 (2384) | 长文案（免费开源声明、项目定位等） |
| 主要开发者 (2402) | `AccountCard { title:"BroNekoX Studio" }` |
| 技术与版本信息 (2419) | `Options.version` / `Options.versionCode` / `Options.pluginApi` / Qt `6.10.3`(Community) / `QRHI/Qt Scene Graph` / `C++/JS/QtQuick` |
| 第三方项目 (2497) | `AccountCard` ×3：QWindowKit / Qt Community / QCloudMusicApi |
| 联系开发者 (2530) | QQ `241422517`、Email `uihg@outlook.com`、Bilibili `695207057`、QQ群 `1105114511` + 4 个 QButton（Github / QueMusic Web / Bug issue / 开源许可，均 `Qt.openUrlExternally`） |
| （尾部） | `Item updater` + `QAlertDialog updateDialog`（`onConfirm` → releases 页） |

**页面 7「Debug」`debugset`**

| 分组 | 设置项 | 绑定键 / 调用 |
|---|---|---|
| 渲染 (2681) | 使用系统标题栏 | `Config.noWindowKit`（**改动需重启**） |
| | 渲染引擎 | `Config.gpuRenderMode`（0~3，**需重启**） |
| | Timer 驱动界面 | `Config.timerAnimator`（**需重启**） |
| | QML 动画引擎 | `Config.qmlAnimator`（**需重启**） |
| 调试 (2752) | 显示渲染帧率 | `Config.displayFps` |
| | 调试模式 | `Config.debug` |
| 日志 (2788) | 启用日志 | `LogManager.enabled` |
| | 记录等级 | `LogManager.minimumLevel` |
| | 打开日志目录 | `LogManager.openLogFolder()` |
| | 实时日志预览 | `LogManager.logPreview`（只读，`Flickable` + `Text`） |

### 5.7.3 `SettingsView.qml` 触达的 C++ 单例 / 类型

| 类型 | 出现位置（行） | 用途 |
|---|---|---|
| `AccountManager` | 45,47,53,74,78,81,82,100,105,114,123,126,135,225,237,244,253,262,297,310,311,312,330,335,343,352,891-930 | 网易云/酷狗扫码、手机号、Cookie 登录；登录态、头像、昵称、登出 |
| `LogManager` | 2805,2806,2815,2817,2831,2864 | 日志开关/等级/打开目录/预览 |
| `LyricsPlugins` | 2162 | 歌词界面插件管理器数据源 |
| `FunctionPlugins` | 2174 | 功能插件管理器数据源 |
| `Songs` | 1073 | `Songs.reload()`（清图片缓存后重载本地列表） |
| `MusicApi` | 894,919,943 | `MusicApi.songSource = 0/1/2`（三个 PlatformCard） |
| `AudioEngine` | 15,1753 | `readonly property AudioEngine player: Playback.player` → 传给 `EqualizerPanel.engine` |
| `QWKButton` | 497 | 返回按钮 |

---

## 5.8 每个页面文件摘要

| 文件 | 行数 | 职责 | 主要数据来源 | 关键子组件 | `exitIndex` 写入 |
|---|---|---|---|---|---|
| `pages/HomePage.qml` | 746 | 首页/推荐：Beta 提示、每日推荐主卡（数据走私人雷达）、继续播放、歌单漫游（窗口内 tabbar 切分类 → 分类歌单列表）、热门歌单（滚动到底自动翻页） | `MusicApi.getHotlistMenu/getHotPlaylistMenu/getHotPlaylists/getPersonalRadar/getMusicPlaylists`；`Options.lastSongs`（继续播放）、`Options.queue`（加入播放列表） | `QDrop`(音乐源 5 项) / `QGridView` / `QBlurTapBar`(分类 tabbar) / `AnimatorWindow`（dailyWindow、recommendWindow、hotlistsWindow） / `PlayListWindow`(winIndex 2) | 1（每日推荐/歌单漫游）、2（分类歌单详情） |
| `pages/SearchPage.qml` | 245 | 搜索结果四页签：歌曲 / 歌单 / 专辑 / 歌词 | `MusicApi.searchSongsResults`、`MusicApi.playlistSong`、`Options.searchText`、`MusicApi.songSource` | `QPages pageList:[searchSong,searchLists,searchAlbum,searchLyrics]` / `QBlurTapBar` / `QDrop` / `QListView` / `QLoadSign` / `PlayListWindow`(winIndex 2) | 1（切源）、2（专辑详情） |
| `pages/PlaylistPage.qml` | 690 | 分类/歌单详情：新歌、分类歌单、榜单、歌手、专辑 | `MusicApi.newSongs` / `musicPlaylists` / `toplistList` / `allPlaylistMenu` / `singerList` / `playlistSong`；`MusicApi.songFilter` | `QMenu filterMenu`（全部/仅免费/仅 VIP）/ `QPages:[musicsPage,musicMenuPage,cloud,album]` / `QGridView` / `Album`(内联 id `album`) / `PlayListWindow`(winIndex 2 ×2) | 2 |
| `pages/FavouritePage.qml` | 486 | 收藏：歌曲 / 歌单 / 歌手 / 播放历史；排序与多选 | `FavoriteSongs`、`FavoritePlaylists`、`FavoriteArtists`（未确认是否使用）、`MusicApi.playlistSong`、`Playback` 历史 | `QSortModel songSort/listSort` + `QMenu sortMenu`(7 种排序) / `QPages:[songsLoader,listsLoader,singer,historyLoader]` / `PlayListWindow`(winIndex 1) | 1 |
| `pages/FilePage.qml` | 1985 | 本地音乐：我的文件夹 / 本地文件夹 / WebDAV 三子页；导入、扫描、删歌、选择模式、元数据与歌词编辑 | `MyFolders`、`LocalFolders`、`Songs`(+`Songs.searchResults`)、`LocalMusicScanner`、`MusicApi.readLocalMetadataAsync/readLocalAudioInfo/readLocalLyricsText/readLocalCoverHint/writeLocalMetadata`；`FolderListModel`(Qt.labs) 管本地目录 | `QBlurTapBar` / `QPages pageList:[myFile,localFile,webDav]` / `WebDavPage`(id `webDav`, 292) / `AnimatorWindow`(winIndex 1 ×2) / `QSortModel` / `QMenu` | 1 |
| `pages/DownloadPage.qml` | 388 | 下载管理：正在下载 / 已下载两页签；任务暂停/删除/重试、打开目录 | `MusicApi.downloader`（`DownloadManager`：`taskCount/hasActiveTasks/effectiveDownloadDir/removeTask/retryTask/completedCount`）、`DownloadedMusicModel`（`downloadDir/reload()`） | `DownloadedMusicModel` / `QBlurTapBar` / `QPages downloadChildPage` / `QButton`（文件夹中显示） | — |
| `pages/WebDavPage.qml` | 496 | WebDAV 分页（**不是顶层导航页**，嵌在 `FilePage` 第 3 个子页）：服务器列表 + 目录浏览 + 增删改服务器 | `WebDavModel`(id `browser`)：`openServer/at()`；`WebDav`：`servers/authHeader/userOf/addServer/updateServer/removeServer/rememberSidecars/sidecarsOf`；`Playback.playItem(..., source: Playback.kSourceWebDav)` | `QPages` / `QOptionDialog serverDialog` / `QInput`(fieldName/fieldUrl/fieldUser/fieldPass) / `QButton` / `QLocalView` | — |

> `pages/WebDavPage.qml` 无侧栏入口、无 `MainContent` Loader；由 `pages/FilePage.qml:292` 直接实例化。文档若按「7 个导航页」理解会漏掉它，按「6 个导航页 + 1 个内嵌页」理解才对。
> `centers/` 目录在 `README.md:392` 有提及，但**当前工作区不存在**该目录 → 相关描述已过时「未确认」。

---

## 5.9 QML ↔ C++ 耦合点清单

### 5.9.1 按 QML 文件

| QML 文件 | 使用的 C++ 类型 / 单例 |
|---|---|
| `main.qml` | `MusicApi`(MusicApiService)、`CoverHelper`、`ColorExtractor`、`GetWave`、`AudioEngine`(+枚举 `LoadedMedia/EndOfMedia/PlayingState`)、`SystemTrayManager`(+`Trigger/DoubleClick/Information`)、`QueueModel`、`WebDavCache`、`MediaDevices`(QtMultimedia) |
| `layout/PlayerControl.qml` | `AudioEngine`、`FavoriteSongs`、`MusicApi`（`searchSongsResults/nowIndex/searchSongs/getMusicInfo`）；经 `Options.queue` 用 `QueueModel` |
| `layout/PlayerMaxCenter.qml` | `AudioEngine`、`MusicApi`（`lyricsData/lyricsTranslate/lyricIndex/lyricFollowActive/lyricPositionMs/lyricOffsetMs`）、`LyricsPlugins`（`source/plugins/selectedId`） |
| `layout/MainLyric.qml` | `MeshGradientItem`、`CustomFlow`(+`AlignLeft/AlignRight`)、`GetWave`（经 `Options.getWave.wavePath`）；import `Qt5Compat.GraphicalEffects`（仅逐字染色用 `LinearGradient`）；片元着色器 `qrc:/shaders/shaders/lyricfade.frag.qsb`。结构：`Shape waveItem`+`MultiEffect`+`MeshGradientItem`+静态渐变 · `Item musicpic`(封面/MultiEffect/阴影) · `titleMax`/`artistMax` · `Loader`(CdAlbum，`active: basicCd`) · `Item lyricContent`(ShaderEffect 渐进模糊；`lyricRep`→`lyricItem`→`lyricAnime`/`lyricsText`/`lyricTransText`/`lyricFlow`) · `waitAnimeSection`/`waitOpenAnime`/`waitOutAnime`/`scrollAnime` · `property Component styleOptions`(660) |
| `layout/SmtcBridge.qml` | `WindowsSmtcManager`(+`Playing/Paused/Stopped/Closed`)、`AudioEngine`、`QueueModel`、`Playback` |
| `layout/WindowControls.qml` | `SystemTrayManager`(+`Information`)、`Config`、`Playback`、`Options` |
| `layout/LeftSideBar.qml` · `components/AnimatorWindow.qml` · `components/QLocalView.qml` · `components/QLoadSign.qml` | 仅 `MusicApi`（`recommendSongs/getRecommendSongs/searchSongsResults/songSource/loadState`） |
| `layout/MainContent.qml` | 无直接触点（只 `import 'qrc:/QueMusic/pages'`） |
| `pages/HomePage.qml` | `MusicApi`（`getHotlistMenu/getHotPlaylistMenu/getHotPlaylists/getPersonalRadar/getMusicPlaylists/getPlaylistSongs/getMusicInfo` 等）、`Options.lastSongs`、`Options.queue` |
| `pages/SearchPage.qml` | `MusicApi`（`songSource/searchSongs/searchSongsResults/playlistSong/nowIndex/getMusicInfo/getPlaylistSongs/loadState`） |
| `pages/PlaylistPage.qml` | `MusicApi`（`playlistSong/newSongs/musicPlaylists/toplistList/allPlaylistMenu/singerList/songFilter/globalid/globaltagid/get*`） |
| `pages/FavouritePage.qml` | `FavoriteSongs`、`FavoritePlaylists`、`MusicApi`（`getMusicInfo/getPlaylistSongs/playlistSong/globalid`） |
| `pages/FilePage.qml` | `Songs`(+`searchResults`)、`MyFolders`、`LocalFolders`、`LocalMusicScanner`、`MusicApi`（`readLocalMetadataAsync/readLocalAudioInfo/readLocalLyricsText/readLocalCoverHint/writeLocalMetadata`）、`AudioEngine`；`FolderListModel`(Qt.labs.folderlistmodel)、`QtQuick.Dialogs` |
| `pages/DownloadPage.qml` | `DownloadedMusicModel`、`MusicApi.downloader` → `DownloadManager`（`taskCount/hasActiveTasks/effectiveDownloadDir/removeTask/retryTask`） |
| `pages/WebDavPage.qml` | `WebDavModel`、`WebDav`(WebDavStore)、`AudioEngine`、`Playback.kSourceWebDav` |
| `SettingsView.qml` | `AccountManager`、`LogManager`、`LyricsPlugins`、`FunctionPlugins`、`Songs`、`MusicApi`、`AudioEngine`（详见 §5.7.3） |
| `components/Playback.qml` | `AudioEngine`、`QueueModel`、`MusicApi`、`WebDav`、`WebDavCache` |
| `components/PlayListWindow.qml` | `MusicApi`（`songSource/playlistSong/getMusicInfo`）、`FavoritePlaylists` |
| `components/QListView.qml` | `FavoriteSongs`、`FavoritePlaylists`、`MusicApi`（`songSource/getMusicInfo/loadState`）；注释提及 `FavoritesModel.favoriteRevision/favoriteStateChanged/errorOccurred` |
| `components/MusicInfo.qml` | `MusicApi`（`getComments/comments/loadState`）、`Playback.musicHash/musicSource` |
| `components/DesktopLyrics.qml` · `components/DesktopPlayerWindow.qml` · `components/PlayerOptions.qml` | `AudioEngine`（经 `Playback.player`）；`DesktopLyrics` 另读 `MusicApi.lyricsData` |
| `components/EqualizerPanel.qml` | `AudioEngine`（`property AudioEngine engine`，由宿主传给 `AudioDsp` 接口） |
| `components/PluginHost.qml` | `FunctionPlugins`（`plugins/enabledIds/setEnabled`） |
| `components/PluginManager.qml` | 经 `store` 属性接收 `LyricsPlugins` / `FunctionPlugins` |
| `components/Options.qml` | 声明 `QueueModel`、`QAlertDialog`、`DesktopPlayer`、`SmtcBridge`、`GetWave`、`QOptionDialog`、`SongInfoDialog`、`CoverHelper` 类型的属性 |
| `components/PlayList.qml` | `FavoriteSongs` |
| `lyricsui/LyricsFree.qml` | `MeshGradientItem`、`CustomFlow` |
| `lyricsui/Lyrics3D.qml` | `LyricsStage`（C++ GPU 点云舞台） |

### 5.9.2 任务点名的 30 个注册名 —— 逐个落位

| 注册名 | 类别 | 是否 QML 可实例化 | 谁在用 |
|---|---|---|---|
| `MusicApi` | `api/MusicApiService.h` | ✅ 单例（`QML_NAMED_ELEMENT`） | `main.qml`、`SettingsView.qml`、`layout/PlayerControl|LeftSideBar|PlayerMaxCenter`、全部 6 个在线页面、`components/*` 多个 |
| `AccountManager` | `cpp/AccountManager.h` | ✅ 单例 | **仅 `SettingsView.qml`** |
| `LogManager` | `cpp/LogManager.h` | ✅ 单例 | **仅 `SettingsView.qml`** |
| `GetWave` | `cpp/GetWave.h` | ✅ 普通类型 | `main.qml` id `getWave` → `Options.getWave`；`layout/MainLyric.qml` 读 `wavePath` |
| `CoverHelper` | `cpp/CoverHelper.h` | ✅ 普通类型 | `main.qml` id `coverHelper` → `Options.coverHelper`；`SettingsView` 清缓存 |
| `ColorExtractor` | `cpp/ColorExtractor.h` | ✅ 普通类型 | `main.qml` id `colorExtractor`（局部使用，未注入 Options） |
| `CustomFlow` | `cpp/CustomFlow.h` | ✅ 普通类型 | `layout/MainLyric.qml:496`、`lyricsui/LyricsFree.qml:571` |
| `LocalMusicScanner` | `cpp/LocalMusicScanner.h` | ✅ 普通类型 | `pages/FilePage.qml:530` |
| `DownloadManager` | `cpp/DownloadManager.h` | ✅ 普通类型（**但 QML 里从不 `DownloadManager {}`**） | 只经 `MusicApi.downloader`（`Q_PROPERTY(DownloadManager*)`）访问：`pages/DownloadPage.qml` |
| `DownloadedMusicModel` | `cpp/DownloadedMusicModel.h` | ✅ 普通类型 | `pages/DownloadPage.qml:14` |
| `FavoritesModel` | `cpp/Favorites.h`（基类） | ❌ 无 `QML_ELEMENT` | 只在注释里被提及（`components/QListView.qml:77,87`）；实际用三个子类单例 |
| `FavoriteSongs` | `cpp/AppModels.h:56` | ✅ 单例（filter `"song"`） | `layout/PlayerControl.qml`、`pages/FavouritePage.qml`、`components/PlayList.qml`、`components/QListView.qml`、`PlayListWindow.qml` |
| `FavoritePlaylists` | `cpp/AppModels.h:57` | ✅ 单例（`"playlist"`） | `pages/FavouritePage.qml`、`components/PlayListWindow.qml`、`components/QListView.qml` |
| `FavoriteArtists` | `cpp/AppModels.h:58` | ✅ 单例（`"artist"`） | 全项目 QML **未见引用**（`pages/FavouritePage.qml` 的「歌手」子页未确认是否使用） |
| `FolderModel` | `cpp/FolderModel.h`（基类） | ❌ 无 `QML_ELEMENT` | 经 `MyFolders` / `LocalFolders` 使用 |
| `MyFolders` | `cpp/AppModels.h:38` | ✅ 单例（`"my"`） | `pages/FilePage.qml` |
| `LocalFolders` | `cpp/AppModels.h:39` | ✅ 单例（`"local"`） | `pages/FilePage.qml` |
| `Songs` | `cpp/AppModels.h:42` | ✅ 单例 | `pages/FilePage.qml`、`SettingsView.qml:1073` |
| `QueueModel` | `cpp/QueueModel.h` | ✅ 普通类型 | `main.qml` id `playListModel`；`Options.queue` / `Playback.queue`；类型被 `layout/SmtcBridge.qml`、`components/Options.qml`、`components/Playback.qml` 声明 |
| `SearchResultModel` | `cpp/SearchResultModel.h` | ❌ 无 `QML_ELEMENT` | 只在注释（`components/QSortModel.qml:55`）；搜索结果实际走 `MusicApi.searchSongsResults`（`OnlineListModel`） |
| `SystemTrayManager` | `cpp/SystemTrayManager.h` | ✅ 普通类型 | `main.qml` id `systemTray`；`layout/WindowControls.qml` 的 `property SystemTrayManager tray` |
| `WindowsSmtcManager` | `cpp/WindowsSmtcManager.h` | ✅ 普通类型 | `layout/SmtcBridge.qml:69`（`property WindowsSmtcManager mgr`） |
| `AudioEngine` | `cpp/audio/AudioEngine.h` | ✅ 普通类型 | `main.qml` id `mainMedia`；被 7 个 QML 文件用 `Playback.player` 声明为 `readonly property AudioEngine player` |
| `WebDav` | `cpp/webdav/WebDavStore.h` | ✅ 单例（`QML_NAMED_ELEMENT`） | `pages/WebDavPage.qml`、`components/Playback.qml` |
| `WebDavCache` | `cpp/webdav/WebDavCache.h` | ✅ 单例 | `main.qml:709`（`Connections`）、`components/Playback.qml:97,112` |
| `LyricsPlugins` | `cpp/plugins/LyricsPluginStore.h` | ✅ 单例 | `layout/PlayerMaxCenter.qml`、`SettingsView.qml:2162` |
| `FunctionPlugins` | `cpp/plugins/FunctionPluginStore.h` | ✅ 单例 | `components/PluginHost.qml`、`SettingsView.qml:2174` |
| `MeshGradientItem` | `shaders/MeshGradientItem.h` | ✅ 普通类型 | `layout/MainLyric.qml:94`、`lyricsui/LyricsFree.qml:100` |
| `LyricsStage` | `shaders/LyricsStage.h` | ✅ 普通类型 | `lyricsui/Lyrics3D.qml:153` |
| `PlayerDatabase` | `cpp/PlayerDatabase.h` | ❌ 无 `QML_ELEMENT` | **QML 层不可见**（纯 C++，经 `DbService` 使用） |
| `DbService` | `cpp/DbService.h` | ❌ 非 `QObject`（无 QML 暴露） | **QML 层不可见** |

---

## 5.10 坑与约定

### 来自源码注释的硬约束

| # | 约定 | 出处 | 违反后果 |
|---|---|---|---|
| 1 | **一律用 QML 单例，不用 `setContextProperty`**（`configDir` 是唯一例外） | `cpp/AppModels.h:6-7`、第 1 部分 §1.2 | context property 无法被 `qmlcachegen` 编译期解析；必须按类型名访问才能编译 |
| 2 | **不能把多个模型塞进「一个单例的多个属性」** | `cpp/AppModels.h:7` | 属性查找会被拒绝继续 |
| 3 | 模型单例**构造函数绝不能带默认参数** | `cpp/AppModels.h:9-11` | `singletonConstructionMode()` 里 `is_default_constructible` 优先于 `HasSingletonFactory`，`create()` 永不执行 → 过滤类型失效、收藏列表列出全部数据（静默） |
| 4 | 播放引擎**不靠上下文继承**访问宿主局部 id；需要它的文件各自声明 `readonly property AudioEngine player: Playback.player` | `layout/PlayerControl.qml:18`、`layout/PlayerMaxCenter.qml:12`、`SettingsView.qml:14` 等 | 跨文件引用局部 id 无法被 AOT 编译 |
| 5 | 歌词界面的**自有设置也必须单例化** | `lyricsui/LyricsFreeConfig.qml:5` | 「以前是界面根里的 `Settings` + 文件内 id（cfg），子组件跨边界引用无法被 AOT 编译，改为单例」 |
| 6 | 歌词界面与宿主的**唯一样式写入口是 `requestStyle` 白名单** | `layout/PlayerMaxCenter.qml:29-47`、`layout/MainLyric.qml:34-38` | 模块直接改宿主状态会破坏「整包替换」契约 |
| 7 | 歌词界面数据**只经 `inject(it)` 注入**，缺失的 key 跳过 | `layout/PlayerMaxCenter.qml:49-77` | 便于直接换插件 |
| 8 | 歌词界面**换主题 = 换 Loader 加载的文件**；宿主不可见时 `active:false` 整体卸载 | `layout/PlayerMaxCenter.qml:126-139` | — |
| 9 | `QOptionDialog` 的 `blurSource` 默认指向 `main.qml` 的 `Options.mainLayout`，**跨文件使用必须调用方显式设置** | `components/ColorPickerDialog.qml:7`、`components/FontFamilyDialog.qml:6` | 模糊源错位 |
| 10 | 需要点击的标题栏子项必须 `windowAgent.setHitTestVisible(item, true)`，否则点击被窗口拖拽吞掉 | `main.qml:111-116`（`syncTitleBarHitTest`） | 按钮点不动 |
| 11 | **原生 WinRT / FFmpeg 同步回调进 QML 时统一经 `Qt.callLater` 延后** | `layout/SmtcBridge.qml:5,25,47` | 重入切歌逻辑 |
| 12 | 切歌必须走 `Playback.swap(callback)` 淡出静音后再换源；`onPlayingChanged` 里 `finishSwap()` 淡回 | `main.qml:729-733,861-865` | 上一首仍在播时直接换 `source` 会截断波形产生爆音 |
| 13 | 会话恢复 / 历史加载 / 封面缓存维护**必须放进 `Qt.callLater`**，不能占首帧 | `main.qml:65-76` | 启动卡顿 |
| 14 | 环境变量类开关（`QSG_RHI_BACKEND` / `QSG_NO_VSYNC` / `QSG_USE_SIMPLE_ANIMATION_DRIVER`）**必须在 `QApplication` 之前设** | `main.cpp:117-128` | 运行时改无效，必须重启 |
| 15 | `QIcon` 只认 `:/xxx`，`qrc:/xxx` 会加载失败 | `main.cpp:143` | 应用图标不显示 |
| 16 | 退出统一走 QML 的 `Qt.quit()` + `WindowControls`，`setQuitOnLastWindowClosed(false)` | `main.cpp:133`、`layout/WindowControls.qml` | 关窗即退出，托盘常驻失效 |
| 17 | 关闭流程必须走 `close.accepted = false` + `win.toClosing()`（即使没开托盘） | `main.qml:130-139` | 会话与窗口几何丢存 |
| 18 | **`Esc` 的返回协议：「先 `Options.exit()`，再 `exitIndex -= 1`」**，子窗口在 `onExit` 里按 `exitIndex <= winIndex` 自分关闭 | `main.qml:151-163`、`AnimatorWindow.qml:37-48`、`PlayListWindow.qml:59-70` | 顺序颠倒会导致子窗口不关或多关一层 |
| 19 | 键位录制期间必须同时 `Config.openShortCut = false` 与 `Options.recordingShortCut = true`，结束后还原 | `SettingsView.qml:1823-1849`、`main.qml:154` | 录制时按键触发播放 |
| 20 | `main.qml` 里 `ShortCut[flag]` 的拼接与 `SettingsView.globalPropertyName()` **必须同构** | `main.qml:194` vs `SettingsView.qml:1772` | 单功能全局开关失效 |
| 21 | 搜索歌曲统一入口 `window.doSearch(text)`：清结果 → `contentIndexed(5)` → 写 `Config.searchList`（去重后插到首位）→ `MusicApi.searchSongs` → `Options.reSearch()` → `exitIndex = 1` | `main.qml:118-127`、`layout/PlayerControl.qml:59-66` | — |
| 22 | 侧栏点击是**两个调用**：`sidebar.indexed(i)`（动画 + 一次性数据）+ `mainCont.contentIndexed(i)`（切页）；`MainContent.pages` 的**顺序即侧栏下标**，改动必须同步 `LeftSideBar.navModel` | `layout/LeftSideBar.qml:260-264`、`layout/MainContent.qml:14` | 点 A 出 B |
| 23 | 搜索页（下标 5）**没有侧栏入口**，只能靠 `contentIndexed(5)`；离开靠 `LeftSideBar` 的 `onExit` 回写 | `layout/MainContent.qml:15`、`layout/LeftSideBar.qml:60-68` | 卡在搜索页 |
| 24 | `SettingsView.navModelSettings` 与 `settingStack.setPages` **顺序必须一一对应**（同为 8 项） | `SettingsView.qml:524-534` vs `629-638` | 点通用出 Debug |
| 25 | `Options` 是**注入容器**：新增全局对象必须在 `main.qml:21-35` 登记并在 `components/Options.qml` 声明类型属性 | `components/Options.qml:10`（「跨文件共享对象：main.qml 启动时注入」） | 其它文件拿不到 |

### 来自 `README.md`「Que Graph UI」的性能约定

| # | 约定 | 原文要点 |
|---|---|---|
| 26 | **少嵌套** | 「能用一个 `Item` 解决的问题不套两层」 |
| 27 | **定位优先级**：`x/y/width/height` → `anchors` → `Row/Column` → `Layouts`（越靠前越省） | 代码实践一致：`layout/` 与 `pages/` 大量用绝对 `x/y/width/height`，几乎不用 `QtQuick.Layouts` |
| 28 | **绑定要精**：只让真正变化的属性参与绑定 | 例：`layout/PlayerControl.qml:21-27` 用 `posSec` 秒级中间层，避免 `position` 20Hz 刷新时每秒重建字符串 20 次 |
| 29 | **页面按需加载**：切页走异步 `Loader`，重对象后台线程构建 | `MainContent` 的 6 个 Loader + `main.qml` 的 `settingsView` / `textWatch` / `desktop*Loader` 全部 `asynchronous: true` |
| 30 | **全项目标注静态类型**，`qmlcachegen` 才能编译成 C++/字节码 | 所有函数签名写 `(x: int): void`、`(s: string): bool`；`property` 声明具体类型（`readonly property AudioEngine player: Playback.player`）；文件头 `pragma ComponentBehavior: Bound` |
| 31 | **拒绝深嵌套**，自研组件库保持轻量 | `components/` 56 个文件里最大的是 `EqualizerPanel.qml`(597) |

补充观察（非文档明写，但代码高度一致）：

| # | 观察 | 证据 |
|---|---|---|
| 32 | 颜色/间距几乎不写字面量，全部走 `Theme.*` / `Style.*`；**定位几乎只用绝对 `x/y/width/height`**，`QtQuick.Layouts` 基本不出现 | 全部 QML 的 `color:` 绑定；`layout/`+`pages/` 的根子项 |
| 33 | 「侧栏 + 选择条 + 页面列表 + 切入动画 + 页头 60px」这套骨架在 `LeftSideBar`/`MainContent` 与 `SettingsView` 里**各复制了一份**（`choicebar` vs `choicebar1`、`pageAnine` vs `setPageAnine`、`indexed()` vs `index1ed()`、`y:60` vs `height-60`） | 两处函数体几乎逐行同构，改一处要手动同步另一处 |
| 34 | 顶层用 `Window` 而非 `ApplicationWindow`，因此没有 `menuBar/header/footer`，一切靠手工定位 | `main.qml:12` |
| 35 | `SettingsView` 的 8 个分类容器**不用 `Loader`**，一次性全构建；只有整个设置页本身是 `Loader`（`asynchronous`） | `main.qml:554`、`SettingsView.qml:629-638` |

### 已知的文档/代码不一致（供后续修订）

| 项 | 事实 |
|---|---|
| 行数口径 | 本文档统一采用 `[System.IO.File]::ReadAllLines()`；`main.qml` = 1068、`SettingsView.qml` = 2934、`FilePage.qml` = 1985，与 `read`/`grep` 行号一致（全量见附录 A）。**不要用 `Get-Content`**，它会漏空行 |
| `README.md:392` 提到的 `centers/` 目录 | 当前工作区**不存在** |
| `README.md:376` 把 `MusicApi.qml` 列为 `components/` 下的文件 | 实际 `components/` 下**没有** `MusicApi.qml`；`MusicApi` 是 C++ 单例（`api/MusicApiService.h`） |
| `main.qml` 的 `musicControl` 快捷键分支 | 引用了不存在的 id `controlMaxLoader` / `minedAnimation` 与未定义方法 `window.playermined()` → 疑似重构遗留死代码「未确认」 |
| `components/Config.qml:63` `property bool displayDebugControl` | 无初值、全项目未见读写「未确认」 |
| `pages/FavouritePage.qml` 的「歌手」子页 | `FavoriteArtists` 在 QML 中未出现，该页数据源「未确认」 |
---

← 返回 [索引 README.md](README.md)
