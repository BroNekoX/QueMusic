# QueMusic 在线音源框架（api/）

在线音源层只做一件事：**把各平台差异收进 `MusicSource`，向 QML 暴露一套稳定 API。**
新增平台 / 修接口时，不需要再动调度层、模型层和 QML。

## 分层

```
QML (MusicApi.*)
   │
   ▼
MusicApiService              薄门面：QML 属性/信号、加载态、歌词跟随、账号上下文
   │  request(action, args)
   ▼
MusicSourceRegistry           source id -> MusicSource*
   │
   ├── platform/KugouApi / NeteaseCloudApi / BilibiliApi / QQMusicApi
   │   / SpotifyApi / AppleMusicApi               （内置平台）
   └── ScriptSourceHost                           （JS 音源宿主，独立工作线程）
          │  emit resultReady(action, data, source)
          ▼
ApiResultRouter              action -> OnlineListModel / 歌词 / 播放链路
   │
   ├── OnlineListModel        列式存储的轻量列表模型（QML delegate 热路径）
   └── PlaybackCoordinator    播放地址、歌词代次、本地搜词、下载元数据
          │
          ├── QualityHashCache   标准/320k/无损 hash 映射
          └── LocalMedia         TagLib 本地标签读写（与在线层解耦）
```

## 文件职责

| 文件 | 职责 |
|---|---|
| `ApiActions.h` | action / 参数键常量、内置 source 编号、列表 action 判定 |
| `MusicSource.{h,cpp}` | 音源基类、能力钩子、`request()` 分发、注册表 |
| `MusicApiService.{h,cpp}` | QML 单例 `MusicApi`；只做门面与宿主状态 |
| `ApiResultRouter.{h,cpp}` | action -> 模型/信号的唯一映射表 |
| `PlaybackCoordinator.{h,cpp}` | 播放/歌词/下载/本地搜词链路 |
| `QualityHashCache.{h,cpp}` | 音质 hash 缓存（含反向表） |
| `LocalMedia.{h,cpp}` | 本地文件 TagLib 读取/写入、封面提示 |
| `SelfCheck.{h,cpp}` | 启动/离屏自检：注册表、模型、路由、归一化、歌词时间轴 |
| `LyricTimeline.h` | 歌词行下标二分查找（自检可独立验证） |
| `ApiCommon.h` | 统一字段、`song()/listResult()/comment()`、结果归一化 |
| `ApiHttp.h` | GET / JSON POST helper（UA/Referer/Cookie/超时） |
| `OnlineListModel.{h,cpp}` | 列式 `QAbstractListModel`，提供 `get/clear/append/remove` |
| `platform/KugouApi.*` / `NeteaseCloudApi.*` / `BilibiliApi.*` / `QQMusicApi.*` / `SpotifyApi.*` / `AppleMusicApi.*` | 平台实现 |
| `sources/ScriptSourceHost.*` | JS 音源宿主（QJSEngine + 独立线程） |

## Action 协议

宿主只通过一个入口发请求：

```cpp
provider->request(sourceId, action, args);
// 音源异步：
emit resultReady(action, data, sourceId);
```

基类 `MusicSource::request()` 会把 action 分发到对应的 `onXxx()` 虚函数；
平台只需要 override 自己支持的钩子，没有实现的 action 会自动返回
`{ "error": -1, "message": "unsupported action" }`。

| action | args | data |
|---|---|---|
| `searchSongs` | keyword, type, page, pageSize | `{info:[song]}` |
| `searchLyrics` | keyword, page, pageSize | `{info:[song]}`（只用于本地/回退搜词） |
| `getPlaylistMenu` | type | `{info:[{title,category,id}]}` |
| `getMusicPlaylists` / `getCategoryPlaylists` | tagid 或 id, page, pageSize | `{info:[...]}` |
| `getPlaylistSongs` | listid, page, pageSize | `{info:[song]}` |
| `getRecommendSongs` / `getNewSongs` | page, pageSize（+ type） | `{info:[song]}` |
| `getHotPlaylistMenu` / `getHotPlaylists` | type / page, pageSize | `{info:[...]}` |
| `getAllToplist` | — | `{info:[...]}` |
| `getMusicToplist` | page, pageSize, rankid | `{info:[song]}` |
| `getHotSingers` / `getSingerCategory` / `getSingerSongs` | area / singerid, page, pageSize | `{info:[...]}` |
| `getMusicInfo` | hash, type(0 播放 / 1 下载 / 2 预取) | 扁平 map（url/backup_url/songName/...） |
| `getLyricInfo` | hash, duration | `{info:[line], translate:[...], hash}` |
| `getComments` | hash, page, pageSize | `{info:[comment], hasMore}` |
| `getPersonalFm` / `getPersonalRadar` | page, pageSize | `{info:[song]}` |

统一字段见 `ApiCommon.h`；结果会在 `ApiCommon::normalizeItem()` 里补齐
`title/artist/cover/hash/hashhq/hashsq/duration/album/paytype/playcount`。

## 新增一个平台

1. 在 `ApiSource` 分配固定 source id：酷狗 0、网易云 1、B站 2、QQ 3、
   Spotify 4、Apple Music 5。脚本音源从 100 开始，不能占用内置编号。
2. 新建 `api/platform/XxxApi : public MusicSource`，override `sourceName()`、
   `applyCredentials()` 和需要的 `onXxx()`。
3. 在 `MusicApiService` 构造函数里创建、映射进 `m_registry`、连接
   `resultReady` 到 `handleResult`。
4. 把 source 加进 `rebuildSources()` 的内置列表；QML 下拉会自动多一项。

**不需要改**：`ApiResultRouter`、`OnlineListModel`、QML 页面。
如果新增的是全新 action，才需要：
- `ApiActions.h` 加常量 + `isListAction()`；
- `MusicSource` 加 `onXxx` 钩子 + `request()` 分发；
- `ApiResultRouter::route()` 加目标模型；
- `MusicApiService` 加一个转发方法（QML 想直接调用时）。

## JS 音源插件

目录：`<配置>/BroNekoX/QueMusic/sources/*.js`（设置页可重载）。

```js
var meta = {
    id: "example",              // 稳定 source id 的依据，改了会变成新音源
    name: "示例音源",
    version: 1,
    author: "you",
    capabilities: ["search", "url", "lyrics"],
};

function search(q) { /* q = {keyword,type,page,pageSize} -> [song] */ }
function searchLyrics(q) { return search(q); }        // 可选
function musicUrl(t) { /* t = {hash,id,type} -> url 字符串或 {url,...} */ }
function lyrics(t)     { /* -> {info:[{time,text,info?}], translate:[]} */ }

// 其它能力如存在同名函数即可被调用，例如：
// function getPlaylistSongs(q) { ... }
// function getHotSingers(q) { ... }
```

所有插件结果与内置音源共用归一化、结果路由、播放/歌词链路；插件缺少
某个函数时只会让该 action 返回错误，不影响其它能力。插件 source id 由
`meta.id` 的 SHA-1 稳定生成，重启/改名排序不会改变已有配置指向。

## 内置平台

| id | 平台 | 实现 | 说明 |
|---|---|---|---|
| 0 | 酷狗音乐 | `platform/KugouApi` | KRC 逐字歌词、多级取址 |
| 1 | 网易云音乐 | `platform/NeteaseCloudApi` | QCloudMusicApi worker |
| 2 | 哔哩哔哩 | `platform/BilibiliApi` | WBI 签名、视频音频流 |
| 3 | QQ音乐 | `platform/QQMusicApi` | 免登录搜索/歌单/榜单/歌词/评论；VIP 播放仍受账号限制 |
| 4 | Spotify | `platform/SpotifyApi` | 登录后 Web API：搜索/歌单/歌手/榜单；30 秒试听；无歌词/评论 |
| 5 | Apple Music | `platform/AppleMusicApi` | 登录后 iTunes/Apple Music 数据：搜索/专辑/歌手/榜单；30 秒试听；无歌词/评论 |

QQ 音乐的接口分散在 `u.y.qq.com`、`c.y.qq.com`、`i.y.qq.com`，
统一在 `QQMusicApi` 内按 action 解析；没有把平台字段泄漏到路由层。
Spotify / Apple Music 必须先登录才能切换；设置页打开浏览器登录页，
回填浏览器 Cookie（Spotify 含 `sp_dc`，Apple Music 含 `media-user-token`）后校验账号。
登录态保存在本机 `Account.ini`，退出账号会清空并把当前音源切回酷狗。
两者播放仍是 30 秒试听，完整播放需要平台自己的 OAuth / DRM。

## 性能相关

- `OnlineListModel` 行数据按列连续存储，`data()` 是 O(1) 下标访问，不再做
  `role -> key -> QVariantMap` 的两次哈希查找。
- 账号 cookie / 音质只在登录态或音质变化时同步给音源，不再每个请求拼接。
- 源列表 / 源名 / 源 id 在音源变化时缓存，避免 QML 绑定反复构造。
- 酷狗 KRC 解码与正则解析放到线程池，避免长歌词卡 UI。
- QQ 搜索结果会缓存歌曲详情（media_mid / 时长 / 付费标记），播放时少一次详情请求；
  歌单歌曲按 playlist id 缓存整表，翻页不再重复拉取；分类菜单首次拉取后在音源内复用。
- 歌单分类菜单与其它列表共用 `OnlineListModel`，QML 统一用 `count/get()`，
  不再把 `QVariantList` 当数组或调用不存在的 `.get()`；菜单加载后自动拉取首个分类。
- 字符串常量、正则、统一字段归一化集中在一处，减少各平台的重复解析。

## QML 侧

原有方法/属性/信号全部保留。新增两个便捷入口，减少页面里重复的音质判断：

```qml
MusicApi.playSong(model.get(index))          // 自动按 soundQuality 取 hash
MusicApi.downloadSong(item, item.source)     // 下载到下载页
MusicApi.sourceNameOf(item.source)           // 插件音源也能显示正确名称
```

## 自检

- 正常启动：`main.qml` 的 `Window.Component.onCompleted` 调用 `MusicApi.selfCheck()`；
  `Config.debug` 打开时记录耗时，失败时 `console.warn` 输出具体 issue。
- 无 GUI：`bin/QueMusic --self-check` 使用 `QCoreApplication`，不加载 QML / 不创建窗口，
  检查注册表、源列表、归一化、模型、路由、音源分发和歌词时间轴；失败退出码为 1。
- 脚本：`tests/api-selfcheck.sh <可执行文件>`（CI 已接入）。
