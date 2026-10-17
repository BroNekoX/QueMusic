# 第 6 部分 · QML 组件库与歌词界面（components/、lyricsui/）

> 本文件是 [QueMusic 项目全景文档索引](README.md) 的一个分片；任务定位与其它部分请见 [README.md](README.md)。

统计口径：`components/*.qml` 61 个文件 + `lyricsui/*.qml` 4 个文件；`根类型` 取文件首个非注释类型声明；`单例` 指带 `pragma Singleton` 且已在 CMake 登记的类型。
`关键属性/信号/方法` 中的属性均为 QML 可写公共属性（`readonly` 已标注），delegate 内 `required property` 在末列以 `delegate:` 前缀给出。

## 6.1 组件目录总表

| 文件 | 根类型 | 单例 | 职责 | 关键属性 / 信号 / 方法 |
|---|---|---|---|---|
| AccountCard.qml | Rectangle | — | 账户/平台信息卡：头像+标题+说明，点击外链 | `source`, `title`, `text`, `openUrl`；方法：无（点击走 `Qt.openUrlExternally`） |
| AnimatorWindow.qml | Item | — | 带动画的全屏子窗口容器（我的文件夹等），页面经 default 属性塞入内部 Loader | `image`, `winIndex`, `title`, `mainTarget`, `haveControl`, `transX`, `originX`, `animeOnList`, `animeOutList`, `animeType`, `songSource`；`default property alias content`；方法：`opened(title,image)`, `closed(title,image)`, `onExit()` |
| CdAlbum.qml | Item | — | 黑胶唱片：`cd.png` 底盘 + `cdpoint.png` 唱针 + 封面旋转 | `source`, `rotation`；无信号 |
| ColorPickerDialog.qml | QOptionDialog | — | 颜色选择：SV 色板 + HSV 滑块 + HEX 输入，派发 `accepted()` | `selectedColor`, `hue`, `sat`, `val`；信号：`accepted()`；方法：`setColor(c)`, `openColor(c)`, `syncFromHsv()`, `applyHex()`, `pick(px,py)` |
| Config.qml | Settings | ✅ | 全局业务配置持久化（category `Options`） | `musicVolume`, `closeToManage`, `rememberWindow`, `winX`, `winY`, `winW`, `winH`, `autoUpdate`, `searchList`, `openShortCut`, `cycleIndex`, `picCache`, `soundQuality`, `useDefaultDevice`, `audioDevice`, `autoPlay`, `playerRateIndex`, `volumeStep`, `seekStep`, `fadeEnabled`, `fadeMs`, `sleepMinutes`, `historyLimit`, `shuffleAvoid`, `autoRestoreQueue`, `playHistory`, `lastQueue`, `lastQueueIndex`, `downloadFolder`, `scanTime`, `cacheUrl`, `dataUrl`, `metaDataSource`, `mainMusicSource`, `serverAgency`, `cacheSize`, `editSource`, `noWindowKit`, `softwareRender`, `gpuRenderMode`, `displayFps`, `debug`, `timerAnimator`, `qmlAnimator`, `displayDebugControl` |
| DesktopLyrics.qml | Window | — | 桌面歌词窗：无边框/置顶/透明，自算当前行 | `active`, `lyricSize`, `player`(ro), `lyricsData`, `currentIndex`, `nextIndex`, `bw`, `dragOffsetX`, `dragOffsetY`；方法：`updateCurrentIndex()`, `onLyricsDataChanged()`（Connections 槽） |
| DesktopPlayer.qml | Popup | — | 桌面部件选择面板，切换两个 Loader 的 `active` | `desktopPlayerMode`；方法：`changeDesktopPlayerMode(index)`；`delegate: required modelData, index` |
| DesktopPlayerWindow.qml | Window | — | 桌面小窗播放器（自绘拖拽/缩放/标题栏） | `topWindow`, `bw`, `player`(ro), `dragOffsetX`, `dragOffsetY` |
| EqualizerPanel.qml | Column | — | 均衡器与音频处理面板：10 段参数 EQ + 声道工具 + ReplayGain + 限幅 | `engine`(AudioEngine), `cardColor`, `display`(0/1/2 分区), `controlWidth`；只读：`eqBands`, `eqMax`, `faderHeight`, `trackTop`, `trackHeight`, `sampleRates`；方法：`apply(y)`；`delegate: required modelData, index` |
| FontFamilyDialog.qml | QOptionDialog | — | 字体族选择（可搜索，每行用自身字体渲染） | `currentFamily`, `selectedFamily`, `scrollToY`；只读：`families`, `filteredFamilies`, `wheelHeightCount`；信号：`accepted()`；方法：`openFamily(family)` |
| Fonts.qml | QtObject | ✅ | 注册自带字体并暴露家族名 | 只读：`iconLoader`, `textLoader`, `icon`, `text` |
| MusicInfo.qml | Popup | — | 音乐详情侧栏：封面/标题常驻，下部分页（信息/评论；本地音乐无评论） | 只读：`player`, `isLocal`, `bodyY`, `wheelHeightCount`, `scrollBottom`；`scrollToY`, `isEnd`；方法：`loadComments()`, `refreshComments()`, `onMusicHashChanged()`；`delegate: required user, avatar, content, liked` |
| Options.qml | QtObject | ✅ | 宿主注入总线 + 版本号 + 主题驱动中心 | `queue`, `dialog`, `desktop`, `smtc`, `mainLayout`, `getWave`, `textWatch`, `picWatch`, `songInfo`, `coverHelper`, `desktopPlayerLoader`, `desktopLyricsLoader`, `openMaxLyric`, `settingOutAnime`, `mainSearchInput`, `exitIndex`, `homeLoaded`, `playlistLoaded`, `recordingShortCut`, `searchText`, `lastSongs`；只读：`isMacOS`, `version`, `versionCode`, `pluginApi`, `darkis`；信号：`changeOptions()`, `exit()`, `changeUi()`, `changeTheme()`, `warned(string,int)`, `reSearch()` |
| OptionsLastSongs.qml | Settings | — | 上次播放记录（category `LastMedia`，带类型便于 AOT） | `name`, `artist`, `cover`, `hash`, `source` |
| PlatformCard.qml | Rectangle | — | 音乐平台选择卡（选中态/登录态/扫码按钮） | `chooseColor`, `chooseColor1`, `choose`, `isLogin`, `showLogin`, `name`, `header`, `idleText`, `text`；信号：`clicked()`, `logined()` |
| Playback.qml | QtObject | ✅ | 播放控制中枢：队列/循环/随机袋/淡入淡出/AB 循环/睡眠定时/历史持久化 | `player`, `queue`, `musicTitle`, `musicArtist`, `musicHash`, `musicSource`, `localLyricsRequestPath`, `autoSkipCount`, `muted`, `armed`, `nextAction`, `afterFade`, `keepSilent`, `abA`, `abB`, `sleepMode`, `sleepRemain`, `history`, `recent`, `bag`, `swapTimer`, `fadeTimer`, `guardTimer`, `sleepTimer`, `saveTimer`, `mediaHook`；只读：`count`, `cycleIndex`, `cycleIcon`, `cycleTip`, `outVolume`, `fadeOutMs`, `fadeInMs`, `guardMs`, `fadeOn`, `volumeStep`, `abArmed`, `sleepLabel`, `kSourceWebDav`；方法：`goTo`, `next(random)`, `previous`, `enqueue(item)`, `playNext(item)`, `playItem(item)`, `sourceText(source)`, `playWebDav(url,name,artist)`, `startTrack(index)`, `noteNowPlaying()`, `playLocalSong(path,name)`, `indexOfPath(p)`, `nextShuffle()`, `notePlayed(i)`, `fadeIn()`, `fadeTo(v,then)`, `swap(action,fadeBack)`, `runNext()`, `armFadeIn()`, `finishSwap()`, `togglePlay()`, `setVolume(v)`, `stepVolume(d)`, `toggleMute()`, `seekBy(ms)`, `seekBack()`, `seekForward()`, `setAbPoint(which)`, `clearAb()`, `onPositionChanged()`, `armSleep(mode,minutes)`, `stopSleep()`, `sleepEnd()`, `pushHistory(e)`, `saveQueue()`, `restoreSession()`, `clearHistory()`, `loadHistory()`, `queueSave()`, `flush()`, `writeHistory()`, `fmt(ms)` |
| PlayerOptions.qml | QOptionDialog | — | 播放器选项：倍速/音质/输出设备/AB/睡眠/淡变/跳转步长 | 只读：`player`, `rates`, `seekSteps`；`onCancel` 打开 `optionsEQ`（EqualizerPanel） |
| PlayList.qml | Popup | — | 播放列表面板（定位当前行/清空/逐行菜单） | `model`(alias), `filter`, `scrollToY`, `scrollMultiplier`；方法：`locateCurrent()`；`delegate: required index, name, songer, path, source` |
| PlayListWindow.qml | Item | — | 歌单详情子窗口（封面信息 + 曲目列表 + 收藏） | `id`, `cover`, `title`, `artist`, `descript`, `duration`, `playcount`, `standTopMargin`, `transX`, `originX`, `animeOnList`, `animeOutList`, `animeType`, `winIndex`, `mainTarget`, `songSource`；`default property alias content`；方法：`opened(info)`, `closed()`, `onExit()` |
| PluginApi.qml | QtObject | — | 功能插件接口对象（宿主为每个插件创建，注入插件根的 `api`） | `pluginId`, `pluginName`, `pluginDir`, `pluginUrl`, `window`, `owned`；只读：`apiVersion`, `settings`, `loaderTemplate`；方法：`slot(name)`, `mount(slotName,source,props)`, `unmount(obj)`, `destroyAll()`, `toast(text,type)`, `warn(text)` |
| PluginHost.qml | Item | — | 功能插件宿主：按 `FunctionPlugins.enabledIds` 起加载器、失败自动停用 | 信号：`pluginLoaded()`；`api`, `instance`, `loading`, `wanted`；方法：`load()`, `finish(component)`, `unload()`, `fail(text)`；`delegate: required id, name, source, path` |
| PluginManager.qml | Item | — | 插件管理页（歌词界面单选 / 功能插件多选共用） | `store`, `multi`, `pluginKind`；只读：`plugins`, `compact`；`delegate: required id, name, builtin, preview, description, author, version`，只读 `isOn` |
| PluginSlot.qml | QtObject | — | 声明一个扩展点：完成时注册 / 销毁时注销 | `slotName`, `target` |
| PluginSlots.qml | QtObject | ✅ | 扩展点注册表（名字 → 容器） | 只读：`items`；方法：`register(name,target)`, `unregister(name)`, `item(name)` |
| QAlertDialog.qml | Popup | — | 模糊底提示框，可带输入框 | `blurSource`, `rectXy`, `title`(alias), `message`(alias), `input`(alias), `cancelText`, `confirmText`, `isInput`, `dismissOnOverlay`；信号：`confirm()`, `cancel()`；方法：`onExit()` |
| QBigDrop.qml | Rectangle | — | 大图卡片单选（图 + 文案） | `model`, `picModel`, `singleWidth`, `choice`；信号：`transformed(int)`；`delegate: required modelData, index` |
| QBlurCard.qml | Item | — | 实时背景模糊卡片（抓区域 → 模糊+饱和+圆角 单 frag） | `blurSource`, `blur`, `blurMax`, `saturation`, `highQuality`, `borderRadius`, `cardColor`, `borderColor`, `borderWidth`, `fillColor`, `shadowEffect`, `masked`, `rectXy`, `src`；`default property alias content` |
| QBlurTapBar.qml | Item | — | 模糊胶囊页签栏（可拖动、可掩码） | `blurSource`, `blurAmount`, `dragable`, `blurMask`, `rectXy`, `blurMax`, `borderRadius`, `borderColor`, `borderWidth`, `tabWidth`, `model`, `choiceIndex`；信号：`tabChange(int)`；`delegate: required modelData, index`，`isSelected` |
| QButton.qml | Button | — | 图标+文字胶囊按钮 | `text`(Button 原生), `iconCharacter`, `iconFontFamily`, `tipText`, `textColor`, `iconColor`, `shadowEnabled`, `textBetween`, `borderWidth`, `borderColor`, `radius`, `pressedScale`, `shadowColor`, `hoverColor`, `buttonColor`, `iconSize`, `fontSize` |
| QDrop.qml | Rectangle | — | 下拉单选（`useId` 模式读 `model[i].description`，用于音频设备列表） | `buttonColor`, `text`, `useId`, `icon`, `model`, `choice`, `enabled`, `textColor`, `iconFontFamily`, `cardRadius`；信号：`transformed(int)`；`delegate: required modelData, model, index` |
| QFloatCard.qml | Rectangle | — | 悬浮卡：hover 抬升 + 阴影加深 | 信号：`clicked()`；`property alias controlItem`（内容专用容器） |
| QGridView.qml | GridView | — | 网格滚动容器（统一滚轮/滚动条/回到顶部） | `scrollToY`, `enabled`；只读：`wheelHeightCount`, `scrollBottom`；信号：`topScroll()`；方法：`scrollTop()` |
| QHead.qml | Item | — | 小节标题（左侧主题色条 + 加粗标题） | `text` |
| QInput.qml | Rectangle | — | 单行输入框（focus 时边框变主题色） | `inputText`(alias), `echoMode`(alias)；信号：`entered()` |
| QListView.qml | ListView | — | 在线歌曲/歌单列表：收藏、下一首、加入列表、下载、信息弹窗内置 | `scrollToY`, `headerModel`, `menuModel`, `selectedIndices`, `isList`, `artistX`, `toolX`, `toolText0`, `toolText1`, `menu`(alias), `isEnd`, `toolHandler`, `menuHandler`；只读：`selectedSet`, `favType`, `favModel`, `localDriveRe`, `wheelHeightCount`, `scrollBottom`；信号：`clicked(int)`, `ended()`；方法：`scrollTop()`, `itemAt(i)`, `keyOf(it)`, `titleOf(it)`, `artistOf(it)`, `coverOf(it)`, `hashOf(it)`, `sourceOf(it)`, `isFav(i)`, `toggleFavorite(i)`, `enqueue(i,next)`, `download(i)`, `showInfo(i)`, `runMenuAction(choice)`, `runToolAction(i,tool)`；`delegate: required index, modelData` + 行级 `required index, model, title, artist, cover, duration` |
| QLoadSign.qml | Item | — | 加载中指示（`loader.gif`），由 MusicApi 的 loaded/finished 驱动 | `loader`；方法：`loadAction()`, `finish()`, `onFinished()`, `onLoaded()` |
| QLocalView.qml | ListView | — | 本地音乐列表：只剩点击/菜单/工具三路信号，动作全交页面处理 | `scrollToY`, `headerModel`, `menuModel`, `selectedIndices`, `artistX`, `toolX`, `toolText0`, `toolText1`, `menu`(alias), `isEnd`；只读：`selectedSet`, `wheelHeightCount`, `scrollBottom`；信号：`clicked(int)`, `menuClicked(int,int)`, `toolClicked(int,int)`, `ended()`；方法：`scrollTop()`；`delegate: required index, modelData` |
| QMenu.qml | Menu | — | 通用菜单（`Instantiator` 动态插 MenuItem） | `current`, `model`；信号：`clicked(int)`；`delegate: required modelData, index` |
| QMessage.qml | ListView | — | 右下角消息堆栈（BottomToTop，自动进出场） | `messageModel`(alias), `viewh`；方法：`dialog(name,text,icon)`；`delegate: required index, model` |
| QOptionDialog.qml | Popup | — | 通用模糊选项对话框（所有设置/信息弹窗的基类） | `blurSource`, `rectXy`, `title`(alias), `options`(alias→content.children), `header`(alias→titleText.children), `cancelText`, `cancelIcon`, `confirmText`, `isInput`, `dismissOnOverlay`, `cancelCloses`, `headerHeight`；信号：`confirm()`, `cancel()`；方法：`goTop()`, `onExit()` |
| QPages.qml | Item | — | 页栈：淡入 + 0.9→1 缩放切换，按 `lastIndex` 隐藏上一页 | `pageList`, `lastIndex`；方法：`stack(index)` |
| QPicture.qml | Item | — | 图片组件：圆角/四角独立圆角 + 缩放 + 失败回退兜底图 | `source`, `radius`, `cache`, `radius1`~`radius4`, `picScale`, `sourceSize`, `loadFailed`；只读：`fallbackSource` |
| QRCodeView.qml | Canvas | — | 二维码渲染（`api/qrcode.js`，`qrText` 变化重绘） | `qrText`, `darkColor`, `lightColor` |
| QScrollView.qml | Flickable | — | 平滑滚动容器（滚轮动画 + 滚动条） | `scrollToY`；只读：`wheelHeightCount` |
| QSlider.qml | Slider | — | 滑杆（轨道/球柄/数值标签） | `valueText`, `leftText`；继承 `from/to/stepSize/value/onMoved` |
| QSortModel.qml | SortFilterProxyModel | — | 排序代理：0 原序 / 1 名称 / 2 歌手 / 3 时长，方向由 `sortDesc` 决定 | `sortMode`, `sortDesc`, `options`, `nameRole`, `artistRole`；只读：`sourceModel`, `menuIndex`；方法：`orderFor(baseDesc)`, `selectMenu(i)`, `get(i)`, `at(i)` |
| QSwitch.qml | Rectangle | — | 开关（轨道+球柄，点击发信号，不自翻转） | `text`, `isBack`, `switchTrue`, `letRight`；信号：`toggled(bool)` |
| QTapBar.qml | Item | — | 文本页签 + 下划线动画条（无信号，监听 `index`） | `spacing`, `index`, `model`；`delegate: required modelData, index` |
| QTextWindow.qml | Window | — | Apache 2.0 许可证文本窗，`visible` 绑 `Options.textWatch.active` | 无公共属性（窗口尺寸/文本内联硬编码） |
| QTip.qml | ToolTip | — | 深色提示气泡（手动 `visible` 绑定，不走 attached 属性） | `radius`（+ 继承 `text`, `delay`, `visible`） |
| QWarn.qml | Popup | — | 顶部提示条（1.5s 自动关闭），3 种语义色 | `title`(alias), `type`(0 warn/1 success/2 error)；方法：`tiped(title,type)` |
| QWideDrop.qml | Item | — | 横向文字单选（扁平胶囊） | `model`, `singleWidth`, `choice`；信号：`transformed(int)`；`delegate: required modelData, index` |
| QWKButton.qml | Button | — | QWindowKit 标题栏图标按钮（SVG `source`） | `source`(alias), `largeicon`, `hoverColor`, `buttonColor` |
| SButton.qml | Button | — | 纯图标圆角按钮（默认带 `QTip`） | `iconCharacter`, `iconFontFamily`, `tipText`, `iconColor`, `shadowEnabled`, `borderWidth`, `borderColor`, `radius`, `pressedScale`, `shadowColor`, `hoverColor`, `buttonColor`, `iconSize` |
| SearchCard.qml | Popup | — | 搜索历史/联想浮层（模糊底） | `blurSource`, `rectXy`；信号：`searchIndex(int)`；`delegate: required modelData, index` |
| SettingItem.qml | Item | — | 设置行：左标签 + 右控件槽 | `label`, `controlWidth`, `isBigItem` |
| SettingItemCard.qml | Item | — | 卡片式设置行：标签 + 提示 + 控件槽 + 底分隔线 | `label`, `tip`, `controlWidth`, `controlItem`(alias), `isBigItem`, `bottomLine` |
| ShortCut.qml | Settings | ✅ | 快捷键绑定 + 全局快捷键开关（category `ShortCuts`，注意带 s） | `play`, `back`, `forward`, `playList`, `musicControl`, `volumeUp`, `volumeDown`, `seekBack`, `seekForward`, `mute`, `abLoop`, `favorite`, `playerOptions`, `globalShortcutPlay`, `globalShortcutBack`, `globalShortcutForward`, `globalShortcutPlayList`, `globalShortcutMusicControl`, `globalShortcutVolumeUp`, `globalShortcutVolumeDown`, `globalShortcutSeekBack`, `globalShortcutSeekForward`, `globalShortcutMute`, `globalShortcutAbLoop`, `globalShortcutFavorite`, `globalShortcutPlayerOptions` |
| SongInfoDialog.qml | QOptionDialog | — | 歌曲/歌单信息弹窗（全应用唯一实例，字段归一在 C++） | `isList`, `info`；只读：`sourceName`, `rows`；方法：`show(item,list)`；`delegate: required modelData` |
| Style.qml | Settings | ✅ | UI 度量/主题/背景配置持久化（category `Style`） | 主题：`theme`, `color`, `colorList`, `backmode`, `backpic`；效果：`shadowBlur`, `shadowXOffset`, `shadowYOffset`, `shadowSize`, `blurSize`, `highQualityBlur`；字号：`textH1`, `textH2`, `textmain`, `text`, `textTip`, `texticon`, `pageTitle`, `texticonH`；控件：`labelRadius`, `cubeRadius`, `noControlRadius`, `borderDepth`, `layerEnabled`, `premiumAnime`, `animeSpeed`, `homeLayout`, `noOpacityControl`, `sidebarColor`, `sidebarStyle`, `animeType`, `menutheme`, `glmode`, `uilevel`；歌词：`lyricSize`, `flowStyle`, `waveDisplay`, `premiumLyricAnime`, `textWidth`, `maskBlur`, `lyricHideGui`；其它：`fontFamily`, `backgroundImage` |
| Theme.qml | QtObject | ✅ | 运行时色板（不是 Settings，不持久化） | `themeColor`, `fontColor`, `textColor`, `fullColor`, `hoverColor`, `sideColor`, `sideBlurColor`, `containColor`, `containOutColor`, `primaryColor`, `primaryBlurColor`, `borderColor`, `secondaryColor`, `secondaryBlurColor`, `blurOverlayColor`, `blurSecondaryColor`, `shadowColor`, `themeShadowColor` |

### 6.1.2 重点组件行为补充（属性/信号清单见上表）

- **QListView**：`isList=false` 走歌曲菜单（下载/下一首/加入/收藏/信息），`isList=true` 走歌单菜单（收藏/信息）；`menuModel`/`headerModel` 按 `isList` 给默认值。`toolHandler(choice,index)`/`menuHandler(tool,index)` 返回 `true` 表示页面自行处理并跳过内置动作。收藏成败由 `FavoriteSongs/FavoritePlaylists` 的 `favoriteStateChanged`/`errorOccurred` 决定，组件内不抢先提示。`atYEnd && !MusicApi.loadState` 才发 `ended()`。
- **QLocalView**：不做任何内置动作，只发 `clicked/menuClicked/toolClicked/ended`；`toolClicked` 的 tool 由右往左编号 2=菜单 / 1=喜欢 / 0=通用。
- **QDrop**：`useId=false` 用 `model[choice]` 文本；`useId=true` 取 `model[choice].description`（设备列表可能为空，越界必须判空）。菜单为内部 `Popup`，`transformed(index)` 后自动 `close()`。
- **QBlurCard / QBlurTapBar**：`blurSource` 为 `null` 时 ShaderEffect 整体 `visible: false`（不画）；`rectXy` 必须用**源坐标系**里本卡片的矩形（调用方写 `Qt.rect(root.x, root.y, w, h)`）。
- **QOptionDialog**：`options` 是 `content.children` 别名，`header` 是标题 Text 的 children 别名；`cancelCloses=false` 时左下按钮只发 `cancel()` 不关窗（网易云手机号登录用）。监听 `Options.onExit` 自动 `close()`。
- **QBlurTapBar** 常与 `QPages` 成对：`blurSource: pageList[lastIndex]`；`QTapBar` 只画下划线，`index` 变化即动。
- **QSortModel**：`get(i)` 先 `mapToSource` 再调 `sourceModel.getRow?.()` 或 `.get()`，因此对 `SearchResultModel`（`getRow`）与 `Favorites/FolderModel`（`get`）都可用。
- **QPicture**：`MultiEffect` + 隐藏 `Rectangle` 遮罩实现圆角；`radius1..4` 直接改遮罩四角；加载失败自动切 `fallbackSource`。
- **QMenu**：必须用 `Instantiator` + `insertItem/removeItem`（`Repeater` 不能把 MenuItem 挂进 `Menu`）；`parent: Overlay.overlay`。
- **QMessage**：`BottomToTop` 列表，`add`/`displaced` 过渡；`dialog(name,text,icon)` 追加一条，组件自身不删除（由数据驱动）。
- **QWarn**：`tiped()` 重置 1.5s 定时器；语义色由 `type` 决定（0 橙 / 1 绿 / 2 红）。
- **Options**：`darkis` = `Style.theme` 0 亮 / 1 暗 / 其它跟随系统；主题色变更只经 `Options.changeTheme()`。
- **Playback**：`muted` 与 `Config.musicVolume` 解耦（`outVolume` 取二者合值）；淡变用 `swap(action, fadeBack)` + 三个 Timer（`swapTimer/fadeTimer/guardTimer`）；`kSourceWebDav = 3`。
- **PluginHost / PluginApi / PluginManager**：见第 5、7 节。
- **EqualizerPanel**：`display` 为分区序号（0 概览 / 1 状态 / 2 后续分区），`engine` 为 `AudioEngine`，`eqGains[index]` 为只读绑定。

## 6.2 单例清单

`import QueMusic 1.0` 后按类型名直接引用。共 10 个单例：components/ 8 个 + lyricsui/ 2 个。

| 单例 | 根类型 | 文件 | 持有内容 | 类别 |
|---|---|---|---|---|
| Config | Settings | components/Config.qml | 业务/播放/媒体/高级配置，category `Options` | 持久化（QSettings） |
| Fonts | QtObject | components/Fonts.qml | 两个 `FontLoader` + 家族名 `icon`/`text` | 运行时 |
| Options | QtObject | components/Options.qml | 宿主对象注入槽 + main 状态 + 主题驱动 + 全局信号总线 | 运行时 |
| Playback | QtObject | components/Playback.qml | 播放状态、队列操作、淡变/AB/睡眠/历史 | 运行时 |
| PluginSlots | QtObject | components/PluginSlots.qml | 扩展点注册表 `{名字 → 容器}` | 运行时 |
| ShortCut | Settings | components/ShortCut.qml | 快捷键序列 + 全局快捷键开关，category `ShortCuts`（带 s） | 持久化 |
| Style | Settings | components/Style.qml | UI 度量/字号/圆角/动画/歌词界面开关，category `Style` | 持久化 |
| Theme | QtObject | components/Theme.qml | 18 个颜色属性（运行时被覆写） | 运行时 |
| Lyrics3DConfig | Settings | lyricsui/Lyrics3DConfig.qml | 「3D」歌词界面自有设置，category `Lyrics3D` | 持久化 |
| LyricsFreeConfig | Settings | lyricsui/LyricsFreeConfig.qml | 「自由」歌词界面自有设置，category `LyricsFree` | 持久化 |

### 6.2.1 Style.qml 与 Theme.qml 的区别

| 维度 | Style.qml | Theme.qml |
|---|---|---|
| 基类 | `Settings`（落盘 QSettings，category `Style`） | `QtObject`（纯内存） |
| 内容 | 度量值：字号、圆角、模糊半径、动画开关、布局开关、歌词界面开关、背景图 | 只有颜色 |
| 谁写 | 用户设置页直接读写 | 只由 `Options.onChangeTheme` 写 |
| 读法 | `Style.cubeRadius`、`Style.textmain` | `Theme.primaryColor`、`Theme.textColor` |
| 可否派生 | 派生值需自己算 | 派生色（如 `containColor`）由 `darkis` + 主题色 HSV 算好写入 |

### 6.2.2 Options.changeTheme() 如何驱动 Theme

1. 触发源：`Options.onDarkisChanged: Options.changeTheme()`；设置页改 `Style.theme`/`Style.color`（SettingsView:715/745/793/1141）、`main.qml` 启动（62–63）也显式调用。
2. `changeTheme()` 处理器取 `Style.colorList[Style.color].hsvHue` 作为色调，按 `darkis` 用 `Qt.hsva(hue, sat, val, alpha)` 覆盖 `Theme` 的全部颜色属性。
3. 语义分组：不随颜色变的（`fontColor`/`textColor`/`fullColor`/`hoverColor`/`sideColor`/`sideBlurColor`）只按亮暗给常量；随颜色变的（`containColor`/`containOutColor`/`themeColor`/`primaryColor`/`primaryBlurColor`/`secondaryColor`/`secondaryBlurColor`/`borderColor`/`blurOverlayColor`/`blurSecondaryColor`/`shadowColor`/`themeShadowColor`）由 HSV 生成。
4. 组件侧依赖绑定：所有组件写 `color: Theme.xxx`，因此改 `Theme` 属性即全库刷新，无需重载组件。

### 6.2.3 单例的注册要求（必须三处一致）

- 文件头 `pragma Singleton`。
- `CMakeLists.txt` 116–127 的 `set_source_files_properties(... PROPERTIES QT_QML_SINGLETON_TYPE TRUE)` 列表（当前 10 个文件）。
- `qt6_add_qml_module` 生成的 `qmldir` 里有对应 `singleton <Name> 1.0 <file>` 行。
- 漏掉任一处会导致 `is not a type` / 无法实例化；新增单例必须同时改 CMakeLists。

## 6.3 命名约定

| 前缀/后缀 | 含义 | 例 |
|---|---|---|
| `Q*` | 项目自研通用控件（Q = QueMusic 自有），跨页面复用、无业务依赖 | `QButton`, `QDrop`, `QListView`, `QBlurCard` |
| `S*` | 小型/次级控件（Small / Simple），更薄一层、常用于图标按钮 | `SButton`（唯一一个） |
| `SettingItem` / `SettingItemCard` | 设置页专用行容器 | 无前缀，属设置页配套件 |
| `Theme` / `Style` | `Theme`=颜色，`Style`=度量与开关（见 2.1） | — |
| `Config` | 无 UI 的持久化业务配置 | `Config`（+ `OptionsLastSongs`、`ShortCut` 同类） |
| `*Config`（lyricsui/） | 歌词界面主题自带设置单例 | `Lyrics3DConfig`, `LyricsFreeConfig` |
| `*Dialog` | 模态/半模态弹窗（多为 `Popup`，设置类继承 `QOptionDialog`） | `QOptionDialog`, `ColorPickerDialog`, `FontFamilyDialog`, `SongInfoDialog`, `QAlertDialog` |
| `*Window` | 真 `Window`（含桌面歌词/小窗/许可证窗） | `DesktopLyrics`, `DesktopPlayerWindow`, `QTextWindow` |
| `*Card` | 内容卡片（模糊/悬浮/信息卡） | `QBlurCard`, `QFloatCard`, `AccountCard`, `PlatformCard`, `SearchCard` |
| `*View` | 列表/滚动容器 | `QListView`, `QLocalView`, `QScrollView` |
| `Q*Drop` | 单选控件族：`QDrop` 下拉 / `QBigDrop` 大图卡 / `QWideDrop` 横向胶囊 | 三者共用 `choice` + `transformed(int)` |

视觉/API 约定（新增组件应照抄）：

- 颜色一律 `Theme.*`，尺寸与字号一律 `Style.*`；不在组件里写死主题色。
- 圆角统一走 `Style.labelRadius`（小件）/ `Style.cubeRadius`（卡片）/ `Style.noControlRadius` 开关：`radius: Style.noControlRadius ? Style.labelRadius : 20`。
- 图标按钮统一暴露 `iconCharacter` + `iconFontFamily`（默认 `Fonts.icon`）+ `tipText`，并用 `SButton` 的内置 `QTip` 模式。
- 状态动画统一 `Behavior on x { NumberAnimation { duration: …; easing.type: … } }`，时长取 80/120/160/240/320/360 档。
- 交互信号命名：`clicked` / `transformed` / `toggled` / `accepted` / `ended` / `confirm` / `cancel` / `tabChange`。
- 列表 delegate 一律 `required property int index`（+`modelData`），列表组件暴露 `scrollToY`、`scrollTop()`、`ended()`。
- 注释一律中文，文件头 `SPDX-License-Identifier: Apache-2.0`。
- 需要跨 delegate 绑定的文件加 `pragma ComponentBehavior: Bound`。

## 6.4 可复用模式

### 6.4.1 模糊卡片（唯一实现，别自己写）

- 结构：`ShaderEffectSource(sourceItem: blurSource, sourceRect: rectXy, textureSize: size(w,h) * Screen.devicePixelRatio, mipmap: true, live: visible && blurSource !== null, visible: false)` → `ShaderEffect(fragmentShader: "qrc:/shaders/shaders/cardblur.frag.qsb")` → 顶层 `Rectangle(color: cardColor, radius: borderRadius, border.*)`，`z: 2/3`。
- 高质量档：`highQuality: true` 或 `QBlurCard.highQuality: Style.highQualityBlur` 切到 `cardblur_hq.frag.qsb`。
- 阴影：`RectangularShadow`（QtQuick.Effects），`offset.y: 8`、`blur: 28`、`color: Theme.shadowColor`。
- uniform：`src, blur, blurMax, saturation, corner, cardSize, texSize, aa, fillColor`；`blurMax` 默认 `Style.blurSize / 2`（设备像素）。
- 典型用法（弹窗背景）：`background: QBlurCard { anchors.fill: parent; blurSource: Options.mainLayout; rectXy: Qt.rect(root.x, root.y, w, h); shadowEffect: true; borderRadius: Style.cubeRadius }`。
- 现用者：`QAlertDialog`、`QOptionDialog`、`SearchCard`、`MusicInfo`、`PlayList`、`DesktopPlayer`、`QBlurTapBar`（自带一份同源实现，`cardblur.frag.qsb`）。
- shader 由 `qt6_add_shaders(... PREFIX "/shaders" ...)` 打包（CMakeLists 272–287），源文件在 `shaders/`。

### 6.4.2 动画曲线（项目惯用值）

| 场景 | 曲线 |
|---|---|
| 通用入场（弹窗/页切换/提示条） | `Easing.OutExpo`，240–420ms |
| 通用退场 | 160ms 线性/`OutCubic` |
| 按压缩放 | `Easing.OutExpo` 120ms（`pressedScale` 0.94–0.96） |
| 大范围布局位移（左栏/首页） | `Easing.BezierSpline`，`[0.50, 0.00, 0.00, 1.00, 1, 1]` / `[1.00, 0.00, 0.50, 1.00, 1, 1]` / `[0.30, 0.08, 0.00, 1.00, 1, 1]` |
| 列表行插入/位移、滚动 | `Easing.Bezier`，`[0.23, 0.06, 0.00, 1.00, 1, 1]`（QMessage/QLocalView/QListView/QBlurTapBar） |
| 歌词行位移 | C++ `SpringAnimator`（阻尼弹簧，`duration`/`delay`/`bounce`）；QML 负责逐行错峰与画面外融合 |
| 弹性强调（卡片缩放/开关球柄） | `Easing.OutBack` + `easing.overshoot ≈ 1.1~1.2`，或 `[0.20, 0.04, 0.00, 1.64, 1, 1]` |
| 滚动容器 | `NumberAnimation 240ms Easing.OutCubic`，target `contentY` → `scrollToY` |

### 6.4.3 hover / focus 状态

- 悬停：`MouseArea { hoverEnabled: true }` + `opacity: area.containsMouse ? 1 : 0`（`QDrop`/`QWideDrop`/`QBigDrop`/`PlatformCard`/`QFloatCard`）。
- 不吞点击的跟随：`HoverHandler { blocking: false }`（`Lyrics3D` 鼠标跟随）。
- 控件层：`Button.hovered`（`QButton`/`SButton`/`QWKButton` 用 `color: hovered ? hoverColor : buttonColor` + `ColorAnimation 80~120ms`）。
- 焦点：仅 `QInput` 使用 `TextInput.focus` 改边框宽度/颜色（`border.width: input.focus ? 2 : 1`）；全库无 `activeFocusOnTab`、无 `Keys.onPressed`（仅 `SettingsView` 快捷键录制用一次）。
- 光标：`cursorShape: Qt.PointingHandCursor` 只加在可点击区域。

### 6.4.4 主题色引用层级

素材层 `Theme`（颜色）→ 结构层 `Style`（度量）→ 组件默认值 `${Theme|Style}.x`。组件可选属性用主题值做默认值（`property color buttonColor: Theme.primaryColor`），调用方覆盖即可，因此主题切换零成本。派生/半透明色一律用现成项（`primaryBlurColor`/`secondaryBlurColor`/`blurOverlayColor`/`sideBlurColor`），不要临时 `Qt.rgba`。

### 6.4.5 列表抽象（QListView / QLocalView / QGridView / QScrollView）

统一四件套，新增列表照抄：

- `property int scrollToY: view.contentY` + `onDraggingChanged: view.scrollToY = view.contentY`（滚动条拖动时同步）。
- `WheelHandler` 里 `wheelHeightCount = Qt.application.styleHints.wheelScrollLines * 0.25`，把 `scrollToY` 夹在 `[originY - topMargin, scrollBottom]`，然后跑同一个 `NumberAnimation(target: contentY, duration: 240)`。
- `ScrollBar.vertical` 手动挂载，`onPressedChanged` 同步 `scrollToY`，动画结束后 `viewBar.active = false`。
- `scrollTop()` 必须先 `listViewAnime.stop()` 再赋值，否则滚轮动画会把 `contentY` 拉回旧位置。
- 性能：`synchronousDrag: true`、`reuseItems: true`、`acceptedButtons: Qt.NoButton`、`clip: true`。
- 分页：`onAtYEndChanged: if (atYEnd && !MusicApi.loadState && count !== 0) ended()`。
- `QScrollView` 是最小版（只 `scrollToY` + `wheelHeightCount`，`contentHeight: contentItem.childrenRect.height`）。

### 6.4.6 弹窗模式

`Popup { parent: Overlay.overlay; modal: true; focus: true; background: QBlurCard{…}; contentItem: Column{…}; enter/exit: Transition{…}; Connections { target: Options; enabled: dialog.visible; function onExit() { dialog.close() } } }`。
设置类弹窗一律继承 `QOptionDialog`，内容写进 `options:`，不要重复实现标题/按钮/模糊底。

### 6.4.7 图标字符按钮

`QButton`（图标+文字）与 `SButton`（纯图标）共享 `iconCharacter` + `iconFontFamily: Fonts.icon`；字符是字体私有码位（见 6.2）。`SButton` 内部自带 `QTip { visible: tipText !== "" && hovered; text: tipText; delay: 480 }`；`PlayerControl` 里另有手写 `QTip { visible: xxx.containsMouse; text: "…" }` 用法。

### 6.4.8 设置行 / 页签 / 页栈

- 设置行：`SettingItem { label; controlWidth; isBigItem }`，控件靠调用方 `anchors.right: parent.right` 塞进去；带提示用 `SettingItemCard`，内容写进 `controlItem` 默认别名。
- 页签：`QTapBar`（文本 + 主题色下划线，适合页内小节）与 `QBlurTapBar`（模糊胶囊，适合悬浮标题栏，`model` + `choiceIndex` + `tabChange`）。
- 页栈：`QPages { pageList: [a,b,c] }` + `QPages.stack(index)`；`QBlurTapBar.blurSource` 一般绑 `pages.pageList[pages.lastIndex]`。

### 6.4.9 新增组件的骨架（直接照抄约定）

通用控件（无业务依赖）：

```qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 一句话职责。
pragma ComponentBehavior: Bound          // 仅当 delegate 里要用 required 顶层属性时加
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic
import QtQuick.Effects

Item {                                     // 根类型：纯装饰用 Item/Rectangle，可交互优先继承 Button/Slider
    id: root
    implicitWidth: 240
    implicitHeight: 36

    // ==== 外部接口 ====
    property string label: ""
    property color cardColor: Theme.primaryColor
    property int radius: Style.noControlRadius ? Style.labelRadius : Style.cubeRadius
    signal activated()

    // ==== 交互/状态 ====
    Behavior on radius { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
    Behavior on scale  { NumberAnimation { duration: 120; easing.type: Easing.OutExpo } }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true                 // hover 一律 containsMouse + opacity 行为
        onClicked: root.activated()        // 状态改动只经信号外抛，不自持业务状态
    }
}
```

模糊弹窗（设置/信息类）：

```qml
QOptionDialog {                            // 继承而非重写：自带模糊底/标题/双按钮/Options.onExit
    id: root
    title: "标题"
    cancelText: "取消"
    confirmText: "确定"
    property var info
    function show(item: var): void { info = item; open(); }

    options: Column {                      // 内容统一塞这里；width: parent.width 自适应
        width: parent.width
        spacing: 16
        SettingItem { label: "某开关"; width: parent.width; QSwitch { switchTrue: Config.xxx } }
    }
    onConfirm: root.close()
}
```

新增文件自检清单：

1. 文件头 SPDX 两行 + 中文职责注释。
2. 颜色用 `Theme.*`、尺寸用 `Style.*`，无硬编码主题色。
3. 信号/方法带静态类型签名（`function f(x: int): void`），`readonly` 派生属性写明类型。
4. `delegate` 显式 `required property int index`（+ `modelData`），文件头按需加 `pragma ComponentBehavior: Bound`。
5. 状态动画用 4.2 的曲线档位。
6. 新增普通文件无需改 CMake；若要成为**单例**必须同时改 `CMakeLists.txt` 的 `QT_QML_SINGLETON_TYPE` 列表。
7. 弹窗类监听 `Options.onExit` 自动关闭。

## 6.5 lyricsui 子系统

### 6.5.1 加载与替换机制（宿主侧）

1. 商店单例是 C++ 的 `LyricsPlugins`（`cpp/plugins/LyricsPluginStore.h`，`QML_NAMED_ELEMENT(LyricsPlugins)`，单选）：`plugins`（内置+已安装）、`dir`、`selectedId`、`source`（当前入口，直接绑 Loader.source）、`rescan()`、`install(folder)`、`remove(id)`、`reveal(id)`。
2. `layout/PlayerMaxCenter.qml:127` 的 `Loader { active: visible; asynchronous: true; source: LyricsPlugins.source; onLoaded: inject(item) }` 负责实例化；换界面 = 换 `source`。
3. `inject(item)` 只对**已声明**的属性赋 `Qt.binding`（`if (k in it)`），因此契约是「按需实现」而非强制全量。
4. 心跳：宿主每 320ms 调 `item.timerFunction()`（`Timer` 在 `PlayerMaxCenter`，仅在最大播放页可见且媒体播放中运行）；索引由宿主的 `MusicApi.lyricIndex` 算好注入。
5. 主题选择 UI：`PlayerMaxCenter` 的「歌词界面」`QOptionDialog` 列 `LyricsPlugins.plugins`，点选写 `LyricsPlugins.selectedId`；`SettingsView` 用 `PluginManager { store: LyricsPlugins; pluginKind: "歌词界面插件" }`。
6. 内置「默认」界面不在 lyricsui/，而是 `layout/MainLyric.qml`（与 LyricsFree 同一套契约与代码的近亲）；`lyricsui/` 两个文件是「自由」与「3D」两个可选界面。
7. 官方文件契约（新歌词界面必须照此实现；`layout/MainLyric.qml` 与 `lyricsui/LyricsFree.qml` 头部注释即为权威副本）：

| 属性 | 注入源 | 说明 |
|---|---|---|
| `position` : real | `Playback.player.position` | 播放进度 ms |
| `playbackRate` : real | `Playback.player.playbackRate` | 用于逐字动画时长换算 |
| `playing` : bool | `Playback.player.playing` | — |
| `mediaActive` : bool | `Playback.player.onMedia` | 是否已挂上媒体 |
| `lyricsModel` : var | `MusicApi.lyricsData \|\| []` | 行模型 `{time, text, info}` |
| `translateModel` : var | `MusicApi.lyricsTranslate \|\| []` | 译文，与行同下标 |
| `currentIndex` : int | `MusicApi.lyricIndex` | 当前行，宿主算好 |
| `title` / `artist` : string | `Playback.musicTitle` / `Playback.musicArtist` | — |
| `coverUrl` : string | `Playback.player.urlStr`（回退 `qrc:/QueMusic/resources/app/musicpic.png`） | — |
| `mainColor` / `secondColor` / `thirdColor` : color | `PlayerMaxCenter` 抽色结果（封面色动画） | 背景着色器主色 |
| `hideHeight` : int | `PlayerMaxCenter.hideHeight` | 沉浸模式偏移（0 / 76） |
| `lyricSize` : int | `Style.lyricSize` | 歌词字号档 |
| `basicCd` : bool | `PlayerMaxCenter.lyricBasicCd` | 封面卡片 / 经典黑胶（宿主状态） |
| `lyricType` : int | `PlayerMaxCenter.lyricThemeMode` | 0 默认 / 1 封面 / 2 歌词 |
| `lyricMove` : int | `PlayerMaxCenter.lyricMoveMs` | 歌词校准 ms |
| `openTranslate` : bool | `PlayerMaxCenter.lyricTranslateOpen` | 是否显示译文 |
| `requestStyle` : var | 宿主 `applyStyleRequest(key,value)` | 受控写入口，可缺省 |
| `timerFunction()` | 宿主 320ms 调用 | **必须实现**（否则宿主心跳调用无效） |
| `styleOptions` : Component | 宿主插入「播放器样式」弹窗 | 可选；`PlayerMaxCenter` 取 `lyricLoader.item.styleOptions` |

- `requestStyle` 白名单（`PlayerMaxCenter.styleAllow`）：`basicCd: "host"`、`lyricType: "host"`、`premiumLyricAnime: 1`、`waveDisplay: 1`；`"host"` 表示写宿主自身状态，`1` 表示数值直写 `Style`，数组表示 `[min,max]` 夹取。未列入白名单的 key 一律被丢弃。
- 主题自有设置必须落自己的 `*Config` 单例（`Lyrics3DConfig` / `LyricsFreeConfig`），**不写宿主 Style、不过白名单**。

### 6.5.2 Lyrics3D.qml

- 根类型 `Item`（`pragma ComponentBehavior: Bound`），426 行。三层渲染：① 背景 `ShaderEffect`（按 `presetTable` 换 `bg_cubes/bg_grid/bg_stars/bg_beams/bg_void/bg_stage` 的 `.frag.qsb`）② 点云舞台 `LyricsStage`（C++，GPU 算点，`preset >= 0` 才可见，`count: 14000`）③ 歌词平面 `lyricplane.frag.qsb`（固定取纹理 LOD0，做字体发光）。
- 契约属性：`lyricsModel`, `currentIndex`, `lyricSize`, `openTranslate`, `translateModel`, `mainColor`, `secondColor`, `hideHeight`；`function timerFunction()` 为空实现（**索引由宿主算，不需要逐帧扫表**）。
- 内部：`presetTable`（8 个预设：方块场/光栅地平/平面星空/光棱/星尘/光隧/星环/舞台）、`presetIndex`（夹取 `Lyrics3DConfig.preset`）、`sliderTable`（15 个滑杆：[键,名,min,max,step,倍数,单位]）、`colorTable`（2 个自定义颜色）、`rowH`/`bigFont` 派生尺寸、`uLow/uMid/uHigh/uAir/uLevel` 五段频谱（60ms Timer 从 `Options.getWave.wavePath` 采 32 点折算）、`styleOptions` Component（同一份滑杆表也提供宿主弹窗）。
- 交互：`HoverHandler { enabled: !diyPanel.visible; blocking: false }` 做鼠标跟随（不吞点击）；右下角 `SButton`（`\uf013`）开 `diyPanel`；`ColorPickerDialog` 写回 `Lyrics3DConfig[key]`。
- 性能要点：歌词纹理 `ShaderEffectSource.live: false`，只在 `onCurrentIndexChanged` 与 460ms `refreshTimer` 各 `scheduleUpdate()` 一次。

### 6.5.3 Lyrics3DConfig.qml

`pragma Singleton` + `Settings { category: "Lyrics3D" }`：`preset`, `bright`, `saturate`, `audio`, `density`, `fieldScale`, `camDist`, `camSens`, `tilt`, `lyricScale`, `lyricY`, `lyricZ`, `glow`, `sweep`, `beatGlow`, `floatAmt`, `textColorCustom`, `glowColorCustom`。空串颜色 = 跟随主题。

### 6.5.4 LyricsFree.qml

- 根类型 `Item`（`pragma ComponentBehavior: Bound`），1006 行（附录 A 记为 1006）。背景：`bgStyle` 0 流体（`CustomFlow`/`flowStyle`）/ 1 图片 / 2 视频（`QtMultimedia`）/ 3 平面星空（`bg_stars.frag.qsb`）；歌词区用 `lyricfade.frag.qsb` 做上下渐隐/模糊遮罩。
- 契约属性：`position`, `playbackRate`, `playing`, `mediaActive`, `lyricsModel`, `translateModel`, `currentIndex`, `title`, `artist`, `coverUrl`, `mainColor`, `secondColor`, `thirdColor`, `hideHeight`, `lyricSize`, `basicCd`, `lyricType`, `openTranslate`, `lyricMove`, `requestStyle`；`function request(key,value)` 转交 `requestStyle`；`function timerFunction()` → `lyricContent.tick(currentIndex)`；`property Component styleOptions`（封面模式 `QBigDrop`、主题模式 `QWideDrop`、`premiumLyricAnime`/`waveDisplay` 开关，全部经 `request()` 回宿主）。
- 内部派生：`hasTranslate`, `standHeight`, `piclong`, `uiReady`（首帧不播位移过渡）、`typeChangeXAnime`。
- 排版微优化：按 `heights`/`prefixSum` 缓存每行高度做前缀和定位；`animeTo(ty)` 对索引距离 < -3 / > 7 的行改用统一绑定的「融合动画」，只在近邻区间跑弹簧动画。

### 6.5.5 LyricsFreeConfig.qml

`pragma Singleton` + `Settings { category: "LyricsFree" }`：`bgStyle`, `bgImage`, `bgVideo`, `showTitle`, `showArtist`, `coverReflection`, `lyricSpacingScale`, `lyricCardAngle`, `lyricCurrentScale`, `lyricIdleOpacity`, `lyricSungColor`, `lyricLineColor`。文件头记录原因：以前是界面根里的 `Settings` + 文件内 `id`，子组件跨边界引用无法被 AOT 编译，故改为单例。

### 6.5.6 逐字 `info` 模型与 `translateModel` 数据通道

- 行结构：`lyricsModel[i] = { time: <ms>, text: <整行文本>, info: <?> }`，由 C++ 提供（`LocalLyricsReader.cpp` 组装本地 `.lrc/.krc`，`api/KugouApi.cpp` 提供在线逐字）。
- `info` 为逐字数组：`info[j] = { text, offset, duration }`；`offset` 是**相对该行 `time`** 的毫秒偏移。空表/缺省表示该行不支持逐字（退回整行渲染）。
- `info` 是 `QVariantList`（有 `length`，但**不是 JS Array**）→ 禁止用 `Array.isArray` 判断，只能 `info && info.length > 0`（`LyricsFree.qml:459` 注释明确）。
- 消费方式（`LyricsFree`）：`flowWords = (modelData.info && modelData.info.length > 0) ? modelData.info : null`；`isFlowActive` 只对当前行与上一行开启；`nowPosition = position + lyricMove - info行.time`；逐字 delegate 用 `required property int index` 取 `flowWords[index]` 的 `offset`/`duration`，用 `LinearGradient` 做逐字染色，动画时长按 `duration / playbackRate` 换算。
- `translateModel`：与行同下标的字符串数组（`translateModel[i] || ""`），宿主注入源 `MusicApi.lyricsTranslate`（服务端翻译字段，`api/MusicApiService.cpp` 里来自响应 `info`）；组件侧仅 `openTranslate && translateModel.length !== 0` 时渲染。
- 另有一条相同形状的通道给桌面歌词：`DesktopLyrics.lyricsData = MusicApi.lyricsData`（自带增量索引推进，240ms 定时）。

### 6.5.7 新歌词界面落地清单

1. 新建 `lyricsui/<Name>.qml`，根 `Item`，`pragma ComponentBehavior: Bound`，头部注释写明契约来源。
2. 声明要用的契约属性（至少 `lyricsModel` + `currentIndex`；需要逐字再加 `position`/`playbackRate`；需要颜色再加 `mainColor`/`secondColor`/`thirdColor`）。宿主按 `in` 判断注入，多声明无害、少声明即接收不到。
3. 实现 `function timerFunction(): void`（可为空实现，宿主 320ms 无条件调用）。
4. 实现 `function request(key: string, value: var): void` 且声明 `property var requestStyle: null`，只在需要改宿主状态（`basicCd`/`lyricType`/`premiumLyricAnime`/`waveDisplay`）时使用。
5. 主题自有设置放 `lyricsui/<Name>Config.qml`（`pragma Singleton` + `Settings`），并把它加入 `CMakeLists.txt` 的单例列表。
6. 需要「播放器样式」弹窗内容时提供 `property Component styleOptions`。
7. 若做成插件：目录内需要入口 `qm` 文件 + 与 `LyricsPlugins.plugins` 一致的信息字段（`id`/`name`/`version`/`author`/`preview`/`description`/`builtin`），入口文件路径进 `LyricsPlugins.source`；安装走 `LyricsPlugins.install(folder)` / 删除走 `remove(id)`，`builtin: true` 的内置项不可卸载。
8. 验证：切 `selectedId` 后 Loader 重载、逐行/逐字都对、暗亮主题 + 4 个主题色都正常、最大播放页关闭再打开（`active:false` 卸载）无残留。

## 6.6 资源与字体

### 6.6.1 自带字体

| 文件 | family（name 表 ID 1/6） | 用途 | 注册/打包 |
|---|---|---|---|
| resources/fonts/feather.ttf | `feather` | 图标字体（`Fonts.icon`） | `Fonts.qml` 的 `FontLoader{source:"qrc:/QueMusic/resources/fonts/feather.ttf"}`；CMake `RESOURCES` 第 182 行 |
| resources/fonts/poppins.ttf | `Poppins` | 正文/界面字体（`Fonts.text`） | 同上，第 183 行 |
| resources/fonts/IconData.js | — | Material Design Icons 名字数组（`var icons = [...]`） | **全库未被任何 QML/C++ 引用**（仅出现在本文档附录 A 的文件清单里）→ 视为死资源，勿据此以为项目用 MDI |

- `components/Fonts.qml`：两个 `readonly FontLoader`，家族名**运行时**从 `iconLoader.name` / `textLoader.name` 取（不硬编码字符串），暴露 `Fonts.icon` / `Fonts.text`。
- 使用方式：`font.family: Fonts.icon`（直接）或控件参数 `iconFontFamily: Fonts.icon`。

### 6.6.2 图标码位真相（重要）

`feather.ttf` 的 name 表家族名是 `feather`，但 cmap 只覆盖 `U+F000`–`U+F120` 区段共 292 个码位（FontAwesome 私有区），**不是官方 Feather 字体的 `U+E900` 区段**。项目里所有图标字符（`\uf013` 齿轮、`\uf025` 音乐、`\uf118` 循环、`\uf095` 加号、`\uf0c8` 方框、`\uf0e2` 撤销、`\uf03b` 暂停、`\uf11a` 警告、`\uf10f` 关闭、`\uf067` 安装、`\uf0f5` 文件夹、`\uf021` 刷新、`\uf092` 列表、`\uf120` 工作台、`\uf0e7` 勾）都取自该区段。**不要替换为官方 Feather TTF**，否则全部图标编号失效。

### 6.6.3 直接使用 `Fonts.icon` / `Fonts.text` 的文件

- `Fonts.icon`：components/`QButton.qml`, `SButton.qml`, `QDrop.qml`, `QWarn.qml`, `QMessage.qml`, `PlayListWindow.qml`, `DesktopPlayer.qml`；layout/`LeftSideBar.qml`, `PlayerMaxCenter.qml`；pages/`HomePage.qml`, `FilePage.qml`, `WebDavPage.qml`, `DownloadPage.qml`；`SettingsView.qml`；`main.qml`(via QMessage/QWarn)。
- `Fonts.text`：`SettingsView.qml:2242`, `layout/LeftSideBar.qml:144`, `main.qml:644`（其余文本走 `Style.fontFamily`）。
- 绝大多数 `Text` 用 `font.family: Style.fontFamily`（用户在设置页选的家族，空则系统默认）。

## 6.7 坑与约定

1. **`SettingItem`/`SettingItemCard` 依赖外部 id**：`width: settingStack.standWidth` 通过 QML 上下文链解析 `settingStack`，只有放在定义了该 id 的 `SettingsView.qml` 里才成立；单独复用会报 `settingStack is not defined`。改这两个文件前先确认调用点。
2. **`QButton.tipText` 是死属性**：仅在 `QButton.qml:16` 声明，内部没有 `QTip`，设置它不会有任何效果；需要 hover 提示请用 `SButton`。
3. **`QOptionDialog.cancelCloses`**（`QOptionDialog.qml:21`）为 `false` 时左下按钮只发 `cancel()` 不关窗；改按钮行为时不要顺手删这个开关。
4. **Loader 规则**：`AnimatorWindow`/`PlayListWindow` 用 `default property alias content: loadWidget.sourceComponent` + `Loader{active:false; asynchronous:true}` 承载整页内容，`onLoaded` 里才 `visible = true` 并起动画——不要在外层提前设 `visible`/`active`。`PlayListWindow.opened(info)` 会先写属性再 `active = true`，顺序不能反。
5. **`PluginApi.mount()` 的 url 分支**：传字符串时建出的是 `Loader`，属性必须走 `obj.setSource(source, props)`；直接设到 Loader 上会报 `Loader does not have a property called …` 并静默丢属性。
6. **delegate 里的 `required property` 必须逐个声明**：`PluginHost.qml:30` 注释说明——在 `createObject` 的 JS 里读 delegate 的隐式上下文变量（`modelData`）在 AOT 下会抛 `ReferenceError`。新增 Repeater/delegate 时也要显式声明 `index`/`modelData`。
7. **`pragma ComponentBehavior: Bound`**：components/ 里 18 个文件加了它（`QListView`/`QLocalView`/`QMenu`/`QMessage`/`QDrop`/`QBigDrop`/`QWideDrop`/`QTapBar`/`QBlurTapBar`/`SearchCard`/`SongInfoDialog`/`FontFamilyDialog`/`MusicInfo`/`EqualizerPanel`/`PluginHost`/`PluginManager`/`DesktopPlayer`/`Playback`），lyricsui/ 两个歌词界面也各有一处，layout/ 与 pages/ 另有多个。需要 delegate 顶层 `required` 属性绑定（`index`/`modelData`）的文件必须加，去掉会导致解析失败。
8. **单例三处一致**（见 2.3）：只写 `pragma Singleton` 而不改 `CMakeLists.txt` 的 `QT_QML_SINGLETON_TYPE` 列表 → 类型不可用。
9. **`Theme` 是运行时对象，别加持久化字段**：它没有 `Settings` 基类；颜色只在 `Options.onChangeTheme` 里写。往里加属性请在 `changeTheme` 里一并赋值，否则暗色模式下不会更新。
10. **`Style` 字段改名 = 用户配置丢失**：`Settings { category: "Style" }` 按属性名落盘；`Config`（`Options`）、`ShortCut`（`ShortCuts`）、`OptionsLastSongs`（`LastMedia`）、`Lyrics3DConfig`（`Lyrics3D`）、`LyricsFreeConfig`（`LyricsFree`）同理，改名前需迁移。
11. **`Options` 是注入总线，启动顺序敏感**：`main.qml:21–35` 在 `Component.onCompleted` 注入 15 个对象。未注入时 `Options.mainLayout === null` → `QBlurCard.blurSource` 为 null → 卡片整体不绘制（`visible: root.blurSource !== null`），且 `QAlertDialog`/`QOptionDialog` 的默认 `blurSource` 失效。
12. **禁止跨文件引用宿主局部 id**：`DesktopLyrics`/`DesktopPlayerWindow`/`MusicInfo`/`PlayerOptions` 已改为 `readonly property AudioEngine player: Playback.player`；新增组件同理，用单例入口（`Playback`/`MusicApi`/`Config`/`Options`）而非上下文 id。
13. **`QMenu` 必须用 `Instantiator`** + `onObjectAdded: dialog.insertItem(i,obj)`；`Repeater` 不会把 `MenuItem` 挂进 `Menu`。
14. **`QDrop.useId`** 读 `model[choice].description`，设备列表为空时返回 `""`；越界下标必须判空（源码注释明确）。
15. **`QPages`**：`pageList` 元素必须是 `Item` 且初始 `visible: false`；`stack(index)` 靠 `lastIndex` 隐藏上一页，外部不要自己改 `visible`。
16. **`QBlurTapBar.blurSource` 不能为 null**：常绑 `pageList[lastIndex]`，页面尚未创建时要注意时序。
17. **`Toast` 通道**：`Options.warned(text, type)` → `main.qml:964` 的 `QWarn.tiped()`；type 0 警告 / 1 成功 / 2 错误。组件内统一走 `Options.warned`，不要各自弹窗。
18. **组件不是独立可复用库**：`QListView`/`QLocalView`/`Playback`/`MusicInfo` 等直接引用 `MusicApi`、`FavoriteSongs`、`Playback`、`Config`、`Options` 等宿主单例，脱离 app 无法单独实例化。
19. **模糊纹理尺寸**：`_texW/_texH` 必须 `Math.max(2, round(w * Screen.devicePixelRatio))`（高 DPI + 下限保护），改小会崩/花屏。
20. **`QTextWindow` 是硬编码许可证窗**：`visible: Options.textWatch.active`，`onClosing` 反向写回；文本内联在文件里（99 行），不要当通用文本查看器用。
21. **新增普通组件无需改 CMake**（`file(GLOB_RECURSE ... CONFIGURE_DEPENDS "components/*.qml")`，CMakeLists 130），但新增**单例**必须改（见 2.3）。
22. **`QWKButton` 需要 QWindowKit**：`source` 指向 `resources/window-bar/*.svg`（CMake `RESOURCES` 打包），只用于自绘标题栏。
23. **`DesktopPlayer.desktopPlayerMode` 注释与实现不一致**：属性注释写「0.无 1.灵动岛 2.小窗播放器 3.歌词栏」，`switch` 实现是 0 全关 / 1 小窗播放器 / 2 歌词栏，无「灵动岛」分支（「未确认」哪个是预期语义，改前先与宿主确认）。
24. **`LoadSign`/`QLoadSign` 状态来自 `MusicApi`**：`Connections` 监听 `onLoaded`/`onFinished` 驱动，不要手动 `visible`。
25. **歌词界面 `timerFunction()` 是硬契约**：宿主 320ms 无条件调用；若界面改成自走定时器，必须保留该方法（可为空实现，如 `Lyrics3D`）。
26. **逐字数据通道判空规则**：`info` 是 `QVariantList` 不是 JS `Array`（见 5.6），用 `length` 判断，别用 `Array.isArray`。

## 6.8 速查索引

### 6.8.1 需求 → 组件

| 我要做… | 用… | 不要用… |
|---|---|---|
| 图标按钮（带提示） | `SButton` | `QButton`（`tipText` 无效） |
| 图标+文字按钮 | `QButton` | 手写 Rectangle+MouseArea |
| 开关 | `QSwitch`（信号 `toggled`，外部翻转 `switchTrue`） | `QtQuick.Controls.Switch` |
| 下拉单选 | `QDrop`（`useId` 兼容设备对象列表） | `ComboBox` |
| 2~3 项横向单选 | `QWideDrop`（文字）/ `QBigDrop`（带图） | `QDrop` |
| 滑杆 | `QSlider`（`valueText` + `leftText` 控制数值标签位置） | `QtQuick.Controls.Slider` |
| 单行输入 | `QInput`（`inputText`/`echoMode` 别名 + `entered`） | `TextField` |
| 大列表（在线） | `QListView`（`isList` 切歌曲/歌单） | 手写 ListView |
| 大列表（本地） | `QLocalView`（三路信号交给页面） | `QListView`（会带内置动作） |
| 网格列表 | `QGridView` | `GridView` 直用（会丢统一滚动） |
| 长内容滚动 | `QScrollView` | `Flickable` 直用 |
| 排序列表 | `QSortModel` 包一层源模型 | 自己写 comparator |
| 圆角/圆形图 | `QPicture` | `Image`+`MultiEffect` 手搓 |
| 二维码 | `QRCodeView` | 引第三方二维码库 |
| 模糊卡片/弹窗底 | `QBlurCard` | `MultiEffect`+`GaussianBlur` 手搓 |
| 模糊页签栏 | `QBlurTapBar` | `QTapBar`（无模糊） |
| 页栈切换 | `QPages.stack(index)` | 手改 visible |
| 设置行 | `SettingItem` / `SettingItemCard`（**仅限 SettingsView 内**） | 跨文件复用（见坑 1） |
| 通用选项弹窗 | 继承 `QOptionDialog` | 新写 Popup+模糊底 |
| 确认/输入提示 | `QAlertDialog` | `MessageDialog`（风格不一致） |
| 顶部轻提示 | `Options.warned(text, type)` → `QWarn` | 自建 toast |
| 右下角消息堆栈 | `QMessage.dialog(name,text,icon)` | — |
| 悬浮卡片入口 | `QFloatCard` + `controlItem` | 手写 hover 抬升 |
| 深色提示气泡 | `QTip`（手动 `visible` 绑定） | `ToolTip` 原生样式 |
| 标题栏按钮 | `QWKButton`（QWindowKit） | QButton |
| 小节标题 | `QHead` | 裸 `Text` |
| 加载动画 | `QLoadSign`（自动跟随 `MusicApi`） | 自建 gif |
| 消息菜单 | `QMenu`（`Instantiator`） | `Repeater` + `MenuItem` |

### 6.8.2 改动 → 影响面

| 改什么 | 连带要改/检查 |
|---|---|
| `Theme` 加颜色 | `Options.onChangeTheme` 里同步赋值（否则暗色不生效） |
| `Style` / `Config` / `ShortCut` 改属性名 | 用户配置迁移（QSettings category 见 §6.2、§6.7-10） |
| 新增单例 | `CMakeLists.txt` 116–127 列表 + 重新生成 qmldir（§6.2.3） |
| 改 `QBlurCard` 的 uniform | `shaders/cardblur.frag`、`cardblur_hq.frag` 与 `QBlurTapBar` 内的同源 `ShaderEffect` 一起改 |
| 改 `QListView` 菜单项 | `menuModel` 默认值、`runMenuAction` 分支、`QListView` 行内菜单注释三处保持一致 |
| 改 `SettingItem` 高度规则 | `SettingsView.qml` 所有 `SettingItem{...}` 调用点的 `y` 偏移 |
| 改歌词界面契约 | `layout/PlayerMaxCenter.qml` 的 `inject()` + `styleAllow` + `layout/MainLyric.qml` + `lyricsui/LyricsFree.qml` 同步 |
| 改 `Options` 注入清单 | `main.qml:21–35` 与 05 文档的 15 条清单同步 |
| 改 `Fonts.icon` 字体文件 | 图标码位属 FontAwesome 区段（§6.6.2），换字体须一并换所有 `\uXXXX` 字符 |
| 删除组件 | 先确认 `qmldir`/CMake 是 GLOB 自动收集（普通组件无需改 CMake，单例需改） |
---

← 返回 [索引 README.md](README.md)
