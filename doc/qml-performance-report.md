# QueMusic 性能优化报告（qmltc 与 QML AOT 实战）

环境：Qt 6.10.3 开源版 + llvm-mingw 17.0.6（clang）+ CMake + Ninja
说明：本文所有结论均在本机实测（编译探针工程 + 真实工程构建 + 启动冒烟测试），不是文档推测。

---

## 一、总览

| 项目 | 状态 |
|---|---|
| QML 绑定 AOT 覆盖率（持续审计的 3 个文件） | **56.9% → 76.1%**（已编译 701 → 937 / 1231） |
| 构建优化 | LTO（`-flto=thin`）+ 函数分节 + `--gc-sections` + 资源前缀策略固定（已落地并验证进入命令行） |
| 上下文属性改造 | 8 个 `setContextProperty` → 8 个 QML 单例类型，161 处访问改为类型名访问 |
| qmltc | **本工程当前结构下无法生效**（5 条实测限制，见第四节）；已接入为可选开关，默认关闭且带安全降级 |
| 运行时验证 | 已做启动冒烟测试：程序正常启动、主界面加载成功、无新告警 |

---

## 二、已落地的改动

### 1. 构建层（`CMakeLists.txt`）

| 改动 | 作用 |
|---|---|
| `qt_policy(SET QTP0001 OLD)` | QML 里全是 `qrc:/QueMusic/...` 绝对 URL，显式固定资源前缀，避免后续 Qt 把默认前缀改成 `:/qt/qml/` 后全线失效 |
| `-ffunction-sections -fdata-sections` + `-Wl,--gc-sections` | 函数级分节 + 链接期回收未引用代码 |
| `INTERPROCEDURAL_OPTIMIZATION_RELEASE/RELWITHDEBINFO` | LTO。实测生效：编译命令含 `-flto=thin`，链接含 `-Wl,--gc-sections`；`check_ipo_supported()` 保护，不支持时自动降级 |
| `QUEMUSIC_ENABLE_LTO` / `QUEMUSIC_QML_LINT` / `QUEMUSIC_ENABLE_QMLTC` | 三个开关；Debug 构建自动跳过 LTO |

### 2. 用有类型的 QML 类型替换匿名内联对象（第一批 AOT 优化）

新增 5 个类型替换 `Style.qml` / `Options.qml` 里的匿名内联对象，**调用点零改动**：

| 新增文件 | 替换了什么 |
|---|---|
| `components/StyleThemes.qml` | `property QtObject themes: QtObject {...}` |
| `components/StyleSettings.qml` | `property Settings settings: Settings {...}` |
| `components/OptionsSettings.qml` | 内联 `Settings`（Options 组） |
| `components/OptionsLastSongs.qml` | 内联 `Settings`（LastMedia 组） |
| `components/OptionsShortCuts.qml` | 内联 `Settings`（ShortCuts 组） |

原理：qmlcachegen 解析内联对象时看到的是它声明的 C++ 类型（`QtObject` / `QQmlSettings`），无法解析具体属性便放弃编译；属性声明挪到有名字的 QML 类型上后即可解析，整条绑定被提前编译成 C++。

对照实测（同一构建、同一批文件）：

| 文件 | 改动前 | 改动后 |
|---|---|---|
| `main.qml` | 203 / 102 | 223 / 82 |
| `SettingsView.qml` | 441 / 393 | 624 / 210 |
| 合计覆盖率 | 56.9% | 73.4% |

> ⚠️ 踩坑记录：这些新类型若只 `import QtCore`，`property list<color>` 会在**运行时**报
> `color is not a type`（`color` 在文档根对象上需要 QtQuick/QtQml 导入；内联对象因为继承外层文件的导入所以原写法没问题）。
> 构建与 qmllint 都不会报这个错，只有真正运行才暴露。`StyleSettings.qml` 已补上 `import QtQuick`。

### 3. 上下文属性 → QML 单例（第二批 AOT 优化，本轮重点）

`main.cpp` 里 8 个 `setContextProperty` 全部改造成 QML 单例类型，共 161 处访问从裸名字改为类型名访问：

| 原上下文属性 | 现在的 QML 单例类型 | 访问处数 |
|---|---|---|
| `myFolderModel` | `MyFolders` | 8 |
| `localFolderModel` | `LocalFolders` | 8 |
| `songModel` | `Songs` | 33 |
| `favoritesSong` | `FavoriteSongs` | 49 |
| `favoritesList` | `FavoritePlaylists` | 21 |
| `favoritesArtist` | `FavoriteArtists` | 3 |
| `accountManager` | `AccountManager` | 31 |
| `logManager` | `LogManager` | 7 |

实现：
- 新增 `cpp/AppModels.h`：6 个模型单例（各自的 `create()` 里设置过滤类型）
- `AccountManager` / `LogManager`：加 `QML_ELEMENT + QML_SINGLETON + create()`（`main.cpp` 提前调用一次，保持账号与日志的初始化时序不变）
- `FolderModel` / `SongModel` / `FavoritesModel`：加 `QML_ANONYMOUS`，让编译器能看到继承来的属性
- 模型改为由引擎在首次访问时创建（不再在 `main.cpp` 里 eager 创建）

**为什么必须"一个对象一个单例类型"，而不是"一个单例持有多个模型属性"**（关键实测）：

我最初写的是 `AppEnv.songs.count` 这种写法，结果编译器报
`Cannot use shadowable base type for further lookups`。
用探针工程对 4 种 C++ 注册方式（`QML_ANONYMOUS` / `QML_UNCREATABLE` / `QML_ELEMENT` / `QML_SINGLETON`）做了对照，
只要是"**属性 → 对象 → 属性**"的访问路径就全部失败；只有直接按类型名访问的单例（`RegSingleton.value`）能编译。
这也正好解释了为什么工程里 `MusicApi`（QML_SINGLETON）的 437 处访问一直是成功编译的。

实测效果：`SettingsView.qml` 的 `Cannot access value for name accountManager` / `shadowable base type` 类失败**全部消失**，
该文件已编译数 626 → 653。

### 4. JS 函数类型注解

给调用点可控且语义明确的函数补上注解（无参函数用 `: void`）：

- `main.qml`：`doSearch(text: string)`、`playLocalSong(path: string, name: string)`、`startTrack(index: int)`、
  `saveQueue(): void`、`restoreSession(): void`、`noteNowPlaying(): void`、`updateSmtcControls(): void`、
  `smtcUpdateMediaInfo(): void`、`toClosing(): void`、`silentUpdateCheck(): void`、
  `openSimpleDialog(title: string, text: string, callBack: var)`、`dialog(...)`
- 信号处理器按 C++ 信号签名补注解：`onUrlplay(...)`、`onWarned(text: string, type: int)`、
  `onLocalLyricsReady(...)`、`onLocalLyricsFailed(filePath: string)`、`onLocalMetadataReady(...)`、`onPlayIndex(index: int)`
- `SettingsView.qml`：`globalPropertyName(actionName: string)`、`keyToString(key: int)`、
  `startRecording(action: string)`、`stopRecording(success: bool, sequence: string)`

效果：`Functions without type annotations won't be compiled` 从 30 处降到 13 处（剩余集中在 `FullCenterView.qml`，
它们的入参是模型行数据，只能标成 `var`，标了也编译不了 —— 见第五节）。

---

## 三、最终实测数据

| 文件 | 会话开始 | 现在 | 变化 |
|---|---|---|---|
| `main.qml` | 223 / 82 | 227 / 78 | +4 |
| `SettingsView.qml` | 626 / 208 | 653 / 181 | **+27** |
| `FullCenterView.qml` | 57 / 35 | 57 / 35 | 持平 |
| 合计 | 904 / 1231（73.4%） | **937 / 1231（76.1%）** | **+33 条绑定/函数转为 C++** |

配合第一批优化，整体从最初 **56.9% → 76.1%（+19.2 个百分点，+236 条）**。

已验证：`cmake --build` 全绿；启动冒烟测试通过（程序存活、主界面加载成功）。

> ⚠️ **修正（重要）**：改造过程中启动日志出现过
> `Shared database not available!` / `Database not open!` / `Driver not loaded` / `Refresh favorites failed`，
> 当时被我误判为"环境问题（插件解析）"。真正原因就是这次改造本身：
> 连接 `shared_player_db` 原先只由 `FolderModel` 的静态函数**创建**，而 `FavoritesModel` 只做**查找**、
> 并把查找结果缓存进函数内静态变量。模型改成 QML 单例后由引擎按首次访问顺序惰性创建，
> 一旦收藏模型先于歌单模型被访问，它就会**永久缓存一个无效连接**，表现为收藏歌曲/歌单/歌手列表为空。
>
> 修复：新增 `cpp/PlayerDatabase.{h,cpp}`，把"创建连接 + 建表 + 打开"收敛到唯一的 `playerDatabase()` 入口，
> `FolderModel` / `SongModel` / `FavoritesModel` 全部改为调用它，从此与构造顺序无关（数据库文件与数据未受影响）。

---

## 四、qmltc 的 5 条实测限制（决定它为什么在本工程用不了）

1. **只能编译「元素类型属于本模块或纯 C++ 模块」的文档。**
   用到其它模块中由 QML 定义的类型就报错：
   `Can't compile the QML base type "TextField" to C++ because it lives in "QtQuick.Controls.Basic" instead of the current file's "QueMusic" QML module.`
   工程 78 个 QML 里有 **56 个**依赖 `QtQuick.Controls.*`（整个 UI 基于 Controls）。
   （`QtQuick.Effects` / `QtQuick.Layouts` / `QtQuick.Shapes` 是纯 C++ 模块，不构成阻塞。）

2. **被引用到的本模块 QML 类型必须一起编译**（生成的 C++ 类互相继承）→ 只能取自洽闭包，实测仅 22 个叶子组件。

3. **【关键】qmltc 无法处理模块子目录中的文档。**
   实测把引用方文件放进子目录后，连同模块的 QML 类型与 C++ `QML_ELEMENT` 类型都解析失败（`lives in ""`）；
   放回模块根目录即恢复正常。而可编译的那批组件全在 `components/`、`centers/` 下。

4. **运行时收益只在 C++ 侧直接实例化生成类时出现。**
   实测加载被编译过的文档，拿到的对象类名是 `Dyn_QMLTYPE_0`（走 QML 源码）；
   只有 `new QueMusic::Dyn(&engine, ...)` 才得到编译产物。`engine.load()` / `Loader{source:...}` 都不会自动使用编译产物。

5. 生成的类不能再暴露给 QML，且不保证跨版本二进制兼容；qmltc 失败会直接中断构建。

> 结论：qmltc 适合「整棵 QML 树可编译 + 由 C++ 创建根节点」的场景。
> 对以 `QtQuick.Controls` 为主、大量使用 `Loader` 的音乐播放器 UI，改造代价远大于收益。
> 已接入为 `-DQUEMUSIC_ENABLE_QMLTC=ON`（默认关闭，布局不满足时只警告不失败），详见 `cmake/qmltc.cmake`。

---

## 五、剩余未编译项与原因（按数量排序）

| 数量 | 原因 | 能否解决 |
|---|---|---|
| 93 | `Cannot generate efficient code for internal conversion ... QColor (stored as QVariant)` | **Qt 编译器限制**：`Style.themes.primaryColor` 这类「两级访问取颜色」无法编译；实测一级访问（`Rectangle { color: ThemeTyped.c }`）可以编译，但 `property alias` 转发无效（`Cannot load property xxx from Style`）。要吃掉它只能把热用颜色平铺成 `Style` 的一级属性并批量改调用点 |
| 20 | `storing an array in a non-sequence type` | 例如 `model: [ ... ]` 直接写字面量数组；需改成 `property list<T>` 再引用。收益低（非热路径） |
| 16 | `call to untyped JavaScript function` | 调用的是仍未编译的函数（含 `FullCenterView` 那批 `var` 入参函数） |
| 13 | `Functions without type annotations won't be compiled` | 集中在 `FullCenterView.qml`：入参是模型行数据，只能标 `var`，标了仍无法编译（需要对行数据做类型化，工作量大） |
| 11 | `Cannot retrieve a non-object type by ID: windowAgent` | `windowAgent` 来自 QWindowKit 的 **QML 插件**；文档明确说明插件提供的类型无法参与编译期解析 |
| 其余 | 各类零散限制 | 逐个收益很小 |

结论：剩下的大头是 Qt 编译器自身限制（颜色两级访问、插件类型），不是写法问题；
继续提升需要"重构 UI 数据结构"级别的改动，建议到此为止。

---

## 六、最终一轮：渲染 / 执行效率 / 稳定性

| 改动 | 说明 |
|---|---|
| `layout/PlayerMaxCenter.qml`：删除 60ms 轮询 Timer | 歌词时间改为直接跟随 `mainMedia.position`（位置本身按音频后端节奏更新）。原先只要加载了媒体就每秒触发 16.7 次 JS 回调去复制一个并不会更快变化的值 —— 纯开销。现在事件驱动，暂停时零回调 |
| `cpp/LogManager`：日志预览合并刷新 | 原先**每写一条日志**都做一次最多 500 行的字符串拼接 + `emit logPreviewChanged()`（会触发 QML 侧重算）。改为：警告及以上立即刷新，其余合并到 300ms 静默后刷新一次（新增单次 `QTimer` 合并器） |
| `cpp/LogManager`：按级别刷盘 | 原先每条日志都 `flush()`（一次 I/O 系统调用）。现在只有警告及以上立即刷盘，其余交给 QFile 缓冲（QFileSize 满或退出时落盘） |
| `components/QDrop.qml`：修真实缺陷 | ① `property string text: model[choice]` 把 `QAudioDevice` 赋给 `string`，运行时报 `Unable to assign QAudioDevice to QString`；② `model[choice].description` 在设备列表为空时会抛 TypeError。已改为类型安全的取值（对象优先 `description`）。修复后启动日志中**再无任何 QML 类型/绑定告警** |
| 死注释代码清理 | 41 个文件、共 134 行被注释掉的旧代码（`//property ...`、`//layer.enabled: true`、`// 旧配色 ...` 等）已删除；`TODO/FIXME/说明性注释` 一律保留。副作用：这些文件末尾补了缺失的换行（每文件 +1/-1 行，符合 POSIX 习惯） |

复核后**刻意未改**的部分（避免"为优化而优化"引入回归）：

- `MeshGradientItem` 的 16ms 刷新已有 `m_animating && isVisible() && window()` 门控，隐藏时自动停止；
- 取色路径是固定 48×48 采样；音频频谱 FFT 已放线程池且缓冲区预分配 —— 两条计算链本身没有浪费；
- 歌词/封面的 `layer.enabled + ShaderEffect` 就是该视觉效果本身（渐变淡出、圆角遮罩），去掉会改变外观；
- `QSG_USE_SIMPLE_ANIMATION_DRIVER` 的默认值来自设置页"使用 vsync 动画引擎"的语义，未擅自翻转。

验证：`cmake --build` 全绿；启动冒烟测试通过（进程存活、主界面加载成功）；**启动日志无类型/绑定告警**；日志文件正常写入 `%APPDATA%/BroNekoX/QueMusic/logs`。

---

## 七、如何自行验证

```powershell
# 1) 配置与构建（Release 自动启用 LTO）
cmake --preset win-llvm-mingw-release
cmake --build build/win-llvm-mingw-release -j 8

# 2) 确认 LTO / 分节标志进入命令行
Select-String -Path build/*/CMakeFiles/QueMusic.dir/flags.make -Pattern "flto"

# 3) 统计 AOT 覆盖率（codegenResult=0 即已编译成 C++；message 字段写明失败原因）
$d = "build/<preset>/.rcc/qmlcache"
Get-ChildItem $d -Filter *.aotstats | ForEach-Object {
  $j = (Get-Content $_.FullName -Raw | ConvertFrom-Json)
  $e = @($j.modules.moduleFiles.entries)
  $ok = @($e | Where-Object { $_.codegenResult -eq 0 }).Count
  "{0,-30} 已编译={1,-5} 未编译={2}" -f $_.Name, $ok, ($e.Count - $ok)
}

# 4) 启动冒烟测试（改 QML 后务必跑，构建通过 ≠ 运行正常）
Start-Process build/<preset>/bin/QueMusic.exe; Start-Sleep 8
```

常用开关：
- `-DQUEMUSIC_QML_LINT=OFF`：构建时跳过 qmllint（只影响构建耗时）
- `-DQUEMUSIC_ENABLE_LTO=OFF`：排查链接/优化问题时临时关闭 LTO
- `-DQUEMUSIC_ENABLE_QMLTC=ON`：尝试启用 qmltc（当前布局下会打印说明并安全跳过）
