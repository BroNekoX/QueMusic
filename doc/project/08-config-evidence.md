# 第 8 部分 · 配置落盘实测清单

> 本文件是 [QueMusic 项目全景文档索引](README.md) 的一个分片；任务定位与其它部分请见 [README.md](README.md)。

> 本部分内容来自**实际运行后产生的文件**，不是从源码推断的。源码侧声明清单见第 7 部分。

## 8.1 文件位置与分组归属

主配置：`%LOCALAPPDATA%\BroNekoX\QueMusic.ini`（由 `main.cpp` 的 `ConfigLocation + "/BroNekoX/QueMusic.ini"` 得到）

分组 ↔ 生产者对照：

| 分组 | 生产者 | 声明位置 |
|---|---|---|
| `[Options]` | `Config` 单例 | `components/Config.qml`（`Settings { category: "Options" }`） |
| `[Style]` | `Style` 单例 | `components/Style.qml`（`category: "Style"`） |
| `[ShortCuts]` | `ShortCut` 单例 | `components/ShortCut.qml`（`category: "ShortCuts"`） |
| `[LyricsFree]` | `LyricsFreeConfig` 单例 | `lyricsui/LyricsFreeConfig.qml`（`category: "LyricsFree"`） |
| `[Lyrics3D]` | `Lyrics3DConfig` 单例 | `lyricsui/Lyrics3DConfig.qml`（`category: "Lyrics3D"`） |
| `[AudioDsp]` | `AudioEngine::saveSettings()` | `cpp/audio/AudioEngine.cpp` |
| `[LastMedia]` | C++（上次播放媒体） | 见第 3 / 4 部分 |
| `[Plugins]` | 插件存储 | `cpp/plugins/*` |
| `[webdav]` | WebDAV 服务器列表 | `cpp/webdav/WebDavStore.*` |
| `[Player]` | 疑似遗留 | 仅 `cycleIndex` 一个键，与 `Config.cycleIndex` 重复 |

另有**独立于主配置**的文件：`<exe 目录>\Account.ini`（`AccountManager` 自建，存网易云 / 酷狗的 Cookie 与登录态，不受 `QSettings::setPath` 影响），以及 `%LOCALAPPDATA%\BroNekoX\QueMusicQuality.json`（音质缓存）。

## 8.2 实测存在的键（运行时真值）

`[Options]`（Config.qml 声明 + 遗留混杂）：
`gpuRenderMode` `timerAnimator` `closeToManage` `soundQuality` `noWindowKit` `openShortCut` `editSource` `searchList` `downloadFolder` `scanTime` `displayDebugControl` `mainMusicSource` `dataUrl` `audioDevice` `cacheSize` `autoPlay` `displayFps` `debug` `musicVolume` `cacheUrl` `softwareRender` `qmlAnimator` `useDefaultDevice` `autoUpdate` `serverAgency` `metaDataSource` `sleepMinutes` `historyLimit` `shuffleAvoid` `playHistory` `lastQueueIndex` `seekStep` `lastQueue` `fadeEnabled` `autoRestoreQueue` `fadeMs` `playerRateIndex` `cycleIndex` `volumeStep` `winX` `rememberWindow` `winY` `winW` `winH` `picCache`

**遗留键（源码里已无声明，实际仍存在于 INI）**：`depth` `bufferSize` `sampleRate` `equalizer` `resumePosition` `globalShortcut*`（13 个，已迁到 `[ShortCuts]`）`recordingShortCut`

`[Style]`：`backpic` `backmode` `shadowBlur` `shadowSize` `theme` `color` `colorList`（`QList<QColor>`，以 `@Variant` 存） `textTip` `shadowYOffset` `labelRadius` `fontFamily` `lyricSize` `uilevel` `text` `sidebarColor` `layerEnabled` `lyricHideGui` `premiumLyricAnime` `borderDepth` `pageTitle` `blurSize` `texticonH` `textmain` `textH1` `sidebarStyle` `noControlRadius` `cubeRadius` `animeSpeed` `textWidth` `highQualityBlur` `maskBlur` `textH2` `backgroundImage`（`file:///...`） `glmode` `shadowXOffset` `premiumAnime` `menutheme` `texticon` `waveDisplay` `flowStyle` `homeLayout` `animeType` `noOpacityControl`

**遗留键（已迁往 `[LyricsFree]` / `[Lyrics3D]`）**：`backFlowQuality` `lightEffect` `spotSpeed` `closeScrollAnime` `lyricSpacingScale` `lyricLineColor` `lyricCardAngle` `lyricCurrentScale` `lyricSungColor` `lyricIdleOpacity` `lyricWallpaper` `lyric3d*`（22 个）

`[ShortCuts]`：13 个键位 `favorite=Ctrl+D` `seekBack=Ctrl+Left` `forward=Right` `musicControl=Up` `play=Space` `playList=Alt` `abLoop=Ctrl+B` `seekForward=Ctrl+Right` `back=Left` `playerOptions=Ctrl+T` `volumeUp=Ctrl+Up` `mute=Ctrl+M` `volumeDown=Ctrl+Down` + 13 个 `globalShortcutXxx=bool`

`[AudioDsp]`：`eqEnabled` `eqGains`（`@ByteArray([6,5,3,0,1,2,3,4,4,3])`，10 段） `eqQ` `preampDb` `autoHeadroom` `balance` `stereoWidth` `mono` `swapChannels` `gainLeft` `gainRight` `rgMode` `rgPreamp` `rgPreventClip` `limiter` `limiterDb` `dspEnabled` `outputSampleRate` `outputBufferMs` `pitchSemitones`

`[LyricsFree]`：`bgStyle` `bgImage` `bgVideo` `showTitle` `showArtist` `coverReflection` `lyricSpacingScale` `lyricCardAngle` `lyricCurrentScale` `lyricIdleOpacity` `lyricSungColor` `lyricLineColor`

`[Lyrics3D]`：`preset` `bright` `saturate` `audio` `density` `fieldScale` `camDist` `camSens` `tilt` `lyricScale` `lyricY` `lyricZ` `glow` `sweep` `beatGlow` `floatAmt` `textColorCustom` `glowColorCustom`

`[Plugins]`：`lyrics = builtin.default`（当前歌词界面插件）；功能插件启用态用 `function\<插件名> = bool`；插件私有设置用 `settings\<插件名>\<键>`（**含反斜杠**，QSettings 子组语义）

`[webdav]`：`servers`（`QVariant` 序列化，含加密后的凭据）

`[LastMedia]`：`source` `name` `artist` `cover` `hash` `position` —— 用于"上次播放"与断点续播

## 8.3 与源码不一致的地方（改配置相关代码前必须知道）

1. **INI 里存在大量已废弃的键**。判断某个键是否还是"活的"，唯一依据是**源码里是否还有 `Settings` 属性声明**（见第 7 部分），INI 里的残留不代表功能存在。
2. **同一个概念可能同时出现在两个分组**（如 `cycleIndex` 在 `[Options]` 与 `[Player]`；`lyricSpacingScale` 在 `[Style]` 与 `[LyricsFree]`）。新增读取逻辑时要选源码里声明的那一份。
3. `%APPDATA%\BroNekoX\QueMusic.ini` 是**旧版本遗留**，与生效配置无关，不要改它。
4. `Options.gpuRenderMode` / `timerAnimator` / `qmlAnimator` / `softwareRender` / `noWindowKit` 这类渲染开关由 `main.cpp` **在 QApplication 之前**读取，改完必须重启进程（不是重启窗口）。
5. 数据库是独立文件（`%APPDATA%\BroNekoX\QueMusic\player_data.db`），**不在 INI 里**；`doc/qm-dbcheck.bat` 是收藏写库失败的专用诊断脚本（会尝试删 `-wal`/`-shm` 并用 sqlite3 直接写库验证）。
6. 账号 Cookie 在 `<exe 目录>\Account.ini`，与主配置分离；排查"登录态丢失"要去那里看。
---

← 返回 [索引 README.md](README.md)
