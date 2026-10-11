# 第 3 部分 · 音频引擎与着色器（cpp/audio/、shaders/）

> 本文件是 [QueMusic 项目全景文档索引](README.md) 的一个分片；任务定位与其它部分请见 [README.md](README.md)。

范围：自研音频引擎 `cpp/audio/`（10 文件）+ `shaders/` 的 C++ 侧（`LyricsStage`、`MeshGradientItem`）与 `shaders/*.frag|*.vert`。
行数基准（见附录 A）：AudioEngine 1338/380、AudioDsp 410/139、AudioRing 85、AudioSpectrumSink 57、FfmpegDecoder 383/88、PitchShifter 90/38、TimeStretch 165/39、LyricsStage 273/132、MeshGradientItem 493/102。

---

## 3.1 音频引擎总览

### 3.1.1 流水线

```
GUI 线程：setSource/setPosition/… 只写状态 + 置标志，不碰音频数据
   │  m_openRequest / m_seekRequest / m_pendingUrl(锁)
   ▼
解码线程 std::thread decodeLoop()
   FfmpegDecoder.open() → setTargetFormat(targetRate, channels)
   readFrames() 每块 kMaxChunkFrames=4096 帧 float32 交错
   → m_rings[m_ringGen & 1].write()          （SPSC 无锁环，2 秒容量）
   │
   ▼
输出线程 QThread("QueMusicAudioOutput")
   QAudioSink(拉模式) → 内部定时器 → EngineIoDevice::readData()
   → AudioEngine::render(data, maxlen)                     ← 音频回调
        ring.read() → [可选] PitchShifter(SOLA+重采样) → AudioDsp::process()
        → 频谱句柄 push → 写设备缓冲（float 直写 / int16 转换）
```

- `EngineIoDevice`（`QIODevice` 子类，非 QObject 派生自 QIODevice）只做转发：`readData() → AudioEngine::render()`；`isSequential()==true`，`bytesAvailable()` 恒返回 `1<<20`。
- 「音频回调线程」在本项目里就是 `m_outputThread`（Qt 拉模式的时间驱动），**不是** OS 实时线程；但它与 GUI 线程隔离，GUI 卡顿不影响出声（代码注释：`sink 与它的拉取定时器都归输出线程，GUI 卡顿影响不到音频`）。

### 3.1.2 线程与对象归属

| 对象/数据 | 所属线程 | 说明 |
|---|---|---|
| `AudioEngine`（QObject, `QML_ELEMENT`） | GUI 线程（QML 创建） | 所有 `Q_PROPERTY` getter/setter、`Q_INVOKABLE`、`signals`、`m_eq`、`m_meta`、`m_playbackState`、`m_mediaStatus`、`m_durationMs`、`m_source`、`m_sourceHeaders`、`m_volume`、`m_playbackRate`、`m_pitchSemitones`、`m_requestedRate`、`m_bufferMs`、`m_deviceId`、`m_urlStr`、`m_noTitle`、`m_album` 均只在此线程读写 |
| `m_pollTimer`(50ms)、`m_saveTimer`(400ms 单次) | GUI 线程 | `pollState()` / `saveSettings()` |
| `m_outputThread` + `m_outputContext`(QObject) | 输出线程 | `m_outputContext` 仅作 `invokeOnOutput` 的投递目标 |
| `m_sink`(QAudioSink)、`m_io`(EngineIoDevice) | 输出线程 | 二者都在 `invokeOnOutput` 内创建/销毁；`m_io` 跨 sink 重建复用 |
| `render()` 及其局部量、`m_dsp`、`m_stretch`(PitchShifter)、`m_scratch`、`m_stretchIn`、`m_localGen`、`m_localMs`、`m_genFadeInFrames`、`m_genFrames`、`m_fadeLocalSeq` | 输出线程（音频回调） | `m_dsp`/`m_stretch` 在构造期也在 GUI 线程 `prepare()`（此时无线程并发） |
| `m_thread`(std::thread) → `decodeLoop()` | 解码线程 | 持有 `m_decoder`(FfmpegDecoder，非 QObject)、局部 `buffer` |
| `m_rings[2]`(AudioRing) | 解码线程写 `[gen&1]` / 输出线程读 `[已latch的gen&1]` | 单生产者单消费者 |
| 频谱 `AudioSpectrumSinkHandle` | 输出线程调用 `push()`；GUI 线程订阅/退订 | 共享 `shared_ptr` |

跨线程回归 GUI 的**唯一**通道：`postToSelf(fn)`（同线程直接调用，否则 `Qt::QueuedConnection`）；解码线程另用 `QMetaObject::invokeMethod(this, …, Qt::QueuedConnection)` 投递 `applyMeta` / `emitError`（注释：`把状态变更与信号发射统一拉回引擎所属线程：音频/解码线程绝不触碰 QML`）。

### 3.1.3 原子量

| 原子量 | 类型 | 写入者 → 读取者 | 含义 |
|---|---|---|---|
| `m_positionMs` | `atomic<qint64>` | 输出线程写 → GUI 读 | 当前播放位置（ms） |
| `m_ringGen` | `atomic<int>` | 解码线程写 → 输出线程读 | 数据段代号，奇偶选环 |
| `m_ringBaseMs` | `atomic<double>` | 解码写 → 输出读 | 数据段起始源的 ms |
| `m_seekRequest` / `m_pendingSeekMs` / `m_lastSeekDoneMs` | `atomic<qint64>` | GUI 写，解码/输出读 | seek 请求；`pendingSeekMs>=0` 时**禁止**输出线程发布位置 |
| `m_decodeEof` / `m_drained` / `m_decodeActive` / `m_openRequest` / `m_quit` | `atomic<bool>` | 解码 ↔ GUI/输出 | EOF、播完、打开请求、退出 |
| `m_audioFrozen` / `m_decodeParked` | `atomic<bool>` | GUI 写 / 解码写 | 缓冲重建握手（见 1.4 与 §3.9） |
| `m_outSampleRate` / `m_outChannels` / `m_decodeRate` | `atomic<int>` | GUI(输出线程) 写 → 解码/输出读 | 设备采样率、声道数、解码输出采样率 |
| `m_paramsPending` | `atomic<bool>` | GUI 写 → 输出读 | DSP 参数脏标志 |
| `m_volumeAtomic` / `m_fadeTargetMilli` / `m_fadeFrames` / `m_fadeSeq` / `m_pendingFadeInFrames` | `atomic<double>` / `atomic<int>` | GUI 写 → 输出读 | 音量、淡变目标(0..1000)、淡变帧数、淡变序号、下段淡入帧数 |
| `m_pitchCompensation` / `m_pitchMilli` / `m_rateMilli` / `m_stretchSpeedMilli` | `atomic<bool>` / `atomic<int>` | GUI 写 → 解码/输出读 | 保持音高开关、音高倍率×1000、倍速×1000（`decodeRateFor` 用 `m_rateMilli`，`render` 用 `m_stretchSpeedMilli`） |
| `m_rgTrackDb` / `m_rgAlbumDb` / `m_rgPeak` | `atomic<double>` | 解码线程写（`applyMeta` 内，在 GUI 线程执行）→ 输出读 | ReplayGain 标签值 |
| `m_playingFlag` / `m_dspEnabled` / `m_sinkErrorFlag` | `atomic<bool>` | GUI/输出 | 播放中标志、DSP 总开关、设备错误标志 |
| `FfmpegDecoder::m_abort` | `atomic<bool>` | GUI 写 → 解码线程/FFmpeg 中断回调读 | 中止（`interrupt_callback`） |

### 3.1.4 互斥量

| 互斥量 | 保护对象 | 约定 |
|---|---|---|
| `m_paramMutex`(QMutex) | `m_eqSnapshot` + `m_paramsPending` | GUI 写快照；输出线程 `applyPendingParams()` 先查 `m_paramsPending`（`acquire`）**热路径不加锁**，仅在脏时取锁 |
| `m_pendingUrlMutex` | `m_pendingUrl`、`m_pendingHeaders` | GUI 写，解码线程取 |
| `m_spectrumMutex` | `m_spectrumHandle` | 输出线程用 `try_to_lock`，**抢不到就跳过本块，绝不阻塞音频回调** |
| `AudioSpectrumSinkHandle::m_mutex`(std::mutex) | `m_sink` 裸指针 | 音频线程持锁转发；`detach()` 持锁置空 = 等待在途推送结束 |

### 3.1.5 数据段（generation）与位置发布

- 环缓冲两份（`m_rings[0..1]`），按 `gen & 1` 乒乓。解码线程 `beginGeneration(baseMs)`：`clear()` 下一个环 → 写 `m_ringBaseMs` → `m_ringGen = next`（release）。输出线程 lache `m_ringGen`，换段时 `m_stretch.reset()`、`m_dsp.reset()`、重算淡入长度。
- 位置推进：`posHz = stretching ? rate/speed : m_decodeRate`；`m_localMs += produced*1000/posHz`；仅当 `m_pendingSeekMs < 0` 才 `m_positionMs.store(qint64(m_localMs))`（注释：`seek 落地前禁止音频线程发布位置，否则进度条会先弹回旧值`）。
- seek 落地判定：换段时若 `qint64(m_localMs) == m_pendingSeekMs` 则清 `m_pendingSeekMs = -1` 恢复发布。
- 播完：`render` 在 `produced==0 && ring.available()==0 && m_decodeEof && m_playingFlag` 时置 `m_drained`；GUI `pollState()` 消费 → `StoppedState` + 位置=时长为 `durationMs` + `EndOfMedia`（注释：`不停止输出：保持设备常开，下一首起播时无设备重开的空档`）。

### 3.1.6 常量（AudioEngine.cpp 匿名 namespace）

| 常量 | 值 | 说明 |
|---|---|---|
| `kRingSeconds` | 2 | 每环容量 = 2s × rate 帧 |
| `kMaxChunkFrames` | 4096 | 解码块、渲染块、`m_scratch` 尺寸上限 |
| `kMaxStretchInputFrames` | 24576 | SOLA 输入上限 |
| `kPollIntervalMs` | 50 | GUI 轮询（位置信号、drained、设备错误） |
| `kSeekThrottleMs` | 90 | 解码线程 seek 节流 |
| `kBufferMsMin/Max/Default` | 10 / 100 / 30 | 设备缓冲 ms（注释：再小易爆音，再大拖慢响应） |
| `kMinBufferFrames` | 256 | 设备缓冲帧数下限（10ms@48k 仅 480 帧） |
| `kClickGuardMs` | 12 | 每次换段的最小淡入，消切换爆音 |

---

## 3.2 AudioEngine 的 QML 接口

QML 侧实例：`main.qml:796`（`id: mainMedia`）。类型名在 QML 里就是 `AudioEngine`。

### 3.2.1 Q_PROPERTY（名 / 类型 / 读写 / NOTIFY）

| 属性 | 类型 | 读写 | NOTIFY | 备注 |
|---|---|---|---|---|
| `source` | QUrl | RW | `sourceChanged` | 空串 = 停止并 `NoMedia`；赋值会重置 duration/meta/位置 |
| `sourceHeaders` | QVariantMap | RW | 无 | 远端附加请求头（WebDAV `Authorization`）；**必须在 `setSource` 之前写** |
| `position` | qint64 | RW | `positionChanged` | 写 = seek；`m_durationMs>0` 时钳到时长 |
| `duration` | qint64 | R | `durationChanged` | 非原子，GUI 线程专用 |
| `playing` | bool | R | `playingChanged` | `playbackState == PlayingState` |
| `playbackState` | PlaybackState | R | `playbackStateChanged` | 见 2.4 |
| `mediaStatus` | MediaStatus | R | `mediaStatusChanged` | 见 2.4 |
| `volume` | qreal | RW | `volumeChanged` | 钳 0..1；不持久化于本类 |
| `playbackRate` | qreal | RW | `playbackRateChanged` | 钳 0.25..4.0；改后 `requestSeek(position())` |
| `pitchCompensation` | bool | RW | `pitchCompensationChanged` | 保持音高 |
| `pitchSemitones` | qreal | RW | `pitchChanged` | 钳 -12..+12；`ratio = 2^(semi/12)` |
| `deviceId` | QString | RW | `deviceIdChanged` | 空 = 默认设备；写后 `reconfigureOutput()` |
| `urlStr` | QString | RW | `urlStrChanged` | 展示用封面 URL（QML 注入，无 NOTIFY 之外的语义） |
| `noTitle` | QString | RW | `noTitleChanged` | 列表显示名/文件名（QML 注入） |
| `album` | QString | RW | `albumChanged` | 展示用专辑名（QML 注入，非 TagLib 值） |
| `onMedia` | bool | R | `mediaStatusChanged` | 派生：`mediaStatus != NoMedia` |
| `title` | QString | R | `metaDataChanged` | 无标签时回退文件名 |
| `tagTitle` | QString | R | `metaDataChanged` | 仅标签值，可空 |
| `artist` | QString | R | `metaDataChanged` | 空则回退 `album_artist` |
| `albumTitle` | QString | R | `metaDataChanged` | = `m_meta.album` |
| `mediaType` | QString | R | `metaDataChanged` | **实现等于 `m_meta.codec`**（与 `codecName` 重复） |
| `mediaDate` | QString | R | `metaDataChanged` | `date` 标签 |
| `bitRate` | int | R | `metaDataChanged` | kbps |
| `sourceSampleRate` | int | R | `metaDataChanged` | 源采样率 |
| `sourceChannels` | int | R | `metaDataChanged` | 源声道数 |
| `codecName` | QString | R | `metaDataChanged` | `avcodec_get_name()` |
| `outputSampleRate` | int | RW | `outputFormatChanged` | 写 = 「请求」采样率，仅设备支持时生效；0 = 设备首选；钳 8000..384000 |
| `decodeSampleRate` | int | R | `outputFormatChanged` | 解码输出采样率（受倍速影响） |
| `bufferMs` | int | RW | `outputFormatChanged` | 写：非正数→30，否则钳 10..100 |
| `outputFormatName` | QString | R | `outputFormatChanged` | 形如 `Float32 · 48000 Hz · 2ch`，无设备时 `无输出设备` |
| `dspEnabled` | bool | RW | `dspChanged` | DSP 总开关（= `AudioDsp` bypass） |
| `dspActive` | bool | R | `dspChanged` | 有任一非默认 DSP 参数时为真 |
| `effectiveGainDb` | qreal | R | `dspChanged` | `preamp + RG - headroom` |
| `eqQ` | qreal | RW | `eqChanged` | 钳 0.2..8.0，默认 1.41 |
| `eqEnabled` | bool | RW | `eqChanged` | 默认 false |
| `eqGains` | QVariantList | RW | `eqChanged` | 10 项，每项钳 ±24 dB |
| `preampDb` | qreal | RW | `eqChanged` | 钳 ±24 |
| `autoHeadroom` | bool | RW | `eqChanged` | 默认 true |
| `balance` | qreal | RW | `dspChanged` | 钳 -1..1 |
| `mono` | bool | RW | `dspChanged` | |
| `stereoWidth` | qreal | RW | `dspChanged` | 钳 0..2，默认 1 |
| `swapChannels` | bool | RW | `dspChanged` | |
| `channelGainLeft` | qreal | RW | `dspChanged` | 钳 ±24 dB |
| `channelGainRight` | qreal | RW | `dspChanged` | 钳 ±24 dB |
| `replayGainMode` | int | RW | `replayGainChanged` | 钳 0..2（0 关 / 1 曲目 / 2 专辑） |
| `replayGainPreampDb` | qreal | RW | `replayGainChanged` | 钳 ±24 |
| `replayGainPreventClip` | bool | RW | `replayGainChanged` | 默认 true |
| `limiterEnabled` | bool | RW | `limiterChanged` | 默认 false |
| `limiterThresholdDb` | qreal | RW | `limiterChanged` | 钳 -24..0，默认 -0.5 |

### 3.2.2 Q_INVOKABLE

```cpp
void play();
void pause();
void stop();
void fadeTo(qreal target, int ms);   // target 0..1；frames = ms*max(8000,outRate)/1000，钳 0..4194304
void fadeOut(int ms);                // = fadeTo(0, ms)
void fadeIn(int ms);                 // = fadeTo(1, ms)
void fadeInOnNextAudio(int ms);      // 记 m_pendingFadeInFrames，换段时才真正淡入；同时调用 fadeIn(ms)
void setEqBand(int index, qreal gainDb);   // index 0..9 越界忽略；gain 钳 ±24
void applyEqPreset(const QString &name);   // 大小写不敏感；未命中 → 全 0；并置 eqEnabled = true
QStringList eqPresetNames() const;
QStringList eqBandLabels() const;
void setSpectrumSink(QObject *sink);       // sink 必须实现 AudioSpectrumSource，否则 qWarning 并忽略
```

### 3.2.3 signals

`sourceChanged` `positionChanged` `durationChanged` `playingChanged` `playbackStateChanged` `mediaStatusChanged` `errorOccurred(int code, const QString &message)` `metaDataChanged` `volumeChanged` `playbackRateChanged` `pitchCompensationChanged` `pitchChanged` `deviceIdChanged` `urlStrChanged` `noTitleChanged` `albumChanged` `outputFormatChanged` `eqChanged` `dspChanged` `replayGainChanged` `limiterChanged`

- `errorOccurred` 当前**唯一**触发点：解码线程打开源失败 → `emitError(1, message)`（`code` 恒为 1）。
- `eqChanged` 覆盖 eq 参数；`dspChanged` 覆盖 balance/mono/width/swap/channelGain/limiter 开关/dspEnabled/effectiveGainDb。

### 3.2.4 枚举

```cpp
enum PlaybackState { StoppedState = 0, PlayingState = 1, PausedState = 2 };   // Q_ENUM
enum MediaStatus  { NoMedia = 0, LoadingMedia = 1, LoadedMedia = 2, StalledMedia = 3,
                    BufferingMedia = 4, BufferedMedia = 5, EndOfMedia = 6, InvalidMedia = 7 }; // Q_ENUM
```

当前实现只置位：`NoMedia`、`LoadingMedia`、`LoadedMedia`、`EndOfMedia`、`InvalidMedia`；
`StalledMedia` / `BufferingMedia` / `BufferedMedia` **从未被 setMediaStatus 使用**（QML 里判 `EndOfMedia`，见 `main.qml:818`）。

---

## 3.3 DSP 链路顺序

### 3.3.1 `AudioEngine::render()` 阶段顺序（逐块 ≤ 4096 帧）

| # | 阶段 | 位置/条件 | 说明 |
|---|---|---|---|
| 1 | 段切换处理 | `gen != m_localGen` | `m_stretch.reset()`、`m_dsp.reset()`、清 `m_pendingSeekMs`（若匹配）、`m_genFadeInFrames = max(m_pendingFadeInFrames, 12ms 帧数)` |
| 2 | 参数上传 `applyPendingParams()` | 每块，`m_paramsPending` 为假时立即返回 | 把 `m_eqSnapshot` + RG 原子值灌进 `AudioDsp`；阈值 `qBound(-24,0)` → 线性 |
| 3 | 音量 `m_dsp.setVolume(m_volumeAtomic)` | 每块 | 只设标量，不在此处乘 |
| 4 | 淡变 `m_dsp.rampFade(target, frames)` | 仅 `m_fadeSeq != m_fadeLocalSeq` | 注释：`淡变在音频线程逐样本完成，与 GUI 调度无关` |
| 5 | 时间伸缩/变调（**在 DSP 之前**） | `stretching = pitch!=1 \|\| (pitchCompensation && speed!=1)` | 需要时 `m_stretch.setRatios(speed, pitch)`；读环 → `m_stretch.push()` → `m_stretch.pull(buf, want)` |
| 6 | `m_dsp.process(buf, got, channels)` | 每块（见 3.2） | EQ → 静态增益 → 声道工具 → 限幅/硬裁剪 → 音量×淡变 |
| 7 | 换段淡入起点 | `m_genFrames==0 && m_genFadeInFrames>0` 且首批样本到位 | `jumpFade(0)` + `rampFade(1, m_genFadeInFrames)` |
| 8 | 频谱推送 | 每块，`acquireSpectrumHandle()` 成功才推 | `handle->push(buf, got, channels, rate)` —— **推的是 DSP 之后的信号** |
| 9 | 设备格式写出 | `direct = (Float 输出 && !stretching)` | Float 且不伸缩：直接把设备缓冲当 DSP 缓冲（省一次整块拷贝）；否则 `m_scratch` → memcpy(float) 或 `qint16(qBound(-32768, v*32767, 32767))` |
| 10 | 欠载补零 + drained 判定 | `produced < frames` | 补零；`produced==0 && ring 空 && decodeEof && playingFlag → m_drained = true` |
| 11 | 位置推进 | `produced > 0` | `m_localMs += produced*1000/posHz`；仅 `m_pendingSeekMs < 0` 时发布 |

`AudioRing` 读取：不伸缩时一次 `ring.read(buf, want*channels)`；伸缩时按 `needIn = want*speed+1`、`give = min(available, min(24576, needIn+4096))` 喂输入。

### 3.3.2 `AudioDsp::process()` 内部精确顺序

```
1) if (m_bypass)  → 只做 processVolume（跳过 EQ/声道/限幅，音量与淡变仍生效）
2) EQ：10 段 peaking biquad；仅 gainDb != 0 的频段参与
   单次遍历：for frame { for ch { for activeBand { tick() } } }   ← 逐样本
3) 静态增益 gain = 10^(outputGainDb()/20)，outputGainDb = preampDb + replayGainDb() - headroom
   headroom = autoHeadroom && eqEnabled ? max(0, max(所有波段 gainDb)) : 0
   replayGainDb = mode<=0 ? 0 : (mode==2 ? album : track) + rgPreamp，preventClip && peak>0 时 min(…, -20log10(peak))
4) applyChannelTools：balance → mono → stereoWidth → swapChannels → channelGain[L]/[R]
   balance 在进入循环前折进 gainL/gainR：balance>0 → gainL *= (1-balance)；balance<0 → gainR *= (1+balance)
   mono：mid=(l+r)/2 同时写回 l、r；width：M/S 变换 side *= width；swap：std::swap
   无工具需求时整块提前返回（needsTools 判定）
5) if (m_limiter) 逐样本限幅（peak→target=thr/peak；attack 1.5ms / release 60ms 一阶平滑；再钳 ±1.0）
   else 逐样本硬裁剪 qBound(-1.0, v, 1.0)     ← 关限幅也有裁剪，防止 float 溢出
6) processVolume：m_fadeStep != 0 → 逐样本 g = volume*fade 并推进 fade；
                  否则块级标量 g = volume*fade（qFuzzyCompare 为 1 时整块跳过）
```

单声道输入（`channels < 2`）走简化分支：只用 `channelGain[0]`，且仅 `balance > 0` 时乘 `(1-balance)`。

### 3.3.3 逐样本 vs 块级

| 处理 | 粒度 | 备注 |
|---|---|---|
| EQ（10 段 biquad） | 逐样本（每帧一次遍历所有启用频段） | 有状态滤波器，必须逐样本 |
| preamp + ReplayGain − headroom | 块级标量乘（按 `frames*channels` 线性扫描） | 静态增益 |
| balance / mono / width / swap / channelGain | 逐样本（按帧，左右成对） | 无需状态，但代码写成帧循环 |
| limiter | 逐样本（含增益包络状态） | 攻击/释放由采样率换算 |
| 硬裁剪（限幅关闭时） | 逐样本 | 永远执行 |
| volume × fade | fade 进行中逐样本，否则块级 | fade 状态在 `AudioDsp` 内 |
| 时间伸缩（SOLA）+ 重采样（PitchShifter） | 块级（窗口 1024·scale，重叠相加） | 在 `render()` 里、DSP 之前 |
| 段切换淡入 | 块级设定 + 之后逐样本 | `jumpFade` + `rampFade` |

### 3.3.4 均衡器参数表

- 频段数 `AudioDsp::kBandCount = 10`，最大声道 `kMaxChannels = 2`。
- 频率（Hz）：`31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000`；标签：`31,62,125,250,500,1k,2k,4k,8k,16k`。
- 滤波器：peaking EQ（RBJ 型），`w0 = 2π·clamp(freq,10,sr*0.49)/sr`，`alpha = sin(w0)/(2·max(0.1,Q))`，`amp = 10^(gainDb/40)`；`gainDb == 0` 时直接置为单位 Biquad（b0=1，其余 0）。
- 增益范围 ±24 dB；Q 范围 0.2..8.0（默认 1.41）。
- 预设（`AudioDsp::presetNames()`，大小写不敏感匹配）：`Flat, Rock, Pop, Jazz, Classical, Dance, Bass Boost, Treble Boost, Vocal, Loudness`（各 10 个增益值，见 `AudioDsp.cpp:26-37`）。

---

## 3.4 文件逐个说明

| 文件 | 主要类/结构 | 职责 | 关键设计点 |
|---|---|---|---|
| `cpp/audio/AudioEngine.h` | `AudioEngine`(QObject, QML_ELEMENT)、私有 `EqParams` | QML 门面 + 播放控制 + 参数发布 + 持久化声明 | 两套状态分离：GUI 线程普通成员 vs `atomic`/`m_eqSnapshot`；`friend class EngineIoDevice` |
| `cpp/audio/AudioEngine.cpp` | 同上、`EngineIoDevice`(QIODevice)、匿名 namespace 常量 | 设备建立/重建、`render()`、`decodeLoop()`、设置读写 | 所有跨线程写回经 `postToSelf`；`m_eq` 与 `m_eqSnapshot` 分离；`reconfigureOutput()` 保位置重开 |
| `cpp/audio/AudioDsp.h/.cpp` | `AudioDsp`、私有 `Biquad`/`Band` | 10 段 EQ + 前级/余量 + ReplayGain + 声道工具 + 限幅 + 音量/淡变 | 无 Qt 对象、无锁、无分配；`setEqGains(const double*,int)` 专供音频线程避免构造 `QList` |
| `cpp/audio/AudioRing.h` | `AudioRing`（header-only） | SPSC 无锁 float 环 | 游标 `quint64` 单调不回卷，取模只在落盘索引；`available()` 夹到 `INT_MAX`；`configure()` 会 **resize**（必须冻结两个音频线程） |
| `cpp/audio/AudioSpectrumSink.h` | `AudioSpectrumSink`、`AudioSpectrumSinkHandle`、`AudioSpectrumSource` | 跨线程频谱订阅协议 | 音频线程只持 `shared_ptr<Handle>`；`detach()` 持锁等待在途推送结束；明确拒绝「裸指针 + destroyed 信号」方案 |
| `cpp/audio/FfmpegDecoder.h/.cpp` | `FfmpegDecoder`、`FfmpegDecoder::Meta` | 解封装 + 解码 + 重采样，统一输出 float32 交错 | `thread_count = 1`；`interrupt_callback` 支持 `abort()`；每帧 PTS 记入 `m_lastPtsMs`，seek 用 `m_skipUntilMs` 丢弃越界样本 |
| `cpp/audio/PitchShifter.h/.cpp` | `PitchShifter` | 变调不变速 = 时长伸缩 + 同倍率重采样 | 内部 `TimeStretch` 速度 = `speed/pitch`；重采样用 Catmull-Rom（4 点三次）读指针步进 = `pitch`；输出钳 ±1 |
| `cpp/audio/TimeStretch.h/.cpp` | `TimeStretch` | SOLA 变速（保持音高） | 窗口 1024·scale（scale = clamp(sr/24000,1,4)），hopOut = 窗/2，搜索 hopOut/2，相关长度 窗/4；相关搜索「粗步进 + 邻域精搜」；`m_dropFrames` 延迟回收避免每跳搬移整块 |

---

## 3.5 音频设置持久化

`QSettings`（默认组织/应用），全部在组 `AudioDsp` 下。`loadSettings()` 在构造函数里调用（早于 `setupSink()`）；`saveSettings()` 由 `scheduleSave()` 触发 `m_saveTimer`（单次 400ms），析构时若定时器活动则立即保存。

| 键 | 类型 | 载入钳制/默认 | 保存值 |
|---|---|---|---|
| `eqEnabled` | bool | 默认 `false` | `m_eq.enabled` |
| `eqGains` | QByteArray(JSON 数组) | `QJsonDocument::fromJson` → 最多 10 项，每项钳 ±24；缺失=全 0 | `QJsonDocument(gains).toJson(Compact)` |
| `eqQ` | double | 钳 0.2..8.0，默认 `1.41` | `m_eq.q` |
| `preampDb` | double | 钳 ±24，默认 `0.0` | 同 |
| `autoHeadroom` | bool | 默认 `true` | 同 |
| `balance` | double | 钳 -1..1，默认 `0.0` | 同 |
| `stereoWidth` | double | 钳 0..2，默认 `1.0` | 同 |
| `mono` | bool | 默认 `false` | 同 |
| `swapChannels` | bool | 默认 `false` | 同 |
| `gainLeft` / `gainRight` | double | 钳 ±24，默认 `0.0` | `channelGain[0]` / `[1]` |
| `rgMode` | int | 钳 0..2，默认 `0` | 同 |
| `rgPreamp` | double | 钳 ±24，默认 `0.0` | 同 |
| `rgPreventClip` | bool | 默认 `true` | 同 |
| `limiter` | bool | 默认 `false` | 同 |
| `limiterDb` | double | 钳 -24..0，默认 `-0.5` | 同 |
| `dspEnabled` | bool | 默认 `true`；同时写 `m_dspEnabled` 与 `m_eq.masterEnabled` | `dspEnabled()` |
| `outputSampleRate` | int | 钳 0..384000，默认 `0`（= 设备首选） | `m_requestedRate` |
| `outputBufferMs` | int | `<=0` → 30，否则钳 10..100 | `m_bufferMs` |
| `pitchSemitones` | double | 钳 -12..12，默认 `0.0`；载入后换算 `m_pitchMilli = round(2^(s/12)*1000)` | `m_pitchSemitones` |

**本类不持久化**：`volume`、`playbackRate`、`pitchCompensation`、`deviceId`、`source`、展示字段（`urlStr`/`noTitle`/`album`）。音量/设备等由 QML 侧（`Playback`/`Config`）另存（键名在 `components/Config.qml`，未在本文件核对）。

---

## 3.6 频谱可视化

### 3.6.1 三个接口（`AudioSpectrumSink.h`）

| 名字 | 形态 | 职责 |
|---|---|---|
| `AudioSpectrumSink` | 抽象基类，`virtual void pushSamples(const float *interleaved, int frames, int channels, int sampleRate) = 0` | 接收端；**由音频回调线程调用，实现方必须无阻塞、不分配、不取长锁** |
| `AudioSpectrumSinkHandle` | 实体类，持 `std::mutex` + `AudioSpectrumSink *m_sink` | 音频线程与订阅者之间的唯一通道；`push()` 持锁转发；`detach()` 持锁置空并等待在途推送结束 |
| `AudioSpectrumSource` | 抽象基类，`virtual std::shared_ptr<AudioSpectrumSinkHandle> spectrumSinkHandle() = 0` | QObject 订阅者自报句柄；引擎不认识具体类型 |

引擎侧：`setSpectrumSink(QObject*)` 用 `dynamic_cast<AudioSpectrumSource*>` 取句柄，失败则 `qWarning` 忽略；`publishSpectrumHandle()` 在 `m_spectrumMutex` 下替换；`acquireSpectrumHandle()` 用 `try_to_lock`，失败返回空 ⇒ 跳过本块。
析构顺序：`m_outputThread.wait()` **之后**才 `publishSpectrumHandle(nullptr)`（此时不会再有回调进订阅者）。

### 3.6.2 QML 侧消费链

```
AudioEngine.render()  --pushSamples(交错 float, 设备采样率)-->  GetWave(cpp/GetWave.cpp, QML_ELEMENT)
   GetWave::pushSamples: 降混单声道 + 写自己的 AudioRing（空间不足直接丢弃）+ m_dataReady=1
   GetWave::updateSpectrum(): 连在 QQuickWindow::afterAnimating → FFT(4096) → m_spectrumData / m_wavePath
   QML: Options.getWave.wavePath ─┬─ layout/MainLyric.qml:76（波形路径）
                                  ├─ lyricsui/LyricsFree.qml:81
                                  └─ lyricsui/Lyrics3D.qml:100（32 点采样折成 uLow/uMid/uHigh/uAir/uLevel）
```

- 实例化位置：`main.qml:788`（`GetWave { engine: mainMedia; renderWindow: window; bands: 128 }`），`main.qml:26` 注入 `Options.getWave`。
- QML 订阅者**必须实现** `AudioSpectrumSource`（返回 `shared_ptr<Handle>`）与 `AudioSpectrumSink`（`pushSamples`）；析构前**必须先** `m_handle->detach()` 再退订，不能依赖 `QObject::destroyed`（`GetWave::~GetWave` 的注释：destroyed 发出时派生类成员已析构）。
- 引擎只在 `frames>0` 的块内推送，采样率为**设备输出采样率**（DSP 之后），不含时间伸缩的重采样前信号。

---

## 3.7 解码层 `FfmpegDecoder`

### 3.7.1 `Meta` 字段（`FfmpegDecoder::Meta`）

| 字段 | 类型 | 填充来源 |
|---|---|---|
| `durationMs` | qint64 | `m_fmt->duration/(AV_TIME_BASE/1000)`，回退 `stream->duration * time_base * 1000` |
| `sampleRate` | int | `m_codecCtx->sample_rate`（**源**采样率） |
| `channels` | int | `ch_layout.nb_channels`，`<=0` 时回退 2 |
| `bitRate` | int | `codecCtx->bit_rate/1000`，回退 `fmt->bit_rate/1000`（kbps） |
| `codec` | QString | `avcodec_get_name(codec_id)` |
| `title` | QString | 标签 `title`；空则 `QFileInfo(pathOrUrl).completeBaseName()` |
| `tagTitle` | QString | 仅标签 `title`（stream 优先后回退 format），可空 |
| `artist` | QString | `artist`，空则 `album_artist` |
| `album` | QString | `album`（stream 优先，回退 format） |
| `date` | QString | `date` |
| `rgTrackDb` / `rgAlbumDb` / `rgTrackPeak` | double | `REPLAYGAIN_TRACK_GAIN` / `REPLAYGAIN_ALBUM_GAIN` / `REPLAYGAIN_TRACK_PEAK`，取字符串首段（空格前）转 double |

### 3.7.2 打开与格式

- **支持格式 = 链接的 FFmpeg 全量支持**，代码内无白名单/黑名单；只要求存在可解码音频流（`av_find_best_stream(AVMEDIA_TYPE_AUDIO)`）。
- 输出统一 `AV_SAMPLE_FMT_FLT` 交错，声道 1..2（`setTargetFormat` 钳制），采样率 8000..384000（钳制后送 `swr`）。
- 网络：`std::call_once` 里 `avformat_network_init()`；`rw_timeout=15000000`、`timeout=15000000`、`reconnect=1`、`reconnect_streamed=1`、`reconnect_delay_max=5`。
- UA/Referer：URL 含 `bilivideo`/`bilibili`（不区分大小写）→ Chrome UA + `referer=https://www.bilibili.com`（B 站 CDN 校验）；否则 `user_agent=QueMusic`。
- 自定义请求头：调用方拼 `"Key: Value\r\n"` 字符串传 `httpHeaders`，经 `av_dict_set(&opts,"headers",…)` 下发（WebDAV 的 `Authorization` 走这条）。
- `setTargetFormat()` 仅在 rate/channels 变化且 `m_codecCtx` 存在时重建 `swr`。
- `convertFrame()` 用 `swr_get_out_samples()` 预估容量 → `m_stage` 复用（`reserve` 后 `resize`，不每帧 new）。

### 3.7.3 seek 行为

```cpp
avformat_seek_file(m_fmt, -1, INT64_MIN, target*1000, INT64_MAX, AVSEEK_FLAG_BACKWARD);
avcodec_flush_buffers(m_codecCtx); swr_init(m_swr); av_packet_unref(m_pkt);
m_stageFrames = m_stageOffset = 0; m_eof = false;
m_skipUntilMs = target; m_lastPtsMs = target;
```

- 以 `AV_TIME_BASE`（微秒）为单位、`stream_index = -1`，即按全局时间基 seek，`AVSEEK_FLAG_BACKWARD` 偏向关键帧之前。
- 落地不精确：seek 后 `m_skipUntilMs > 0` 时在 `convertFrame()` 里用 `m_lastPtsMs` 逐帧丢弃；若目标落在帧中间，按 `dropMs * m_outRate / 1000` 设置 `m_stageOffset` 做样本级偏移，丢弃后清 `m_skipUntilMs`。
- PTS：优先 `best_effort_timestamp`，否则 `pts`；每帧后 `m_lastPtsMs += nb_samples/sampleRate`。
- 上层节流：`AudioEngine::decodeLoop()` 用 `kSeekThrottleMs = 90` 限制实际 seek 频率（拖动进度条会连续发请求）。

### 3.7.4 单线程理由（`FfmpegDecoder.cpp:138-141` 原注释）

> 音频解码用不上帧级多线程：`thread_count=0` 会让 FFmpeg 按 CPU 核数创建线程上下文及其私有缓冲（16 核即 16 份）。固定 1：48 kHz 立体声的解码开销远小于一个线程上下文的代价，而线程数与常驻缓冲会一直挂着。

- `abort()` 置 `m_abort`，`interrupt_callback` 返回 1 中断阻塞 IO；`AudioEngine::stopThread()` 先 `abort()` 再 `join()`。
- **API 不一致（坑）**：头文件注释写 `readFrames` 返回 `-1 = 出错`，实现从不返回 -1，出错路径一律返回 0（与 EOF 无法区分）。

---

## 3.8 着色器清单与 C++ 侧

### 3.8.1 着色器表

`qt6_add_shaders` 分两批（`CMakeLists.txt:272-299`，均 `BATCHABLE`）：
- `effectshaders`，`PREFIX "/shaders"` → 资源路径 `qrc:/shaders/shaders/<名>.frag.qsb`（QML `ShaderEffect` 用）。
- `bgshaders`，`PREFIX "/"` → 资源路径 `:/shaders/<名>.qsb`（C++ `setShaderFileName` 用）。

| 文件 | 类型 | 用途 | 被谁使用 |
|---|---|---|---|
| `background.vert` | vert | 通用顶点：`vUV = aTexCoord`，`gl_Position = qt_Matrix * aPosition`；std140 buf = mat4 qt_Matrix + qt_Opacity + u_time + vec2 u_resolution + vec4 u_colors[4] | `shaders/MeshGradientItem.cpp:126`（Fluid / Classic 共用） |
| `fluid.frag` | frag | 算法 0「Fluid」：域扭曲 + 8 个色斑（4 色各 2 条轨迹）反距离加权 + 暗角；不用封面纹理 | `MeshGradientItem.cpp:128`（algorithm == 0）；QML：`layout/MainLyric.qml:94`、`lyricsui/LyricsFree.qml:100`（`algorithm: Style.flowStyle`） |
| `classic.frag` | frag | 算法 1「Classic」：`sampler2D albumMap` 绕 (0.35,0.35) 缓慢旋转 + 暗角 + 抖动去色带；几何形变由扭曲网格顶点承担 | `MeshGradientItem.cpp:129`（algorithm != 0） |
| `stage.vert` | vert | `LyricsStage` 点云：`aId=(点序号,角序号)`，按 `uPreset` 算 3D 位置；与背景共用相机（yaw/pitch/lookAt、`kFocal=1.9`）；透视尺寸衰减 + `1/(1+0.02·depth²)` 亮度衰减；preset 0 星尘(环形体积云) / 1 光隧(圆管) / 2 星环(球面+赤道环) / 3 环绕星尘 | `shaders/LyricsStage.cpp:26`（`StageShader`） |
| `stage.frag` | frag | 点精灵：`discard` 圆外；core 硬盘 + halo `pow(1-d,3.5)*0.28`；输出 `alpha=0` + 预乘颜色 ⇒ Qt 预乘混合下等效加色发光 | `shaders/LyricsStage.cpp:27` |
| `lyricfade.frag` | frag | 歌词区上下 **渐隐 + 渐进高斯模糊**（uniform：fadeTop/fadeBottom/blurTop/blurBottom/blurRadius/srcSize；`sampler2D source`） | `layout/MainLyric.qml:244`、`lyricsui/LyricsFree.qml:300` |
| `lyricplane.frag` | frag | 3D 歌词平板：真透视投影（`uCamDist`/`uCamSens`/`uLyricScale`/`uLyricY`/`uLyricZ`/`uFloat`）；**固定 LOD 0** 采样保字形锐利；两圈 8 向采样累加 alpha 做锐利辉光；`uLineV`+`uSweep` 当前行发光、`uBeatGlow` 鼓点溢光；`sampler2D uLyrics` | `lyricsui/Lyrics3D.qml:273`（源为 `ShaderEffectSource id: lyricTex`） |
| `cardblur.frag` | frag | 卡片背景模糊：同一 mip 层级 + stride-1 的 2D tent 核（`TAPS = 7`）+ 饱和度 + 补底色 + 圆角 SDF（uniform：blur/blurMax/saturation/corner/cardSize/texSize/aa/fillColor；`sampler2D src`） | `components/QBlurCard.qml:74`（默认档）、`components/QBlurTapBar.qml:70` |
| `cardblur_hq.frag` | frag | 同算法，`TAPS = 13`（169 采样）；半径公式 `lod0 = log2(半径/R)` 使 R 变大后基底层级细一级、采样密一倍而模糊半径不变 | `components/QBlurCard.qml:74`（`root.highQuality`） |
| `bg_cubes.frag` | frag | 3D 预设 0「方块场」：同心圆波纹跳动的方块阵 + 场地圆丘 + 周边星尘 + 天空多层圆环 | `lyricsui/Lyrics3D.qml:29`（presetTable）→ `:149` 动态拼 `"qrc:/shaders/shaders/" + preset.s + ".frag.qsb"` |
| `bg_grid.frag` | frag | 预设 1「光栅地平」：波面网格，线条锐利自发光（`uField` 控制格距） | `Lyrics3D.qml:30` |
| `bg_stars.frag` | frag | 预设 2「平面星空」：多层星点 + 鼠标视差 + 银河带 + 闪烁，不做相机环绕 | `Lyrics3D.qml:31`；另 `lyricsui/LyricsFree.qml:174`（`bgStyle === 3`，用李萨如"假鼠标"） |
| `bg_beams.frag` | frag | 预设 3「光棱」：多层竖光柱，边缘锐利 + 上升流动 | `Lyrics3D.qml:32` |
| `bg_void.frag` | frag | 预设 4/5/6 底色：近黑 + 极淡星云，给点云做纵深对比（画面主体是 `LyricsStage`） | `Lyrics3D.qml:33-35` |
| `bg_stage.frag` | frag | 预设 7「舞台」：射线步进 SDF 实体场景（圆形多层平台 + 基座 + 多个 Torus 环，环随音频变粗/旋转） | `Lyrics3D.qml:36`（配合 `LyricsStage` preset 3 的点云） |

背景类 frag 共用同一套 uniform 名（`uColor,uColor2,uResolution,uMouse,uTime,uLow,uMid,uHigh,uAir,uLevel,uBright,uSaturate,uAudio,uField,uCamDist,uCamSens`），由 `Lyrics3D.qml:131-150` 的 `ShaderEffect` 注入；点云与背景共用同一相机参数是刻意的（`stage.vert` 顶部注释）。

### 3.8.2 `LyricsStage`（`shaders/LyricsStage.h/.cpp`）

- 类型：`QQuickItem` + `QML_ELEMENT`；`StageMaterial : QSGMaterial`（`Blending` 置位 = 加色发光）。
- Q_PROPERTY（全部 FINAL，均 `update()` 触发重绘）：`preset:int`、`count:int`（setter 里 `qMax(16,c)`，默认 14000）、`time:qreal`、`mouseX:qreal`、`mouseY:qreal`、`audioLow`、`audioMid`、`audioHigh`、`audioAir`、`audioLevel`（qreal，进 `StageMaterial::audio[0..4]` = Low/Mid/High/Air/Level）、`color1:QColor`（默认 `#00ee66`）、`color2:QColor`（默认 `#00b1ee`）、`audioGain:qreal`(1.0)、`camDist:qreal`(9.0)、`camSens:qreal`(1.0)、`density:qreal`(1.0)。
- `Q_INVOKABLE`：**无**。信号：每个属性声明了 `<name>Changed()`，但**只有 `setPreset`/`setCount`/`setColor1`/`setColor2` 真正 emit**；`setTime`/`setMouseX`/`setMouseY`/`setAudioLow..Level`/`setAudioGain`/`setCamDist`/`setCamSens`/`setDensity` 只调 `update()`（QML 用 `onTimeChanged` 之类的信号驱动逻辑会失效，见 §3.9-37）。
- 渲染：`updatePaintNode()` 建/复用 `QSGGeometryNode`；几何 = `count*6` 顶点、每顶点 1 个 `vec2` 属性 `aId=(点序号, 角序号)`（`Attribute::create(0,2,FloatType,true)`，`DrawTriangles`，`StaticPattern`），`count` 变化才重建（`m_geoCount`）；uniform 由 `StageShader::updateUniformData` 写进 **176 字节** std140 缓冲（`kBase = 80`；`Q_ASSERT(buf->size() >= 176)`）。`geometryChange()` 尺寸变化 → `update()`。
- `StageMaterial::compare()` 只比 `preset` 与 `count`（批次分组）；`BackgroundShader` 的 `compare` 比 algorithm/texture/colors。
- 实例化：**只有** `lyricsui/Lyrics3D.qml:153`（`visible: preset.p >= 0`，`count: 14000`，音频五段接 `uLow..uLevel`）。

### 3.8.3 `MeshGradientItem`（`shaders/MeshGradientItem.h/.cpp`）

- 类型：`QQuickItem` + `QML_ELEMENT`；`BackgroundMaterial : QSGMaterial`（`Blending` 关闭）。
- Q_PROPERTY：`coverUrl:QUrl`(RW)、`color1`/`color2`/`color3:QColor`(RW)、`algorithm:int`(RW，设置后 `update()`；材质类型由 algorithm 决定 `fluidType`/`classicType`)、`animating:bool`(默认 true)、`flowSpeed:qreal`(默认 1.0)。
- `Q_INVOKABLE`：**无**。信号：每个属性一个 `<name>Changed()`。
- 封面管线：`coverUrl` → http(s) 走 `QNetworkAccessManager`，其余（本地 / `data:` base64 / `qrc:`）走 `QtConcurrent::run` → `decodeCover()`（缩到 32×32 → 半透明像素按 alpha 合成到均值色 → 预压对比 0.5 → 饱和 2.6 → 对比 1.4 → 亮度 0.8 → `boxBlur(2,4)`）→ `applyCover()` 存 `m_coverImage` 并**清空 `m_geoSize`**（切歌重掷地形）→ `updatePaintNode()` 内 `window()->createTextureFromImage()`，纹理设 `Linear` + `MirroredRepeat`；无封面用 4×4 纯色兜底。
- 几何：algorithm 0 → 4 顶点 `DrawTriangleStrip` 全屏四边形（形变全在片元）；algorithm != 0 → `makeWarpGeometry()` 28×28 格 = 841 顶点 / 4704 三角（`UnsignedShort` 索引），顶点位置 = 基底 + 钉边驻波模态（山坡）+ `tanh` 折叠面（悬崖），地形预设从 5 组 `kTerrainPresets` 随机取且不与上次重复（**静态 `lastPreset`，非线程安全但只在 GUI/渲染线程调用**）。
- 动画：`QTimer` 16ms（≈60fps），`m_time += elapsed/1000 * flowSpeed`；`syncTimer()` 仅当 `m_animating && isVisible() && window()` 时运行（`itemChange(ItemVisibleHasChanged)` 与 `windowChanged` 触发）。
- uniform：**144 字节** std140（mat4 + qt_Opacity + time + vec2 resolution + vec4 colors[4]）写在偏移 0/64/68/72/80+16i；`colors[3] = color3.darker(200)`（第 4 色不可由 QML 直接设置）；封面纹理经 `updateSampledImage(binding == 1)` 绑定。
- 实例化：`layout/MainLyric.qml:94`、`lyricsui/LyricsFree.qml:100`（后者 `visible: LyricsFreeConfig.bgStyle === 0 && Style.flowStyle !== 2`）。

### 3.8.4 构建约定

- Qt 6 **不做运行时着色器编译**：改 `.frag`/`.vert` 后必须由 `qt6_add_shaders` 重新生成 `.qsb`（见 `CONTRIBUTING.md:44`）。
- QML 侧 `ShaderEffect` 只能引用 `qrc:/shaders/shaders/*.qsb`（`effectshaders` 的 PREFIX 造成名字里出现两个 `shaders`）；C++ 侧用 `:/shaders/*.qsb`。混用会静默拿不到文件。

---

## 3.9 坑与约定

线程 / 所有权

1. **音频与解码线程绝不触碰 QObject/QML**：状态变更与信号一律经 `postToSelf()`（同线程直调，否则 `QueuedConnection`）；解码线程自己用 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 投递 `applyMeta`/`emitError`。
2. **缓冲重建必须冻结两个音频线程**（`setupSink()` 注释）：`resize/configure` 会释放旧内存，而音频线程手里还攥着旧指针，并发即踩坏堆。协议：`m_audioFrozen = true` → 等 `m_decodeParked`（最多 200×1ms）→ `invokeOnOutput` 内重建（与 `render` 同线程，天然互斥）→ `m_audioFrozen = false`。
3. **构造期不要等解码线程停靠**（`AudioEngine.cpp:227-234`）：`startThread()` 尚未执行时没人会置 `m_decodeParked`，200 次 `sleep_for(1ms)` 在 Windows 上按 ~15.6ms 计时粒度会阻塞约 3.1 秒；因此加了 `if (m_thread.joinable())` 守卫。
4. **解码线程绝不改动缓冲尺寸**：`beginGeneration()` 只 `clear()`；尺寸只在重建路径更新（更新前先停靠解码线程）。
5. **`sink` 与拉取定时器必须在输出线程创建**（`setupSink` 注释）：否则 `readData` 落在 GUI 线程，窗口一卡音频就断。
6. **设备对象（`m_io`）跨 sink 重建复用**：Qt 音频后端可能在 sink 销毁后仍投递一次读取，此时设备必须还活着。销毁顺序：`m_quit` → `stopThread()` → `teardownSink()` → 在输出线程内 `close()+delete m_io` → `m_outputThread.quit()/wait()` → `publishSpectrumHandle(nullptr)` → `delete m_outputContext`。
7. **`QAudioSink::stateChanged` 回调要校验 `sender() == m_sink`**：信号队列投递到 GUI 线程时 `m_sink` 可能已换新对象，否则切歌会被误判成设备故障而重建输出（重建另有 3000ms 节流 `m_lastSinkErrorMs`）。

seek / 位置

8. **seek 落地前禁止发布位置**：`m_pendingSeekMs >= 0` 时 `render()` 不写 `m_positionMs`；换段时 `qint64(m_localMs)==m_pendingSeekMs` 才清除标记。
9. **没人执行的 seek 必须丢弃**：`requestSeek()` 在 `!m_decodeActive && !m_openRequest` 时把 `m_seekRequest` 置 -1，避免留到下次解码时莫名生效；`setPosition()` 同理不能挂起标记（`canSeek` 判定），否则位置会永久停住。
10. **拖动会排队多个 seek**：`beginGeneration()` 只在 `pending < 0 || pending == baseMs` 时写 `m_positionMs`，防止旧请求落地覆盖 GUI 刚发布的目标；解码线程另有 90ms 节流。
11. 换段时位置按**源时长**推进：`posHz = stretching ? rate/speed : m_decodeRate`（倍速 + 保持音高时环内采样率与设备不同）。
12. 换段必有 ≥12ms 淡入（`kClickGuardMs`）防爆音；`fadeInOnNextAudio` 同时立刻 `fadeIn()` 一次（`jumpFade(0)` + `rampFade(1)` 会在下一段首批样本到位时再次执行）。

DSP / 音频路径

13. **`dspEnabled == false`（bypass）会跳过 EQ、声道工具、限幅与硬裁剪**，只保留音量与淡变 —— 也就是说关闭 DSP 后输出不会被钳到 ±1.0。
14. **限幅关闭时仍逐样本硬裁剪** `qBound(-1.0, v, 1.0)`（float 溢出保护），不要误以为「关限幅 = 不裁剪」。
15. **auto headroom 用所有波段增益的最大值**（不区分该波段是否启用，只看 `gainDb != 0` 与 `max(0, ·)`），从 `preamp + ReplayGain` 里减掉。
16. **ReplayGain 的 preventClip 上限**：`peak > 0` 时 `gain = min(gain, -20*log10(peak))`；`peak` 为 0（无标签）时不限制。
17. **热路径不加锁**：`applyPendingParams()` 先查 `m_paramsPending`（acquire）直接返回；`acquireSpectrumHandle()` 用 `try_to_lock`，抢不到就跳过本块（只在换订阅者的一瞬间发生）。
18. **音频回调内禁止分配**：`AudioDsp::setEqGains(const double*, int)` 专为音频线程提供（不构造 `QList`）；`m_scratch`/`m_stretchIn`/`m_stretch` 全部预分配；`PitchShifter::pull` 里的 `m_buf.erase/resize` 是唯一会移动内存的路径（在音频线程内，容量受 `frames` 约束）。
19. **float + 不伸缩时直接写设备缓冲**（`direct`），此时 `m_scratch` 不参与；Int16 输出或伸缩模式下才走 `m_scratch` → 转换。
20. `AudioRing` 游标 `quint64` 单调不回卷（注释：48 kHz 下 `int` 会在约 6 小时后溢出）；`available()` 夹到 `INT_MAX`；`configure()` **会 resize**，只能在校准过的冻结窗口内调用。
21. `reconfigureOutput()` 会保留当前曲目与位置：停 sink → `setupSink()` → 重设 `m_pendingUrl/m_positionMs/m_pendingSeekMs/m_seekRequest` → `m_openRequest = true`（解码线程重新 open 并 seek）。
22. `decodeRateFor()`：`pitchCompensation || pitch != 1` 时解码按设备采样率原速（变速交给 SOLA），否则 `rate = deviceRate*1000/max(250,rateMilli)` 并钳 8000..384000；倍速改后必须 `requestSeek(position())` 重建数据段。
23. `AudioEngine::m_chunkFrames` 是**未被任何代码读取的死字段**（构造为 4096）。
24. `outputFormatName()` 直接读 `m_format`（GUI 线程）；`m_format` 由输出线程在 `setupSink()` 内写 —— 当前调用点都在 sink 建立之后，属既有顺序假设。

频谱

25. **绝不保存订阅者的裸指针**：`destroyed` 信号在 QObject 析构过程中发出，派生类成员往往已析构，音频线程此前读到旧地址仍会踩已销毁对象。改为 `shared_ptr<Handle>` + `detach()`。
26. `detach()` 必须在订阅者析构体**开头**调用（`GetWave::~GetWave`），之后才退订（`engine->setSpectrumSink(nullptr)`）。
27. `pushSamples` 必须无阻塞、不分配、不取长锁；`GetWave` 的做法是降混后写自己的 `AudioRing`，满了直接丢弃（`绝不阻塞音频线程`）。
28. 引擎只在 `frames > 0` 的块推送，且推的是 **DSP 之后**、设备采样率下的交错 float。

解码

29. `readFrames` 的头文件注释声称 `-1 = 出错`，实现从不返回 -1（**以实现为准**）。
30. seek 是「粗 seek + 逐帧 PTS 丢弃」，落地精度受帧长与重采样器状态影响；seek 后必须 `swr_init` + `avcodec_flush_buffers`，否则残留样本会串到新位置。
31. `setTargetFormat()` 会重建 swr（丢弃其内部残留），仅允许解码线程调用；`targetSampleRate()` 供解码线程比对。

QSG / 着色器

32. `QSGMaterialShader::updateUniformData` 内必须按 std140 布局精确写偏移：`LyricsStage` 176B（`kBase = 80`，`Q_ASSERT`），`MeshGradientItem` 144B（偏移 0/64/68/72/80+16i）。改 `.vert`/`.frag` 的 `buf` 成员顺序必须同步改 C++。
33. `QSGGeometryNode` 用 `OwnsGeometry` + `OwnsMaterial`，重建几何/材质时旧对象由其释放；几何只在 `count` / `algorithm` / 尺寸变化时重建。
34. `LyricsStage::StageMaterial` 置 `Blending`；`stage.frag` 输出 `alpha = 0` + 预乘颜色，靠预乘混合得到加色发光（若改成普通 alpha 混合会变成黑块）。
35. 着色器改动必须重新生成 `.qsb`（Qt 6 无运行时编译），且注意两批 `qt6_add_shaders` 的 PREFIX 差异（`qrc:/shaders/shaders/…` vs `:/shaders/…`）。
36. `MeshGradientItem::makeWarpGeometry()` 用函数内 `static int lastPreset` 保证「不连续重复」——只在 GUI/渲染线程调用；若将来从多线程调用需改为实例成员。
37. **`LyricsStage` 多数 setter 不发信号**：`time`/`mouseX`/`mouseY`/`audioLow..audioLevel`/`audioGain`/`camDist`/`camSens`/`density` 只 `update()`，对应的 `*Changed` 信号永不 emit（也不做 `qFuzzyCompare` 去抖）；依赖这些信号做二次计算或属性绑定的 QML 会静默不更新。`MeshGradientItem` 则所有 setter 都正确 emit。

未确认项

- 音量 / 播放设备 / 播放倍速的持久化键名在 `components/Config.qml` / `components/Playback.qml`，本文档未逐一核对（只确认 `AudioEngine` 自身不保存）。
- `bg_cubes.frag` / `bg_beams.frag` 注释里的「预设 0 / 预设 5」是着色器内部的旧编号，与 `Lyrics3D.qml` 的 `presetTable` 序号（0/3）不一致 —— 以 QML 表为准。
- `AudioDsp::presetGains()` 命中失败返回全 0 数组，而 `applyEqPreset()` 仍会把 `eqEnabled` 置 true（QML 若传空串会得到「空 EQ 但显示开启」）。
---

← 返回 [索引 README.md](README.md)
