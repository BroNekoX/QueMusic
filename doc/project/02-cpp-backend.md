# 第 2 部分 · C++ 后端层（cpp/，不含 cpp/audio/）

> 本文件是 [QueMusic 项目全景文档索引](README.md) 的一个分片；任务定位与其它部分请见 [README.md](README.md)。

范围：`cpp/*.{h,cpp}`（顶层）、`cpp/webdav/*.{h,cpp}`、`cpp/plugins/*.{h,cpp}`。
`cpp/audio/` 由另一份文档覆盖，本文只在必要处提及被引用点。
所有标识符按源码原样书写；不确定处标「未确认」。

---

## 2.1 总览表

> `.h`/`.cpp` 成对组件合并为一行，两个文件名都写全（表内覆盖范围内每个文件）。
> 「QML 暴露名」= QML 侧可用的类型名；`QML_ELEMENT` 即可创建类型（需 QML 侧 `TypeName {}` 实例化），
> `QML_SINGLETON` 为单例，`QML_NAMED_ELEMENT` 为重命名，`QML_ANONYMOUS` 为匿名基类（QML 不能按名实例化）。

| 文件 | 类名 | QML 暴露名 | 一句话职责 |
|---|---|---|---|
| `cpp/AccountManager.h` `cpp/AccountManager.cpp` | `AccountManager` | `AccountManager`（`QML_ELEMENT` + `QML_SINGLETON`） | 网易云/酷狗登录态：扫码登录、Cookie 持久化、设备身份、备用登录入口。 |
| `cpp/AppModels.h` | `MyFolders` `LocalFolders` `Songs` `FavoriteSongs` `FavoritePlaylists` `FavoriteArtists` | 同名（均 `QML_ELEMENT` + `QML_SINGLETON`） | 用宏 `QUEMUSIC_DECLARE_MODEL_SINGLETON` 派生的 QML 模型单例集合。 |
| `cpp/ColorExtractor.h` `cpp/ColorExtractor.cpp` | `ColorExtractor` | `ColorExtractor`（`QML_ELEMENT`） | 封面主色提取，工作线程执行，带 QCache 与代次丢弃。 |
| `cpp/CoverHelper.h` `cpp/CoverHelper.cpp` | `CoverHelper` | `CoverHelper`（`QML_ELEMENT`） | 本地/内嵌封面查找与缓存；顺带提供 TagLib 元数据读取静态接口。 |
| `cpp/CustomFlow.h` `cpp/CustomFlow.cpp` | `CustomFlow` | `CustomFlow`（`QML_ELEMENT`） | 自实现的 QML 流式布局容器（分行 + 左/右/居中）。 |
| `cpp/DbService.h` `cpp/DbService.cpp` | `DbService`、内部 `DbWorker` | 未暴露（非 QObject，纯 C++ 单例） | 串行 SQLite 工作线程 + 建表/迁移/数据目录探测。 |
| `cpp/DownloadedMusicModel.h` `cpp/DownloadedMusicModel.cpp` | `DownloadedMusicModel` | `DownloadedMusicModel`（`QML_ELEMENT`） | 已下载列表：扫下载目录 + 读同名 `.json` 元数据（工作线程）。 |
| `cpp/DownloadManager.h` `cpp/DownloadManager.cpp` | `DownloadManager`（内含 `DownloadTask`） | `DownloadManager`（`QML_ELEMENT`） | 串行下载队列模型 + 下载完成后写同名 `.json` 元数据。 |
| `cpp/Favorites.h` `cpp/Favorites.cpp` | `FavoritesModel`（内含 `FavoriteItem`） | `FavoritesModel`（`QML_ANONYMOUS`），实际由 AppModels 的三个单例暴露 | 收藏列表模型 + 内存版 `isFavorite` + 全局收藏版本号。 |
| `cpp/FolderModel.h` `cpp/FolderModel.cpp` | `FolderModel`、`SongModel`（内含 `FolderItem`/`SongItem`） | 均 `QML_ANONYMOUS`；`FolderModel` 由 `MyFolders`/`LocalFolders` 暴露，`SongModel` 由 `Songs` 暴露 | 歌单/歌曲列表模型，SQL 走 DbService，TAG 结果回写 songs 表。 |
| `cpp/GetWave.h` `cpp/GetWave.cpp` | `GetWave` | `GetWave`（`QML_ELEMENT`） | 频谱/波形：实现 `AudioSpectrumSink`/`AudioSpectrumSource`，FFT + 无锁环。 |
| `cpp/LocalLyricsReader.h` `cpp/LocalLyricsReader.cpp` | `LocalLyricsReader`（全静态） | 未暴露（非 QObject；由 `api/MusicApiService.cpp` 调用） | 解析 sidecar `.lrc`/`.txt` 与音频内嵌歌词（USLT/LYRICS/SYLT）。 |
| `cpp/LocalMusicScanner.h` `cpp/LocalMusicScanner.cpp` | `LocalMusicScanner`、`LocalScanWorker`（内含 `LocalFileEntry`） | `LocalMusicScanner`（`QML_ELEMENT`） | 本地目录列表：独立线程扫描 + TAG 富集分批回传 + 资源管理器式搜索 + 批量移入回收站。 |
| `cpp/LogManager.h` `cpp/LogManager.cpp` | `LogManager` | `LogManager`（`QML_ELEMENT` + `QML_SINGLETON`） | 应用级日志：接管 Qt 全局消息、按日写文件、QML 预览与等级开关。 |
| `cpp/MediaTypes.h` | 命名空间 `MediaTypes` | 未暴露 | 可播放后缀白名单的**唯一来源**（本地扫描/WebDAV/已下载共用）。 |
| `cpp/PlayerDatabase.h` `cpp/PlayerDatabase.cpp` | 自由函数 `playerDatabase()` | 未暴露 | GUI 线程共享的 `QSqlDatabase` 连接（懒建、自动建表）。 |
| `cpp/QueueModel.h` `cpp/QueueModel.cpp` | `QueueModel` | `QueueModel`（`QML_ELEMENT`） | 播放队列模型：增删移、批量追加去重、path→行号索引。 |
| `cpp/SearchResultModel.h` `cpp/SearchResultModel.cpp` | `SearchResultModel`（内含 `Row`） | `SearchResultModel`（`QML_ANONYMOUS`），经 `searchResults` 属性暴露 | 通用搜索结果模型：角色集合由构造时传入，分批追加。 |
| `cpp/SystemTrayManager.h` `cpp/SystemTrayManager.cpp` | `SystemTrayManager` | `SystemTrayManager`（`QML_ELEMENT`） | 系统托盘图标 + 原生菜单，交互以信号回发 QML；依赖 QtWidgets。 |
| `cpp/WindowsSmtcManager.h` `cpp/WindowsSmtcManager.cpp` | `WindowsSmtcManager`（私有 `Private`） | `WindowsSmtcManager`（`QML_ELEMENT`） | Windows SMTC 集成：媒体信息/时间线/系统按钮回发；非 Windows 为空实现。 |
| `cpp/webdav/WebDavStore.h` `cpp/webdav/WebDavStore.cpp` | `WebDavStore` | `WebDav`（`QML_NAMED_ELEMENT(WebDav)` + `QML_SINGLETON`） | WebDAV 服务器配置与凭据（DPAPI 加密落 QSettings）+ 鉴权头生成。 |
| `cpp/webdav/WebDavClient.h` `cpp/webdav/WebDavClient.cpp` | `WebDavClient`（内含 `Entry`） | 未暴露（非 QML 类型） | 极简 WebDAV 客户端：仅 PROPFIND Depth:1 列目录 + MultiStatus 解析。 |
| `cpp/webdav/WebDavModel.h` `cpp/webdav/WebDavModel.cpp` | `WebDavModel` | `WebDavModel`（`QML_ELEMENT`） | 远端目录列表模型，角色与 `LocalMusicScanner` 对齐并加 `isDir`。 |
| `cpp/webdav/WebDavCache.h` `cpp/webdav/WebDavCache.cpp` | `WebDavCache`（内含 `Task`） | `WebDavCache`（`QML_ELEMENT` + `QML_SINGLETON`） | 远端「音频 + 同名歌词 + 同目录封面」落地缓存，带 1GB 上限与清理。 |
| `cpp/plugins/PluginStore.h` `cpp/plugins/PluginStore.cpp` | `PluginStore` | `PluginStore`（`QML_ANONYMOUS`，两个插件单例的基类） | 插件目录通用逻辑：扫描/安装/删除/打开目录。 |
| `cpp/plugins/LyricsPluginStore.h` `cpp/plugins/LyricsPluginStore.cpp` | `LyricsPluginStore` | `LyricsPlugins`（`QML_NAMED_ELEMENT` + `QML_SINGLETON`） | 歌词界面插件：单选（同时只用一个），暴露 `source` 给 Loader。 |
| `cpp/plugins/FunctionPluginStore.h` `cpp/plugins/FunctionPluginStore.cpp` | `FunctionPluginStore` | `FunctionPlugins`（`QML_NAMED_ELEMENT` + `QML_SINGLETON`） | 功能插件：可多选启用，暴露 `enabledIds`。 |

补充：QML 侧**没有**用 `setContextProperty` 注入任何后端对象；`main.cpp:155` 只注入了一个字符串 `configDir`。
（`AppModels.h` 顶部注释明确说明这是刻意选择，理由见第 8 节。）

---

## 2.2 数据库

### 2.2.1 文件与目录

| 项 | 值 |
|---|---|
| 数据库文件 | `<DbService::dataDir()>/player_data.db` |
| `dataDir()` | 优先 `QStandardPaths::AppDataLocation`；该目录**真切一条连接建表试写**失败时退到 `QCoreApplication::applicationDirPath() + "/data"`；两者都失败回退 `preferred` |
| 迁移 | 换目录时若旧库存在、新库不存在，且旧库 `SELECT count(*) FROM favorites` > 0，则整库 `QFile::copy` 搬迁；旧库保留不删并打日志 |
| 缓存目录 | `DbService::cacheDir()` = `dataDir() + "/cache"`（自动 `mkpath`）。注意 `LocalMusicScanner::startScan()` 另算了一次 `AppDataLocation + "/cache"`（见第 8 节） |
| 连接名 | DB 线程：`quemusic_db_worker`；GUI 线程：`shared_player_db`（`playerDatabase()`）；临时：`writable_probe`、`migrate_probe` |

### 2.2.2 表与属主

建表与增量迁移全部在 `DbService::ensureSchema(QSqlDatabase&)`，由 DB 线程 `DbWorker::open()` 调用，
`playerDatabase()` 首次打开时也会调用一次。

| 表 | 列 | 属主（唯一写入方） | 备注 |
|---|---|---|---|
| `folders` | `id INTEGER PRIMARY KEY AUTOINCREMENT`、`name TEXT NOT NULL`、`type TEXT NOT NULL DEFAULT 'my'`、`path TEXT DEFAULT ''`、`created_at DATETIME DEFAULT CURRENT_TIMESTAMP` | `FolderModel`（`cpp/FolderModel.cpp`） | 索引 `idx_folders_type(type)`；首次运行插入一条 `name='默认文件夹', type='my'` |
| `songs` | `id INTEGER PRIMARY KEY AUTOINCREMENT`、`folder_id INTEGER NOT NULL`、`name TEXT NOT NULL`、`path TEXT NOT NULL`、`singer TEXT DEFAULT ''`、`duration INTEGER DEFAULT 0`、`tag_title TEXT DEFAULT ''`、`tag_artist TEXT DEFAULT ''`、`tag_cover TEXT DEFAULT ''`、`tagged INTEGER DEFAULT 0` | `SongModel`（`cpp/FolderModel.cpp`）；`CoverHelper::clearCache()` 会重置 `tagged/tag_cover` | `FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE CASCADE`；索引 `idx_songs_folder(folder_id)`、唯一索引 `idx_songs_path(folder_id, path)`；`tag_*`/`tagged` 四列由 `addColumnIfMissing` 增量 ALTER 加入 |
| `favorites` | `id TEXT NOT NULL`、`title TEXT NOT NULL`、`artist TEXT`、`cover TEXT`、`source INTEGER NOT NULL`、`duration INTEGER DEFAULT 0`、`type TEXT NOT NULL`、`folder_id INTEGER`、`created_at DATETIME DEFAULT CURRENT_TIMESTAMP`、`PRIMARY KEY (type, id)` | `FavoritesModel`（三个单例共用同一张表；`folder_id` 归属由 `FavoriteFoldersModel` 删夹时清理） | 写入用 `INSERT ... ON CONFLICT(type, id) DO UPDATE`（UPSERT，取代「先查再写」）；`folder_id` 由 `addColumnIfMissing` 增量加入，仅 `type='song'` 使用；启动时把无归属歌曲收藏迁入 `type='fav'` 默认夹（见第 10 部分） |
| `db_probe` | `x INTEGER` | `DbWorker::open()` | 开库自检用，建完立刻 `DROP TABLE db_probe`，不常驻 |

### 2.2.3 线程模型

- `DbService::instance()`：函数内 `static DbService *service = new DbService;`，**故意不析构**（规避静态销毁顺序问题）。
- 构造即 `new DbWorker` → `moveToThread(&thread)`，线程 `setObjectName("quemusic-db")` 并 `start()`；
  `QThread::started` → `DbWorker::open`，`QThread::finished` → `DbWorker::close`。
- `submit(Job)`：按 `QMutexLocker` 保护把任务压入 `std::deque`，再 `invokeMethod(worker, "drain", Qt::QueuedConnection)`。
  `drain()` FIFO 取任务执行，**天然串行**，一个连接只在 DB 线程使用。
- 结果回投：`DbService::post(target, fn)` → `QMetaObject::invokeMethod(target, fn, Qt::QueuedConnection)`；`target` 已销毁（`QPointer` 为空）则丢弃。
- 所有 QML 侧读写都遵循「工作线程碰 db、结果 post 回对象所在线程」；`Favorites.h` 注释明确：
  `isFavorite` 在 GUI 线程**绝不能**查库（连接名被 DB 线程持有），只能读内存集合。
- GUI 线程只读通道 `playerDatabase()`（`shared_player_db`）懒建并自动 `ensureSchema`。
  注：`Favorites.cpp` `#include "PlayerDatabase.h"` 但全文件未调用 `playerDatabase()`（未确认是否有意保留）。
- PRAGMA（DB 线程 `open()` 内）：`journal_mode = DELETE`、`synchronous = NORMAL`、`foreign_keys = ON`；
  连接选项 `QSQLITE_BUSY_TIMEOUT=8000`。
- 写失败自愈：写操作失败时调用 `DbService::markWriteFailed(err)`，识别 `nativeErrorCode == "8"`（SQLITE_READONLY）
  或文本含 `readonly`/`read-only` → 置 `DbWorker::writeFailed`，`drain()` 下一轮 `reopen()`（close + removeDatabase + open）后**重试同一个 job 一次**。

---

## 2.3 重要类接口摘要（仅 QML 可见面）

### AccountManager（`AccountManager` 单例）

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `neteaseLoggedIn`、`neteaseNickname`、`neteaseAvatar`、`neteaseCookie`、`neteaseQrText`、`neteaseQrState`、`neteaseQrMessage`、`neteaseVerifyUrl`、`kugouLoggedIn`、`kugouNickname`、`kugouAvatar`、`kugouCookie`、`kugouQrText`、`kugouQrState`、`kugouQrMessage`（全部 `NOTIFY`：`neteaseLoginChanged` / `kugouLoginChanged` / `neteaseQrChanged` / `kugouQrChanged`） |
| Q_ENUM | `QrState { QrWaiting=0, QrScanned=1, QrSuccess=2, QrExpired=3, QrError=4 }` |
| Q_INVOKABLE | `void loadPersisted()`、`void openNeteaseVerifyUrl()`、`void setNeteaseBrowserCookie(cookie, nickname, avatar)`、`void setKugouBrowserCookie(cookie, nickname, avatar)`、`void loginNeteaseWithCookie(cookie)`、`void sendNeteaseCaptcha(phone)`、`void loginNeteaseWithCellphone(phone, captcha)` |
| public slots（QML 可调） | `startNeteaseQrLogin()`、`cancelNeteaseQrLogin()`、`logoutNetease()`、`startKugouQrLogin()`、`cancelKugouQrLogin()`、`logoutKugou()` |
| signals | `neteaseLoginChanged()`、`kugouLoginChanged()`、`neteaseQrChanged()`、`kugouQrChanged()`、`message(const QString &text, int type)`（type：0 普通 / 1 成功 / 2 警告 / 3 错误） |
| 仅 C++ 用 | `neteaseApiCookie()`（合并身份后的 cookie）、`kugouMid()`、`kugouDfid()`；工厂 `static create(QQmlEngine*, QJSEngine*)` 内 `static` 缓存实例，`main.cpp` 会提前调用一次 |
| 持久化 | `QCoreApplication::applicationDirPath() + "/Account.ini"`；组 `Netease`：`deviceId`、`anonymousToken`、`cookie`、`nickname`、`avatar`；组 `Kugou`：`cookie`、`nickname`、`avatar`、`guid`、`dfid` |
| 外部接口 | 网易云走 `ApiHelper`（`QCloudMusicApi` 的 `login_qr_*`）；酷狗走 `KugouApi.h` + 自实现签名 GET；直连端点 `https://music.163.com/api/nuser/account/get`、`https://login-user.kugou.com` |
| 轮询 | `m_neteasePollTimer` 4000ms、`m_kugouPollTimer` 3000ms |

### DownloadManager（`DownloadManager`，可创建 QML 类型）

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `int currentTaskId`、`bool hasActiveTasks`（= `m_currentTaskId >= 0`）、`int completedCount`、`int taskCount`、`QString downloadPath`（可写） |
| Q_INVOKABLE | `void addDownload(url, fileName, meta)`、`void retryTask(taskId)`、`void removeTask(taskId)`、`QString effectiveDownloadDir()` |
| signals | `currentTaskIdChanged`、`hasActiveTasksChanged`、`completedCountChanged`、`taskCountChanged`、`downloadPathChanged` |
| 枚举 | `DownloadTask::Status { Queued, Downloading, Completed, Error }`（未 `Q_ENUM`，QML 侧拿的是 `int`） |
| 目录规则 | `effectiveDownloadDir()` = 自定义 `downloadPath` 或 `QStandardPaths::MusicLocation`，再拼 `/QueMusic`；同名文件加 `(1)(2)` 后缀 |

### FolderModel / SongModel

`FolderModel`（`QML_ANONYMOUS`）

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `QString filterType`（`MyFolders`="my"、`LocalFolders`="local"）、`enum Roles { IdRole, NameRole, TypeRole, PathRole, CreatedAtRole }` |
| Q_INVOKABLE | `loadFromDatabase()`、`addFolder(name, type, path = QString())`、`deleteFolder(folderId)`、`deleteFolders(folderIds)`、`renameFolder(folderId, newName)` |
| signals | `filterTypeChanged()`、`errorOccurred(const QString &message)` |

`SongModel`（`QML_ANONYMOUS`，`Songs` 单例为其子类）

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `int folderId`（可写）、`bool searchActive`、`SearchResultModel *searchResults`（`CONSTANT`） |
| Q_INVOKABLE | `loadByFolder(folderId)`、`reload()`、`QVariantMap get(index)`、`addSong(folderId, name, path, singer = QString())`、`addSongs(folderId, songs)`、`deleteSong(songId)`、`deleteSongs(songIds)`、`rescanTags(path = QString())`、`startSearch(text)`、`clearSearch()` |
| signals | `folderIdChanged()`、`errorOccurred(const QString &message)`、`searchActiveChanged()`、`searchFinished(int count)`、`songsAdded(int count)` |
| 富集 | 每批 `kEnrichBatchSize = 64` 个文件，`QtConcurrent` 解析 TAG，结果 `persistTags()` 事务写回 |

### FavoritesModel

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `QString filterType`（`FavoriteSongs`="song"、`FavoritePlaylists`="playlist"、`FavoriteArtists`="artist"）、`int count`、`bool loading`、`int favoriteRevision` |
| Q_INVOKABLE | `QVariantMap get(row)`、`addFavorite(id, title, artist, cover, source, duration, type)`、`removeFavorite(id, type)`、`bool isFavorite(id, type)` |
| signals | `filterTypeChanged()`、`countChanged()`、`loadingChanged()`、`favoriteStateChanged()`、`errorOccurred(const QString &message)` |

### LocalMusicScanner

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `QUrl folder`（可写）、`QStringList nameFilters`、`bool showDirs`、`int sortField`（0 默认 / 1 名称 / 2 时间 / 3 大小）、`bool sortReversed`、`bool scanning`、`bool deleting`、`int count`、`bool searchActive`、`SearchResultModel *searchResults`（`CONSTANT`） |
| Q_INVOKABLE | `QVariant get(index, role)`、`deleteFiles(paths)`、`startSearch(text)`、`clearSearch()`、`rescan()` |
| signals | `folderChanged`、`nameFiltersChanged`、`showDirsChanged`、`sortFieldChanged`、`sortReversedChanged`、`scanningChanged`、`countChanged`、`scanProgress(int count)`、`scanFinished(int count)`、`metadataReady(int count)`、`searchActiveChanged`、`searchFinished(int count)`、`deletingChanged`、`deleteProgress(int processed, int total)`、`deleteFinished(int removed, int failedCount)` |
| worker 信号（内部，跨线程） | `progress`、`finished`、`enriched`、`enrichDone`、`failed`、`deleteProgress`、`deleteFinished`、`searchBatch`、`searchDone`（均带 `generation`/`searchGeneration`） |

### LocalLyricsReader（全静态，非 QObject）

| 接口 | 说明 |
|---|---|
| `static QVariantList parseLrc(contents)` | 标准行时间戳 + 增强逐字时间戳 |
| `static Parsed parseLrcWithTranslation(contents)` | 额外把「与上一行共用时间轴」的翻译行拆成 `translate` |
| `static QVariantMap read(filePath)` | 返回 `{found, source, lyrics, translate}`；`source` ∈ `sidecar` / `embedded` / `embedded-sylt` / `none` |
| 数据契约 | 每行 `{time(ms), text, info}`；`info` 无逐字时为 `null`，有则 `[{offset, duration, text}]`；`isOther=true` 表示括号包裹的「其他」行 |

### CoverHelper

| 类别 | 内容 |
|---|---|
| Q_INVOKABLE | `QString findLocalCover(sourcePath)`、`QString findEmbeddedCover(sourcePath)`、`void findEmbeddedCoverAsync(sourcePath)`、`void clearCache()`、`void setCacheDir(path)`、`void pruneCache(maxMB)` |
| signal | `localCoverReady(const QString &sourcePath, const QString &coverUrl)`（空结果也回传） |
| 静态接口 | `readCoverFromTag(sourcePath, cacheDir, Metadata *metaOut, bool *failedOut)`、`readMetadata(fileInfo, TagLib::FileRef *openRef = nullptr)`、`bool isLegacyCover(coverUrl)`、`constexpr int kCoverSize = 512` |
| 缓存名 | `<cacheDir>/cover-<sha1(absolutePath + '@' + mtime_ms)>.jpg`（写不出 JPEG 才退 `.png`） |

### ColorExtractor

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `QUrl imageSource`（可写，写入即触发提取）、`QVector<QColor> dominantColors`、`bool busy` |
| Q_INVOKABLE | `void extractColorsFromUrl(const QUrl &url)` |
| signals | `imageSourceChanged()`、`colorsExtracted(const QVector<QColor> &)`、`colorsExtractedAsString(const QStringList &)`、`busyChanged()` |
| 参数 | 取 3 色、采样 48px、相似距离 120、`QCache` maxCost 128；失败兜底色 `#00EE66 / #00B1EE / #9D4EDD` |

### GetWave

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `AudioEngine *engine`（可写）、`QList<qreal> spectrumData`、`int bands`（默认 96，强制偶数且 ≥4）、`QVector<QPointF> wavePath`、`bool enabled`、`QQuickWindow *renderWindow` |
| Q_INVOKABLE | `void updateSpectrum()` |
| signals | `engineChanged`、`spectrumChanged`、`bandsChanged`、`wavePathChanged`、`enabledChanged`、`renderWindowChanged` |
| 线程契约 | 实现 `AudioSpectrumSource::spectrumSinkHandle()` 与 `AudioSpectrumSink::pushSamples(...)`；音频线程只经 `AudioRing` 单向投递，`m_dataReady` 用 `QAtomicInteger` 置位 |
| 参数 | `kRingFrames = 16384`、`kMaxPushFrames = 8192`、`m_fftSize = 4096`、`kMinFreq = 30Hz`、`kDbFloor = -62`、`kDbRange = 56`、`kAttack = 0.6`、`kRelease = 0.3` |

### QueueModel

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `int count`、`int playListIndex`（可写，写入自动 clamp 到 `[-1, size-1]`，空列表为 -1） |
| Q_ENUM | `Roles { NameRole = Qt::UserRole + 1, PathRole, SongerRole, SourceRole }` |
| Q_INVOKABLE | `QVariantMap get(index)`、`append(item)`、`int appendBatch(items)`（返回实际加入数，首条下标 = `count() - 返回值`）、`insert(index, item)`、`moveTo(from, to)`、`remove(index, count = 1)`、`clear()`、`int indexOfPath(path)` |
| signals | `countChanged()`、`playListIndexChanged()` |

### SearchResultModel

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `int count` |
| Q_INVOKABLE | `QVariantMap getRow(int index)` |
| 构造 | `SearchResultModel(const QList<int> &roles, QObject *parent = nullptr)`，`roleNames()` 只返回传入的角色子集 |
| 非 QML | `appendBatch(QList<Row>)`、`clearRows()`、`static QByteArray roleName(int role)` |
| signal | `countChanged()` |

### DownloadedMusicModel

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `QString downloadDir`（可写）、`int count` |
| Q_INVOKABLE | `reload()`、`QVariantMap get(row)` |
| signals | `downloadDirChanged()`、`countChanged()` |
| 元数据契约 | 与音频同 basename 的 `.json`：`title`、`artist`、`cover`、`duration`、`hash`、`lyrics`、`translate`；`title` 空则退 `completeBaseName()` |

### LogManager（`QML_SINGLETON`）

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `bool enabled`（可写）、`int minimumLevel`（可写）、`QString logDirectory`（`CONSTANT`）、`QString currentLogFile`、`QString logPreview` |
| Q_ENUM | `Level { Debug=0, Info=1, Warning=2, Error=3, Fatal=4 }` |
| Q_INVOKABLE | `debug(msg, category = QString())`、`info(...)`、`warning(...)`、`error(...)`、`fatal(...)`、`openLogFolder()` |
| signals | `enabledChanged()`、`minimumLevelChanged()`、`currentLogFileChanged()`、`logPreviewChanged()` |
| 落盘 | 目录 `<AppDataLocation>/logs`，文件 `QueMusic_yyyyMMdd.log`（UTF-8 + BOM，每日一份，追加写） |
| 配置 | 与 QML Settings 同一个 ini：`<ConfigLocation>/BroNekoX/QueMusic.ini`，键 `Options/logEnabled`、`Options/logLevel` |

### SystemTrayManager

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `bool available`（`CONSTANT`）、`bool visible`（可写）、`QString iconSource`（可写）、`QString title`（可写，默认 `"QueMusic"`）、`QString nowPlaying`（可写）、`bool playing`（可写） |
| Q_ENUM | `ActivationReason { Unknown=0, Context=1, DoubleClick=2, Trigger=3, MiddleClick=4 }`、`MessageIcon { NoIcon=0, Information=1, Warning=2, Critical=3 }` |
| Q_INVOKABLE | `void showMessage(title, body, icon = Information, msecs = 4000)`、`void quitApplication()` |
| signals | `visibleChanged`、`iconSourceChanged`、`titleChanged`、`nowPlayingChanged`、`playingChanged`、`activated(SystemTrayManager::ActivationReason)`、`showWindowRequested()`、`playPauseRequested()`、`previousRequested()`、`nextRequested()`、`quitRequested()` |
| 菜单项 | 「显示主界面」/ 当前曲目（禁用、无曲目隐藏）/ 「播放|暂停」/「上一首」/「下一首」/「退出 QueMusic」 |

### WindowsSmtcManager

| 类别 | 内容 |
|---|---|
| Q_PROPERTY | `bool available`（= `Private::initialized`） |
| Q_ENUM | `PlaybackStatus { Closed=0, Changing=1, Stopped=2, Playing=3, Paused=4 }` |
| Q_INVOKABLE | `initialize(QWindow *window)`、`shutdown()`、`setControlsEnabled(play, pause, next, previous)`、`setPlaybackStatus(int status)`、`updateMediaInfo(title, artist, album = QString(), cover = QString(), mediaId = QString())`、`updateTimeline(positionMs, durationMs)` |
| signals | `availableChanged()`、`playPressed()`、`pausePressed()`、`nextPressed()`、`previousPressed()`、`seekRequested(qint64 positionMs)` |
| 非 QML | `postButtonPressed(int)`、`postSeekRequested(qint64)`（内部线程安全入口） |

---

## 2.4 模型 / 角色（roles）

`roleNames()` 返回的字符串即 QML delegate 里可用的属性名。

| 模型 | role 名（按 `enum` 顺序） | 实例化方式 |
|---|---|---|
| `FolderModel` | `folderId`、`name`、`type`、`path`、`createdAt` | `QML_ANONYMOUS`；经 `AppModels.h` 宏生成 `MyFolders`（filter `"my"`）、`LocalFolders`（filter `"local"`）两个 `QML_SINGLETON` |
| `SongModel` | `songId`、`folderId`、`name`、`path`、`singer`、`duration`、`tagTitle`、`tagArtist`、`tagCoverUrl` | `QML_ANONYMOUS`；`Songs`（`QML_SINGLETON`，显式 `create()`，不设 filterType） |
| `FavoritesModel` | `favId`、`title`、`artist`、`cover`、`source`、`duration`、`type`、`createdAt`、`paytype`（恒 0） | `QML_ANONYMOUS`；`FavoriteSongs`/`FavoritePlaylists`/`FavoriteArtists` 三个单例（filter `song`/`playlist`/`artist`） |
| `SearchResultModel` | **动态**：构造时传入的角色集决定。`SongModel` 传 `songId`、`folderId`、`name`、`path`、`singer`、`duration`、`tagTitle`、`tagArtist`、`tagCoverUrl`；`LocalMusicScanner` 传 `fileName`、`fileUrl`、`fileSize`、`fileModified`、`title`、`artist`、`coverUrl` | 两个模型的构造函数里 `new ... (roles, this)`，作为 `searchResults` 属性暴露 |
| `LocalMusicScanner` | `fileName`、`fileSize`、`fileModified`、`fileUrl`、`title`、`artist`、`coverUrl`（`data()` 的 switch 顺序与 `roleNames()` 声明顺序略不同，名称一致） | `QML_ELEMENT`，QML 侧实例化（具体位置未确认） |
| `WebDavModel` | `fileName`、`fileUrl`、`fileSize`、`fileModified`、`title`、`artist`、`coverUrl`（**恒空**）、`isDir`、`localCover` | `QML_ELEMENT`，QML 侧实例化（具体位置未确认） |
| `QueueModel` | `name`、`path`、`songer`、`source` | `QML_ELEMENT`，QML 侧实例化（具体位置未确认） |
| `DownloadManager` | `taskId`、`url`、`fileName`、`filePath`、`progress`、`status`、`errorString`、`title`（→ `fileName`）、`artist`（→ `filePath`）、`cover`（恒空）、`duration`（恒 0）、`hash`（→ `id`，兼容 QListView 别名） | `QML_ELEMENT`，QML 侧实例化（具体位置未确认） |
| `DownloadedMusicModel` | `title`、`artist`、`cover`、`duration`、`hash`、`fileName`、`fileUrl`、`lyrics`、`translate`、`paytype`（恒 0） | `QML_ELEMENT`，QML 侧实例化（具体位置未确认） |

`AppModels.h` 宏生成的单例（`QML_ELEMENT` + `QML_SINGLETON`，QML 侧按类型名访问，如 `FavoriteSongs.count`）：

| 单例 | 基类 | 过滤类型 |
|---|---|---|
| `MyFolders` | `FolderModel` | `"my"` |
| `LocalFolders` | `FolderModel` | `"local"` |
| `Songs` | `SongModel` | （不使用） |
| `FavoriteSongs` | `FavoritesModel` | `"song"` |
| `FavoritePlaylists` | `FavoritesModel` | `"playlist"` |
| `FavoriteArtists` | `FavoritesModel` | `"artist"` |
| `FavoriteFolders` | `FavoriteFoldersModel` | （收藏夹列表，无过滤类型；第 10 部分） |

---

## 2.5 WebDAV 子系统

### 2.5.1 关系

| 组件 | 角色 | 依赖 |
|---|---|---|
| `WebDavStore`（QML `WebDav` 单例） | 配置与凭据的唯一持有者；产生 Basic 鉴权头；会话内记录「音频 URL → (歌词 URL, 封面 URL)」 | `QSettings`、DPAPI、`WebDavClient::basicAuth` |
| `WebDavClient` | 纯传输层：`listDir(url, authHeader, callback)` 发 PROPFIND Depth:1，解析 MultiStatus | 自带 `QNetworkAccessManager` |
| `WebDavModel` | 目录列表模型 + 目录级缓存 + sidecar 探测 + 顺序封面预热 | `WebDavClient m_client`（成员）、`WebDavCache`、`CoverHelper`、`MediaTypes`、`DbService::cacheDir()` |
| `WebDavCache`（单例） | 远端文件落地：音频 → 同名歌词 → `cover.*`，串行队列 | `QNetworkAccessManager`、`DbService::cacheDir()`、`MediaTypes` |

数据流：`WebDavStore` 提供 `authHeader(id)` → QML 调 `WebDavModel.openServer(serverId, authHeader, rootUrl)` 或 `setAuthHeader`；
选中曲目时 `WebDavStore::rememberSidecars(audioUrl, lyricsUrl, coverUrl)` 记录 sidecar，播放时 QML 调 `WebDavCache.cache(url, authHeader, lyricsUrl, coverUrl)`；
落地后标签/歌词/封面复用本地那套（`CoverHelper` + `LocalLyricsReader`）。

### 2.5.2 认证与凭据

- 存储：`QSettings`，键 `webdav/servers`（`QVariantList`，每项 `id`(UUID 无花括号)、`name`、`url`、`user`、`password`(密文)）。
- 加密：Windows 用 DPAPI `CryptProtectData` / `CryptUnprotectData`（描述串 `L"QueMusic"`，绑当前用户），密文 Base64 存 ini。
  非 Windows 平台**只是 Base64 编码，不是加密**（源码注释：其它平台暂无系统凭据库，Keychain 后续接）。
- 加密失败必须中止落盘（`encryptPassword` 返回 false → `addServer`/`updateServer` 直接返回），避免把密码写成空串还当成有效配置。
- `servers()` 读属性会剥掉 `password` 字段，**凭据不进 QML**。
- `authHeader(id)` 结果按 id 缓存（`m_authCacheId`/`m_authCacheHeader`），避免每请求重复 DPAPI 解密；`save()` 时清空该缓存。
- `authHeaderOfUrl(url)` 用 `QUrl` 的 host + path 分段比较（不是字符串前缀），否则 `dav-attacker/` 会命中 `dav/` 的凭据。
- URL 规范化：缺 scheme 补 `https://`，末尾补 `/`；同地址重复添加会转为更新。

### 2.5.3 缓存策略

| 层 | 策略 |
|---|---|
| 目录列表（`WebDavModel::m_cache`） | 每目录 30 秒内复用；最多保留最近 12 个目录（超限淘汰最久未用）；`setAuthHeader` 变更时整表清空 |
| 在途请求 | `m_generation` 代次作废；同目录且 `m_busy` 时忽略重复点击（`refresh()`/`openServer()` 走 `force = true` 绕过） |
| 落盘（`WebDavCache`） | 目录 `<cacheDir>/webdav/<sha1(url)>/`；步骤 0 音频（文件名 = URL 末段，空则 `audio`）、步骤 1 同名 `.lrc`、步骤 2 `cover.<原扩展名|jpg>`；先写 `.part` 再 rename；请求 `setTransferTimeout(20000)`；同 URL 已在队列则忽略；已落地直接 `emit cached` |
| 容量 | 上限 1GB（`kCacheLimitBytes`）；按目录 `latest mtime` 从旧到新删；最近 5 分钟（`kProtectMs`）用过的目录不删；`prune()` 限频 1 分钟一次 |
| 封面 | `cachedCoverFor` 按 `cover.{jpg,jpeg,png,webp,bmp,gif}` 探测；`WebDavModel::warmCovers()` 只对**已落地**音频工作线程提取内嵌封面，结果进 `m_coverCache`（>512 全清），经 `dataChanged(..., {LocalCoverRole})` 刷新 |
| sidecar 匹配 | 同目录按「同名 → `cover`/`folder`/`albumart`」找封面；歌词找同名 `.lrc`/`.txt`；匹配大小写不敏感；`TitleRole`/`ArtistRole` 按文件名第一个 `-` 拆成「歌名 - 歌手」 |

---

## 2.6 插件子系统

### 2.6.1 目录与清单

| 项 | 值 |
|---|---|
| 安装根目录（可写） | `QStandardPaths::AppDataLocation + "/plugins/" + m_folder`；`m_folder` = `lyrics`（歌词界面插件）或 `function`（功能插件） |
| 第二搜索根 | `QCoreApplication::applicationDirPath() + "/plugins/" + m_folder`（注释：便于开发期直接放插件调试）；用户目录优先，同 `id` 去重 |
| 清单文件 | `<插件目录>/info.json` |
| 清单字段 | `apiVersion`（默认 1，与 `apiVersion()` 不符则跳过）、`entry`（默认 `plugin.qml`，文件不存在则跳过）、`preview`（默认 `info.png`）、`name`（默认目录名）、`author`、`version`、`description` |
| 插件 id | 目录名（`QFileInfo(path).fileName()`） |
| 暴露给 QML 的字段 | `id`、`name`、`author`、`version`、`description`、`source`（`entry` 的 `file://` URL）、`preview`、`path`、`builtin` |
| apiVersion | 基类默认 `1`；`LyricsPluginStore`/`FunctionPluginStore` **均未覆盖** |

### 2.6.2 两个派生的 QML 单例

| 单例 | QML 名 | 专有属性/方法 | 持久化键 | 生命周期差异 |
|---|---|---|---|---|
| `LyricsPluginStore`（kind=`歌词界面插件`，folder=`lyrics`） | `LyricsPlugins` | `Q_PROPERTY QString selectedId`（可写）、`Q_PROPERTY QString source`（当前入口，绑给 `Loader.source`）、`Q_INVOKABLE void rescan() override` | `QSettings` 键 `Plugins/lyrics`（默认 `builtin.default`） | 单选：`setSelectedId` 时刷新 `source` 并落盘；`rescan()` 后若 `source` 为空 ⇒ 回落 `builtin.default` |
| `FunctionPluginStore`（kind=`功能插件`，folder=`function`） | `FunctionPlugins` | `Q_PROPERTY QStringList enabledIds`、`Q_INVOKABLE void setEnabled(id, bool on)`、`Q_INVOKABLE void rescan() override` | `QSettings` 键 `Plugins/function/<id>`（默认 `true`） | 多选：`loadEnabled()` 遍历 `m_plugins`，新插件默认启用；卸载后键保留，重装恢复原选择 |

两个派生的构造函数里都先调 `PluginStore::load()`（会回调 `builtinPlugins()`）。

### 2.6.3 生命周期动作（均在基类 `PluginStore`）

| 动作 | 行为 |
|---|---|
| 扫描 `load()` | 先放内置插件，再扫两个根目录；`readPlugin()` 缺 info.json / 入口不存在 / apiVersion 不匹配都返回空并跳过 |
| `rescan()` | `load()` + `emit pluginsChanged()`；派生类额外做选中/启用的修正 |
| `install(folder)` | 必须含 `info.json`；目标 = `<writableRoot>/<源目录名>`；禁止把插件装到它自己的子目录；已存在同名则 `removeRecursively()` 后整体复制；返回空串表示成功，否则中文错误文本 |
| `remove(id)` | 找不到 / `builtin == true` 拒绝；否则删目录后 `rescan()` |
| `reveal(id)` | `id` 为空打开插件根目录（先 `mkpath`），否则打开该插件目录（`QDesktopServices`） |
| 查找 `find(id, *info)` | 线性遍历（注释：插件十几个量级，线性比维护 id→下标映射更省） |

### 2.6.4 扩展点

- **歌词界面插件**：只需提供 QML 入口（默认 `plugin.qml`）。宿主读 `LyricsPlugins.source` 作为 `Loader.source`，因此插件是一个「整屏歌词界面」组件。内置三项（`builtin.*`，不可卸载）：
  `builtin.default` → `qrc:/QueMusic/layout/MainLyric.qml`、`builtin.free` → `qrc:/QueMusic/lyricsui/LyricsFree.qml`、`builtin.3d(Beta)` → `qrc:/QueMusic/lyricsui/Lyrics3D.qml`。
- **功能插件**：宿主 `components/PluginHost.qml` 按 `FunctionPlugins.enabledIds` 为每个启用插件起一个加载器；插件可多个同时生效。
- 两类插件目前**没有**额外的 C++ 扩展接口（无约定函数/无后端回调），扩展点就是 QML 入口文件本身。

---

## 2.7 跨模块关系

- **AppModels.h 派生层**：`MyFolders`/`LocalFolders` → `FolderModel`；`Songs` → `SongModel`；`FavoriteSongs`/`FavoritePlaylists`/`FavoriteArtists` → `FavoritesModel`（`doc/` 宏统一 `setFilterType`）。
- **DbService 被依赖方**：`FolderModel`（folders/songs 全部读写 + 搜索）、`FavoritesModel`（favorites 全部读写）、`CoverHelper::clearCache()`（重置 `songs.tagged/tag_cover`）。
- **缓存目录依赖**：`CoverHelper`（构造取 `DbService::cacheDir()`）、`SongModel::enrichBatch()`、`WebDavCache`、`WebDavModel::warmCovers()` 都走 `DbService::cacheDir()`；
  例外是 `LocalMusicScanner::startScan()`，它独立算 `AppDataLocation + "/cache"`。
- **CoverHelper 被依赖方**：`SongModel::enrichBatch()`、`LocalScanWorker::run()`（`CoverHelper::readCoverFromTag`）、`WebDavModel::warmCovers()`（静态方法，无需实例）。
- **LocalLyricsReader 调用方**：`api/MusicApiService.cpp`（`QtConcurrent::run(&LocalLyricsReader::read, ...)` 等）。本层内无调用者。
- **AccountManager 依赖**：`apihelper.h`（`api/QCloudMusicApi/QCloudMusicApi/apihelper.h`）与 `api/KugouApi.h`（均在 `cpp/` 之外）。
- **GetWave 依赖**：`audio/AudioEngine.h`（`setSpectrumSink`）、`audio/AudioRing.h`、`audio/AudioSpectrumSink.h`；引擎只认句柄，不认识 `GetWave` 类型。
- **WebDAV 依赖链**：`WebDavModel` →（成员）`WebDavClient` + `WebDavCache` + `CoverHelper` + `MediaTypes` + `DbService`；`WebDavStore` → `WebDavClient::basicAuth`。
- **单例清单（QML 侧可见）**：`AccountManager`、`LogManager`、`WebDav`、`WebDavCache`、`LyricsPlugins`、`FunctionPlugins`、`MyFolders`、`LocalFolders`、`Songs`、`FavoriteSongs`、`FavoritePlaylists`、`FavoriteArtists`。
- **非单例（QML 侧自行实例化）**：`ColorExtractor`、`CoverHelper`、`CustomFlow`、`DownloadManager`、`DownloadedMusicModel`、`GetWave`、`LocalMusicScanner`、`QueueModel`、`SystemTrayManager`、`WindowsSmtcManager`、`WebDavModel`。
- **QML 外壳注入**：仅 `configDir`（`main.cpp:155` 的 `setContextProperty`）。其余一切经 QML 单例按类型名访问。
  具体 QML 文件里在何处实例化 `QueueModel`/`DownloadManager` 等未确认（不在本次阅读范围）。
- **`AccountManager::create()` 与 `LogManager::create()`**：`main.cpp` 会提前调用一次以保证时序（账号初始化、日志尽早接管 Qt 消息），之后 QML 首次访问复用同一实例。

---

## 2.8 坑与约定（源码注释里的不变量/陷阱）

**单例与 QML 注册**

1. `AppModels.h`：构造函数**绝不能**带默认参数 —— `singletonConstructionMode()` 中 `is_default_constructible` 优先于 `HasSingletonFactory`，
   写成 `X(QObject *parent = nullptr)` 会走默认构造、`create()` 永不执行，表现为「过滤类型失效、收藏列表列出全部数据」。
   `AccountManager.h`、`LogManager.h` 有同款注释：会导致 `main.cpp` 提前创建的实例与 QML 侧看到的不是同一个。
2. `AppModels.h`：用独立单例而非 `setContextProperty`，因为上下文属性无法被 `qmlcachegen` 编译期解析，按类型名访问才有编译期绑定；
   也**不能**合并成「一个单例持多个模型属性」，会拒绝继续属性查找。同理，经对象属性间接访问 `AccountManager` 会被判为
   shadowable base type 而拒绝编译（实测）。

**数据库**

3. `DbService`：连接有线程亲和性，同步查询会冻住 GUI；任务体里只碰 `db`，结果用 `post()` 投回对象所属线程。
4. SQLite 的 `readonly` 是**连接级粘性状态**：某次写失败后这条连接会一直报 readonly，之后所有写操作静默失效 ⇒ 必须重开连接，而不是继续用它。
5. 用单文件 `DELETE` 日志而非 WAL：写只在一条线程（GUI 侧只读），不需要 WAL 的读写并发，而 WAL 依赖 `-wal`/`-shm` 两个附属文件，
   被杀软/同步盘拦住时会让整库写不进去（表现为 `attempt to write a readonly database`）。
6. 建表也是写操作：拿不到写锁就是一整库只读，开机就要说清楚，别等用户点收藏才发现（`CREATE TABLE db_probe` 探测）。
7. 只判断目录权限不够：安全软件会在 SQLite 真正写入时才拦，所以 `dbWritable()` 要真建一条连接试写一次。
8. `DbService::instance()` 故意不析构，避开静态销毁顺序问题；`markWriteFailed` 后重试本轮任务**一次**。

**收藏 / 歌单 / 歌曲模型**

9. `Favorites.h`：`isFavorite` 用内存 id 集合回答（delegate 每行都会调用）；`refreshModel` 在 DB 线程一次性填好**全部类型**的 id，
   `isFavorite` 只读它、不做任何 SQL。类型尚未装进内存时会补一次后台加载，否则界面一直显示「未收藏」。
10. 收藏写库失败**不更新内存缓存**（图标保持未收藏，用户看到的就是真实状态）；调用方不要抢先提示成功。
11. 三个收藏单例共用一张表：`s_revision` 全局版本号 + `s_instances` 列表，任一实例增删都让所有列表的收藏图标重算。
12. `FolderModel`/`SongModel`：用 `generation` 丢弃过期结果；行集未变（size 相同且 id 顺序一致）只发 `dataChanged`，
    避免整表 reset 重建视图缓存。
13. TAG 读取**失败**（`failed == true`）时不写 `tagged`，留待下次重试；「确实没有封面」才落库。
    `tag_cover` 指向不存在的文件、或还是旧版带尺寸后缀的缓存名，都要重提（`coverStale`）。
14. 搜索用 SQL 侧 `LIKE`，用户输入里的 `%`/`_`/`\` 要转义（`ESCAPE '\'`）。
15. `LocalMusicScanner`：删除**不检查 cancel**（用户确认过的操作，中途取消会出现「删一半」）；空删除列表也要回 `deleteFinished`，
    否则 QML 的模态进度框不关闭、整个窗口卡死；删除结果**不参与** generation 过期判断。
16. 扫描线程的 `m_index` 与 UI 侧 `m_entries` 行下标必须继续对齐，删除后 worker 与模型各自按同一规则收缩。
17. `rescan()` 存在的理由：避免 QML 侧用 `folder=""` → `folder=原值` 去「骗」一次重扫 —— 那会让 `setFolder("")` 真的触发
    `clearEntries()` 先清空、紧接着再扫一遍，等于连续两次模型重置。
18. `LocalMusicScanner::startScan()` 里的封面缓存目录是 `AppDataLocation + "/cache"`，与 `DbService::cacheDir()` 的写法不同
    （后者可能退到 `applicationDirPath()/data/cache`）。是否有意为之：未确认。

**队列 / 下载**

19. `QueueModel`：唯一不变量是 `m_indexOfPath` 永远与 `m_items` 行号一致（重复 path 取首次出现，空 path 不入缓存，查不到返回 -1）；
    `insert`/`remove`/`move` 都在模型变更信号**之后**整体重建，不做增量维护。
20. `QueueModel::moveTo(from, to)`：`to` 是「想要占据的最终下标」；`beginMoveRows` 的目的行是「移动后占据的下标」，
    向后移要 +1；当前播放行跟着走，否则高亮跑偏。
21. `DownloadManager::abortCurrentDownload()`：`abort()` 会**同步**触发 `finished`，必须先断开回调并清空成员，
    否则 `onFinished()` 会顺手启动下一个任务，回来后这里又把新任务的 reply/file 删一遍。
22. 只认当前 `reply`（`reply == m_reply`）：旧 reply 的延迟回调不能写进新任务的文件。
23. 必须先定好 `filePath` 再发 `dataChanged`，否则 QML 拿到空的 `filePath`。
24. 远端来的文件名要 `safeFileName()` 净化（剥目录、剔除非法/控制字符、限长 120），否则 `"../x"` 能写出下载目录外。
25. B 站 CDN 会校验 `Referer`/`UA`，缺失返回 403；重定向用 `NoLessSafeRedirectPolicy`。
26. 下载**出错**时删除半成品文件；**取消**时保留已下载的部分。

**封面 / 取色 / 频谱**

27. `CoverHelper`：一首歌只有一个封面缓存文件（JPEG，写不出去才退 PNG），与用途无关；统一按 `kCoverSize = 512` 覆盖式缩放，
    解码尺寸交给 QML 的 `sourceSize`；带 alpha 的图先转 RGB，「免得存出来发黑」。
28. `isLegacyCover()`：旧版缓存名带尺寸后缀（`cover-<sha1>-64.jpg`），命中说明是过期缓存、需要重提。
29. `clearCache()` 必须同步清 DB：文件删了，DB 里存的封面路径就成了死链 ⇒ `UPDATE songs SET tagged = 0, tag_cover = ''`。
30. `setCacheDir("")` 忽略，与当前目录相同则不变更；`pruneCache` 丢到线程池（丢弃 `QFuture`），按 mtime 从旧到新删，
    新写入的排最末不会被误删。
31. `ColorExtractor`：`m_generation` 丢弃切歌后的过期结果；本地文件与网络图都走后台线程解码+取色。
32. `GetWave`：析构时**必须先** `m_handle->detach()` —— 它返回后才保证音频线程不再进入本对象；
    并主动 `setSpectrumSink(nullptr)` 退订，不依赖 `QObject::destroyed`（那个信号发出时派生类成员已析构）。
33. `GetWave`：音频回调加真降混（双声道取平均）；只取左声道时相位相反的段落会互相抵消，「看起来就是抖」。
    环形空间不足时丢弃本批，**绝不阻塞音频线程**。
34. `GetWave`：汉宁窗分母用 `copyLen` 而不是 `n`，否则每帧窗形不同、频谱会跟着抖；上升/下降用双系数（`kAttack`/`kRelease`），
    单系数平滑会让柱子逐帧乱抖；相邻三点混合削孤立尖峰。
35. `GetWave`：`spectrumData`/`wavePath` 只在 GUI 线程读写（音频线程经 `m_ring` 单向投递），因此无需加锁。

**日志 / 托盘 / 系统集成**

36. `LogManager`：文件写入统一在**主线程**执行（跨线程用 `QueuedConnection` 转发），避免跨线程访问 `QFile`/QML 预览的竞态；
    排到队时单例可能已析构（析构里清了 `s_instance`），必须再判一次。
37. 预览串重建 + 通知 QML 的开销与条数成正比：警告及以上立即刷新，其余由 300ms 单次 `QTimer` 合并；
    警告及以上才 `flush()`；`Fatal` 无视开关始终记录，写完 `std::abort()`；控制台（stderr）输出保留。
38. `LogManager.h` 注释写「安装目录/logs/...」，实现里用的是 `QStandardPaths::AppDataLocation + "/logs"`（注释与实现不一致，以代码为准）。
39. `SystemTrayManager`：`QMenu` 的 parent 必须是 `QWidget`，故手动释放；析构先 `setContextMenu(nullptr)` 再 `delete m_menu`，避免悬空指针。
40. `QIcon::isNull()` 不代表图标真的可用，必须请求一次小尺寸 pixmap 才能确认；图标为空时通知区域什么都不显示，
    表现为「隐藏到托盘后程序不见了」。解析顺序：QML 指定 → 应用窗口图标 → 系统标准图标。
41. `SystemTrayManager` 依赖 QtWidgets，应用必须用 `QApplication` 启动；`quitApplication()` 是显式结束进程
    （窗口可能正隐藏在托盘里，`QWindow::close()` 对不可见窗口不生效）。
42. `WindowsSmtcManager`：走**直连 WinRT ABI**（不使用 WRL），因此 MSVC 与 MinGW-w64 都可编译；非 Windows 下全部为空操作，
    QML 侧可直接调用。
43. SMTC 需要 native window：`initialize()` 里若 `!window->handle()` 先 `window->create()`，再取 `winId()`；拿不到 HWND 就失败。
44. 封面只处理 `http`/`https`/`file`；`qrc:/` 等 Qt 内部 scheme 对 Windows 无意义，「塞给 SMTC 只会失败」。
45. `mediaId`/`album`/缩略图在无值时要写空 HSTRING/空引用，清除上一首的残留。
46. 时间线有节流（记录最近推送的 position/duration）：仅首次、duration 变化、回退或前进超过阈值时才重建 WinRT 时间线；
    `duration <= 0` 时用全零时间线重置并复位节流状态。
47. SMTC 按钮回调来自 WinRT 工作线程，`postButtonPressed`/`postSeekRequested` 用 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`
    投递到主线程；进程内只有主窗口一个 SMTC 实例（全局 `s_smtcManager`）；`deinit()` 先 `remove_*` 事件再 reset 接口。

**WebDAV / 插件 / 其他**

48. `WebDavStore`：加密失败必须让调用方中止落盘 —— 「以前静默写成空串，等于把密码丢了还当成有效配置」。
49. `authHeaderOfUrl` 必须按 `QUrl` 分段比较：纯字符串前缀会让 `dav-attacker/` 也命中 `dav/` 的凭据。
50. `WebDavClient` 只看元素**本地名**（兼容 `<D:response>` / `<response>`）；`setTransferTimeout(15000)` 让服务器不回包时快速失败，
    避免目录一直转圈；结果里含自身要跳过；排序为目录在前、名称 `localeAwareCompare` 升序（服务器返回顺序各家不一）。
51. `WebDavModel`：`CoverUrlRole` **恒返回空** —— 侧车封面是远端 URL，列表加载不了它（要带鉴权头），只暴露已落地到本地的 `localCover`。
52. `WebDavModel::warmCovers()`：无论成功与否都要把本批 URL 从 `m_coverPending` 出队，否则这批 URL 永远不会再被检查；
    `m_coverCache` 是跨目录累积，超过 512 条宁可全清重算。
53. `WebDavModel` 文件名解析约定：网盘命名习惯是「歌名 - 歌手」（与酷狗接口的「歌手 - 歌名」相反），只在第一个 `-` 处分割；
    「若你的库是『歌手 - 歌名』，把两个函数对调即可」。
54. `WebDavCache`：`prune` 要遍历整个缓存目录，限频到每分钟一次；正在听（最近 5 分钟）的目录不清理。
55. `PluginStore`：缺 `info.json` / 入口不存在 / `apiVersion` 不匹配都跳过 —— 「避免装坏插件后崩启动」；
    派生类构造函数里要调一次 `load()`（它会回调 `builtinPlugins()`）；不能把插件装到它自己的子目录里。
56. `PluginStore` 注释：插件数很少（十几个量级），线性查找比额外维护 id → 下标映射更省。
57. `MediaTypes.h`：可播放后缀的**唯一来源**，本地扫描、WebDAV 列表、已下载列表共用一份，
    「避免三处白名单分叉（同一文件在网盘能播、下载页却不显示）」。
58. `CustomFlow`：布局时跳过 `QQuickRepeater`（`isFlowItem`）；`markDirty()` + `polish()` 批量布局，
    在 `updatePolish()` 里才真正排版；`setImplicitHeight(y - spacing)`。
---

← 返回 [索引 README.md](README.md)
