# JIZURA PV 歌词性能剖析报告

## 剖析元信息

| 项 | 值 |
|---|---|
| 剖析模式 | `javascript, compiling, binding, animations, creating, handlingsignal`（未采集 memory / pixmapcache） |
| 被剖析进程 | `pvrunner.exe`（临时的最小 QML 宿主，只加载 JIZURA PV 插件并驱动 `position`，用于把 PV 自身的渲染循环单独隔离出来；非 QueMusic 主程序） |
| 被剖析配置 | 画质固定为 q0（`applyQuality(0)`，自适应关闭），C++ 内核 `LyricPvCore` 已启用，1280×720 |
| trace 文件 | `profiler/traces/qmlprofiler-trace-pvrunner-2026-10-13-181944.qtd`（385 MB，解析完成后已删除，复现命令见文末） |
| 运行时长（wall_ms_est） | **7,159 ms**（由帧数与平均帧率推算） |
| range_events_total_ms | **32,486 ms** —— 这是所有 range 事件时长之和，**不是墙钟时间**：JS 事件存在父子嵌套，父函数与其子函数的时间被重复计入，因此这个数字远大于实际耗时。判读每帧成本请看下表的 `ms_per_frame` 与"每帧时间构成"一节。 |
| total_events | 4,624,077 |

## 事件类型汇总

| 类型 | count | total_ms | ms_per_frame |
|---|---|---|---|
| Javascript | 4,615,685 | 27,678.5 | 50.60 |
| HandlingSignal | 1,953 | 3,612.1 | **6.60** |
| Binding | 1,896 | 501.0 | 0.92 |
| Compiling | 8 | 385.6 | 0.71 |
| Creating | 3,988 | 309.0 | 0.57 |

> `count` 随运行时长线性增长，不能当作主要指标。Javascript 的 `ms_per_frame` 同样受嵌套重复计入影响而虚高；**可信的每帧 CPU 数字是 HandlingSignal 的 6.60 ms/frame**——这正好是"每帧渲染触发"这一个信号的完整（含嵌套）耗时。
>
> Compiling / Creating / Binding 的高值集中在启动阶段（首次编译 20k 行 bundle、创建 Text 池、建立绑定），属于一次性开销，不在每帧稳态里。

## 帧时间汇总

**百分位怎么读**

- 帧时间 = 相邻两帧的墙钟间隔，越低越顺。
- p50 是中位数；p95 / p99 表示有 5% / 1% 的帧比该值更差；max 是最差单帧。
- 60 Hz 参考线 ≈ 16.67 ms/帧；> 33 ms 是肉眼可见的卡顿，> 50 ms 是停顿。

本次运行 `frame_count = 547`，折算成具体帧数：

- p95 = 15.15 ms → 约 **27 帧** ≥ 15.15 ms
- p99 = 27.03 ms → 约 **5 帧** ≥ 27.03 ms

| 指标 | 值 |
|---|---|
| **frame_ms_p50** | **13.16 ms** |
| **frame_ms_p95** | **15.15 ms** |
| **frame_ms_p99** | **27.03 ms** |
| **frame_ms_max** | **27.03 ms** |
| **frames_over_25ms** | **15** |
| **frames_over_33ms** | **0** |
| **frames_over_50ms** | **0** |
| 平均帧率 | 76.4 fps（未锁 60 Hz，故 p50 低于 16.67 ms） |
| 最低帧率 | 37 fps |

结论：稳态帧率不低（p50 13.2 ms），但存在抖动——最差单帧 27 ms（约 37 fps），有 15 帧超过 25 ms。抖动来源不是渲染，而是每帧 JS 计算的波动（见下文）。

## 内存 / 图片缓存

本次采集未包含 `memory` 与 `pixmapcache` 特性，故无数据。如需分析 GC 压力与图片加载，请用 `--include memory,pixmapcache,creating` 重新采集。

## Top 30 热点

下行 `filename` 与 `line` 在本次 trace 中为空（JS range 事件未带源位置信息），因此源位置列统一标注 `[source unresolved]`。`details` 里的函数名仍足以定位到具体代码路径。

| # | total_ms | count | avg_ms | ms/frame | 类型 | 源位置 | details |
|---|---|---|---|---|---|---|---|
| 1 | 3,135.5 | 529 | 5.927 | 5.73 | HandlingSignal | [source unresolved] | （每帧渲染触发的 `onTriggered`） |
| 2 | 3,134.1 | 529 | 5.924 | 5.73 | Javascript | [source unresolved] | `expression for onTriggered` |
| 3 | 3,133.2 | 530 | 5.912 | 5.73 | Javascript | [source unresolved] | `render` |
| 4 | 2,819.8 | 588 | 4.796 | 5.16 | Javascript | [source unresolved] | （匿名） |
| 5 | 2,605.3 | 530 | 4.916 | 4.76 | Javascript | [source unresolved] | `frame` |
| 6 | 2,469.1 | 588 | 4.199 | 4.51 | Javascript | [source unresolved] | `frame` |
| 7 | 1,739.5 | 1,717 | 1.013 | 3.18 | Javascript | [source unresolved] | （匿名，约 3.2 次/帧，每次 ~1.0 ms） |
| 8 | 1,117.3 | 12,409 | 0.090 | 2.04 | Javascript | [source unresolved] | （匿名，约 23 次/帧，每次 ~0.09 ms） |
| 9 | 545.5 | 5,490 | 0.099 | 1.00 | Javascript | [source unresolved] | （匿名，约 10 次/帧） |
| 10 | 492.4 | 530 | 0.929 | 0.90 | Javascript | [source unresolved] | `apply` |
| 11 | 475.5 | 2,650 | 0.179 | 0.87 | Javascript | [source unresolved] | `apply`（5 个图层 × 530 帧） |
| 12 | 445.6 | 1 | 445.563 | — | HandlingSignal | [source unresolved] | `Component.onCompleted` |
| 13 | 445.6 | 1 | 445.559 | — | Javascript | [source unresolved] | `expression for onCompleted` |
| 14 | 444.7 | 2 | 222.334 | — | Binding | [source unresolved] | （一次性绑定求值） |
| 15 | 434.0 | 55,721 | 0.008 | 0.79 | Javascript | [source unresolved] | （匿名，约 105 次/帧） |
| 16 | 413.4 | 6,249 | 0.066 | 0.76 | Javascript | [source unresolved] | `draw`（各 pack 的绘制函数，约 11 次/帧） |
| 17 | 355.6 | 166 | 2.142 | 0.65 | Javascript | [source unresolved] | `render` |
| 18 | 345.5 | 530 | 0.652 | 0.63 | Javascript | [source unresolved] | （匿名，1 次/帧） |
| 19 | 336.4 | 79,138 | 0.004 | 0.61 | Javascript | [source unresolved] | （匿名，约 149 次/帧） |
| 20 | 330.8 | 171 | 1.935 | 0.60 | Javascript | [source unresolved] | `render` |
| 21 | 249.0 | 219 | 1.137 | 0.46 | Javascript | [source unresolved] | `render` |
| 22 | 244.5 | 876 | 0.279 | 0.45 | Javascript | [source unresolved] | （匿名，约 1.7 次/帧） |
| 23 | 223.9 | 2 | 111.957 | — | Creating | [source unresolved] | `QtQuick/Text`（池首次创建） |
| 24 | 211.2 | 62,011 | 0.003 | 0.39 | Javascript | [source unresolved] | （匿名，约 117 次/帧） |
| 25 | 197.3 | 1 | 197.269 | — | Compiling | [source unresolved] | `jizura.qml` |
| 26 | 196.2 | 264 | 0.743 | 0.36 | Javascript | [source unresolved] | `draw` |
| 27 | 190.8 | 157 | 1.215 | 0.35 | Javascript | [source unresolved] | `draw` |
| 28 | 164.8 | 1 | 164.800 | — | Compiling | [source unresolved] | `jizura.bundle.js` |
| 29 | 161.4 | 184 | 0.877 | 0.30 | Javascript | [source unresolved] | `render` |
| 30 | 136.9 | 23,416 | 0.006 | 0.25 | Javascript | [source unresolved] | （匿名，约 44 次/帧） |

## 每帧时间构成（独立直测）

trace 只能给出调用耗时，无法区分"纯计算"和"写进 QML 属性"。插件本身对这两段分别打了表（`recordMs` / `applyMs`），另用一个窗口逐帧采样 8 秒（433–573 帧）：

| 分段 | 每帧耗时 | 说明 |
|---|---|---|
| **recordMs（纯 JS 计算）** | **5.6 – 5.8 ms** | 跑 JIZURA 渲染器、构建显示列表、生成 SVG |
| **applyMs（写 QML 属性）** | **1.0 ms** | 把显示列表写进 `Shape` / `Text` 项 |
| lastMs（合计） | 6.6 – 6.8 ms | 与 trace 的 HandlingSignal 6.60 ms/frame 吻合 |

同时测得的每帧工作负载：

| 量 | 每帧 |
|---|---|
| Shape（矢量图形项） | **14.9** |
| Text（文本项） | **106.8** |
| 交给 C++ 的 SVG 路径条数 | 9.8 |
| 路径数字总数 | **1,175**（≈294 条指令） |

**判读**：每帧回调的 6.6 ms 里，**计算占 ~85%（5.6 ms），渲染侧属性写入只占 ~15%（1.0 ms）**。这与"GPU 占用极低、CPU 占用高"的现象一致：瓶颈完全在计算层，不在渲染层。

## 详细分析（Top 5）

### 1–3. 每帧渲染触发链 · `onTriggered` → `render` · 5.73 ms/帧 · [source unresolved]

整个每帧工作都挂在这条链上：`HandlingSignal(onTriggered)` 5.73 ms ⊃ `render` 5.73 ms。它就是插件的帧驱动函数，内部做三件事：调用 JIZURA 渲染器生成显示列表（`jz.frame`）、生成 SVG（`finalize`）、把结果应用到 QML 舞台（`stage.apply`）。这一段是全部优化的目标区域。

**建议**：整条链的稳态成本就是可优化上限（约 6.6 ms/帧）。要真正降下来，必须减少"每帧重建的数据量"，而不是继续优化链上的单个叶子函数（叶子函数已经优化过一轮，见下）。

### 4–6. `frame` · 4.51–4.76 ms/帧 · [source unresolved]

这是 JIZURA 的**每帧显示列表构建**：逐层遍历 5 个合成层，重跑各效果包的绘制函数、把成千上万次 Canvas2D 调用重新录制进操作池。它占每帧回调的 **~70–80%**，是当前最大的单块成本。

值得强调的是：**这部分每帧都是从零重算的**——即使某一层的画面与上一帧完全相同（同一歌词、同一时间步、没有动画），绘制函数依然会被完整重跑一遍。

**建议**：按"时间步 + 图层"缓存显示列表。JIZURA 的时间本来就被量化成步（koma），步内静态图层的内容是逐帧一致的；把 `(cut, step, layer)` 作为键复用上一帧已录制的操作池，可让静态图层从 4.5 ms 里整块消失。这是收益最大的结构性改动。

### 7–9. 匿名子块 · 约 3.2 次/帧 × 1.0 ms + 23 次/帧 × 0.09 ms · [source unresolved]

第 7 条每次调用 ~1.0 ms、每帧只发生约 3.2 次，说明它是**大块、低频**的工作——量级上最像"逐层装配/最终化"这类整层操作（5 个图层量级）。第 8 条 23 次/帧 × 0.09 ms 是另一个次大块。

**建议**：trace 缺少源位置，无法进一步拆解到函数。建议后续在插件里给这几处加显式命名（把匿名闭包赋给具名变量），重新采集一次即可精确定位。

### 10–11. `apply` · 0.90 + 0.87 ms/帧 · [source unresolved]

显示列表写入 QML：每帧约 15 个 `Shape` 与 107 个 `Text` 的属性被重写，5 个图层各一次。这是渲染侧的全部成本，合计约 1.0 ms/帧。

**建议**：与 `frame` 合并考虑——若显示列表按步缓存，`apply` 也可以只在**内容真正变化时**才写属性，把 1.0 ms 压到接近 0。

### 15–16, 19, 24, 30. 录制原语 · 合计约 2 ms/帧 · [source unresolved]

`draw`（各效果包绘制函数，~11 次/帧，0.76 ms）加上四条高频匿名调用（105、149、117、44 次/帧，单次 3–8 µs），合计约 **2 ms/帧**。这些是路径录制原语（`moveTo`/`lineTo`/`quadTo`/`__push`/`__emitPath` 一类），单次极便宜但次数极多——实测约 **4,800 次/帧**（原先据 trace 估的"约 450 次"只是 trace 中某一档计数桶，已更正；且这些 3–8 µs 的单次耗时主要来自剖析器自身的每事件开销，其总时长不可信，见"相位分解"一节）。

**建议**：这类"次数极多、单次极便宜"的调用不适合逐个搬到 C++（跨语言边界比函数本身还贵）。正确做法是把**操作池与录制原语整体搬进 C++**，让 JS 只提交语义级调用，从而消除每帧数百次边界穿越。

## C++ 内核（LyricPvCore）已完成部分的实测归因

为确认已搬运部分到底买到了什么，用四个配置在同一宿主、同一画质下各跑一次（`tests/harness.qml --run 10`）：

| 配置 | 接管内容 | avg | 相对纯 JS |
|---|---|---|---|
| 纯 JS | 无 | 13.46 ms | — |
| 仅颜色 | `normalizeColor` / `mix` / `lum` | 13.27 ms | **−0.19 ms** |
| 仅 SVG | `finalize` → `pathsToSvg` | 11.64 ms | **−1.82 ms** |
| 全部 | 两者 | 11.46 ms | −2.00 ms |

**SVG 那 1.82 ms 是真实收益**，成本来源不是字符串拼接，而是 V4 的**数字→字符串格式化**：每帧约 1,175 个坐标要做 `Math.round` + `toString`，V4 的 dtoa 路径约 1.5 µs/次（1,175 × 1.5 µs ≈ 1.8 ms，与实测吻合）。C++ 用整数快路径 + 预分配缓冲后这块基本归零。

**颜色接管只值 0.19 ms**，此前用"把 `normColor` 改成直接返回入参"做的消融得出 1.8 ms 是**被污染的**——那种改法会让未归一化的颜色串流到 QML 侧，改变了下游解析与渲染行为，把差值算到了 `normColor` 头上。此处以四配置直测为准。

## 相位分解（低开销自计时，每帧只读 3 次时钟）

qmlprofiler 的每事件开销会淹没高频小函数（这次 7 秒录了 460 万个事件，单次 3–8 µs 的"小函数"基本是剖析器自身开销），因此在插件内部插入低开销计时：只在 `R.prototype.frame` 的包装器里读 3 次时钟，分别圈出 JIZURA 渲染器、离屏嵌套渲染与 `finalize`。

| 相位 | ms/帧 | 占 rec 比例 |
|---|---|---|
| `rendererRoot`（JIZURA 渲染器 + 效果包 + 录制器） | **4.20** | ~65% |
| `finalize`（SVG 桥接：JS 组包 + 跨界 + C++ 生成） | **1.55** | ~24% |
| `offscreenNest`（离屏画面重渲染，0.13 次/帧，已含在上面） | 0.44 | 7% |
| `pre`（`__beginFrame` 等） | 0.01 | ~0% |
| 插件侧显示列表装配（残差） | ~0.5 | ~8% |
| （单列）`stage.apply` 写 QML 属性 | 1.30 | — |

另一轮给 200 个已注册效果包的 `draw` 与 Ctx/Path2D 的 65 个原语方法加了计时（该轮单次调用开销抬高约 1–2 ms，故只用来看**相对排序**）：

| 效果包 | 每帧 | 单次调用 | 归属 |
|---|---|---|---|
| `BG/triTess` | **0.60 ms** | 1.75 ms | 背景：三角面片细分 |
| `BG/rainWindow` | **0.54 ms** | 1.64 ms | 背景：雨窗 |
| `DECOR/guides` | 0.37 ms | 1.14 ms | 装饰：辅助线 |
| `BG/snowLayers` | 0.32 ms | 1.20 ms | 背景：雪层 |
| `DECOR/brushStroke` | 0.24 ms | 0.44 ms | 装饰：笔刷 |
| `TRANS/whipPan` | 0.12 ms | 5.45 ms | 转场（低频） |
| `TRANS/shatterTiles` | 0.10 ms | 2.68 ms | 转场（低频） |
| 已注册效果包合计 | **3.10 ms** | — | 5.3 次/帧 |

被计时器覆盖到的 Ctx/Path2D 原语调用量为 **每帧约 4,800 次**（此前从 trace 推断的"约 450 次"是错的，那只是 trace 中某一档计数桶）。

**结论：`rendererRoot` 那 4.2 ms 里过半是少数几个"程序化生成几何"的效果包**（每帧只调用 5.3 次，单次却要 1.1–1.75 ms），其余是渲染器自身的层装配/切段逻辑与约 4,800 次录制原语调用。离屏重渲染与字体测量都不是问题（0.44 ms / 0 次命中）。

## 后续步骤（按优先级，已按实测重排）

1. **先消掉 `finalize` 的 1.55 ms**（这是 `LyricPvCore` 桥接自身的开销，不是 JIZURA 的）：每帧要组装 `cmds`/`offs`/`mats`/`pending` 四个数组（约 1,200 次 push）、把 JS 数组转成 `QList<double>`、再把 `QStringList` 转回 JS 数组。改法不需要任何行为变更：无位移时不传 `mats`、暂存数组跨帧复用、省掉 `pending` 中间数组。预计 1.55 → 0.4 ms 量级，**最便宜的一笔**。
2. **把前 5 个效果包搬进 C++**（合计 2.07 ms/帧，占计算 ~30%）：`triTess`、`rainWindow`、`guides`、`snowLayers`、`brushStroke`。它们都是"循环生成大量几何"的程序化效果，正是 C++ 的主场，且彼此解耦可逐个搬——先搬 `triTess` + `rainWindow`（1.14 ms）。
3. **渲染器自身逻辑 + 约 4,800 次/帧的录制原语**（残差 ~1.1–2.4 ms）：这才是需要 `PvDisplayList` 的地方——把操作池与路径提交整体搬进 C++，以**批量提交**取代逐条调用（逐条暴露成 C++ 反而更慢，跨界比函数本身贵）。
4. **`apply` 只在内容真正变化时写属性**（1.30 ms/帧）。实测当前默认下相邻帧的**整层显示列表有 32.4% 完全一致**（L1 45%、L3 54%），这部分可跳过属性写入；但它省不掉 4.2 ms 的重算——要先算出来才知道没变。
5. **减少文本项数量**：每帧 106.8 个 `Text` 对 14.9 个 `Shape`。若某些图层不需要逐字动画，改成整行文本可显著降低项数与布局成本。

## 后续步骤（旧版，保留以对照）

1. ~~**按步缓存显示列表**~~（`frame` 4.5 ms/帧 → 静态图层接近 0）。收益最大，且不需要新的 C++ 代码：在插件里以 `(cut, step, layer)` 为键复用已录制的操作池，只在内容真正变化时重建。
2. **把操作池与录制原语整体搬进 C++**（实测约 4,800 次/帧的录制调用 + 每帧重建的数组）。这是 `LyricPvCore` 的自然下一阶段：新增 `PvDisplayList` 类型，JS/效果包只提交语义级绘制调用，几何与字符串全部留在 C++，跨界从"每帧一次大数组"降到"仅提交变更"。
3. **`apply` 只在内容变化时写属性**（1.0 ms/帧 → 接近 0）。与第 1 条同时做最省事。
4. **给 trace 里那几处匿名大块具名**（第 7、8 条），重新采集一次以精确定位剩余 3 ms 的分布。
5. **减少文本项数量**：每帧 106.8 个 `Text` 对 14.9 个 `Shape`——工作量由文本主导。若某些图层不需要逐字动画，改成整行文本可显著降低项数与布局成本。

热点集中在 `plugins/lyrics/jizura/`（渲染循环、录制器、显示列表）这一处，建议对该目录跑一次 `qt-qml-review` 做更广的结构性检查。

---

AI assistance has been used to create this output.