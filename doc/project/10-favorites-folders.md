# 第 10 部分 · 歌曲收藏夹（多文件夹收藏）

> 生成时间：2026-06；对象：歌曲收藏的多收藏夹能力。改动前请先读本文，避免踩到「老收藏消失」。
> 歌单收藏（`type='playlist'`）与歌手收藏（`type='artist'`）**不使用收藏夹**，行为保持不变。

## 10.1 一句话结论

歌曲收藏夹复用本地音乐「我的文件夹」的 `folders` 表，仅以 `type='fav'` 区分；
歌曲收藏行通过 `favorites.folder_id` 归属到某个夹。老库升级时，所有无归属的歌曲收藏会被自动迁入「默认收藏夹」，保证原收藏列表不丢。

## 10.2 数据层（`cpp/DbService.cpp` `ensureSchema`）

| 对象 | 定义 | 说明 |
|---|---|---|
| `folders.type='fav'` | 与 `my`/`local` 同表 | 收藏夹就是 `folders` 的一类行 |
| `favorites.folder_id INTEGER` | 新增列 | 仅 `type='song'` 使用；playlist/artist 恒 NULL |
| `idx_favorites_folder` | `favorites(folder_id)` | 按夹过滤的索引 |

启动迁移（幂等，每次 `ensureSchema` 会跑）：

1. `addColumnIfMissing(favorites, folder_id)`：老库增量 ALTER，无需用户干预。
2. 取 `type='fav'` 最小 id 作为默认夹；不存在则插入 `name='默认收藏夹', type='fav'`。
3. `UPDATE favorites SET folder_id=<默认夹> WHERE type='song' AND folder_id IS NULL`。
   **这一步是「原收藏列表不消失」的保证**：老库所有歌曲收藏没有 `folder_id`，统一落到默认夹。

含义：默认夹 = `type='fav'` 中 id 最小的那一条，UI 不允许重命名/删除它。

## 10.3 C++ 模型（`cpp/Favorites.h` / `cpp/Favorites.cpp`）

### FavoritesModel（三个收藏单例共用）

新增：

| 成员 | 作用 |
|---|---|
| `Q_PROPERTY int folderId` | `>=0` 时列表只显示该夹歌曲；`-1` 不过滤 |
| `Q_PROPERTY QString searchText` | 内存内按标题/歌手过滤，不查库；`setSearchText` → `applyFilter` |
| `Q_INVOKABLE addToFolder(id,title,artist,cover,source,duration,folderId)` | 收藏到指定夹；UPSERT 覆盖旧归属 |
| `Q_INVOKABLE refresh()` | 重拉当前过滤（收藏夹被删后由 C++ 自动调用） |
| `static reloadAll()` | 删夹后重拉所有收藏实例，仅 GUI 线程调用 |
| `applyItems` / `applyFilter` | 全量存 `m_allItems`，视图 `m_items` 由搜索词过滤 |

`refreshModel()` 的 SQL 依据 `filterType` + `folderId` 拼 `WHERE`；`isFavorite` 仍只读内存 `m_idsByType`，不做 SQL。
写库统一走私有 `writeFavorite(..., moveExisting)`：
- `moveExisting=false`（老 `addFavorite`）：新收藏自动落默认夹，已有归属不动（`COALESCE(favorites.folder_id, excluded.folder_id)`）。
- `moveExisting=true`（`addToFolder`）：强制写入目标夹。

### FavoriteFoldersModel（新类，`QML_ANONYMOUS`）

| 接口 | 说明 |
|---|---|
| roles `folderId` / `name` / `isDefault` | 收藏夹列表 |
| `defaultFolderId` 属性 | 默认夹 id（id 最小者） |
| `addFolder(name)` / `renameFolder(id,name)` | 增改，`type='fav'` |
| `deleteFolder(id)` | 单事务删夹 + 删除夹内歌曲收藏，随后 `FavoritesModel::reloadAll()` |
| `count` / `foldersChanged` / `errorOccurred` | 通知 |

## 10.4 单例注册（`cpp/AppModels.h`）

```cpp
class FavoriteFolders : public FavoriteFoldersModel {
    Q_OBJECT QML_ELEMENT QML_SINGLETON
public:
    explicit FavoriteFolders(QObject *parent) : FavoriteFoldersModel(parent) {}
    static FavoriteFolders *create(QQmlEngine *, QJSEngine *) { return new FavoriteFolders(...); }
};
```

注意：基类构造无默认参数，避免触发「宏单例默认构造优先于 create()」的坑（同 `FavoriteSongs`）。

## 10.5 QML 装配

| 文件 | 作用 |
|---|---|
| `components/FavoriteFolderDialog.qml` | 全应用共用的「收藏到」弹窗：列出收藏夹，点某夹即 `FavoriteSongs.addToFolder`；底部「取消收藏」按当前是否收藏决定删/不删，「完成」仅关闭 |
| `components/Options.qml` | 新增 `property FavoriteFolderDialog favPicker` |
| `main.qml` | 实例化 `FavoriteFolderDialog` 并注入 `Options.favPicker` |
| `pages/FavouritePage.qml` | 「歌曲」Tab 只放收藏夹列表（`QLocalView` + 仿本地文件夹 delegate，行内重命名/删除）；`AnimatorWindow id=songFavListMusic` 承载夹内歌曲（`QListView` + 搜索/排序/刷新/播放全部/批量取消收藏），顶部「新建收藏夹 / 排序 / 清空历史 / 选择」按 Tab 显隐 |
| `components/QListView.qml` | 歌曲收藏走 `Options.favPicker.show(...)`；歌单收藏仍直接切换 |
| `layout/PlayerControl.qml` | 播放栏收藏按钮（含 `toggleFavorite()`）走弹窗；图标改为绑 `FavoriteSongs.favoriteRevision` |
| `components/PlayList.qml` | 播放列表行收藏走弹窗 |
| `pages/FavouritePage.qml` 历史 `toggleFav` | 历史列表收藏走弹窗 |

弹窗入参统一为 `{ id, title, artist, cover, source, duration }`；`id` 即歌曲 key（在线为 hash，本地为路径）。

## 10.6 交互约定

- 点歌曲收藏按钮 → 弹窗；点某收藏夹 → **直接收藏进去**并关闭。
- 弹窗底部「取消收藏」：`isFavorite(id,'song')` 为真才删除，否则无动作（不弹错误）。
- 弹窗底部「完成」：不产生任何收藏变更。
- 收藏夹列表里的默认夹：禁止重命名/删除；其它夹可改名、可删除（删除会一并删掉夹内收藏）。
- 「歌曲」Tab 点某个收藏夹 → `songFavListMusic` 滑出，显示该夹歌曲；夹内支持搜索（内存过滤）、排序（标题/歌手/时长）、刷新、播放全部、加入播放列表、批量取消收藏；已移除导入按钮。
- 页面级选择模式仅用于「歌单」Tab；歌曲的批量操作在收藏夹窗口内完成。

## 10.7 改动文件清单

- `cpp/DbService.cpp`：`favorites.folder_id`、默认夹、迁移、索引。
- `cpp/Favorites.h` / `cpp/Favorites.cpp`：`FavoriteFoldersModel`、`folderId`、`addToFolder`、`writeFavorite`。
- `cpp/AppModels.h`：`FavoriteFolders` 单例。
- `components/FavoriteFolderDialog.qml`（新）、`components/Options.qml`、`main.qml`。
- `pages/FavouritePage.qml`、`components/QListView.qml`、`layout/PlayerControl.qml`、`components/PlayList.qml`。

## 10.8 坑

1. **不要删除默认夹**：迁移与新增收藏都依赖「`type='fav'` 最小 id」。UI 已用 `isDefault` 挡住重命名/删除。
2. **别把 `folderId` 用在歌单收藏上**：playlist 行 `folder_id` 恒 NULL，过滤会得到空列表。
3. `favorites` 的 `folder_id` 是 `ALTER` 加的，**没有外键级联**；删夹时必须在 `FavoriteFoldersModel::deleteFolder` 里手动删收藏（已实现，单事务）。
4. 收藏夹增删后要重拉歌曲列表：只有 `deleteFolder` 会动 `favorites` 行，`mutate(..., true)` 才触发 `FavoritesModel::reloadAll()`；不要在 DB 线程调 `refreshModel()`。
5. 老库若已经存在一个 `type='fav'` 的行（极端情况），迁移会把它当默认夹，不再新建。
