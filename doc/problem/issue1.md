## 概述

当前版本存在两处彼此独立的宿主侧判定缺陷，都导致正常播放流程被错误分支接管：

- **问题一**：登录平台账号后，VIP / 付费歌曲只播放 30 秒试听，且音源插件链一次都不触发。
- **问题二**：QQ 音乐歌曲（`source = 3`）在队列 / 历史中无法播放，被当作 WebDAV 文件处理。

两者都发生在宿主侧的分派与判定逻辑中，与音源插件本身无关。

---

## 问题一：VIP 歌曲登录后退化为 30 秒试听，插件链不触发

### 背景

我为该播放器编写了一个第三方脚本音源插件，用途是：当内置平台（网易云 / 酷狗等）因 VIP 或版权限制拿不到完整音频时，由插件链切换到其他音源，播放同一首歌的完整版本。

该插件完全依赖宿主的既有约定工作——**宿主自己拿不到可用地址时，才把播放交给音源插件链**。因此它能否生效，取决于宿主能否正确区分「拿到了完整地址」和「只拿到了试听片段」。

### 现象

1. 未登录时，同一首付费歌可正常换源并播放完整音频（插件链被正常触发）；
2. 登录账号后，宿主返回 30 秒试听地址并**直接播放该片段**，插件链一次都不触发；
3. 插件侧没有任何日志输出，插件日志窗口一条都没有——说明请求根本没有到达插件，而不是插件内部失败；
4. 结果是该插件的核心用途（替换 VIP 歌曲）在登录态下完全失效。

### 根因

`api/ApiCommon.h` 的 `isTrial()` 只认 `freeTrialInfo / trial / is_trial / try_begin / size_try` 这 5 个字段：

```cpp
// 平台返回的"试听片段"：有地址但只有 30 秒，视同无地址走音源链
inline bool isTrial(const QVariantMap &map)
{
    if (map.contains("freeTrialInfo") && !map.value("freeTrialInfo").isNull())
        return true;
    for (const char *key : {"trial", "is_trial", "try_begin"}) {
        const QVariant value = map.value(QLatin1String(key));
        if (value.isValid() && value.toLongLong() > 0)
            return true;
    }
    return map.value("size_try").toLongLong() > 0;
}
```

但**没有任何平台源文件写入这 5 个字段**：全仓库搜索 `trial` 仅命中 `ApiCommon.h`、`PlaybackCoordinator.cpp`、`PlaybackCoordinator.h`。因此对内置平台 `isTrial()` 恒为 `false`。

而 `api/PlaybackCoordinator.cpp` 的 `handleMusicInfo()` 只在「地址为空」或「判定为试听」时启动插件链：

```cpp
const bool trial = type == 0 && ApiCommon::isTrial(data);
if (type == 0 && ApiSource::isBuiltin(source) && !m_chainActive
    && (playUrl.isEmpty() || trial)) {
    startChain(data, playHash, source);
```

| 状态 | 宿主返回的 url | `playUrl.isEmpty()` | `isTrial()` | 插件链 |
|---|---|---|---|---|
| 未登录 | 空 | `true` | — | 启动，正常换源 |
| 登录后 | 30 秒试听地址 | `false` | `false`（死代码） | 跳过，直接播试听 |

即注释中的设计意图（试听视同无地址）因缺少标记从未生效。

### 建议

平台在「付费歌却拿到地址」时补一个 `isTrial()` 认的标记。网易示例（`NeteaseCloudApi.cpp` 的 `CloudWorker::mapResult()`，`getMusicInfo` 分支，构造 `detail` 之后）：

```cpp
if (!playUrl.isEmpty() && paytypeFromFee(feeOf(song)) == ApiCommon::kPaytypePaid)
    detail.insert("freeTrialInfo", QVariantMap{{"length", 30000}});
```

酷狗同理，在 `KugouApi.cpp` 的 `finishWithFallback()` / `requestSongInfo()` 里补同一标记。两处 `paytypeFromFee()` / `paytypeFromKugou()` 已算出付费状态，只是未用于该标记，改动量很小。

补充一点：这类第三方换源插件是本播放器音源机制的设计特性之一，问题一实际上使该特性对已登录用户完全失效，影响面不止单首歌曲。

---

## 问题二：QQ 音乐歌曲被当作 WebDAV，队列 / 历史无法播放

### 现象

1. QQ 音乐歌曲首次播放正常，进入队列或从历史重播即「无法播放」；
2. 队列中这些歌曲的来源被显示为「本地」而非「在线」；
3. 连续多首后提示「连续多首无法播放，已停止自动跳过」。

### 根因

`cpp/playback/Playback.h` 的 WebDAV 编号与 QQ 音乐音源编号撞车：

```cpp
int kSourceWebDav() const { return 3; }        // cpp/playback/Playback.h
inline constexpr int QQMusic = 3;              // api/ApiActions.h
```

`Playback::playIndex()` 因此把所有 `source == 3` 的歌曲分流到 WebDAV：

```cpp
if (track.source < 0)                     openLocal(track, track.path);
else if (track.source == kSourceWebDav()) openWebDav(track);   // QQ 歌曲命中此处
else                                      openOnline(track);
```

`openWebDav()` 把 `track.path` 当作远程 URL：

```cpp
const QString cached = WebDavCache::cachedAudioFor(track.path);  // songmid 无缓存 → 空
m_engine.playSource(QUrl(track.path), headers, ...);             // songmid 当 URL 播放 → 失败
```

QQ 音乐的 `track.path` 是 songmid（如 `0039MnYb0qxYhV`），不是 WebDAV URL，故必然失败，且不会发出 `getMusicInfo` 请求。同一根因还导致三处连带问题：

- `sourceText()` 把 3 显示为「本地」；
- `prepareNext()` 中 `track.source != kSourceWebDav()` 使 QQ 歌曲永不预取；
- `adoptAuto()` 走 WebDAV 缓存分支，QQ 歌曲拿不到 `getLyricInfo`。

### 建议

WebDAV 不是在线音源，不应占用 `ApiSource` 的 0–5 编号空间。最小改动是给 WebDAV 分配不冲突的编号（如 `6`），并同步替换 `Playback.cpp` 中 `playIndex` / `prepareNext` / `adoptAuto` / `sourceText` 的四处比较（已存 `source == 3` 的 WebDAV 队列条目需迁移）；更彻底的做法是改用 `enum class TrackKind { Local, Online, WebDav }`，避免用同一个 int 表达三种语义。

---

## 环境

- QueMusic 最新发布版，Windows x64
- 源码核对：`main` @ `3707a73`
