# Changelog

本项目所有重要的变更都会记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### 🐞 修复
- **切歌闪退（AOT 根因）**：`main.qml` 的 `startTrack()` 用 `!e` 取反队列条目、又用 `===` 比较 `undefined`，AOT 下紧随其后的属性查找会推导出空 `metaObject` 而崩溃（解释执行不崩）；改为 `e.path == undefined` 后 AOT 保持开启也不再闪退
- **音频线程安全**：Windows 上 `QAudioSink` 拉回调跑在 GUI 线程（界面卡顿即饿死音频）⇒ sink 移入专用输出线程；输出缓冲一律在该线程内重建（原先 GUI 侧重建会释放音频线程正在写的缓冲，属堆破坏）；音频线程只置原子标志，不再直接改 QML 状态
- **播放稳定性**：自动跳下一首改为连续 2 次即停（坏条目会形成高速跳歌循环）；`QueueModel::get()` 越界返回的空 map 在 QML 里是真值、`if (!e)` 拦不住；`DownloadManager` 析构期重入
- **列表与歌词**：收藏 / 排序列表操作全部失效（`QSortModel` 缺 `get(行)`，且收藏行字段是 `id` 而非 `favId`）；拖进度条回闪；逐字歌词首行整行重叠；内嵌 `SYLT` 与 `USLT` 同时存在时歌词与翻译二选一
- **数据源**：WebDAV 左键点歌不入播放列表、歌手列错位、已缓存行缺封面；酷狗分类页歌单曲目数恒为 0（字段是 `songcount`，且需 `withsong=1` 才返回）
- **自由歌词界面把「纯音乐」显示成一串重复字**：逐字渲染的 Repeater 用**歌词行号**去索引逐字数组
  （`linesText.model[lyricItem.index]`），同一行的每个字都取到同一项，整行于是变成重复的首字（默认主题用的是
  delegate 自身的下标，所以看起来正常）。现在按 delegate 下标取字，逐字判定收敛为 `info && info.length > 0`
  —— C++ 传来的是 `QVariantList`，有 `length` 但**不是 JS `Array`**，所以判定不能用 `Array.isArray`
- **本地歌曲不入播放列表（同名在线曲挡路）**：`FilePage` 的本地列表用 `Options.queue.indexOfName(歌名)` 查重，
  队列里已有同名在线曲时就跳过入队（于是播放器在放本地歌、队列索引却指向那首在线曲）。现在查重与入队统一按
  `path`（在线曲的 path 是 hash，不会与本地路径撞名），四处手写入队改走 `Playback.playItem()` /
  `Playback.enqueue()`（后者顺带给出「已在播放列表中」的反馈）
- **清理图片缓存后满屏「无法打开 cover-xxx.png」**：缓存文件删了，但 DB 的 `songs.tag_cover` 与模型里的
  `tagCoverUrl` 仍指向旧路径，列表每行都会尝试加载一次。现在 `clearCache()` 同时在 DB 线程把 `tagged` 置 0、
  `tag_cover` 清空，设置页清理后重载当前列表，封面回到占位并在下次打开时重新提取
- **`qmllint` 属性名错误（MusicInfo 等 5 处）**：`Playback.player` 的静态类型是 `AudioEngine`，而 `noTitle` / `urlStr` /
  `onMedia` 是 `main.qml` 里给 `mainMedia` 现加的 QML 属性，`album` / `date` / `type` / `audioBit` 则是元数据字段的
  别名 —— 这 7 个成员 lint 都看不到，于是报 `missing-property` 并让 CI 退出码为 1。现在 `noTitle` / `urlStr` /
  `onMedia` 提升为 `AudioEngine` 的正式属性（`onMedia` 派生自 `mediaStatus`），其余 4 个直接改用引擎自带的
  `albumTitle` / `mediaDate` / `mediaType` / `bitRate`，`mainMedia` 上的 6 个影子属性与 `onMetaDataChanged`
  里的镜像赋值一并删除
- **功能插件加载即失败**：`PluginHost` 的 delegate 里读了没声明成 `required property` 的 `modelData`（AOT 下抛 `ReferenceError`，接口建不出来 ⇒ 插件被自动停用）；`PluginApi.mount()` 用 url 挂载时把 `props` 当成
  `Loader` 自己的初始属性（`Loader does not have a property called api`，面板拿不到 `api`），现在改走
  `Loader.setSource(url, props)`，属性作为加载项的初始属性生效

### 🧩 重构
- **音频后端自研**：弃用 `QMediaPlayer` / `QAudioOutput`，改为 FFmpeg 解码 + 无锁环形缓冲 + `QAudioSink` 拉模式（DSP 全程在音频线程内）；频谱由引擎后处理直接驱动，不再依赖 `QAudioBufferOutput`
- **歌词界面模块化**：`PlayerMaxCenter` 拆成宿主框架 + 界面模块（`MainLyric`），宿主按注入契约传数据、样式改动走白名单 `request()`；内置 4 套界面（默认 / 视频壁纸 / 自由 / 3d），主题设置由模块自持持久化
- **插件系统**：歌词界面插件与功能插件两套（`PluginHost` 开放 titlebar / player / sidebar / window.overlay 扩展点），目录扫描 / 安装 / 校验抽到公共基类
- **在线列表统一（`QListView`）**：收藏 / 下一首 / 加入列表 / 下载 / 信息弹窗全部下沉进组件，7 个页面清掉二十余处重复实现
- **WebDAV**：新增 `cpp/webdav/`——PROPFIND 浏览、DPAPI 加密凭据、播放时后台缓存音频 / 歌词 / 封面（1GB LRU），已缓存曲目走本地播放

### ⚡ 优化
- **部署体积 −113MB**：解码层改用 Qt 自带 FFmpeg 7.1.3 替代 8.x 全量包；移除断点续播与未使用的 `ffmpegmediaplugin.dll` / `swscale`
- **音频回调零分配、无锁**：EQ 增益改直读快照数组；频谱通路改无锁环形缓冲，窗口 resize 不再卡住音频线程
- **在线 API 收敛**到 `api/ApiHttp.h`（统一 UA / Referer / Cookie + 15s 超时），评论接口加缓存、游标按稿件隔离
- **频谱**：段间混合 + 上升快 / 下降慢平滑，每帧 `exp` 从 128 次降到 2 次
- 清理：`CoverHelper::toImage()` 无调用已删除；`FilePage` 本地列表的播放/入队抽成 `playLocalEntry()` /
  `enqueueLocalEntry()`，四处重复的入队代码各收成一行
- **封面缓存瘦身**：内嵌封面缓存从「512×512 PNG」改为「JPEG（q88）」，并分两档 —— 列表用 **64×64 缩略图**
  （我的文件夹 / 本地文件夹 / WebDAV），播放页与歌曲信息仍取 512 大图；缓存文件名带尺寸，两档互不覆盖；
  缩放改为 `KeepAspectRatioByExpanding + SmoothTransformation`（覆盖式，配合 `PreserveAspectCrop` 不会被拉糊），
  带透明通道的封面先转 RGB 再存，免得 JPEG 发黑。旧版 `cover-*.png` 不再复用，会随缓存上限淘汰或手动清理消失

### ✨ 新增
- 音高调节 ±12 半音（与倍速可叠加）；自由歌词界面新增「显示歌曲名 / 歌手名」开关与「封面倒影」
- 沉浸中心翻新（`FullCenterView` + `centers/`）：Aurora 背景 + Liquid Glass 播放坞 / 队列 / 详情，列表分页与空状态补齐
- 音频自测扩到 11 项：切歌压力、变速换算方向、进度不回退、配置持久化、变调时长校验

## [0.1.0] - 2026-08-01

### ✨ 新增
- Qt 6.9 / QML / RHI 跨平台框架
- 集成网易云、酷狗音乐 API（搜索、歌单、排行榜、新歌、歌词）
- QWindowKit 无边框窗口（毛玻璃 / 云母效果）
- 多平台支持：Windows / macOS / Linux

### 🧩 依赖
- Qt 6.9.3（Core, Gui, Qml, Quick, Network, Multimedia, Concurrent, Sql, ShaderTools）
- QWindowKit（Apache-2.0）
- pako.js（MIT）


### [0.2.5] - 2026-08-10

### 新增
- 全面将在线api转到c++提升性能，稳定性，为之后接入QCloudMusicApi以及QQ音乐做好准备
- 全面重做歌词界面，背景效果由AMLL Core移植，歌词组件抛弃ListView转用自定义排列，效果大更新
- 增加更多功能，在歌词界面，本地文件夹，设置调节功能，界面功能都有大量更新

### 其他
- 修复大量Bug，大量之前一直存在的一些小问题，部分优化性能
- 规范部分代码，规范协议

### [0.3.0] - 2026-08-16

- 全面接入QCloudMusicApi，网易云音乐接口已基本完毕，已支持登陆。
- UI全面翻新，优化UI布局，配色。
- 歌词界面性能大优化，优化。
- 新增快捷键功能，支持自定义快捷键以实现快速控制播放器。
- 还有许多许多的小更新以及功能补充，例如：播放列表全部删除按钮，关于页面更多按钮，新增补充一些设置项功能。
- 修复大量Bug，修复大量布局问题。

### [0.4.0] - 2026-08-23

### 功能
- 大幅完善在线功能：如私人漫游，分类下的歌单，排行榜，歌手。（重点）
- 歌词界面新增对唱歌词，并大幅优化访问开销，稳定性和性能大幅增长。
- 新增搜索历史记录。
- 新增桌面歌词功能。
- 新增点击歌曲名和歌手使用搜索。

### 修复
- Linux版本修复设置问题（实际已在Beta0.3.1的新增加的补丁修复）。
- 优化部分ui界面细节。
- 修复本地歌曲播放的部分问题，如背景流体不跟随封面更改。
- 修复右键菜单和设置部分卡片主题颜色问题（特别是深色）。
- 还修复一些小bug，详情可自己体验。

### [0.5.0] - 2026-9-16

### 新增（基础体验大更新）
- 本地文件夹体验大更新，按钮功能补全，音乐文件夹内新增多选、排序、目录打开、加入播放列表，增加多种提示，完善体验
- 新增沉浸中心预览
- 歌词界面背景着色器更新，改为自制+三方双着色器，替代AMLL着色器，并且在歌词界面进行多个优化
- 音乐播放控制改进，增加或完善多种功能，如音高补偿、真随机播放、间隔循环、睡眠模式等，并且对应增加设置项
- 在线歌曲功能改进，网易云修复无法登陆问题，酷狗修复登陆账号为摆设，真正登陆的账号可以听高品质歌曲，以及vip歌曲，并优化许多在线歌曲的问题
- 跨平台改进，Linux增加Wayland与fcitx插件，解决Linux问题，MacOS适配窗口标题栏样式，在macos上显示左置的红绿灯
- 大幅优化性能，大幅采用多线程，移至c++进行计算，大幅简化QML框架，渲染性能更高，优化AOT编译占比。
- 补全一些设置项内容，补全一些内容
- 新增许多快捷键

### 其他
- 修复一些Bugs，修复一些平台上的问题
- 规范项目许可证
