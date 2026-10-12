# 第 4 部分 · 在线媒体层（api/）

> 本文件是 [QueMusic 项目全景文档索引](README.md) 的一个分片；任务定位与其它部分请见 [README.md](README.md)。
## 4.1 总览与架构

在线媒体层位于 `api/`，职责是把三个平台（酷狗 / 网易云 / 哔哩哔哩）的差异**全部吃掉**，只向 QML 暴露一套统一字段与一组统一方法。

| 文件 | 角色 | 行数（文件真实行数 = `0x0A` 计数） |
|---|---|---|
| `api/ApiCommon.h` | 纯头文件统一字段层：字段名常量 + `song()` / `listResult()` / `comment()` 构造器 | 79 |
| `api/ApiHttp.h` | 平台自建请求的公共 GET/GET-JSON helper（UA/Referer/Cookie/15s 超时） | 57 |
| `api/MusicApiService.{h,cpp}` | QML 单例 `MusicApi`：**调度中枢**，按 `source` 分发 + 统一处理 `resultReady` | 343 / 1325 |
| `api/NeteaseCloudApi.{h,cpp}` | 网易云实现（源 1），包 `QCloudMusicApi::ApiHelper`，跑在独立线程 | 63 / 720 |
| `api/KugouApi.{h,cpp}` | 酷狗实现（源 0），自建 GET + KRC 解码 + 两套签名 | 109 / 1497 |
| `api/BilibiliApi.{h,cpp}` | 哔哩哔哩实现（源 2），WBI 签名 + DASH 音频流 | 108 / 773 |
| `api/OnlineListModel.{h,cpp}` | 仿 `QML ListModel` 的 C++ 列表模型（动态 role） | 50 / 121 |
| `api/qrcode.js` | 纯 JS 二维码生成器（供登录二维码 Canvas 渲染，约 21.5 KB） | 514 |
| `api/QCloudMusicApi/` | 内嵌第三方库（网易云 API 的 Qt 移植，MIT） | — |

> **行号 / 行数口径（重要）**：本文 `file:line` 与 `read`、`grep`（ripgrep）一致，等于文件真实行号。⚠️ 本机 PowerShell 的 `Get-Content` 读这些文件时会**丢掉部分换行**（实测 `api/ApiCommon.h`：`0x0A` 字节 79 个，但 `Get-Content -Raw` 只剩 68 个 LF；`api/KugouApi.cpp` 真实 1497 行、`Get-Content` 只有 1448），因此**不要用 `Get-Content` / `(Get-Content).Count` 统计行数**，核对时请用 `[System.IO.File]::ReadAllLines()`（本文档与附录 A 采用的口径），或直接用 grep 行号。

### 4.1.1 单例注册与生命周期

- `MusicApiService` 用 `QML_NAMED_ELEMENT(MusicApi)` + `QML_SINGLETON` 注册进 QML 模块 `QueMusic`（`qt6_add_qml_module(URI QueMusic)`，`main.cpp` 里 `engine.addImportPath("qrc:/")`）。QML 同模块内**无需 import 即可直接用 `MusicApi`**。
- 构造函数签名是 `explicit MusicApiService(QObject *parent)`，**故意不给 parent 默认值**：否则 QML 引擎走「默认构造」分支而不调用 `create()`，`setAccountManager()` 的登录态注入永远不会执行（头文件 82-84 行有明确注释）。
- 单例工厂：`static MusicApiService *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)` → `new MusicApiService(qmlEngine)` + `service->setAccountManager(s_accountManager)`。
- 账号注入：`main.cpp` 顺序为 `AccountManager::create(...)` → `MusicApiService::setSharedAccountManager(accountManager)` → `engine.load(...)`。静态字段 `MusicApiService::s_accountManager`。

### 4.1.2 按 source 分发

三个平台各有平台编号，分散定义在两处（**注意是三个不同写法**）：

| 平台 | source | 常量定义处 |
|---|---|---|
| 酷狗 | `0` | `MusicApiService.cpp` 匿名命名空间 `kSourceKugou`；`KugouApi::Source = 0` |
| 网易云 | `1` | `kSourceNetease`；`NeteaseCloudApi.cpp` 匿名命名空间 `constexpr int kSource = 1`（头文件无公开常量） |
| 哔哩哔哩 | `2` | `kSourceBilibili`；`BilibiliApi::Source = 2` |

分发链路：

```
QML → MusicApi.xxx(...)            // source 省略 = -1
  → beginRequest()                  // 未完成请求计数 +1，loadState=true
  → resolve(source)                 // source<0 → 当前默认源 m_source（QML 属性 songSource）
  → syncSource(resolved)            // 只推「当前平台需要的那份」登录态/音质
  → DISPATCH 宏 switch(source)      // m_netease / m_kugou / m_bilibili 同名方法
  → 平台 resultReady(action, data, source)
  → handleResult(...)               // 统一销账 + 填模型/属性/发信号
```

- `DISPATCH(source, expr)` 宏（`MusicApiService.cpp:353`）内部把变量名写成 `quemusicSource`，注释说明是为了避免调用点传入同名变量时被自身初始化式遮蔽（历史上误传 `s` 读到未初始化值）。
- **唯一不走 `DISPATCH` 的入口是 `getCategoryPlaylists`**：它手写 `setLoadState(true)` + `if/else if/else` 三分支（酷狗 `getCategoryPlaylists`、网易云转发 `getMusicPlaylists`、B 站转发 `getMusicPlaylists`），**因此不 `beginRequest()`、不武装看门狗**（计数字段不进位，靠 `handleResult` 的 `endRequest()` 归零把 `loadState` 关掉）。改这里要一并想清楚加载态。
- 平台类不是指针而是**值成员**：`NeteaseCloudApi m_netease; KugouApi m_kugou; BilibiliApi m_bilibili;`（`MusicApiService.h:280-282`），三者的 `resultReady` 在构造函数里一并 connect 到 `handleResult`。
- 未实现的平台：`default:` 分支只 `qWarning() << "[api] 未实现的平台:"`，**不会销账**（靠 20s 看门狗兜底）。新增平台必须同时改 `DISPATCH` 宏、`syncSource` 与所有 `if (action == ...)` 分支。

### 4.1.3 `resultReady(action, data, source)` 契约

信号声明（三个平台完全一致）：

```cpp
void resultReady(const QString &action, const QVariant &data, int source);
```

`data` 的三种形状（`handleResult` 里的解析约定）：

| 形状 | 产生者 | 含义 |
|---|---|---|
| `{ "info": [...] }` | `ApiCommon::listResult()`，绝大多数列表接口 | 列表数据 |
| 扁平 map（`songName/author_name/album_img/timeLength/url/backup_url/fileName/hash/type/errReason`） | `getMusicInfo` | 播放/下载信息 |
| `{ "info": [...], "translate": [...], "hash": "..." }` | `getLyricInfo` | 歌词 + 翻译 |

`handleResult` 的固定前置逻辑（顺序不可调换）：

1. `if (action != "getLyricInfo") endRequest();` —— **歌词请求不计数**，跟着减会让别的请求的加载态提前收尾。
2. `const bool requestFailed = d.contains("error");` —— 带 `error` 的失败响应**不能**当成「没有数据/歌词」，否则网络故障会显示成「纯音乐，请欣赏」。
3. `const QVariant info = d.contains("info") ? d.value("info") : data;`
4. 列表结果若含 `hashhq/hashsq` → `rememberHashes()`，播放时才能按音质升级 hash。
5. 按 `action` 分支填模型 / 属性 / 发信号；未知 action 只 `qWarning() << "MusicApiService: 未知消息类型:"`。

---

## 4.2 MusicApiService 接口清单

### 4.2.1 Q_INVOKABLE（28 个，按功能分组）

| 分组 | 精确签名（默认值照抄） |
|---|---|
| 搜索 | `void searchSongs(const QString &keyword, int type, int page, int pageSize, int source = -1)` |
| 歌单 | `void getPlaylistMenu(int type, int source = -1)` |
| 歌单 | `void getMusicPlaylists(const QString &tagid, int page = 1, int pageSize = 20, int source = -1)` |
| 歌单 | `void getCategoryPlaylists(const QString &id, int page = 1, int pageSize = 20, int source = -1)` |
| 歌单 | `void getPlaylistSongs(const QString &listid, int page = 1, int pageSize = 20, int source = -1)` |
| 歌单 | `void getHotPlaylistMenu(int type, int source = -1)` |
| 歌单 | `void getHotPlaylists(int page = 1, int pageSize = 20, int source = -1)` |
| 推荐/新歌 | `void getRecommendSongs(int page = 1, int pageSize = 20, int source = -1)` |
| 推荐/新歌 | `void getNewSongs(int type, int page = 1, int pageSize = 20, int source = -1)` |
| 推荐/新歌 | `void getPersonalFm(int page = 1, int pageSize = 20, int source = -1)` |
| 推荐/新歌 | `void getPersonalRadar(int page = 1, int pageSize = 20, int source = -1)` |
| 排行榜 | `void getAllToplist(int source = -1)` |
| 排行榜 | `void getAllToplists()` |
| 排行榜 | `void getMusicToplist(int page, int pageSize, int rankid, int source = -1)` |
| 歌手 | `void getHotSingers(int page = 1, int pageSize = 20, int source = -1)` |
| 歌手 | `void getSingerCategory(int area, int page = 1, int pageSize = 30, int source = -1)` |
| 歌手 | `void getSingerSongs(const QString &singerid, int page = 1, int pageSize = 20, int source = -1)` |
| 播放/歌词/评论 | `void getMusicInfo(const QString &hash, int type = 0, int source = -1)` |
| 播放/歌词/评论 | `void getLyricInfo(const QString &hash, int duration, int source = -1)` |
| 播放/歌词/评论 | `void getComments(const QString &hash, int page = 1, int pageSize = 30, int source = -1)` |
| 本地歌词 | `void setLocalLyrics()` |
| 本地元数据 | `void readLocalMetadataAsync(const QString &filePath)` |
| 本地元数据 | `QString readLocalCoverHint(const QString &filePath)` |
| 本地元数据 | `QVariantMap readLocalAudioInfo(const QString &filePath)` |
| 本地元数据 | `QString readLocalLyricsText(const QString &filePath)` |
| 本地元数据 | `void writeLocalMetadata(const QString &filePath, const QVariantMap &meta)` |
| 本地歌词 | `void readLocalLyricsAsync(const QString &filePath, const QString &title, const QString &artist, int duration = 0, bool allowOnlineSearch = true)` |
| 本地歌词 | `void findLocalLyrics(const QString &filePath, const QString &title, const QString &artist, int duration = 0, int source = -1)` |

**不是 Q_INVOKABLE**（C++ 内部调用，QML 不可见）：`void findOnlineLyrics(const QString &title, const QString &artist, int duration, int source = 0)`（B 站无字幕时复用在线搜词链路，默认回退酷狗）。

**登录/账号类刻意不在本类**：登录、扫码、登出全部由 `cpp/AccountManager.h` 的 QML 单例 `AccountManager`（`QML_ELEMENT` + `QML_SINGLETON`）承担 → 见 §4.7。

### 4.2.2 分发目标（每个 Q_INVOKABLE 落到哪个平台方法）

| MusicApi 方法 | 网易云（1） | 酷狗（0） | 哔哩哔哩（2） |
|---|---|---|---|
| `searchSongs` | `cloudsearch`，type 1/1000/10/1006 | 3 个不同端点（见 §4.5.2） | 稿件搜索 / 合集搜索 |
| `getPlaylistMenu` | `playlist_catlist` | `category/list` | 本地恒表（9 个音乐区子分区） |
| `getMusicPlaylists` | `top_playlist?cat=` | `tag/specialList` | 分区稿件搜索 |
| `getCategoryPlaylists` | 转发 `getMusicPlaylists`（**action 仍是 `getMusicPlaylists`**） | `category/special`（action `getCategoryPlaylists`） | 转发 `getMusicPlaylists` |
| `getPlaylistSongs` | `playlist_track_all` | `special/song` | 合集 `season:<sid>:<mid>` 或单稿件 |
| `getRecommendSongs` | `personalized_newsong` | `rank/newsong?type=1` | 音乐区排行 |
| `getHotPlaylistMenu` | `playlist_hot` | `tag/recommend` | 本地恒表 |
| `getHotPlaylists` | `top_playlist?cat=全部` | 关键词轮换搜索 | 排行 UP 主 → 合集 |
| `getNewSongs` | `top_song` | `rank/newsong?type=` | `newlist?rid=3` |
| `getAllToplist` | `toplist` | `rank/list`（失败回退内置 6666/8888） | 本地恒表 |
| `getAllToplists` | **同时**调 `m_kugou.getAllToplist()` + `m_netease.getAllToplist()`，两次 `beginRequest()` | | |
| `getMusicToplist` | `top_list`（榜单 id 即歌单 id，客户端切片） | `rank/song?rankid=` | `ranking/v2?rid=3` 或分区搜索 |
| `getHotSingers` | `top_artists` | `singer/rank` | 排行 UP 主去重 |
| `getSingerCategory` | `artist_list`（area 7/96/8/16） | `singer/list`（area 1/2/3/4） | 忽略 area，同 `getHotSingers` |
| `getSingerSongs` | `artist_top_song`（忽略 page/pageSize） | `singer/song` | 按昵称搜索后按 mid 过滤 |
| `getMusicInfo` | `song_detail` + `song_url_v1` | `getSongInfo.php` ∥ `trackercdn/i/v2` | `view` + `player/playurl` |
| `getLyricInfo` | `lyric` | `lyrics.kugou.com/search` → `/download` | `player/v2` 字幕 |
| `getComments` | `comment_music` | `mcomment/v1/cmtlist` | `reply/wbi/main` |
| `getPersonalFm` | `personal_fm` | TOP500（rankid 8888） | 音乐区排行 |
| `getPersonalRadar` | `recommend_songs`（page>1 直接回空表） | 飙升榜（rankid 6666） | 音乐区最新投稿 |

### 4.2.3 Q_PROPERTY（全部，精确名字与类型）

| 属性 | 类型 | 访问 | 说明 |
|---|---|---|---|
| `songSource` | `int` | RW / NOTIFY `songSourceChanged` | 默认源；QML `Config.mainMusicSource` 写入 |
| `soundQuality` | `int` | RW / NOTIFY `soundQualityChanged` | 0 标准 128k / 1 高清 320k / 2+ 无损 flac |
| `searchSongsResults` | `OnlineListModel*` | CONSTANT | 搜索结果 |
| `newSongs` | `OnlineListModel*` | CONSTANT | 新歌 |
| `recommendSongs` | `OnlineListModel*` | CONSTANT | 推荐 |
| `musicPlaylists` | `OnlineListModel*` | CONSTANT | 歌单列表（首页分类 + 分类页共用） |
| `playlistSong` | `OnlineListModel*` | CONSTANT | **歌单/榜单/歌手歌曲共用**（见 §4.6） |
| `hotPlayLists` | `OnlineListModel*` | CONSTANT | 热门歌单 |
| `getHotlistMenu` | `OnlineListModel*` | CONSTANT | 热门歌单分类菜单（每次 clear 后重填） |
| `musicToplist` | `OnlineListModel*` | CONSTANT | 单平台榜单列表（每次 clear 后重填） |
| `toplistList` | `OnlineListModel*` | CONSTANT | 双平台榜单合并列表，每条注入 `source` |
| `singerList` | `OnlineListModel*` | CONSTANT | 歌手列表 |
| `personalFm` | `OnlineListModel*` | CONSTANT | 私人漫游 |
| `personalRadar` | `OnlineListModel*` | CONSTANT | 私人雷达 |
| `comments` | `OnlineListModel*` | CONSTANT | 歌曲评论 |
| `allPlaylistMenu` | `QVariant` | RW / NOTIFY `allPlaylistMenuChanged` | 歌单分类菜单（map 数组） |
| `lyricsData` | `QVariantList` | RW / NOTIFY `lyricsDataChanged` | 歌词 `[{time, text, info?, isOther?}]` |
| `lyricsTranslate` | `QVariantList` | RW / NOTIFY `lyricsTranslateChanged` | 翻译（与歌词行同序） |
| `lyricFollowActive` | `bool` | RW / NOTIFY `lyricFollowActiveChanged` | 是否跟随播放滚动 |
| `lyricPositionMs` | `qlonglong` | RW / NOTIFY `lyricPositionMsChanged` | 播放位置（ms），QML Binding 注入 |
| `lyricOffsetMs` | `int` | RW / NOTIFY `lyricOffsetMsChanged` | 用户歌词微调（ms） |
| `lyricIndex` | `int` | 只读 / NOTIFY `lyricIndexChanged` | 当前歌词行号 |
| `loadState` | `bool` | RW / NOTIFY `loadStateChanged` | 加载中（派生自未完成请求计数） |
| `globalid` | `QVariant` | RW / NOTIFY `globalidChanged` | 通用上下文 id（歌单 id / 榜单 id / 分类序号） |
| `globaltagid` | `QVariant` | RW / NOTIFY `globaltagidChanged` | 通用上下文 tag |
| `globalinfo` | `QVariant` | RW / NOTIFY `globalinfoChanged` | 通用上下文附加信息 |
| `nowIndex` | `int` | RW / NOTIFY `nowIndexChanged` | 搜索类型（搜索页用 0/1/2/3 表示歌曲/歌单/专辑/歌词） |
| `songFilter` | `int` | MEMBER `m_songFilter` / NOTIFY `songFilterChanged` | 0 全部 / 1 仅免费 / 2 仅 VIP |
| `downloader` | `DownloadManager*` | CONSTANT | 下载管理器（DownloadPage 直接用） |
| `downloadPath` | `QString` | RW / NOTIFY `downloadPathChanged` | 转发 `m_downloader.downloadPath()` |

### 4.2.4 signals

| 信号 | 用途 / 消费者 |
|---|---|
| `void loaded()` | `loadState` 置 true（`components/QLoadSign.qml` 显示加载动画） |
| `void finished()` | `loadState` 置 false（QLoadSign 结束动画） |
| `void urlplay(const QString &playurl, const QString &title, const QString &artist, const QString &cover, const QString &solve, const QString &hash, int source)` | **播放主链路**，`main.qml:718-742` 的 `Connections` 换源 + 写播放列表（把 `hash/source` 写进队列行） |
| `void warned(const QString &text, int type)` | 气泡提示，`main.qml` 转发给 `mainWarn.tiped()`；`type` 语义见 AccountManager（0 普通 / 1 成功 / 2 警告 / 3 错误） |
| `songSourceChanged()` `soundQualityChanged()` `allPlaylistMenuChanged()` `lyricsDataChanged()` `lyricsTranslateChanged()` `lyricFollowActiveChanged()` `lyricPositionMsChanged()` `lyricOffsetMsChanged()` `lyricIndexChanged()` `loadStateChanged()` `globalidChanged()` `globaltagidChanged()` `globalinfoChanged()` `nowIndexChanged()` `songFilterChanged()` `downloadPathChanged()` | 属性变更通知 |
| `void localLyricsReady(const QString &filePath, const QVariantList &lyrics, const QVariantList &translate)` | `main.qml:746` 校验 `filePath === Playback.localLyricsRequestPath` 后写入 `lyricsData` |
| `void localLyricsFailed(const QString &filePath)` | `main.qml:752`，已有歌词（>1 行）时保留、否则 `setLocalLyrics()` 占位 |
| `void localMetadataReady(const QString &filePath, const QVariantMap &meta)` | `main.qml:759`（工作线程读标签后回传） |
| `void localMetadataSaved(const QString &filePath, bool ok)` | `pages/FilePage.qml:1972` 提示保存结果 |

---

## 4.3 action 字符串字典

**重要事实：QML 侧不存在 `action ===` 分支，也拿不到 action 字符串。** action 是 C++ 内部的平台→调度中枢协议；QML 只消费「模型 / 属性 / signals」。全仓 grep 到的 `action` 判定只出现在 4 个 C++ 文件（`MusicApiService.cpp`、`NeteaseCloudApi.cpp`、`KugouApi.cpp`、`BilibiliApi.cpp`）。因此「谁消费哪个 action」= 「`handleResult` 的哪个分支 + 哪个 QML 页面绑定该模型」。

共 **19 个** action 字面量（全部精确列出）：

| # | action | 发出平台 | `handleResult` 去向 | 主要 QML 消费者 |
|---|---|---|---|---|
| 1 | `searchSongs` | 0/1/2 | 有本地搜词等待者时**不填模型**（走歌词链路）；否则 `m_searchSongsResults.append(normalizeList(info))` | `pages/SearchPage.qml`、`main.qml:120`、`layout/LeftSideBar.qml:65`、`layout/PlayerControl.qml:64` |
| 2 | `getPlaylistMenu` | 0/1/2 | 归一化后只保留 `title/category/id`，`setAllPlaylistMenu(menu)` | `pages/PlaylistPage.qml:51,279` |
| 3 | `getMusicPlaylists` | 0/1/2（1、2 也用于「分类歌单」） | `m_musicPlaylists.append(...)` | `pages/HomePage.qml:876`、`pages/PlaylistPage.qml:330` |
| 4 | `getCategoryPlaylists` | **仅 0（酷狗）** | `m_musicPlaylists.append(...)` | `pages/PlaylistPage.qml:330` |
| 5 | `getPlaylistSongs` | 0/1/2 | `m_playlistSong.append(songList(info))`（走 paytype 过滤） | `PlaylistPage`、`HomePage:915`、`SearchPage:216`、`FavouritePage:456`、`components/PlayListWindow.qml` |
| 6 | `getRecommendSongs` | 0/1/2 | `m_recommendSongs.append(...)` | `layout/LeftSideBar.qml:55`（首页每日推荐卡已改为走 `getPersonalRadar`） |
| 7 | `getHotPlaylistMenu` | 0/1/2 | **先 `m_getHotlistMenu.clear()`** 再 append | `HomePage:431` |
| 8 | `getHotPlaylists` | 0/1/2 | `m_hotPlayLists.append(...)` | `HomePage:639` |
| 9 | `getNewSongs` | 0/1/2 | `m_newSongs.append(songList(info))` | `pages/PlaylistPage.qml:243` |
| 10 | `getAllToplist` | 0/1/2 | `m_musicToplist.clear()` + append；另复制一份注入 `source` 后 `m_toplistList.append(...)` | `pages/PlaylistPage.qml:370` |
| 11 | `getMusicToplist` | 0/1/2 | **填 `m_playlistSong`**（不是 `musicToplist`！供 `playListSongsWindow` 展示） | `PlaylistPage:628,665` |
| 12 | `getHotSingers` | 0/1/2 | `m_singerList.append(...)`（分页累积，首页由 UI 侧 clear） | `pages/PlaylistPage.qml:565` |
| 13 | `getSingerCategory` | 0/1/2 | `m_singerList.append(...)`（与 12 同模型） | `pages/PlaylistPage.qml:565` |
| 14 | `getSingerSongs` | 0/1/2 | `m_playlistSong.append(songList(info))` | `pages/PlaylistPage.qml:665` |
| 15 | `getPersonalFm` | 0/1/2 | 空表且非失败 → `warned("私人漫游需要先在设置中登录账号", 2)`；否则 `m_personalFm.append(...)` | 暂无 UI 入口（首页歌单漫游已改为分类歌单，模型与接口保留） |
| 16 | `getPersonalRadar` | 0/1/2 | 同上，提示文案「私人雷达需要先在设置中登录账号」，模型 `m_personalRadar` | `pages/HomePage.qml`（`dailyWindow`，每日推荐主卡；只有一页，窗口不翻页） |
| 17 | `getMusicInfo` | 0/1/2 | `handleMusicInfo(d, source)` → `urlplay` 信号（type 0）或暂存待下载（type 1） | 所有 `QListView`/`QGridView` 委托；`main.qml` 的 `onUrlplay` |
| 18 | `getLyricInfo` | 0/1/2 | 代次校验 → `setLyricsData/setLyricsTranslate` 或本地歌词回传；末尾触发待下载 | 所有歌词视图（`layout/PlayerMaxCenter.qml`、`components/DesktopLyrics.qml`） |
| 19 | `getComments` | 0/1/2 | **提前 return**：`m_comments.append(...)`，不参与 hash/音质逻辑 | `components/MusicInfo.qml:308` |

补充细节：

- `getLyricInfo` 是**唯一不参与 `endRequest()`** 的 action。
- 网易云的 `getMusicInfo` 在 worker 里额外调 `song_url_v1`，但 `action` 仍是 `getMusicInfo`。
- B 站所有列表接口共用 `emitList(action, ...)`，`action` 由辅助函数参数一路传递（`searchVideos` / `searchPlaylists` / `seasonsOf` / `ranking` / `newVideos` / `rankingUsers` 都带 `const QString &action` 形参）。
- 新增 action 必须同时改：平台 `emit` → `handleResult` 分支 → 模型/属性 → §4.2.2 表。

---

## 4.4 统一字段层 ApiCommon

`namespace ApiCommon`，纯 `inline` 常量与函数，无 .cpp。

| 常量 | 字面量 | 含义 |
|---|---|---|
| `kTitle` | `"title"` | 歌名 / 歌单名 / 歌手名 / 榜名单（同一槽位多语义） |
| `kArtist` | `"artist"` | 歌手 / 歌单创建者 / 榜单更新频率 |
| `kCover` | `"cover"` | 封面 URL（酷狗常带 `{size}` 占位符） |
| `kHash` | `"hash"` | **通用 ID**：酷狗=文件 hash，网易云=数字 id 字符串，B站=bvid / `season:<sid>:<mid>` / 排行 tid |
| `kHashHq` | `"hashhq"` | 高清（320k）ID，缺省回退 `hash` |
| `kHashSq` | `"hashsq"` | 无损（flac）ID，缺省回退 `hash` |
| `kPaytype` | `"paytype"` | 付费标记（见下） |
| `kDuration` | `"duration"` | **秒**（歌单卡片槽位里存的是「曲目数」） |
| `kAlbum` | `"album"` | 专辑名 / 歌单简介 / 榜单简介 |
| `kPlaycount` | `"playcount"` | 播放量（部分平台用 `4294967295` 表示缺失 → 归一为 0） |

`paytype` 取值约定（唯一权威，`ApiCommon.h:23-30` 有长注释）：

| 值 | 常量 | 含义 |
|---|---|---|
| `0` | `kPaytypeFree` | 免费可播放 |
| `3` | `kPaytypePaid` | 需要会员/付费 |

**消费端只认 `3`**：VIP 角标（`components/QListView.qml`）、「仅免费 / 仅 VIP」筛选（`MusicApiService::songList`）。平台侧绝不能把自家枚举原样透传 —— 注释里点名了历史 bug：网易云 `fee=1` → 被映射成 `1`、酷狗 `pay_type=1` → 被直传成 `1`，两边都落进没有消费端认的槽位，会员曲目反而不显示 VIP 角标。现在三家的归一函数：

| 平台 | 原始字段 | 归一逻辑（精确） |
|---|---|---|
| 网易云 | `privilege.fee`，回退 `song.fee` | `(fee == 1 \|\| fee == 4) ? kPaytypePaid : kPaytypeFree` |
| 酷狗 | `pay_type` | `pay_type == 0 ? kPaytypeFree : kPaytypePaid`（实测只出现 0/1/3，非 0 一律付费） |
| 哔哩哔哩 | 无 | 恒为 `0`（`ApiCommon::song(...)` 的 `paytype` 默认参数） |

### 4.4.1 构造器

```cpp
QVariantMap song(QString title, QString artist, QString cover, QString hash,
                 int duration = 0, QString album = QString(),
                 QString hashhq = QString(), QString hashsq = QString(),
                 int paytype = 0, qint64 playcount = 0);
QVariantMap listResult(const QVariantList &items);   // → { "info": items }
QVariantMap comment(QString user, QString avatar, QString content,
                    qint64 time = 0, int liked = 0, int replies = 0);
```

- `song()` 内 `hashhq/hashsq` 为空时**自动回退为 `hash`**，所以各平台可以偷懒只传 `hash`（网易云传 `id, id`，B 站全不传，酷狗传真实的高清/无损 hash）。
- 评论字段：`user / avatar / content / time`（**秒**）/ `liked` / `replies`，三家解析后完全同构（网易云 `user.nickname/avatarUrl`、酷狗 `user_name/user_pic`、B 站 `member.uname/avatar`）。

### 4.4.2 音质档位与降级

档位（`m_soundQuality`，与 QML `Config.soundQuality` 同步，`main.qml:80-86` 用 `Binding` 注入，构造时另读一次 ini 兜底）：

| 值 | 语义 | 注释里的档位名 |
|---|---|---|
| `0` | 标准 128k | `standard` |
| `1` | 高清 320k | `high`（默认值，`m_soundQuality = 1`） |
| `2`+ | 无损 flac | `lossless` |

各平台取用方式：

| 平台 | 音质如何体现 | 降级/回退 |
|---|---|---|
| 酷狗 | **同一首歌的不同音质是三个不同 hash**，由 `MusicApiService::resolveQualityHash(hash)` 选 | `soundQuality>=2` 且有 `sq` → sq；`==1` 且有 `hq` → hq；否则 base（普通 hash） |
| 网易云 | hash 即 id，音质由请求参数决定：worker 固定 `level=exhigh`；未登录只得 128k | 服务端明确不给地址 → `url_unavailable=true` → 地址留空给上层提示；仅当请求本身失败才回退公开外链 `http://music.163.com/song/media/outer/url?id=<hash>.mp3` |
| 哔哩哔哩 | `BilibiliApi::setQuality(q)`，`pickAudio()` 按 DASH audio `id` 距离选题（目标 `q>=2 → 30251` / `q==1 → 30280` / 否则 `30216`） | 距离最近者胜；`baseUrl` 空则试 `base_url`；再空则回退 `durl[0].url`（音视频合流） |

hash ↔ 音质映射缓存（酷狗用，网易云/B站因 `hashhq==hashsq==hash` 实际不产生条目）：

- 内存：`QHash<QString, QualityAlts> m_alts`（base → {hq, sq}）+ `QHash<QString, QString> m_baseOf`（任意 hash → base）。
- 来源：`rememberHashes()` 扫列表结果的 `hashhq/hashsq`；`m_alts.size() > 5000` 直接放弃加速（不做无界增长）。
- 落盘：`ConfigLocation/BroNekoX/QueMusicQuality.json`，结构 `{ "hq": {base: hqHash}, "sq": {base: sqHash} }`，`scheduleSaveQualityCache()` 1500ms 去抖（`m_altsSaveTimer`），析构时若 dirty 再存一次。
- 队列/收藏统一用**普通 hash 当身份**（`m_baseOf.value(playHash, playHash)`），否则同一首歌会因音质不同在队列里出现多条。

---

## 4.5 平台实现对比

| 平台 | 类名 | 认证方式 | 取播放地址方式 | 歌词来源 | 特殊限制 |
|---|---|---|---|---|---|
| 酷狗（0） | `KugouApi` | Cookie（`token`/`userid`）+ 设备标识 `mid`/`dfid`；扫码登录由 AccountManager 完成，两套签名 | `getSongInfo.php`（128k 兜底）∥ `trackercdn/i/v2`（按 hash 精确音质，`status==1` 有权、`==2` 需会员）；无地址且已登录 → `wwwapi.kugou.com/play/songinfo`（web 签名） | `lyrics.kugou.com/search` → `/download?fmt=krc`，原生 KRC 解码（base64→跳 4→XOR→inflate） | 大量 `http://` 端点（凭证按 `ApiHttp` 规则不随明文出网）；JSON 可能被 `<!--KG_TAG_RES_START-->` 包裹；`pay_type` 非 0 即会员 |
| 网易云（1） | `NeteaseCloudApi`（内含 `CloudWorker`） | 走 `QCloudMusicApi::ApiHelper` 的 cookie 管理（`set_cookie`）；AccountManager 保证始终带稳定 `deviceId + os=pc` 身份 | worker 内 `song_detail` + 追加 `song_url_v1{level:exhigh}`；免登录 128k，带登录态可更高 | `lyric` 的 `lrc.lyric` / `tlyric.lyric`，LRC 正则解析 + 翻译按时间戳对齐 | 跑在独立 `QThread`，`ApiHelper` 只能在该线程创建/使用；`invoke()` 返回 `{status, body, cookie}`，业务数据在 `body`；`song_detail` 的 `ids` **必须是字符串** |
| 哔哩哔哩（2） | `BilibiliApi` | **无需登录**（`setCookie` 可选）；搜索类接口需 WBI 签名 + `buvid3/buvid4` 指纹（自动获取并缓存） | `x/web-interface/view` 取 `cid` → `x/player/playurl?fnval=16&fourk=1` 取 DASH `audio[]` | `x/player/v2` 的 `subtitle.subtitles[].subtitle_url` → 字幕 JSON `body[]` | 音频区已下线，音乐载体=音乐区稿件（hash 即 bvid）；歌单=合集 `season:<sid>:<mid>`；`ranking/v2` 只有 `rid=3` 可用；榜单/歌手/推荐全是「排行 + 合集」的等价实现 |

### 4.5.1 网易云 `NeteaseCloudApi`

公开方法（`public slots`，**不是 Q_INVOKABLE** —— 只被 `MusicApiService` 调用）：

```
setCookie(const QString&)
searchSongs(const QString&, int, int, int)          getPlaylistMenu(int)
getMusicPlaylists(const QString&, int, int)         getPlaylistSongs(const QString&, int, int)
getRecommendSongs(int, int)                         getHotPlaylistMenu(int)
getHotPlaylists(int, int)                           getNewSongs(int, int, int)
getAllToplist()                                     getMusicToplist(int, int, int)
getHotSingers(int, int)                             getSingerCategory(int, int, int)
getSingerSongs(const QString&, int, int)            getMusicInfo(const QString&, int)
getLyricInfo(const QString&, int)                   getComments(const QString&, int, int)
getPersonalFm(int, int)                             getPersonalRadar(int, int)
```

- 架构：`NeteaseCloudApi`（GUI 线程）持有 `QThread *m_thread` + `CloudWorker *m_worker`（`moveToThread`）。请求经信号 `runRequest(action, call)` 投递，cookie 经 `setCookieRequest(cookie)` 投递；结果 `CloudWorker::result` → `onWorkerResult` → `resultReady`。析构 `quit()+wait()` 后才 `delete m_worker`。
- `call` map 约定：`{ "member": <QCloudMusicApi 方法名>, "arg": {...}, 以及可选 "ntype"/"hash"/"type" }`。
- 用到的 QCloudMusicApi 方法：`cloudsearch`、`playlist_catlist`、`top_playlist`、`playlist_track_all`、`personalized_newsong`、`playlist_hot`、`top_song`、`toplist`、`top_list`、`top_artists`、`artist_list`、`artist_top_song`、`song_detail`、`song_url_v1`、`lyric`、`comment_music`、`personal_fm`、`recommend_songs`。（AccountManager 另用 `login_qr_*` 系列。）
- 首次创建 `ApiHelper` 时立刻 `setFilterRules("QCloudMusicApi.debug=false")`（其 debug 日志是「每个请求三行」，会刷满控制台）。
- 自带 action 字面量（18 个）：`getComments`、`searchSongs`、`getPlaylistMenu`、`getMusicPlaylists`、`getHotPlaylists`、`getPlaylistSongs`、`getMusicToplist`、`getSingerSongs`、`getPersonalFm`、`getPersonalRadar`、`getRecommendSongs`、`getHotPlaylistMenu`、`getNewSongs`、`getAllToplist`、`getHotSingers`、`getSingerCategory`、`getMusicInfo`、`getLyricInfo`（**无 `getCategoryPlaylists`**）；未知 action 只 `qWarning() << "[NeteaseCloudApi] 未知 action:"`。
- 字段映射要点：
  - `parseSongList()`：`ar[0].name`→`artist`（空数组用 `artists`，再空→`"未知歌手"`）、`al.picUrl`→`cover`、`al.name`→`album`、`id`（数字转字符串）→ `hash/hashhq/hashsq`、`dt>0 ? dt/1000 : duration/1000`→`duration`。
  - `parsePlaylists()`：`name/creator.nickname/coverImgUrl/id/trackCount/description/playCount`。
  - `searchSongs` 按 `ntype`：`1`/`1006` → 歌曲、`1000` → 歌单、`10` → 专辑（`artist.name` 当歌手）。
  - `getMusicInfo` 产出：`backup_url`+`url`、`songName`、`author_name`（`ar[0].name`）、`singer_img`（`img1v1Url` 回退 `picUrl`）、`album_img`（`al.picUrl`）、`timeLength`（秒）、`fileName`（`<歌名>.mp3`）、`hash`（**回传入参 hash 原值**，不取请求结果里的 id）、`type`；出错时 `error` 键存在。
  - `getLyricInfo` 产出 `info`（`{time,text,info}`，`info` 恒为无效 `QVariant`）、`translate`（按时间戳对齐的字符串数组）、`hash`（来自 `call.arg.id`）。
  - `getComments`：`hotComments + comments` 拼接，`time/1000`、`likedCount`、`beReplied.size()`。
- 分页特例：`getPersonalFm` 忽略 page/pageSize；`getPersonalRadar` 在 `page > 1` 时**直接 emit 空表**（所以首页每日推荐窗口收到 `ended` 只置 `isEnd`，不翻页）；`getMusicToplist` 因 `top_list` 不支持服务端分页，按 `offset/limit` 在客户端 `mid()` 切片；`getSingerSongs` 忽略 page/pageSize。

### 4.5.2 酷狗 `KugouApi`

公开方法（`public slots`/普通成员）+ 静态工具：

```
setCookie(const QString&)                          setDeviceInfo(const QString &mid, const QString &dfid)
static QByteArray kugouWebSignature(const QJsonObject&)
static QString randomGuid()
static QString kugouMidFromGuid(const QString&)
searchSongs / getPlaylistMenu / getMusicPlaylists / getCategoryPlaylists / getPlaylistSongs /
getRecommendSongs / getHotPlaylistMenu / getHotPlaylists / getNewSongs / getAllToplist /
getMusicToplist / getHotSingers / getSingerCategory / getSingerSongs / getMusicInfo /
getLyricInfo / getComments / getPersonalFm / getPersonalRadar    // 形参同 MusicApiService（无 source）
```

自带 action 字面量（19 个，**唯一会发出 `getCategoryPlaylists` 的平台**）：`searchSongs`、`getPlaylistMenu`、`getCategoryPlaylists`、`getMusicPlaylists`、`getPlaylistSongs`、`getRecommendSongs`、`getHotPlaylistMenu`、`getHotPlaylists`、`getNewSongs`、`getAllToplist`、`getMusicToplist`、`getHotSingers`、`getSingerCategory`、`getSingerSongs`、`getMusicInfo`、`getLyricInfo`、`getComments`、`getPersonalFm`、`getPersonalRadar`。

端点与签名（精确）：

| 用途 | 端点 | 备注 |
|---|---|---|
| 歌曲搜索 | `http://mobilecdn.kugou.com/api/v3/search/song` | `format=json` |
| 歌单搜索 | `http://mobilecdnbj.kugou.com/api/v3/search/special` | |
| 专辑搜索 | `http://msearch.kugou.com/api/v3/search/album` | |
| 歌词搜索 | `http://mobileservice.kugou.com/api/v3/lyric/search` | |
| 歌单分类 | `http://mobilecdnbj.kugou.com/api/v3/category/list` | 21 个真实分类 |
| 分类歌单 | `http://mobilecdnbj.kugou.com/api/v3/category/special` | `withsong=1` 才带 `songcount`（体积约 4 倍） |
| tag 歌单 | `http://mobilecdnbj.kugou.com/api/v3/tag/specialList` | |
| 热门分类 | `http://mobilecdnbj.kugou.com/api/v3/tag/recommend` | |
| 歌单内歌曲 | `http://mobilecdnbj.kugou.com/api/v3/special/song` | |
| 新歌/推荐 | `http://mobilecdnbj.kugou.com/api/v3/rank/newsong` | |
| 榜单列表 | `http://mobilecdnbj.kugou.com/api/v3/rank/list` | 空结果回退内置「酷狗飙升榜 6666 / 酷狗TOP500 8888」 |
| 榜单歌曲 | `http://mobilecdnbj.kugou.com/api/v3/rank/song` | |
| 歌手榜/分类/歌曲 | `.../api/v3/singer/rank`、`/singer/list`、`/singer/song` | |
| 播放元信息 | `https://m.kugou.com/app/i/getSongInfo.php?cmd=playInfo&hash=` | `data` 包裹与裸对象都兼容 |
| 精确取址 | `https://trackercdn.kugou.com/i/v2/` | `key = MD5(hash + "kgcloudv2")`；`br=hq` 无实际作用（音质由 hash 决定） |
| 签名播放 | `https://wwwapi.kugou.com/play/songinfo` | `appid=1014`、`platid=4`、`srcappid=2919`、`type=1` |
| 评论 | `https://gateway.kugou.com/mcomment/v1/cmtlist` | `appid=1005`、`clientver=20489`、`code=fc4be23b4e972707f36b8a828a93ba8a` |
| 歌词 | `http://lyrics.kugou.com/search` → `http://lyrics.kugou.com/download` | `ver=1&client=pc&fmt=krc&charset=utf8` |

签名与设备：

| 函数 | salt（精确） | 算法 |
|---|---|---|
| `kugouWebSignature(QJsonObject)` | `NVPh5oo715z5DIWAeQlhMDsWXXQV4hwt` | `MD5(salt + sorted("k=v"...).join("") + salt)`，各端只差 salt |
| `kugouAndroidSignature(QVariantMap)` | `OIlwieks28dk2k092lksi2UIkp` | 同上 |
| `randomGuid()` | — | 标准 UUID v4 形状（第 3 段以 `4` 开头、第 4 段首字符 8-11） |
| `kugouMidFromGuid(guid)` | — | `MD5(guid)` 的**大端字节转十进制字符串**（内置 `BigUint` 简化实现），登录时生成/持久化 |

- `getMusicInfo` 两路并行：`requestPlayMeta`（元信息 + 128k 兜底 URL）+ `requestTrackerUrl`（按 hash 精确音质），共用 `QSharedPointer<PlayRequest>`，两者都 `done` 才 `finishPlayInfo`。合并优先级：trackercdn → `metaUrl` → `metaBackupUrl`；文件名后缀跟随实际音质（无损 `.flac`）；`trackerStatus == 2` → `errReason = "vip"`，否则 `"unavailable"`；有 `token` cookie 时先转 `requestSignedPlayInfo` 重试；`hash` 为空直接回 `errReason = "unavailable"`。
- 音质 hash 派生（列表解析阶段）：`hqHashFrom()`：`pay_type_320 == 3` → 回退 base；否则 `320hash`，再否则 `trans_param.ogg_320_hash`。`sqHashFrom()`：`pay_type_sq != 0` → 回退 base；否则 `sqhash`。
- KRC 解码：`Base64 → 丢弃前 4 字节 → 逐字节 XOR（16 字节密钥 `{64,71,97,119,94,50,116,71,81,54,49,45,206,210,110,105}`）→ 依次试偏移 0/4/2 → zlib uncompress，失败再补 zlib 头（`78 9C` + adler32）解 raw-deflate`。`krcToLyrics()` 输出 `{time, text, info:[{offset,duration,text}], isOther}`（`info` 是**字级**数组，`isOther` 表示整行被全角括号包裹的对唱行）；`krcTranslations()` 解析 `[language:<base64>]`，取 `content[0]`（`type != 1` 且有第 2 项时取 `content[1]`）的 `lyricContent`，纯空格行归一为 `""`。
- 歌词 `duration` 归一：`> 10000` 视为毫秒并 `/1000`。
- `getComments`：先用 `m_mixIdCache`（hash → 数字 `mixsongid`），未命中先 `getSongInfo.php` 取 `album_audio_id`；为空或 `"0"` 直接回空表；时间用 `yyyy-MM-dd HH:mm:ss` 解析成 epoch 秒。
- 数值常量：歌手分类 area `1 华语 / 2 欧美 / 3 日本 / 4 韩国`；私人漫游 = TOP500（`8888`）、私人雷达 = 飙升榜（`6666`）；`playcount == 4294967295` → `0`；热门歌单用 8 个关键词（热门/古风/粤语/摇滚/车载/电音/民谣/爵士）轮换、每词 20 页 = 160 页；歌手名优先 `authors[0].author_name`（**必须先判空数组**，`QJsonArray::at(0)` 越界是 UB）再回退 `singername`；新歌取前两位作者用 `,` 拼接。

### 4.5.3 哔哩哔哩 `BilibiliApi`

公开方法（`public slots`/普通成员）：

```
setQuality(int)                                    setCookie(const QString&)
searchSongs / getPlaylistMenu / getMusicPlaylists / getPlaylistSongs / getRecommendSongs /
getHotPlaylistMenu / getHotPlaylists / getNewSongs / getAllToplist / getMusicToplist /
getHotSingers / getSingerCategory / getSingerSongs / getMusicInfo / getLyricInfo /
getComments / getPersonalFm / getPersonalRadar
```

自带 action 字面量：`getComments`、`getLyricInfo`、`getPlaylistSongs`、`getPlaylistMenu`、`getMusicPlaylists`、`getRecommendSongs`、`getHotPlaylistMenu`、`getHotPlaylists`、`getNewSongs`、`getAllToplist`、`getMusicToplist`、`getHotSingers`、`getSingerCategory`、`getSingerSongs`、`getMusicInfo`、`getPersonalFm`、`getPersonalRadar`（**无 `getCategoryPlaylists`**）。

端点与常量：

| 项 | 值 |
|---|---|
| 基址 | `https://api.bilibili.com` |
| 搜索 | `/x/web-interface/wbi/search/type`（需 WBI 签名） |
| 密钥/指纹 | `/x/web-interface/nav`（`wbi_img.img_url` / `sub_url`）→ `/x/frontend/finger/spi`（`b_3` / `b_4`） |
| 稿件详情 | `/x/web-interface/view?bvid=` |
| 播放地址 | `/x/player/playurl?bvid=&cid=&fnval=16&fourk=1` |
| 字幕 | `/x/player/v2?bvid=&cid=` |
| 合集 | `/x/polymer/web-space/home/seasons_series?mid=&page_num=1&page_size=10`、`/x/polymer/web-space/seasons_archives_list?mid=&season_id=&sort_reverse=false&page_num=&page_size=` |
| 排行 | `/x/web-interface/ranking/v2?rid=3&type=all`（**rid 只有 3 可用**） |
| 最新 | `/x/web-interface/newlist?rid=3&type=0&pn=&ps=` |
| 评论 | `/x/v2/reply/wbi/main`（需签名；`type=1`、`mode=3`、`ps` 上限 30、`web_location=1315875`） |
| UA | `Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36` |
| Referer | `https://www.bilibili.com` |

- WBI 签名（`getSigned`）：取 64 字节混淆表 `kMixinTab[64]`，`mixinKey = (imgKey + subKey)` 按表取 32 个字符；参数加 `wts`（秒级时间戳）→ key 排序 → `k=v` 用 `QUrl::toPercentEncoding` 编码并以 `&` 连接，值先剔除 `[!'()*]` → `w_rid = MD5(query + mixinKey).toHex()`。
- `ensureKeys()`：密钥与 buvid 只取一次；`m_keyRequesting` 为真时把任务压入 `m_keyWaiters` 排队；取完先把**发起者自己**的 `then()` 调掉再跑等待队列（注释指出漏掉自己会让「启动后第一次签名请求」被静默丢弃）。
- 指纹策略：`get(url, cb, withCookie=true)` 时带 `buvid3=<b_3>; buvid4=<b_4>`（有登录 Cookie 再追加）；**评论只带登录态**（`!m_loginCookie.isEmpty()`），因为「有指纹、无登录票据」会被风控把结果砍到 3 条。
- `hash` 编码：稿件 = `bvid`；合集 = `season:<sid>:<mid>`（`getPlaylistSongs` 按 `split(':')` 判定）；榜单/歌手项 = tid / 数字 mid 的字符串。
- `toSong()`：`cleanTitle(title)`（去 `<em class="keyword">` 高亮标签）、`artist` 优先 `owner.name` 回退 `author`（再回退 `fallbackArtist`）、`cover` 取 `pic` 并补 `https:`、`hash = bvid`、`duration` 兼容 `"m:ss"`/`"h:mm:ss"` 与秒、`playcount` 取 `stat.view` 回退 `play`；`QString(), QString(), QString(), 0` 表示 `hashhq/hashsq` 不单独提供（自动回退 hash）。
- `getMusicInfo`：`view` 取 `cid` → `playurl` 取 `dash.audio[]` → `pickAudio()` 按目标 `id` 距离最近；`baseUrl` 空试 `base_url`；仍空回退 `durl[0].url`；全空或 `cid` 缺失 → `fail()`（`{hash, type, errReason:"unavailable"}`）。成功产出 `backup_url`+`url`、`songName`（去标签标题）、`author_name`（`owner.name`）、`singer_img`（`owner.face`）、`album_img`（`pic`）、`timeLength`（秒）、`fileName`（`<标题>.m4a`）、`hash`、`type`。
- `getLyricInfo`：`player/v2` → `subtitle.subtitles[].subtitle_url`（第一个非空）→ 字幕 JSON `body[]`，输出 `{time: int(from*1000), text: content}`，`translate` 恒为空数组；**无字幕时 emit 空表**，由 `MusicApiService` 检测 `source == 2 && lyricHash == m_biliTrack.hash && onlineLyrics.isEmpty()` → `findOnlineLyrics(..., source=0)` 复用酷狗搜词。
- 音乐区子分区恒表（`getPlaylistMenu` / `getAllToplist` / `getMusicToplist` 共用）：`音乐综合 3`（`kMusicTid`）、`原创音乐 28`、`翻唱 31`、`VOCALOID 30`、`演奏 59`、`电音 194`、`三次元音乐 29`、`乐评盘点 243`、`AI音乐 265`；榜单名 = `<分区名>榜`；`getMusicToplist(rankid)` 中 `rankid==3` 走排行接口、其余走分区搜索。
- 合集检索：`getHotPlaylists` / `searchSongs(type==1)` 先搜稿件 → `upMids(..., kUpLimit=5)` 取去重 UP 主 → 并发 `fetchSeasons` → 全部返回后合并（`PlaylistSearch{pending, limit, items, seen}`）；`total < 2` 的单集合集丢弃；结果超 `limit` 截断。
- `getComments`：`m_aidCache`（bvid → aid）避免重复 `view`；翻页游标 `m_commentCursors` 按 hash 分开存（多首歌并发取评论不串页）；`next = page <= 1 ? 0 : cursor`。
- 静默忽略/等价替代：`getSingerCategory` 忽略 `area`，与 `getHotSingers` 同实现（排行 UP 主去重）；`getNewSongs` 忽略 `type`；`getSingerSongs` 需要 `m_upNames` 里有该 mid 的昵称，否则回空表；`getHotPlaylists` 在 `page > 1` 直接回空表。

---

## 4.6 OnlineListModel

`QAbstractListModel` + `QML_ANONYMOUS`，用 C++ 复刻 QML `ListModel` 的 API 与语义（旧 `musicWorker.mjs` 的输出字段不变，QML 无需改动）。

| Q_PROPERTY / 方法 | 说明 |
|---|---|
| `Q_PROPERTY(int count READ rowCount NOTIFY countChanged)` | QML 侧 `.count` |
| `Q_INVOKABLE QVariantMap get(int index) const` | 越界返回空 map |
| `Q_INVOKABLE void clear()` | `beginResetModel()`，空表时不发信号 |
| `Q_INVOKABLE void append(const QVariant &items)` | 支持单个 map 或 map 数组；**先 `collectRoles()` 再 `beginInsertRows()`**（roleNames 变化必须在插入前完成） |
| `Q_INVOKABLE void remove(int index, int count = 1)` | `beginRemoveRows` + `QList::remove` |
| `void setItems(const QVariantList &items)`（非 Q_INVOKABLE） | 清空 + 整体填充（reset 语义）；**当前调度中枢未调用它**，`handleResult` 一律用 `append` |

- roles：构造时用 `kKnownRoles[]` **预注册 33 个已知字段**（否则委托声明 `required property` 时缺字段的行会整行不渲染），随后任何新字段在 `append`/`setItems` 时动态分配 role（`m_nextRole = Qt::UserRole + 1` 起）。role 内部用 `m_roleKeys`（role id → 原字段名）取值，`data()` 热路径免二次转换。
- 预注册字段精确清单：`title, artist, album, cover, hash, duration, source, playcount, paytype, category, id, tagid, url, fileName, hashhq, hashsq, lyric, mvid, singer, singername, songname, specialname, specialid, album_name, albumid, imgurl, intro, username, author_name, user, avatar, content, liked`。
- 追加 vs 替换（**由 `handleResult` 决定，模型自身不提供策略**）：

| 行为 | 模型 |
|---|---|
| 纯追加（首页由 UI 侧 `clear()`） | `searchSongsResults`、`newSongs`、`recommendSongs`、`musicPlaylists`、`playlistSong`、`hotPlayLists`、`singerList`、`personalFm`、`personalRadar`、`comments` |
| 每次结果先 clear | `getHotlistMenu`、`musicToplist` |
| 结果前先 clear（在 `MusicApiService` 里） | `toplistList`（`getAllToplist` / `getAllToplists` 入口都 `clear()` 一次，再按平台累积） |

- 过滤：**只有歌曲列表**走 `songList()` → `normalizeList()` + 按 `m_songFilter` 丢条目（`paytype != 3` 记 free；`songFilter==1` 留 free、`==2` 留非 free）。受影响：`getPlaylistSongs`、`getMusicToplist`、`getSingerSongs`、`getNewSongs`。歌单/歌手/榜单列表用 `normalizeList()`，不受筛选影响。
- 排序：**模型层没有排序能力**，各列表的排序由平台接口参数决定（酷狗 `sort=1` 热度、网易云 `order=hot`/`ranktype=2`、B 站接口自带顺序），`normalizeItem()` 只补齐字段、绝不动顺序。
- 归一化补充（`normalizeItem`）：缺失时补 `title/artist/cover/hash/duration/album/playcount/paytype(0)/hashhq(=hash)/hashsq(=hash)`，并写旧字段别名 `specialname/username/imgurl/songname/singername/intro/album_name/specialid/albumid`（兼容旧 QML 页面）。
- QML 使用方（按模型）：

| 模型 | 使用页面 |
|---|---|
| `searchSongsResults` | `pages/SearchPage.qml`、`main.qml`、`layout/LeftSideBar.qml`、`layout/PlayerControl.qml` |
| `newSongs` | `pages/PlaylistPage.qml` |
| `recommendSongs` | `layout/LeftSideBar.qml`（首页每日推荐卡已改为走 `personalRadar`） |
| `musicPlaylists` | `pages/HomePage.qml`、`pages/PlaylistPage.qml` |
| `playlistSong` | `pages/PlaylistPage.qml`、`pages/HomePage.qml`、`pages/SearchPage.qml`、`pages/FavouritePage.qml`、`components/PlayListWindow.qml` |
| `hotPlayLists` | `pages/HomePage.qml` |
| `getHotlistMenu` | `pages/HomePage.qml` |
| `toplistList` | `pages/PlaylistPage.qml`（`toplistCard.source` 回传给 `getMusicToplist` 强制指定平台） |
| `musicToplist` | 无 QML 直接引用（仅 C++ 内部保留旧模型；未确认是否有插件读取） |
| `singerList` | `pages/PlaylistPage.qml` |
| `personalFm` | 无 QML 引用（首页歌单漫游已改为分类歌单，模型保留） |
| `personalRadar` | `pages/HomePage.qml`（`dailyWindow`） |
| `comments` | `components/MusicInfo.qml` |
| 通用消费 | `components/QListView.qml` 定义 `hashOf()` / `sourceOf()`：`hashOf()` 按 `Config.soundQuality` 取 `hashsq/hashhq/hash`（注意与 `getMusicInfo` 内部的 `resolveQualityHash` 是**两套并行的音质选择**）；`sourceOf()` 在模型无 `source` 字段时推断：`file:` 前缀或本地盘符 → `-1`，否则 `MusicApi.songSource`。其余列表组件（`QGridView.qml`、`QLocalView.qml` 等）**未确认**是否复用同一套函数 |

---

## 4.7 AccountManager 的关系

| 环节 | 精确事实 |
|---|---|
| 定义 | `cpp/AccountManager.h`，`QML_ELEMENT` + `QML_SINGLETON`，QML 侧按类型名 `AccountManager` 访问（注释：必须这样，经对象属性间接访问会被 qmlcachegen 判为 shadowable base type 而拒绝编译） |
| 创建 | `main.cpp:151` `AccountManager::create(&engine, &engine)`（提前创建以保证初始化时序；内部 `static` 实例，QML 首次访问复用同一对象） |
| 注入 | `main.cpp:153` `MusicApiService::setSharedAccountManager(accountManager)` → 静态 `s_accountManager` → 单例 `create()` 里 `service->setAccountManager(s_accountManager)` |
| 消费点 | 仅 `MusicApiService::syncSource(int source)`：B 站只 `m_bilibili.setQuality(m_soundQuality)`；网易云 `m_netease.setCookie(m_account->neteaseApiCookie())`；酷狗 `m_kugou.setCookie(m_account->kugouCookie())` + `m_kugou.setDeviceInfo(m_account->kugouMid(), m_account->kugouDfid())` |
| 另一次使用 | `handleMusicInfo()` 里判断 `source == kSourceKugou && m_account && !m_account->isKugouLoggedIn()`，用于区分提示「该歌曲需要 VIP 会员，请先登录酷狗账号」/「…或购买后才能播放」/「…可登录酷狗账号后重试」 |
| 登录态流向 | **拉模式**：`syncSource()` 在**每次请求分发前**调用一次，把当时的 cookie/设备信息推给平台类。`MusicApiService` **没有** connect `AccountManager::neteaseLoginChanged` / `kugouLoginChanged` |
| 直接后果 | 登录/登出**不会**影响已发出的请求；下一次请求才生效；不需要（也不应）在别处手动 setCookie，否则会被 `syncSource()` 覆盖 |
| 关键取值 | 网易云用 `neteaseApiCookie()`（= `mergeNeteaseIdentity(m_neteaseCookie)`，**未登录也会带稳定 deviceId + os=pc 身份**，避免被判异常环境），不是 `neteaseCookie()`；酷狗用 `kugouCookie()` + `kugouMid()` + `kugouDfid()`（mid/dfid 是播放签名与评论签名的必需项，缺失时酷狗内部 `kugouMidFromGuid(randomGuid())` 临时生成） |
| 二维码 | `AccountManager.neteaseQrText` / `kugouQrText` 由 `SettingsView.qml` 的两个弹窗喂给 `QRCodeView.qrText`（§4.8） |
| 网易云扫码实现 | 基于 QCloudMusicApi 的 `login_qr_*` 接口，跑在 `m_pool`（专用 `QThreadPool`）里，`m_neteaseBusy` / `m_neteaseCancelled` 做去重与取消 |

---

## 4.8 qrcode.js

| 项 | 内容 |
|---|---|
| 文件 | `api/qrcode.js`（21559 字节 / 514 行，行尾 `0x0A`） |
| 性质 | 纯 JS、零依赖的轻量二维码生成器；算法结构参照 `kazuhikoarase/qrcode-generator`（MIT） |
| 能力 | 字节模式（byte mode）；纠错等级 `L`、`M`；版本 1-10（注释：「登录二维码足够」） |
| 导出 | 顶层 `function qrcode(text, ecl)` → `modules[row][col]` 布尔二维数组（`true` = 黑块）；内部另有 `make()` / `selectVersion()` / `QRCodeModel` 等 |
| 导出约定 | 注释明确：**顶层必须用 `function` 声明导出**（QML 的 JS import 只暴露顶层函数/变量名） |
| 加载者 | `components/QRCodeView.qml:7` `import "qrc:/QueMusic/api/qrcode.js" as QRCode`，调用 `QRCode.qrcode(root.qrText, "M")` 后用 `Canvas` 逐格 `fillRect` 绘制；`qrText` 变化即 `requestPaint()`，生成异常时 `console.warn` 并退化为纯色背景 |
| 宿主 | `SettingsView.qml:95`（网易云扫码，`qrText: AccountManager.neteaseQrText`）与 `SettingsView.qml:325`（酷狗扫码，`qrText: AccountManager.kugouQrText`） |
| 构建 | 作为 QML 模块文件参与 `qt6_add_qml_module(... QML_FILES api/qrcode.js ...)`（`CMakeLists.txt:140`），并列入 `cmake/qmltc.cmake` 的跳过清单（与 `components/QRCodeView.qml` 一起被 qmltc 跳过） |
| 风险 | 版本上限 10 决定容量上限；超长 URL 会抛异常 → `QRCodeView` 静默画白屏（不崩溃） |

---

## 4.9 QCloudMusicApi 依赖

| 项 | 值 |
|---|---|
| 位置 | `api/QCloudMusicApi/` |
| 来源 | Qt 版网易云 API，翻译自 Node.js 项目 `Binaryify/NeteaseCloudMusicApi`，仓库 `s12mmm3/QCloudMusicApi` |
| 版本 | `project(QCloudMusicApiProject VERSION 1.0)`（`api/QCloudMusicApi/CMakeLists.txt:3`）；`QCloudMusicApi` 子工程不再单独写版本 |
| 许可证 | **MIT**（`api/QCloudMusicApi/LICENSE`，README `## License` → The MIT License） |
| 语言标准 | 该子工程自己 `set(CMAKE_CXX_STANDARD 17)`（主工程是 C++20 与 `CMAKE_CXX_STANDARD 20` 无关） |
| 最低 CMake | 3.14；另 `set(CMAKE_POLICY_VERSION_MINIMUM 3.5)` 兼容新版 CMake 的旧策略 |
| 目录布局 | `QCloudMusicApi/`（源码：`apihelper.{h,cpp}`、`module.{h,cpp}`、`plugins.{h,cpp}`、`api_c.{h,cpp}`、`qcloudmusicapiglobal.h`、`qcloudmusicapiplugin.h`、`util/`）；`3rdparty/`（`cryptopp`、`cryptopp-cmake`、`libqrencode`）；`doc/`（`API.md`、`BINDINGS.md`）；`example/`（C# / Python / QCMPlugin / Rust 跨语言示例） |
| 提供的能力 | ① `ApiHelper`：cookie 管理 + `invoke(member, arg)` 反射调用 + `invokeUrl(url)` + `set_cookie` + `setFilterRules` + 插件加载（7 个 `Q_INVOKABLE`）；② `NeteaseCloudMusicApi`（`module.h`，**341 个 `Q_INVOKABLE QVariantMap xxx(QVariantMap)`**，覆盖 NeteaseCloudMusicApi 绝大多数接口）；③ C 接口（`invoke(char*, char*)`）与插件机制；④ `util/` 里的 weapi/eapi 加密与请求实现（`request.cpp`、`crypto.cpp`） |

构建与链接（根 `CMakeLists.txt:301-323`）：

```cmake
set(QCLOUDMUSICAPI_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(QCLOUDMUSICAPI_BUILD_SHARED OFF CACHE BOOL "" FORCE)   # → 静态库
add_subdirectory(api/QCloudMusicApi)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/api/QCloudMusicApi/QCloudMusicApi)   # ← 实际使用的 include 路径
target_link_libraries(${PROJECT_NAME} PRIVATE ... QCloudMusicApi cryptopp::cryptopp ...)
```

- 因此源码里是 `#include "apihelper.h"`（`NeteaseCloudApi.cpp:5`），不是 `#include "QCloudMusicApi/apihelper.h"`（README 的示例写法与本地 include 路径不同）。
- 子依赖 crypto++：git 子模块 `api/QCloudMusicApi/3rdparty/cryptopp`（`.gitmodules`，`weidai11/cryptopp`，`shallow = true`）。`api/QCloudMusicApi/CMakeLists.txt` 若发现该目录存在，就 `set(CRYPTOPP_SOURCES ... CACHE PATH ... FORCE)` 强制走本地源码**跳过联网下载**（否则 `message(WARNING ...)` 并可能联网拉取）；随后 `add_subdirectory(3rdparty/cryptopp-cmake/)` → 目标 `cryptopp` / 别名 `cryptopp::cryptopp`。
- 子依赖 libqrencode：`3rdparty/libqrencode/` 已随仓库存在，但**当前 CMake 链路里没有被 `add_subdirectory`**（`api/QCloudMusicApi/CMakeLists.txt` 只加 `cryptopp-cmake` 与 `QCloudMusicApi`），且 `QCloudMusicApi/*.cpp`、`*.h` 中未出现任何 `qrencode` 引用 → 属未编译/未使用的 vendored 目录。
- 工程内其他文件的 `Q_INVOKABLE` 计数（grep 时容易误判）：`api/QCloudMusicApi/**/*.h` 里全是第三方接口，本项目的在线层 `Q_INVOKABLE` 只分布在 `MusicApiService.h`（28 个）与 `OnlineListModel.h`（4 个）。
- `AccountManager` 也直接链接 `ApiHelper`（`m_api`，用于 `login_qr_*`），因此该库有两个使用方。

---

## 4.10 坑与约定

### 4.10.1 请求计数 / 加载态

- `loadState` **派生自未完成请求计数**：`beginRequest()` ++、`endRequest()` --，归零才置 false 并 `stop()` 看门狗（否则并发请求里先回来的会把动画提前关掉）。
- 看门狗 `m_requestWatchdog` 20 000 ms（`kRequestWatchdogMs`），超时把 `m_pendingRequests` 归零并强制结束加载态。**它只由 `beginRequest()` 启动**，所以 `getCategoryPlaylists` 这条不计数路径没有看门狗保护（平台若不回结果，只能等玩家点别的东西触发 `endRequest`）。
- `endRequest()` 在计数为 0 时也不会减成负数（`if (m_pendingRequests > 0)`），并且仍会 `setLoadState(false)` —— 这是上面那条不计数路径「能正常工作」的原因。
- **任何新请求路径必须保证「无论如何都 emit 一次 resultReady」**，否则加载动画不会停（酷狗注释里专门强调：无权限/风控时也必须回传结果）。`ApiHttp` 失败时也回调（`body` 为空），但空 body 解析出的空 `QJsonObject` 仍需各平台走到 `emit`。
- `getLyricInfo` 是唯一不 `endRequest()` 的 action（它在 `getMusicInfo` 的链路内触发，见 `handleResult` 首行注释）。
- 未实现的平台（`DISPATCH` 的 `default`）**不销账**，只警告 → 靠 20s 看门狗兜底；新增平台时这是第一个要补的地方。

### 4.10.2 过期响应

- 播放代次：`getMusicInfo(hash, type=0, source)` 每次 `++m_playGeneration` 并记 `m_playHash` / `m_playSource`；`isStalePlayResponse(playHash, source)` 判 `playHash != m_playHash || source != m_playSource` 即丢弃（`playHash` 或 `m_playHash` 为空时**不**判过期，以免吞掉真实错误）。过期响应若继续处理会把音源切回上一首并按它的 hash 改队列索引。
- 歌词代次：`QHash<QString,int> m_playLyricGenerations`（hash → 代次），响应回来时若 `generation != m_playGeneration` 丢弃；表超过 32 条时清理非当前代次项。
- 本地歌词：`m_localLyricsGeneration` 计数，连续切歌丢弃过期解析结果。

### 4.10.3 音质 / hash

- 酷狗同一首歌的音质是**不同 hash**：列表解析必须提供 `hashhq/hashsq`，否则 `resolveQualityHash()` 没有可升级目标（会原样请求 base）。`ApiCommon::song()` 的空值回退会让「没有高档音质」与「高档 hash 等于 base」不可区分 —— 这是有意的（Kugou 侧用 `pay_type_320/pay_type_sq` 提前判付费并回退 base）。
- 队列/收藏/播放列表用**普通 hash 当身份**（`m_baseOf`），`urlplay` 回传的也是 `identityHash`；改回「实际请求的 hash」会让同一首歌因音质不同在队列里出现多条。
- 删除 `QueMusicQuality.json` 只损失一次音质升级（下一轮列表解析会重建），但**格式（`hq`/`sq` 两个对象）不能被别的用途复用**。
- `Config.soundQuality` 与 `MusicApi.soundQuality` 是两个值：QML `components/QListView.qml::hashOf()` 用 `Config.soundQuality` 预选 hash，`MusicApiService::resolveQualityHash()` 用属性再选一次。两者语义必须一致（0/1/2+），否则会出现「点了 320k 却请求 128k hash」。

### 4.10.4 错误处理约定

- 失败统一用 `data["error"]` 标记（存在即失败）。**三个消费点必须区分「失败」与「空结果」**：`getPersonalFm`/`getPersonalRadar`（空表非失败才提示未登录）、`getLyricInfo`（失败提示重试并保留当前歌词，不能显示成「纯音乐，请欣赏」）、本地搜词（`localLyricsFailed` 只在非失败时发）。
- 无播放地址的提示分级（`handleMusicInfo`）：网络失败 → 「网络请求失败，请检查网络后重试」；`errReason == "vip"` → 未登录酷狗账号时「该歌曲需要 VIP 会员，请先登录酷狗账号」否则「该歌曲需要 VIP 会员或购买后才能播放」；酷狗未登录 → 「该歌曲暂无可用播放地址，可登录酷狗账号后重试」；其余 → 「该歌曲受版权或会员限制，暂时无法获取播放地址」。`warned(msg, 2)`。
- `warned` 的 type 语义：`0` 普通 / `1` 成功 / `2` 警告 / `3` 错误。
- `ApiHttp::get` 的失败日志只打 `scheme://host/path`（`clean.setQuery(QString())`），因为查询串里可能带 token。

### 4.10.5 Cookie / 会话 / 凭证

- `ApiHttp::get` **只在 `https` 时携带 Cookie**（明文 `http://` 端点不传凭证）。酷狗仍有多个 `http://mobilecdnbj.*` 端点，所以「加了 cookie 却依然未登录」在这些接口上是预期行为。
- `ApiHttp::get` 统一 `Accept-Encoding: identity` + `setTransferTimeout(15000)`；改动这两项会影响所有平台的解析与超时行为。
- 网易云：`ApiHelper::set_cookie()` 必须经 `setCookieRequest` 信号投递到 worker 线程，**不能**在 GUI 线程直接调 worker 的 `helper()`。
- 酷狗：播放签名依赖 `cookieValue("token")` 与 `cookieValue("userid")`，`mid`/`dfid` 来自 AccountManager；`mid` 为空时内部临时 `kugouMidFromGuid(randomGuid())`（不持久化 → 下次请求又是新身份）。
- B 站：`buvid3/buvid4` 是会话内缓存（不落盘，重启后重新获取）；搜索/评论等签名接口**必须**先 `ensureKeys()` 成功，`m_mixinKey` 为空时 `getSigned` 直接回空 `QJsonObject`。

### 4.10.6 限流 / 重试 / 频率

- **没有统一重试机制**：失败即失败（唯一例外是酷狗「免费通道无地址且有 token → 转 `requestSignedPlayInfo` 重试一次」）。
- 评论分页上限：`MusicApiService::getComments` 归一 `pageSize` 为 `qBound(1, pageSize, 50)`、`page` 为 `qMax(1, page)`；B 站评论 `ps` 再夹到 30；网易云 `offset` `qMax(0, ...)`。
- 酷狗热门歌单刻意用 8 个关键词 × 每词 20 页「轮换」来规避单关键词翻页上限（注释：实测各词均 ≥22 页，不会翻空，共 160 页持续出新）。改关键词表或 `kPagesPerKeyword` 会直接改变翻页映射（`page` ↔ 关键词/页号是算式算出，不是游标）。
- 网易云 worker 的 `ApiHelper` debug 日志被关闭（`setFilterRules("QCloudMusicApi.debug=false")`），别打开：每个请求三行会刷满控制台。
- 网易云的 `personal_fm` / `recommend_songs` 无分页能力，`page > 1` 直接回空表以避免重复追加。
- `m_commentCursors` / `m_aidCache` / `m_mixIdCache` / `m_upNames` / `m_coverHintCache` 都是**无上限内存缓存**（未做淘汰，未确认是否需要）。

### 4.10.7 容易改坏的地方（清单）

1. `MusicApiService` 构造函数**不能**加 `parent` 默认值 → QML 单例会走默认构造、账号注入失效。
2. `paytype` 必须归一到 `0/3`；平台自家枚举（网易云 `fee`、酷狗 `pay_type`）直传会让 VIP 角标与筛选静默失效。
3. `DISPATCH(source, expr)` 内层变量名 `quemusicSource` 不要改回短名（曾因变量遮蔽误传未初始化值）。
4. `handleResult` 的 `endRequest()` 必须在函数开头（中间有 `return`；放出口会漏减）。
5. `appends` 与 `clear` 的责任分工：模型默认只追加，**首页清空由 QML 负责**（如 `SearchPage` 切换 tab 时 `clear()`）；`getHotlistMenu`/`musicToplist`/`toplistList` 例外，在 C++ 内 clear。改错会出现「切 tab 后列表叠加」。
6. `OnlineListModel::append` 里 `collectRoles()` 必须在 `beginInsertRows()` 之前。
7. `handleMusicInfo` 里 `album_img` 的 `{size}` 占位符被替换为 `512`（`solve` 用 `128`）；`timeLength` 做了「秒 → 毫秒」兼容（`timeLength < 1000 ? *1000 : 1`），改动会直接破坏歌词滚动与下载时长。
8. 本地歌词搜词复用 `searchSongs`，等待者用 `m_localLyricsSearches`（**按 source 单槽**，同一平台同时只能有一个待匹配本地文件）与 `m_biliFallbackSearches`（按 source 单槽）；`m_biliTrack` 也是**单槽**（只保存最后一次 B 站稿件）。并发场景下这是已知的窄点。
9. 下载流程：`getMusicInfo(type=1)` 只暂存 `m_pendingDownloads[hash]`，真正 `addDownload()` 在歌词响应回来后触发 —— 平台若不回 `getLyricInfo` 结果，下载永不开始。
10. `getSingerCategory` 的 `area` **由 QML 按平台映射**（酷狗 `1/2/3/4`，网易云 `7/96/8/16`），C++ 侧不做映射；详情见 `pages/PlaylistPage.qml:481-485`。
11. QML 侧 `QListView.qml::sourceOf()` 用 `file:` 前缀 / 本地盘符判定 `source == -1`（本地文件）；在线模型无 `source` 字段时回退 `MusicApi.songSource`。只有 `toplistList` 的条目带 `source`（`getAllToplist` 注入）。
---

← 返回 [索引 README.md](README.md)
