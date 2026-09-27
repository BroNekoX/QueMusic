# Changelog

本项目所有重要的变更都会记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### 🧩 重构
- **歌词界面模块化**：`layout/PlayerMaxCenter.qml`（873 行）拆为宿主框架（269 行：沉浸偏移、控件自动隐藏、
  控制按钮、主题色动画）+ 歌词界面模块 `layout/MainLyric.qml`（649 行：流体背景 / 封面 / 歌曲文本 / 歌词内容）。
  宿主用 `Loader` + `sourceComponent` 声明式注入 `position`/`playbackRate`/`playing`/`mediaActive`/`lyricsModel`/
  `translateModel`/`title`/`artist`/`coverUrl`/三主色/`hideHeight`/`lyricSize`，模块只读这些属性、不反向依赖宿主实现
  （为后续"可替换的歌词界面插件"预留同一套契约）
- **按需加载**：`Loader.active: musicControlMax.visible` ⇒ 沉浸页不可见时整体卸载，320ms 歌词定位定时器与
  各动画/着色器随之停止（原先仅在定时器 `running` 上判 `visible`，卸载后无任何空转）
- 样式弹窗保留在宿主 `PlayerMaxCenter`（它同时控制壳层沉浸模式与模块的封面/主题模式、位置校准）；
  封面模式/主题模式/位置校准/翻译开关等界面状态**由宿主持有**并注入模块，避免 `Loader` 卸载重建后状态丢失
  （主题模式切换的位移动画改由模块在 `lyricType` 变化时自行触发）
- 顺带修正：歌词换源重排改由模块内 `Connections` 触发（原绑定 `MusicApi.lyricsDataChanged`），
  避免创建期属性初始化提前触发；删除两处调试 `console.log`
- **歌词界面可整主题替换**：`Loader` 由 `sourceComponent` 改为 `source`，绑定 `lyricThemes` 列表条目
  ⇒ 换主题 = 换加载的文件（新增「播放器主题」弹窗作为入口，后续插件在 `lyricThemes` 中注册即可）
- **数据注入改为运行时 `inject(item)`**（`onLoaded` 中用 `Qt.binding` 注入 position/lyricsModel/currentIndex 等），
  模块属性只读；样式类改动一律经白名单 `requestStyle(key, value)` → `applyStyleRequest()` 请求宿主处理，
  模块不直接改宿主状态（为插件预留受控写入口）
- **样式白名单收口**：`applyStyleRequest` 改为 `styleAllow` 登记表（11 个键，数值带 `[min,max]` 钳制与
  NaN 守卫，未登记的键一律忽略）；此前「默认/自由/视频壁纸」主题里 12 处**直接写 `Style.settings`** 的代码
  全部改走 `request(key, value)`，并给「视频壁纸」补齐了缺失的 `requestStyle` 契约
- **宿主控件去重**：`PlayerMaxCenter` 顶部三个同款 `SButton` 抽成内联组件 `MaxButton`（样式单一来源）
- **320ms 歌词定位定时器上移到宿主**：宿主遍历列表按播放进度算出 `lyricIndex` 并注入，再调用模块的
  `timerFunction()` 刷新列表位置与等待动画；模块内定时器已移除，页面不可见时宿主定时器不运行
- **「播放器样式」弹窗拆分**：`标准歌词大小`/`歌词位置校准`/`自动进入沉浸模式` 为常驻项；其余项由当前歌词
  界面模块经 `styleOptions` 组件提供（`MainLyric` 提供 封面模式/主题模式/弹簧动画/音波），换模块即换这部分，
  未提供则不显示；这些项按需实例化（`Loader.active` 跟随弹窗可见性），弹窗关闭时不建对象
- **「播放器主题」弹窗改用 `GridView`**：主题数量不定，网格自适应（1 个占满整行、多个两列），选中态用
  `Style.themes.themeColor`；不启用内层滚动（滚动交给 `QOptionDialog` 自带的 `Flickable`，避免嵌套滚动冲突）
- **函数补全类型标注**（`function(name: type): type`）并简化：`inject()` 改为键值表 + `k in it` 判存（缺属性自动
  跳过，便于直接换插件）；`tick()` 拆出 `checkWaitAnime()`、`springValue` 用 `Math.max` 收口、循环内边界值提出复用；
  `animeTo()` 去掉未使用的 `instant` 参数
- **歌词界面主题 4 种（`lyricsui/`）**：与 `MainLyric` 同一份注入契约（只声明自己用到的属性即可），
  在「播放器主题」里切换
  - 默认：`layout/MainLyric.qml`（保持原样）
  - 极简 `lyricsui/LyricsSimple.qml`：布局与默认一致，去掉封面阴影与全部复杂动画（逐行弹簧 / 等待 / 逐字），
    歌词改为普通 `ListView` 由 `contentY` 直接驱动
  - 自由 `lyricsui/LyricsFree.qml`：原版 + 大量自定义项（歌词间隔、歌词卡片**翻转**、当前行放大、
    非当前行透明度、逐字唱后颜色、唱行颜色），全部写入 `Style.settings` 持久化，颜色用 `ColorPickerDialog` 选取；
    卡片绕竖轴翻转（类似 `Flipable`），统一向左、幅度随离界面中心的距离增大
  - 3d `lyricsui/Lyrics3D.qml`：背景是 `shaders/cubes.frag` 的方块场，**相机完全由鼠标驱动、不自转**
    （`HoverHandler` 跟踪、不吞点击）；天空为**彩色双色调星云**（fbm）+ 两层视差星空 + 上浮星火粒子，
    均随音频增亮 —— 修掉原先空白处一片黑
  - **3d 主题三层结构**：① 背景着色器 `shaders/bg_*.frag`（方块场 / 光栅地平 / 光棱 / 近黑底，按预设换
    `fragmentShader`）② **点云舞台（新增 C++）** ③ `lyricplane.frag` 歌词平面（负责发光）
  - **点云舞台 = 真 3D 点云**（与 MineRadio 同一套做法）：新增 `shaders/LyricsStage.{h,cpp}` +
    `shaders/stage.vert/frag`。14000 个发光点的**位置 / 大小 / 亮度全部由顶点着色器在 GPU 算出**，
    按深度做尺寸与亮度衰减 ⇒ 真景深真视差；片元输出 `alpha=0 + 预乘颜色` ⇒ 在 Qt 预乘混合下等效
    **加色发光**。与背景共用同一套相机（yaw/pitch/lookAt + kFocal=1.9）⇒ 点和方块场在同一个三维空间里。
    分布 4 种：星尘（体积云）/ 光隧（圆管流动）/ 星环（球面 + 赤道细环）/ 舞台（地面圆盘 + 上升光柱）
  - **修复歌词视角左右半边相反**：倾斜量原先加在**世界坐标系**，相机转到另一半时同一个偏移变成反向 ⇒
    改为加在**相机坐标系**（cb[0]=右 / cb[1]=上），左右一致
  - **当前行发光改为"字体发光"**：去掉背后的矩形光晕条，也去掉滚动扫过带/已唱提亮（观感像"叠了层渐变
    在后面滚"）⇒ 整行**按字形**发光：着色器沿字形轮廓加强近圈晕 + 字形自发光，按 `uLineV` 只作用当前行，
    面板项更名「当前行发光」；行距 ×1.45，当前行放大到 1.22、其它行缩小 ⇒ 层次分明
  - **点云粒子放大**：点半径原先算出只有 ~0.2 像素（`0.25% × 屏高/深度`）⇒ 改为 `5.5%~10% × 屏高/深度`
    （深度 9 时约 5~7 像素，近处更大），透视尺寸衰减保留
  - **歌词上下渐隐**：纹理边缘在 3D 倾斜下会被拉伸 ⇒ 着色器对顶部/底部 12% 做 alpha 渐隐，直接隐去
  - **"糊"的两个根因已修**：① 歌词纹理原先走 mip 采样（字形被 mip 化）⇒ 改为**固定 LOD 0** 采样且
    `ShaderEffectSource.mipmap: false`；② 发光原先用 `textureLod(...,3.0)` 当模糊（mip 层可能不存在 ⇒
    字根本不发光）⇒ 改为**两圈 8 向明采样**累加 alpha：近圈紧贴字形、远圈做大范围晕，锐利不糊，
    字形本身也加自发光
  - 配色统一为**近黑底 + 加色发光 + 高饱和**（饱和度提升 + 亮度地板 + tonemap），方块场恢复可见
  - 自定义入口移到**翻译按钮左边**（同一行、同尺寸），面板保持**卡片弹出**（预设网格 + 14 个滑杆 +
    2 个取色器），与宿主「播放器样式」弹窗共用同一份 `styleOptions`
  - 许可：预设与特效思路参考 MineRadio（GPL-3.0-only），**未移植其任何代码**，全部原创实现（本项目 Apache-2.0）
  - **许可说明**：3d 背景预设与歌词特效的**思路**参考 MineRadio（GPL-3.0-only），但**未移植其任何代码**，
    全部为本项目原创实现（本项目为 Apache-2.0）
  - **歌词改用真实透视**：QML 的 `Rotation`/`Matrix4x4` 都是仿射（实测带 `m34` 透视项会直接塌成一条线），
    故歌词先平铺进 `ShaderEffectSource`，再由同一个着色器投影成场景中的一块全息平面；平面法线始终朝向
    相机再叠加鼠标倾斜（0.46 / 0.30 rad）⇒ 任何鼠标位置都读得到字，又能看出真的立体翻转
  - 频谱注入改用**标量 uniform**（`uLow/uMid/uHigh/uAir/uLevel`）：Qt 对 `float uWave[64]` 这类从
    JS 数组绑定的数组 uniform 不保证生效（方块不跳动的根因），标量必然生效且更省；
    同时修正着色器的纹理坐标 y 方向 —— 原先地面被渲染到屏幕上方
  - 移除曾压住最下面歌词行的下半部分 3D 音波层（背景已有地面方块场，且少一次全屏 pass）；
    无歌词（未播放）时 `ShaderEffectSource.live` 为 false，不逐帧离屏渲染
  - `inject()` 对缺省属性加守卫（`if ("requestStyle" in it)` 等），避免换插件/预设时刷
    `Cannot assign to non-existent property` 告警
- **「自由」颜色修正**：`唱行颜色` 原先只作用在普通 `Text` 上，而逐字歌词实际由 `CustomFlow` 渲染
  （`lyricFlowText`）⇒ 看起来"没颜色"；现在同时作用于 `lyricFlowText`
- **「极简」→「视频壁纸」**：歌词样式保持极简（`ListView` + `contentY` 直接驱动），背景改为循环播放的
  视频（`Video` + `autoPlay` + 静音 + `PreserveAspectCrop`），未设置时回退流体背景；「播放器样式」新增
  「视频壁纸」项（`FileDialog` 选择 / 清除），路径存 `Style.settings.lyricWallpaper`。
  注：Qt 6 的 `Video` 没有 `playing`/`volume` 属性、`playbackState` 只读 ⇒ 播放控制只用 `autoPlay`
- **「3d」背景改为着色器**：新增 `shaders/cubes.frag`（原创实现）——高度场步进的 3D 方块场，方块高度按
  频谱低/中/中高/高四段电平起伏（标量 uniform）；`shaders/wave3d.frag` 已删除（会压住歌词行，
  文件与 CMake 登记一并移除）
- **「视频壁纸」主题并入「自由」**：删除 `lyricsui/LyricsVideoWall.qml`（宿主主题列表 4 → 3），
  自由模式新增「背景样式」四选一 —— 流体 / 图片 / 视频 / 星空（星空复用 `bg_stars.frag`，`uMouse` 固定 0
  ⇒ 不受鼠标影响），图片与视频各带选择 / 清除（`FileDialog`）
- **各主题设置改为模块自持**：`lyricsui/LyricsFree.qml` 内的 `Settings`（category "LyricsFree"）持有
  `bgStyle/bgImage/bgVideo` + 原 6 项（`lyricSpacingScale` 等，读取与写入 21 处机械迁移）；写入由
  `request(key, value)` 改为直接写 `cfg`；`StyleSettings.qml` 与宿主白名单里对应的死键已删除，
  白名单只保留「宿主状态 + 跨主题共用」的 `basicCd/lyricType/premiumLyricAnime/waveDisplay`
- **「自由」卡片翻转改为整块翻**：原先是逐行各自按距离算角度（观感假）⇒ 改为在 `lyricContent` 上加
  单个绕竖轴旋转（`angle: -cfg.lyricCardAngle`，320ms 过渡），整块歌词统一向左翻
- **死代码清理**：删除 `lyricplane.frag` 中声明但从未使用的 `uColor`（着色器与 QML 两侧同步删除，
  16/16 uniform 仍逐个对齐）；删除已无引用的 `lyricWallpaper` 设置键
  **注**：`example/cubescape.frag`（Inigo Quilez 作品）授权明确禁止用于任何产品，项目未采用其代码
  - 「播放器主题」由 `GridView` 改为 `Flow + Repeater`（不额外引入滚动容器，滚动交给弹窗自身）；
    `components/StyleSettings.qml` 新增 6 个歌词自定义项

- **注释清理**：`CMakeLists.txt` 注释 30 → 20 行（构建开关 / LTO·gc-sections / QML 收集·qmltc / Windows 库·
  QWK 拷贝 / FFmpeg 部署 / 部署清理 六处只留关键事实，并修正一处 `endif()` 缩进）；`main.cpp` 去掉口语化
  说明、把"为什么"改成约束（如渲染后端开关须在 `QApplication` 之前设环境变量）；删除 7 处**被注释掉的
  死代码**（PlayerOptions / PlayList / QOptionDialog / MainContent / PlayerControl / HomePage / Style）；
  `cpp/AppModels.h` 头说明 10 → 7 行（保留"为何用单例"与"构造函数陷阱"两个关键点）、
  `cpp/LocalLyricsReader.h` 中英混杂注释统一为中文
- **内嵌歌词解析修正**：音频标签同时有逐音节 `SYLT` 与行级 `USLT/LYRICS` 时，原先命中 `SYLT` 就返回 ⇒
  每行只有一个字、且丢掉 `USLT` 里的翻译。改为**行级歌词优先**（保留原行与翻译配对），再把 `SYLT` 音节
  按时间贴回各行 ⇒ 逐字与翻译同时保留；仅当没有行级歌词时才用 `SYLT` 按"停顿"分组重建显示行
  （间隔自适应、行内音节进 `info`）。同名歌词文件除 `.lrc` 外也接受 `.txt`

### ⚡ 优化
- **音频回调内零分配**：`applyPendingParams()` 原在参数变化时构造 `QList<qreal>`（拖 EQ 滑块即每次回调都分配
  内存）。新增 `AudioDsp::setEqGains(const double *, int)` 直读快照数组，音频线程不再触碰容器分配
- **部署精简（−113MB）**：解码层改用 Qt 自带的 FFmpeg 7.1.3（`avcodec-61`/`avformat-61`/`avutil-59`/
  `swresample-5`，合计约 19MB），替代原先 8.x 全量包的 111MB；这四个库由解码层直接链接，**必须随应用保留**
- **删除未被使用的 `multimedia/ffmpegmediaplugin.dll` + `swscale-8.dll`**（约 1.7MB）：全项目已无
  `MediaPlayer` 声明，实测移走后 14 项音频测试全过、应用正常启动；`windowsmediaplugin` 保留。
  **注意**：Qt 的 `qt_deploy_runtime` 只在上述插件存在时才顺带部署 `avcodec-61` 等库，插件删除后这批
  DLL 只能由项目自带，故 `ffmpeg/bin` 缺失时 CMake 直接 `FATAL_ERROR`（原先静默跳过，会导致能启动但无法播放）

### 🐞 修复
- **切歌闪退（0xC0000005 @ `Qt6Core!QMetaObject::indexOfProperty`）【根因已定位并修复】**：崩溃是按名字
  查属性时拿到空 `metaObject`，栈为 `Qt6Core!QMetaObject::indexOfProperty(NULL)` ←
  `Qt6Qml!AOTCompiledContext::initGetValueLookup`。**根因只有一行，在 `main.qml` 的 `startTrack()`**：
  `if (!e || e.path === undefined) return` —— `e` 是队列条目（非布尔值）却用 `!e` 取反，又用 `===`
  比较没有类型的 `undefined`，AOT 编译下这会让紧随其后的属性查找推导出空 `metaObject` 而直接崩
  （解释执行不走这条查找，所以关闭 AOT 就不崩）。改为 `if (e.path == undefined) return`
  （`==` 同样覆盖 null，且 AOT 安全）后，**AOT（qmlcachegen）保持开启也不再闪退，无需任何环境变量**。
  该行由 `Playback` 的 `playIndex` 信号 → `Connections.onPlayIndex` → `startTrack` 这条切歌链路触发，
  故表现为按「上一首/下一首」时闪退、播放列表点击正常
- **同类隐患写法（未在本轮触发，建议后续留意）**：`layout/PlayerMaxCenter.qml:375-378,661,663`、
  `FullCenterView.qml:108`、`components/QDrop.qml:19,60,139` 使用了 `X !== undefined` 判断；
  与上述根因同源，在 AOT 下同样有推导出空 `metaObject` 的风险，建议统一改为 `== undefined` 形式
- **切歌级联中的 SMTC 重入、`musicControlMin` 跨作用域引用**：两者都是真实缺陷，但**均非本次闪退的
  根因**（各自修复后崩溃仍复现）。已一并修正：`musicControlMin` 的跨文件引用统一走 `Playback` 单例；
  原生 SMTC 推送合并进 `pushSmtc()` 并由 `Qt.callLater` 延后，`onSourceChanged` 中的重复推送已清除
- **`Playback.qml` 切歌链路整理**：`next/previous` 不再各自调用 `notePlayed`，统一由 `goTo()` 记录
  （此前播放列表点击走的是 `goTo`、不记录，导致真随机的「最近播放」避让在两条路径上不一致）；
  链路本身保持 `next/previous → goTo → playIndex 信号 → Connections → startTrack` 不变
- **上一首/下一首闪退（自动跳歌死循环）**：`onErrorOccurred` 里任何一次播放失败都会自动跳下一首，
  且没有次数上限——队列里存在失效条目（文件被移走/在线源失效）时，会陷入「失败→跳→失败→跳」的
  高速循环，每轮都在重建音频输出，最终闪退。点播放列表能正常播是因为点的就是看得见、能播的那首，
  按队列顺序走的上一首/下一首才会撞上坏条目。现在连续失败 2 次即停止并提示，成功起播后清零
- **`startTrack` 的越界判断失效**：`QueueModel::get()` 越界返回空 `QVariantMap`，而空对象在 QML 中
  是真值，`if (!e) return` 拦不住，会带着 `undefined` 走在线分支。改为按字段判断 `e.path`
- **切歌闪退（0xC0000005）**：`reconfigureOutput()` 在 GUI 线程重建缓冲区时会 `resize`/`configure`
  音频线程正在写入的 `m_scratch` / `m_stretchIn` / 环形缓冲——旧内存被释放而音频线程仍持有指针，
  属于堆破坏。现在缓冲一律在输出线程内重建（与 `render()` 同线程，天然互斥），解码线程先停靠再重建；
  并且重建不再销毁供 sink 拉取的 QIODevice（Qt 音频后端可能在销毁后仍投递一次读取，导致空指针解引用）；
  `stateChanged` 回调增加 sender 校验，避免切歌时把旧 sink 的延迟信号误判为设备故障而触发重建。
  新增 `reconfigureWhilePlaying` 回归测试（播放中反复重建输出 + 切歌）
- **窗口缩放导致音频停顿/消失**：实测确认 Windows 上 `QAudioSink` 拉模式的 `readData` 回调跑在
  **GUI 线程**（探针实测 `render thread = "Qt mainThread", sameAsGui = true`），任何界面卡顿都会
  直接饿死音频输出。现在 sink 在专用输出线程（`QueMusicAudioOutput`）内创建，拉取定时器随之归属
  该线程，音频渲染彻底脱离 GUI（复测 `sameAsGui = false`）。新增回归测试锁死线程归属
- **切歌/上一首/下一首闪退**：`QAudioSink::stateChanged` 由音频线程发出，回调里直接改了 QML 状态并发
  `mediaStatusChanged`，导致 QML 处理器在音频线程上执行（崩溃定位：`Qt6Core.dll+0xb8b4c`，`0xc0000005`，
  三次现场偏移完全一致）。现在音频线程只置原子标志，状态变更统一回引擎线程处理
- **改变窗口宽度导致音频一顿一顿**：频谱通路用互斥量在音频回调与渲染线程之间传数据，窗口 resize 时渲染
  线程变慢会把音频线程卡在锁上造成欠载。改为无锁环形缓冲，音频回调全程不取锁
- 音高/倍速联动错误：重采样读指针压实缓冲时多减了一次基准偏移，导致读位置回退、时长被拉长约 2 倍
- **拖动进度条时位置回闪**：连续拖动会排队多个 seek，旧请求落地时 `beginGeneration()` 无条件把位置回写成
  该旧请求的时间点，覆盖掉 GUI 侧刚发布的目标位置。现在挂起 seek 期间只有落到目标段才回写位置；
  另修复无人执行 seek（源未就绪/打开失败）时挂起标记无人清除、进度条永久停住的问题
- **逐字歌词首行整行重叠**：`CustomFlow` 只在自己尺寸或子项增删时重排，子项（逐字文本）宽高由字体与字号
  稍后算出的情况不会触发重排，首帧按 0 尺寸摊平后一直保持重叠。现在跟踪子项的宽高与可见性变化并重排，
  同时不再把 `Repeater` 本身当作参与流式排布的内容
- **收藏 / 本地排序列表的操作全部失效**：列表按 ListModel 约定用 `model.get(行)` 取行，而 `QSortModel`
  （`SortFilterProxyModel`）没有该方法，抛 `TypeError: Property 'get' … is not a function`。现在代理按源模型的
  接口取行（`Favorites` / `SongModel` 走 `get(row)`，`SearchResultModel` 走 `getRow(row)`，并用 `mapToSource`
  把代理行号换回源行号）；另修正收藏行字段：`get()` 返回的键是 `id`（只有 delegate 的 role 名叫 `favId`），
  原先按 `favId` 取值恒为 `undefined`，收藏列表的播放/详情/加入列表因此拿不到 key

### 🔧 变更
- **在线列表统一（`QListView`）**：收藏 / 加入播放列表 / 下一首播放 / 下载 / 歌曲与歌单信息全部下沉到组件内部
  —— 右键菜单歌曲 5 项（下载、下一首播放、添加到列表、收藏、歌曲信息）、歌单 2 项（收藏、歌单信息），
  行尾 3 个按钮统一为「更多 / 收藏 / 加入列表（歌单信息）」，信息弹窗用组件内置的 `QOptionDialog`；
  页面只保留 `onClicked`（播放或打开详情）与 `onEnded`（分页），删除了散落在 7 个页面中的重复实现
  （SearchPage 7 处 / PlaylistPage 4 处 / HomePage 4 处 / FavouritePage 6 处 / DownloadPage 1 处）；
  收藏图标只在悬停时查询，滚动时不再逐行访问数据库
- 音量与淡入淡出下沉到音频线程逐样本完成（`AudioDsp::processVolume`），不再由 QML 动画在 GUI 线程
  每帧调 `QAudioSink::setVolume`（那是 COM 调用，GUI 忙时会抖动）
- 切歌过渡：淡出在 C++ 完成，淡入在**新数据段真正出声的第一批样本**上开始推进；每次切段另有 12ms
  保护性淡入消除爆音
- 结束不再关闭输出设备（保持常开），下一首起播没有设备重开的空档
- 播放缓冲限制在 **10–100 ms**（默认 30 ms）；UI 滑杆同步收窄
- 频谱波形 `GetWave`：音频线程只降混写入环形历史，渲染线程仍跟 `frameSwapped` 逐帧取最近 4096 帧做 FFT。
  波形路径改为双缓冲换手，去掉未使用的 `spectrumData` 属性与整段计算持有的大锁
- 频谱观感：段间三点混合 + 上升快/下降慢的非对称平滑，柱子不再逐帧乱抖；dB 映射放宽到 −62~−6 dB，低位更饱满。
  频段边界改为按比例递推，每帧的 `exp` 从 128 次降到 2 次
- `AudioDsp` 新增旁路开关：总开关关闭时跳过均衡/声道/限幅，但音量与淡变仍生效

### ✨ 新增
- **沉浸中心翻新（FullCenterView + centers/）**：Aurora 背景（封面主色驱动的三束漂移光带 + 可读性蒙层，
  随歌换色）、Liquid Glass 播放坞/队列抽屉/详情页（实时模糊 + 顶部高光缝 + 微描边 + 玻璃徽章）、
  folia 式 Hero 大标题排版与统一分区标题（主色短线 + 加宽字距）；窗口控制补上最小化功能
- **沉浸中心二轮（Shapes 极光 + 功能修复）**：背景升级为 `QtQuick.Shapes` 真径向光团 + 双色极光丝带；
  修复歌单详情/播放队列列表高度溢出导致的底部显示不全；播放坞新增播放呼吸光晕与跳动的"正在播放"角标，
  选中页签加主题微光、封面卡悬浮投影；队列/详情补滚动条与空状态提示
- **沉浸中心四轮（空白页根因）**：`Flickable.availableWidth` 无 NOTIFY，构造期为 0 后绑定不再更新，
  首页/分类页内容整块 0 宽不可见（改用 `scroll.width`）；Window 直接子项改显式宽高绑定
  （`anchors.fill: parent` 会因 contentItem 就绪前尺寸为 0 而把整条布局链锁死）；
  `PathAngleArc` 误用不存在的 `radius` 致极光光团完全不渲染（改 `radiusX`/`radiusY`）；
  `CenterLocalPage` 缺 `import 'qrc:/QueMusic/components'` 使 `Style` 未定义、卡片底色失效并导致文字不可见；
  `enter()` 补发一次 `Style.changeTheme()`（色板仅在 darkis 变化时刷新，启动即深色会停在浅色默认值）；
  各列表补 `Style.themes.primaryColor` 卡片背景
- **沉浸中心三轮（分页 + 兼容修复）**：修复 Center 全部列表不接 `onEnded` 分页导致的数据只有第一页
  （搜索/新歌/歌单详情/榜单/歌手现已滚动自动加载下一页，`CenterDetail` 用 `loadMore` 闭包注入）；
  极光丝带改用闭合波浪带 + `fillGradient` 实现（运行时 `ShapePath` 无 `strokeGradient`，原先静默失效）；
  搜索统一按 20 条/页请求；推荐页滚动区改 `availableWidth` 防滚动条遮挡
- **音高调节**：±12 半音（一个八度），不影响播放速度与进度推进；与倍速可叠加
- 自测增加到 11 项，新增：切歌压力（连续换源 + seek + 自然结束）、变速换算方向、
  进度不回退、配置持久化、变调时长/频率校验（过零计数）

### 🐞 修复（前一轮）
- 开启音高补偿后崩溃：SOLA 相关搜索窗相对偏移算错，倍速 <1 时会读到输入缓冲之前的地址（加中心偏移 + 上下界夹取）
- 倍速方向反了：解码输出采样率与倍速写成正比，0.8x 实际按 1.2x 播放（改为反比）
- 播放进度不随倍速换算，且变速时进度按设备采样率推进导致刻度漂移
- 拖动进度条会先闪回旧位置：seek 落地前音频线程仍在发布上一数据段的位置（新增 pending-seek 抑制 + 解码线程 seek 节流）
- 拖动进度条时反复冲刷解码缓冲造成断续（节流至 ~11 次/秒，最后一次请求必定生效）
- `TimeStretch` 每跳一步都搬移整个输入缓冲（改为每次 push 回收一次）；缓冲无上限增长
- `QAudioSink` 用 `deleteLater` 释放，与紧随其后的 IO 设备销毁顺序不确定（改为同步销毁）

### 🔧 变更
- 升级到 Qt 6.10.3（Windows 主用 LLVM-MinGW 工具链）
- 第三方依赖（QWindowKit / TagLib / zlib）的导入统一收敛到 `cmake/external/`
- “设置-关于”中的 Qt 版本改为运行时动态获取，不再硬编码
- 移除调试遗留的 `QSG_INFO` 输出
- **音频后端重写**：不再使用 `QMediaPlayer` / `QAudioOutput`，改为 `cpp/audio/` 自研播放引擎
  - 解码：FFmpeg（libavformat/libavcodec/libswresample），支持全部常见音视频封装与音频编码
  - 管线：解码线程 → 无锁环形缓冲 → 设备输出回调（DSP 在音频线程内完成，低延迟）
  - 输出：`QAudioSink` 拉模式，优先 Float32，回退 Int16；支持指定输出设备
  - 频谱：由引擎后处理结果直接驱动 `GetWave`，不再依赖 `QAudioBufferOutput`

### ✨ 新增
- 音频 DSP：10 段参数均衡（±24 dB，含 10 组预设）、可调频段 Q 值、前级增益、自动余量、
  声道平衡、左右独立增益、单声道、立体声宽度、声道交换、ReplayGain（单曲/专辑 + 削波保护）、
  峰值限幅器、保持音高的 0.25×~4× 变速
- 音频处理总开关（一键直通）、输出采样率与输出缓冲可调（改后重开输出并保留播放位置）
- 设置页新增「均衡器与音频处理」面板：状态行显示编码/源采样率/位率/解码采样率/缓冲/DSP 指示
- 全部音频处理参数持久化到 `Options.ini`（写盘去抖 400ms，退出时兜底刷写）
- 新增 `quemusic_audio_pipeline_test` 离线自测：解码 / seek / DSP 边界 / SOLA 全量程变速 /
  倍速换算方向 / 进度不回退 / 配置持久化（9 项）
- 设置页（主题、界面、功能、播放、快捷键、插件、关于、Debug 八大模块）
- 桌面歌词 / 桌面部件
- 下载管理器（队列、进度、重试、已完成列表）
- 本地音乐库（我的文件夹 / 本地文件夹、导入、重命名、删除）

### 🎨 界面
- 全局单例样式系统（Style.qml），支持浅色/深色/跟随系统
- 自适应封面主色提取（ColorExtractor）
- 歌词逐字滚动 + 逐行弹簧动画 + 背景动态流体着色器

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
