// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "AudioEngine.h"

#include "AudioSpectrumSink.h"

#include <QAudio>
#include <QAudioSink>
#include <QDir>
#include <QFileInfo>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMediaDevices>
#include <QMetaObject>
#include <QSettings>
#include <QStringList>
#include <QThread>
#include <QTimer>

#include <chrono>
#include <cstring>

namespace {

constexpr int kRingSeconds = 2;
constexpr int kMaxChunkFrames = 4096;
constexpr int kMaxStretchInputFrames = 24576;
constexpr int kPollIntervalMs = 50;
constexpr qint64 kSeekThrottleMs = 90;
// 缓冲 10~100ms：再小易爆音，再大拖慢进度与切歌响应
constexpr int kBufferMsMin = 10;
constexpr int kBufferMsMax = 100;
constexpr int kBufferMsDefault = 30;
// 设备缓冲的帧数下限：10ms@48k 仅 480 帧，下限过大会把它顶回去
constexpr int kMinBufferFrames = 256;
// 每次切换数据段的最小淡入长度，消除切换爆音
constexpr int kClickGuardMs = 12;

qint64 steadyMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// QML 侧既可能传 file:// URL，也可能直接传裸路径（Windows 盘符会被 QUrl 当成 scheme）
QString toSourceUrl(const QUrl &url)
{
    if (url.isEmpty())
        return QString();

    QStringList candidates;
    if (url.isLocalFile())
        candidates.append(url.toLocalFile());
    const QString raw = url.toString(QUrl::FullyDecoded);
    candidates.append(raw);
    if (url.scheme().size() == 1)
        candidates.append(url.scheme() + QLatin1Char(':') + url.path());
    candidates.append(url.path());

    for (const QString &candidate : candidates) {
        if (!candidate.isEmpty() && QFileInfo::exists(candidate))
            return QString(candidate).replace(QLatin1Char('\\'), QLatin1Char('/'));
    }
    return raw;
}

int framesForBytes(const QAudioFormat &format, qsizetype bytes)
{
    const int frameBytes = format.bytesPerFrame();
    return frameBytes > 0 ? int(bytes / frameBytes) : 0;
}

} // namespace

class EngineIoDevice : public QIODevice
{
public:
    explicit EngineIoDevice(AudioEngine *engine) : m_engine(engine) {}

    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override { return 1 << 20; }

protected:
    qint64 readData(char *data, qint64 maxlen) override
    {
        return m_engine ? m_engine->render(data, maxlen) : 0;
    }
    qint64 writeData(const char *, qint64) override { return 0; }

private:
    AudioEngine *m_engine;
};

AudioEngine::AudioEngine(QObject *parent)
    : QObject(parent)
{
    for (auto &ring : m_rings)
        ring.configure(kRingSeconds * 48000, 2);
    m_scratch.resize(size_t(kMaxChunkFrames) * 2);
    m_stretchIn.resize(size_t(kMaxStretchInputFrames) * 2);
    m_stretch.prepare(2, 48000);
    m_dsp.prepare(48000.0);
    loadSettings();
    {
        QMutexLocker lock(&m_paramMutex);
        m_eqSnapshot = m_eq;
        m_paramsPending.store(true, std::memory_order_release);
    }

    m_outputContext = new QObject;
    m_outputContext->moveToThread(&m_outputThread);
    m_outputThread.setObjectName(QStringLiteral("QueMusicAudioOutput"));
    m_outputThread.start();

    setupSink();
    startThread();

    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(400);
    connect(m_saveTimer, &QTimer::timeout, this, &AudioEngine::saveSettings);

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(kPollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &AudioEngine::pollState);
    m_pollTimer->start();
}

AudioEngine::~AudioEngine()
{
    if (m_pollTimer)
        m_pollTimer->stop();
    if (m_saveTimer && m_saveTimer->isActive()) {
        m_saveTimer->stop();
        saveSettings();
    }
    m_quit.store(true);
    stopThread();
    teardownSink();
    // 输出线程还活着时在本线程内释放设备，避免跨线程析构
    invokeOnOutput([this] {
        if (m_io) {
            m_io->close();
            delete m_io;
            m_io = nullptr;
        }
    });
    m_outputThread.quit();
    m_outputThread.wait();
    delete m_outputContext;
    m_outputContext = nullptr;
}

// ---------------------------------------------------------------- 设备与输出

QAudioDevice AudioEngine::resolveDevice() const
{
    const QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();
    if (!m_deviceId.isEmpty()) {
        for (const QAudioDevice &device : outputs) {
            if (QString::fromUtf8(device.id()) == m_deviceId)
                return device;
        }
    }
    return QMediaDevices::defaultAudioOutput();
}

// 解码输出采样率与倍速成反比：设备每秒固定取走 deviceRate 个样本，
// 采样率越低则每秒消耗的源时长越多，播放越快。
int AudioEngine::decodeRateFor(int deviceRate) const
{
    // 保持音高或变调时，变速交给 SOLA/重采样链，解码按原速
    if (m_pitchCompensation.load(std::memory_order_relaxed)
        || m_pitchMilli.load(std::memory_order_relaxed) != 1000)
        return deviceRate;
    const int rateMilli = qMax(250, m_rateMilli.load(std::memory_order_relaxed));
    return qBound(8000, int(qint64(deviceRate) * 1000 / rateMilli), 384000);
}

bool AudioEngine::setupSink()
{
    teardownSink();

    const QAudioDevice device = resolveDevice();
    if (device.isNull())
        return false;

    QAudioFormat format = device.preferredFormat();
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Float);
    if (!device.isFormatSupported(format)) {
        format.setSampleFormat(QAudioFormat::Int16);
        if (!device.isFormatSupported(format))
            format = device.preferredFormat();
    }
    if (format.sampleRate() <= 0)
        format.setSampleRate(48000);
    if (format.channelCount() <= 0)
        format.setChannelCount(2);

    // 指定采样率只在设备支持时生效，否则回退到设备首选
    if (m_requestedRate > 0) {
        QAudioFormat requested = format;
        requested.setSampleRate(m_requestedRate);
        if (device.isFormatSupported(requested))
            format = requested;
    }

    m_format = format;
    const int rate = format.sampleRate();
    const int channels = format.channelCount();
    m_outSampleRate.store(rate, std::memory_order_relaxed);
    m_outChannels.store(channels, std::memory_order_relaxed);

    // 重建缓冲必须与两个音频线程互斥：resize/configure 会释放旧内存，
    // 而音频线程手里还攥着旧指针，并发就是直接踩坏堆。
    // 解码线程先停靠，缓冲本身在输出线程内重建（与 render 同线程，天然互斥）
    m_audioFrozen.store(true, std::memory_order_release);
    // 只有解码线程在跑时才需要等它停靠。构造期 startThread() 尚未执行，没有任何线程会
    // 置位 m_decodeParked，这个循环必然跑满 200 次；而 Windows 上 sleep_for(1ms) 实际按
    // 系统计时粒度（约 15.6ms）休眠，200 次就是约 3.1 秒的启动阻塞。
    // 解码线程不活跃时它只在 sleep 循环里等待、不触碰环形缓冲，因此跳过等待是安全的。
    if (m_thread.joinable()) {
        for (int i = 0; i < 200 && !m_decodeParked.load(std::memory_order_acquire); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    m_decodeActive.store(false, std::memory_order_relaxed);

    invokeOnOutput([this, rate, channels] {
        m_dsp.prepare(double(rate));
        m_stretch.prepare(channels, rate);
        m_stretchIn.resize(size_t(kMaxStretchInputFrames) * size_t(qMax(1, channels)));
        m_scratch.resize(size_t(kMaxChunkFrames) * size_t(qMax(1, channels)));
        for (auto &ring : m_rings)
            ring.configure(kRingSeconds * rate, channels);
    });

    m_audioFrozen.store(false, std::memory_order_release);
    m_ringGen.store(0, std::memory_order_relaxed);
    m_ringBaseMs.store(0.0, std::memory_order_relaxed);

    {
        QMutexLocker lock(&m_paramMutex);
        m_eqSnapshot = m_eq;
        m_paramsPending.store(true, std::memory_order_release);
    }

    // sink 必须在输出线程里创建：拉取定时器跟着 sink 所在线程走，
    // 否则 readData 会落在 GUI 线程上，窗口一卡音频就断
    invokeOnOutput([&] {
        m_sink = new QAudioSink(device, m_format);
        m_sink->setBufferSize(qMax<qsizetype>(
            m_format.bytesForDuration(qBound(kBufferMsMin, m_bufferMs, kBufferMsMax) * 1000),
            m_format.bytesForFrames(kMinBufferFrames)));
        m_sink->setVolume(1.0);
        // 设备对象跨重建复用：Qt 音频后端可能在 sink 销毁后仍投递一次读取，此时设备必须还活着
        if (!m_io) {
            m_io = new EngineIoDevice(this);
            m_io->open(QIODevice::ReadOnly);
        }

        // 信号经队列投递到 GUI 线程，届时 m_sink 可能已换成新对象；
        // 只有发出者仍是当前 sink 时才采信，否则切歌会被误判成设备故障而重建输出
        connect(m_sink, &QAudioSink::stateChanged, this, [this](QAudio::State state) {
            auto *sink = qobject_cast<QAudioSink *>(sender());
            if (!sink || sink != m_sink)
                return;
            if (state == QAudio::StoppedState && sink->error() != QAudio::NoError)
                m_sinkErrorFlag.store(true, std::memory_order_relaxed);
        });
    });

    emit outputFormatChanged();

    return true;
}

void AudioEngine::invokeOnOutput(std::function<void()> fn)
{
    if (!m_outputContext || !m_outputThread.isRunning()
        || QThread::currentThread() == &m_outputThread) {
        fn();
        return;
    }
    QMetaObject::invokeMethod(m_outputContext, std::move(fn), Qt::BlockingQueuedConnection);
}

void AudioEngine::teardownSink()
{
    if (!m_sink)
        return;
    invokeOnOutput([this] {
        if (m_sink) {
            m_sink->stop();
            delete m_sink;
            m_sink = nullptr;
        }
    });
}

void AudioEngine::startSink()
{
    if (!m_sink || !m_io)
        return;
    invokeOnOutput([this] {
        if (!m_sink || !m_io)
            return;
        if (m_sink->state() == QAudio::SuspendedState)
            m_sink->resume();
        else if (m_sink->state() == QAudio::StoppedState)
            m_sink->start(m_io);
    });
}

void AudioEngine::stopSink()
{
    if (!m_sink)
        return;
    invokeOnOutput([this] {
        if (m_sink && m_sink->state() != QAudio::StoppedState)
            m_sink->stop();
    });
}

QString AudioEngine::outputFormatName() const
{
    if (!m_sink)
        return tr("无输出设备");
    const QString sampleFormat = (m_format.sampleFormat() == QAudioFormat::Float) ? QStringLiteral("Float32")
                              : (m_format.sampleFormat() == QAudioFormat::Int16) ? QStringLiteral("Int16")
                                                                                 : QStringLiteral("PCM");
    return QStringLiteral("%1 · %2 Hz · %3ch")
        .arg(sampleFormat)
        .arg(m_outSampleRate.load(std::memory_order_relaxed))
        .arg(m_outChannels.load(std::memory_order_relaxed));
}

// ------------------------------------------------------------------ 播放控制

void AudioEngine::setSource(const QUrl &url)
{
    if (m_source == url)
        return;
    m_source = url;
    emit sourceChanged();
    if (url.isEmpty()) {
        setPlaybackState(StoppedState);
        setMediaStatus(NoMedia);
        return;
    }

    m_durationMs = 0;
    m_meta = FfmpegDecoder::Meta();
    m_rgTrackDb.store(0.0, std::memory_order_relaxed);
    m_rgAlbumDb.store(0.0, std::memory_order_relaxed);
    m_rgPeak.store(0.0, std::memory_order_relaxed);
    m_positionMs.store(0, std::memory_order_relaxed);
    m_lastEmittedPosition = -1;
    m_decodeEof.store(false, std::memory_order_relaxed);
    m_drained.store(false, std::memory_order_relaxed);
    m_seekRequest.store(-1, std::memory_order_relaxed);
    // 新音源从 0 开始：在首段落地前不发布位置，避免进度条弹回上一首
    m_pendingSeekMs.store(0, std::memory_order_release);

    {
        QMutexLocker lock(&m_pendingUrlMutex);
        m_pendingUrl = url;
    }
    setMediaStatus(LoadingMedia);
    emit durationChanged();
    emit metaDataChanged();
    m_openRequest.store(true);
}

void AudioEngine::play()
{
    if (m_source.isEmpty())
        return;
    // 上次若停在静音（淡出/睡眠），起播时补一次淡入，避免"能播但没声"
    if (m_fadeTargetMilli.load(std::memory_order_relaxed) < 1000)
        fadeInOnNextAudio(180);
    if (m_mediaStatus == EndOfMedia) {
        setMediaStatus(LoadedMedia);
        requestSeek(0);
        m_decodeEof.store(false, std::memory_order_relaxed);
    }
    m_drained.store(false, std::memory_order_relaxed);
    startSink();
    setPlaybackState(PlayingState);
}

void AudioEngine::pause()
{
    if (m_playbackState != PlayingState)
        return;
    if (m_sink)
        invokeOnOutput([this] { if (m_sink) m_sink->suspend(); });
    setPlaybackState(PausedState);
}

void AudioEngine::stop()
{
    stopSink();
    m_drained.store(false, std::memory_order_relaxed);
    m_positionMs.store(0, std::memory_order_relaxed);
    m_pendingSeekMs.store(0, std::memory_order_release);
    emit positionChanged();
    if (m_playbackState != StoppedState)
        setPlaybackState(StoppedState);
    if (!m_source.isEmpty() && m_mediaStatus != NoMedia)
        setMediaStatus(LoadedMedia);
    requestSeek(0);
}

void AudioEngine::togglePause()
{
    if (m_playbackState == PlayingState)
        pause();
    else
        play();
}

void AudioEngine::setPosition(qint64 ms)
{
    const qint64 bound = m_durationMs > 0 ? qMin(ms, m_durationMs) : ms;
    const qint64 target = qMax<qint64>(0, bound);
    // 挂起位置发布，直到新数据段落地；无人执行 seek 时不能挂起，
    // 否则标记没人清除，位置会永久停住
    const bool canSeek = m_decodeActive.load(std::memory_order_relaxed)
                         || m_openRequest.load(std::memory_order_relaxed);
    m_pendingSeekMs.store(canSeek ? target : -1, std::memory_order_release);
    m_positionMs.store(target, std::memory_order_relaxed);
    emit positionChanged();
    if (m_mediaStatus == EndOfMedia) {
        setMediaStatus(LoadedMedia);
        m_decodeEof.store(false, std::memory_order_relaxed);
        m_drained.store(false, std::memory_order_relaxed);
    }
    requestSeek(target);
}

void AudioEngine::requestSeek(qint64 ms)
{
    if (!m_decodeActive.load(std::memory_order_relaxed) && !m_openRequest.load(std::memory_order_relaxed)) {
        // 没人执行就丢掉，别留到下次解码时莫名生效
        m_seekRequest.store(-1, std::memory_order_relaxed);
        return;
    }
    m_drained.store(false, std::memory_order_relaxed);
    m_seekRequest.store(ms, std::memory_order_relaxed);
}

void AudioEngine::setVolume(qreal v)
{
    const qreal clamped = qBound(0.0, v, 1.0);
    if (qFuzzyCompare(m_volume, clamped))
        return;
    m_volume = clamped;
    m_volumeAtomic.store(double(clamped), std::memory_order_relaxed);
    emit volumeChanged();
}

void AudioEngine::fadeTo(qreal target, int ms)
{
    m_fadeTargetMilli.store(qBound(0, int(std::lround(double(target) * 1000.0)), 1000),
                            std::memory_order_relaxed);
    m_fadeFrames.store(qBound(0, int(double(ms) * qMax(8000, m_outSampleRate.load()) / 1000.0),
                              1 << 22), std::memory_order_relaxed);
    m_fadeSeq.fetch_add(1, std::memory_order_release);
}

void AudioEngine::fadeOut(int ms) { fadeTo(0.0, ms); }

void AudioEngine::fadeIn(int ms) { fadeTo(1.0, ms); }

void AudioEngine::setPitchSemitones(qreal semitones)
{
    const qreal clamped = qBound(-12.0, semitones, 12.0);
    if (qFuzzyCompare(m_pitchSemitones, clamped))
        return;
    m_pitchSemitones = clamped;
    const double ratio = std::pow(2.0, double(clamped) / 12.0);
    m_pitchMilli.store(int(std::lround(ratio * 1000.0)), std::memory_order_relaxed);
    emit pitchChanged();
    emit outputFormatChanged();
    scheduleSave();
    requestSeek(position());
}

void AudioEngine::fadeInOnNextAudio(int ms)
{
    m_pendingFadeInFrames.store(qBound(0, int(double(ms) * qMax(8000, m_outSampleRate.load())
                                             / 1000.0), 1 << 22),
                                std::memory_order_relaxed);
    fadeIn(ms);
}

void AudioEngine::setPlaybackRate(qreal rate)
{
    const qreal clamped = qBound(0.25, rate, 4.0);
    if (qFuzzyCompare(m_playbackRate, clamped))
        return;
    m_playbackRate = clamped;
    m_rateMilli.store(int(std::lround(clamped * 1000.0)), std::memory_order_relaxed);
    m_stretchSpeedMilli.store(m_rateMilli.load(std::memory_order_relaxed), std::memory_order_relaxed);
    emit playbackRateChanged();
    emit outputFormatChanged();
    // 变速会改变解码输出采样率，必须重建数据段并从当前位置续播
    requestSeek(position());
}

void AudioEngine::setPitchCompensation(bool on)
{
    if (pitchCompensation() == on)
        return;
    m_pitchCompensation.store(on, std::memory_order_relaxed);
    emit pitchCompensationChanged();
    emit outputFormatChanged();
    requestSeek(position());
}

void AudioEngine::setOutputSampleRate(int rate)
{
    const int clamped = rate <= 0 ? 0 : qBound(8000, rate, 384000);
    if (m_requestedRate == clamped)
        return;
    m_requestedRate = clamped;
    scheduleSave();
    reconfigureOutput();
}

int AudioEngine::bufferMs() const { return m_bufferMs; }

void AudioEngine::setBufferMs(int ms)
{
    // 非正数视为未设置，落回默认缓冲
    const int clamped = ms <= 0 ? kBufferMsDefault : qBound(kBufferMsMin, ms, kBufferMsMax);
    if (m_bufferMs == clamped)
        return;
    m_bufferMs = clamped;
    scheduleSave();
    reconfigureOutput();
}

void AudioEngine::setDspEnabled(bool on)
{
    if (dspEnabled() == on)
        return;
    m_eq.masterEnabled = on;
    m_dspEnabled.store(on, std::memory_order_relaxed);
    publishParams(false, true, false, false);
    scheduleSave();
}

void AudioEngine::setEqQ(qreal q)
{
    const double value = qBound(0.2, double(q), 8.0);
    if (qFuzzyCompare(m_eq.q, value))
        return;
    m_eq.q = value;
    publishParams(true, true, false, false);
}

// 改设备格式/缓冲后重开输出：保留当前曲目与播放位置
void AudioEngine::reconfigureOutput()
{
    const bool wasPlaying = (m_playbackState == PlayingState);
    const qint64 pos = position();
    stopSink();
    if (!setupSink()) {
        setPlaybackState(StoppedState);
        emit outputFormatChanged();
        return;
    }
    if (m_source.isEmpty()) {
        setPlaybackState(wasPlaying ? StoppedState : m_playbackState);
        emit outputFormatChanged();
        return;
    }

    {
        QMutexLocker lock(&m_pendingUrlMutex);
        m_pendingUrl = m_source;
    }
    m_positionMs.store(pos, std::memory_order_relaxed);
    m_pendingSeekMs.store(pos, std::memory_order_release);
    m_seekRequest.store(pos, std::memory_order_relaxed);
    m_openRequest.store(true);
    setPlaybackState(wasPlaying ? PlayingState : StoppedState);
    emit outputFormatChanged();
}

void AudioEngine::setDeviceId(const QString &id)
{
    if (m_deviceId == id)
        return;
    m_deviceId = id;
    emit deviceIdChanged();
    reconfigureOutput();
}

// ------------------------------------------------------------------ 状态发布

void AudioEngine::setPlaybackState(PlaybackState state)
{
    if (m_playbackState == state)
        return;
    const bool wasPlaying = (m_playbackState == PlayingState);
    m_playbackState = state;
    m_playingFlag.store(state == PlayingState, std::memory_order_relaxed);
    emit playbackStateChanged();
    if (wasPlaying != (state == PlayingState))
        emit playingChanged();
}

void AudioEngine::setMediaStatus(MediaStatus status)
{
    if (m_mediaStatus == status)
        return;
    m_mediaStatus = status;
    emit mediaStatusChanged();
}

void AudioEngine::postToSelf(std::function<void()> fn)
{
    if (QThread::currentThread() == thread())
        fn();
    else
        QMetaObject::invokeMethod(this, std::move(fn), Qt::QueuedConnection);
}

void AudioEngine::emitError(int code, const QString &message)
{
    postToSelf([this, code, message] {
        setMediaStatus(InvalidMedia);
        emit errorOccurred(code, message);
    });
}

void AudioEngine::reportSinkError()
{
    if (!m_sinkErrorFlag.exchange(false, std::memory_order_relaxed))
        return;
    const qint64 now = steadyMs();
    if (now - m_lastSinkErrorMs < 3000)
        return;
    m_lastSinkErrorMs = now;
    qWarning() << "AudioEngine: 音频输出设备异常，正在重建输出";
    reconfigureOutput();
}

void AudioEngine::applyMeta(const FfmpegDecoder::Meta &meta)
{
    m_meta = meta;
    m_durationMs = meta.durationMs;
    m_rgTrackDb.store(meta.rgTrackDb, std::memory_order_relaxed);
    m_rgAlbumDb.store(meta.rgAlbumDb, std::memory_order_relaxed);
    m_rgPeak.store(meta.rgTrackPeak, std::memory_order_relaxed);
    {
        QMutexLocker lock(&m_paramMutex);
        m_paramsPending.store(true, std::memory_order_release);
    }
    emit durationChanged();
    emit metaDataChanged();
    setMediaStatus(LoadedMedia);
}

void AudioEngine::pollState()
{
    reportSinkError();

    if (m_drained.exchange(false, std::memory_order_relaxed)) {
        // 不停止输出：保持设备常开，下一首起播时无设备重开的空档
        setPlaybackState(StoppedState);
        if (m_durationMs > 0)
            m_positionMs.store(m_durationMs, std::memory_order_relaxed);
        emit positionChanged();
        setMediaStatus(EndOfMedia);
        return;
    }

    if (m_playbackState == StoppedState)
        return;
    const qint64 pos = position();
    if (pos != m_lastEmittedPosition) {
        m_lastEmittedPosition = pos;
        emit positionChanged();
    }
}

// ------------------------------------------------------------------- 音频线程

void AudioEngine::applyPendingParams()
{
    // 热路径快速返回：稳态下音频线程不触碰互斥量
    if (!m_paramsPending.load(std::memory_order_acquire))
        return;

    EqParams p;
    {
        QMutexLocker lock(&m_paramMutex);
        m_paramsPending.store(false, std::memory_order_release);
        p = m_eqSnapshot;
    }

    const double threshold = qBound(-24.0, p.limiterThresholdDb, 0.0);
    m_dsp.setEqEnabled(p.enabled);
    m_dsp.setEqQ(p.q);
    m_dsp.setEqGains(p.gains, AudioDsp::kBandCount);
    m_dsp.setPreampDb(p.preampDb);
    m_dsp.setAutoHeadroom(p.autoHeadroom);
    m_dsp.setBalance(p.balance);
    m_dsp.setChannelGain(0, p.channelGain[0]);
    m_dsp.setChannelGain(1, p.channelGain[1]);
    m_dsp.setMono(p.mono);
    m_dsp.setStereoWidth(p.stereoWidth);
    m_dsp.setSwapChannels(p.swapChannels);
    m_dsp.setReplayGain(m_rgTrackDb.load(std::memory_order_relaxed),
                        m_rgAlbumDb.load(std::memory_order_relaxed),
                        p.rgMode, p.rgPreampDb, p.rgPreventClip,
                        m_rgPeak.load(std::memory_order_relaxed));
    m_dsp.setBypass(!p.masterEnabled);
    m_dsp.setLimiter(p.limiter, std::pow(10.0, threshold / 20.0));
}

qint64 AudioEngine::render(char *data, qint64 maxlen)
{
    const int channels = qMax(1, m_outChannels.load(std::memory_order_relaxed));
    const int rate = qMax(8000, m_outSampleRate.load(std::memory_order_relaxed));
    const bool floatFormat = (m_format.sampleFormat() == QAudioFormat::Float);
    const int bytesPerSample = floatFormat ? 4 : 2;
    const int frameBytes = channels * bytesPerSample;

    int frames = int(maxlen / frameBytes);
    if (frames <= 0)
        return 0;
    if (frames > kMaxChunkFrames)
        frames = kMaxChunkFrames;

    const int gen = m_ringGen.load(std::memory_order_acquire);
    if (gen != m_localGen) {
        m_localGen = gen;
        m_localMs = m_ringBaseMs.load(std::memory_order_relaxed);
        m_stretch.reset();
        m_dsp.reset();
        // 数据段起点与挂起的 seek 目标一致时，说明 seek 已落地，恢复位置发布
        const qint64 pending = m_pendingSeekMs.load(std::memory_order_acquire);
        if (pending >= 0 && qint64(m_localMs) == pending)
            m_pendingSeekMs.store(-1, std::memory_order_release);
        // 切段时至少给一个极短淡入防爆音，请求更长淡入时以请求为准
        const int guard = qMax(1, kClickGuardMs * qMax(8000, rate) / 1000);
        m_genFadeInFrames = qMax(m_pendingFadeInFrames.exchange(0, std::memory_order_acq_rel), guard);
        m_genFrames = 0;
    }

    applyPendingParams();

    // 音量与淡变：音频线程自行取值，不被 GUI 线程调度拖住
    m_dsp.setVolume(m_volumeAtomic.load(std::memory_order_relaxed));
    const int fadeSeq = m_fadeSeq.load(std::memory_order_acquire);
    if (fadeSeq != m_fadeLocalSeq) {
        m_fadeLocalSeq = fadeSeq;
        m_dsp.rampFade(m_fadeTargetMilli.load(std::memory_order_relaxed) / 1000.0,
                       m_fadeFrames.load(std::memory_order_relaxed));
    }

    const int speedMilli = m_stretchSpeedMilli.load(std::memory_order_relaxed);
    const int pitchMilli = m_pitchMilli.load(std::memory_order_relaxed);
    const bool speedChanged = speedMilli != 1000;
    const bool pitchChanged = pitchMilli != 1000;
    // 变调，或倍速且要求保持音高时，才走时长伸缩 + 重采样链
    const bool stretching = pitchChanged
                            || (m_pitchCompensation.load(std::memory_order_relaxed) && speedChanged);
    if (stretching) {
        const double speed = speedChanged ? speedMilli / 1000.0 : 1.0;
        const double pitch = pitchMilli / 1000.0;
        if (!qFuzzyCompare(m_stretch.speed(), speed) || !qFuzzyCompare(m_stretch.pitch(), pitch))
            m_stretch.setRatios(speed, pitch);
    }

    AudioRing &ring = m_rings[gen & 1];
    // 浮点输出且不伸缩时直接写设备缓冲，省掉每块一次整块拷贝
    const bool direct = floatFormat && !stretching;
    int produced = 0;

    while (produced < frames) {
        const int want = qMin(frames - produced, kMaxChunkFrames);
        char *dst = data + size_t(produced) * frameBytes;
        float *buf = direct ? reinterpret_cast<float *>(dst) : m_scratch.data();
        int got = 0;

        if (stretching) {
            const int available = ring.available() / channels;
            const int needIn = int(want * m_stretch.speed()) + 1;
            const int give = qMin(available, qMin(kMaxStretchInputFrames, needIn + kMaxChunkFrames));
            if (give > 0) {
                const int read = ring.read(m_stretchIn.data(), give * channels) / channels;
                if (read > 0)
                    m_stretch.push(m_stretchIn.data(), read);
            }
            got = m_stretch.pull(buf, want);
        } else {
            got = ring.read(buf, want * channels) / channels;
        }

        if (got <= 0)
            break;

        // 新数据段的第一批样本到位时才开始淡入
        if (m_genFrames == 0 && m_genFadeInFrames > 0) {
            m_dsp.jumpFade(0.0);
            m_dsp.rampFade(1.0, m_genFadeInFrames);
            m_genFadeInFrames = 0;
        }
        m_genFrames += got;

        m_dsp.process(buf, got, channels);
        if (m_spectrumSink)
            m_spectrumSink->pushSamples(buf, got, channels, rate);

        if (!direct) {
            if (floatFormat) {
                std::memcpy(dst, buf, size_t(got) * channels * sizeof(float));
            } else {
                auto *out = reinterpret_cast<qint16 *>(dst);
                for (int i = 0, n = got * channels; i < n; ++i) {
                    const float v = buf[i];
                    out[i] = qint16(qBound(-32768.0f, v * 32767.0f, 32767.0f));
                }
            }
        }
        produced += got;
    }

    if (produced < frames) {
        std::memset(data + size_t(produced) * frameBytes, 0,
                    size_t(frames - produced) * frameBytes);
        if (produced == 0 && ring.available() == 0
            && m_decodeEof.load(std::memory_order_relaxed)
            && m_playingFlag.load(std::memory_order_relaxed)) {
            m_drained.store(true, std::memory_order_relaxed);
        }
    }

    if (produced > 0) {
        // 进度按“源时长”推进：倍速/保持音高时环形缓冲的采样率与设备不同
        const double posHz = stretching ? double(rate) / m_stretch.speed()
                                        : double(m_decodeRate.load(std::memory_order_relaxed));
        if (posHz > 1.0)
            m_localMs += double(produced) * 1000.0 / posHz;
    }
    if (m_pendingSeekMs.load(std::memory_order_relaxed) < 0)
        m_positionMs.store(qint64(m_localMs), std::memory_order_relaxed);
    return qint64(frames) * frameBytes;
}

// ------------------------------------------------------------------- 解码线程

void AudioEngine::beginGeneration(qint64 baseMs)
{
    const int next = m_ringGen.load(std::memory_order_relaxed) + 1;
    // 解码线程绝不改动缓冲尺寸：尺寸只由重建路径更新（更新前会先停靠本线程）
    m_rings[next & 1].clear();
    m_ringBaseMs.store(double(baseMs), std::memory_order_relaxed);
    m_ringGen.store(next, std::memory_order_release);
    // 拖动会排队多个 seek：旧请求落地时不能覆盖 GUI 刚发布的目标位置
    const qint64 pending = m_pendingSeekMs.load(std::memory_order_acquire);
    if (pending < 0 || pending == baseMs)
        m_positionMs.store(baseMs, std::memory_order_relaxed);
}

void AudioEngine::startThread()
{
    m_quit.store(false);
    m_thread = std::thread([this] { decodeLoop(); });
}

void AudioEngine::stopThread()
{
    if (!m_thread.joinable())
        return;
    m_quit.store(true);
    m_decoder.abort();
    m_thread.join();
}

void AudioEngine::decodeLoop()
{
    std::vector<float> buffer(size_t(kMaxChunkFrames) * 2);

    while (!m_quit.load(std::memory_order_relaxed)) {
        // 停靠点：置位后本线程不再触碰环形缓冲，重建路径才会改动它们
        if (m_audioFrozen.load(std::memory_order_acquire)) {
            m_decodeParked.store(true, std::memory_order_release);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }
        m_decodeParked.store(false, std::memory_order_release);

        if (m_openRequest.exchange(false, std::memory_order_relaxed)) {
            QUrl url;
            {
                QMutexLocker lock(&m_pendingUrlMutex);
                url = m_pendingUrl;
            }
            const int channels = qMax(1, m_outChannels.load(std::memory_order_relaxed));
            const int rate = qMax(8000, m_outSampleRate.load(std::memory_order_relaxed));
            const int targetRate = decodeRateFor(rate);
            m_decoder.setTargetFormat(targetRate, channels);
            m_decodeRate.store(targetRate, std::memory_order_relaxed);

            QString error;
            const bool ok = !url.isEmpty() && m_decoder.open(toSourceUrl(url), &error);
            if (!ok) {
                m_decodeActive.store(false, std::memory_order_relaxed);
                const QString message = error.isEmpty() ? tr("无法打开音频源") : error;
                QMetaObject::invokeMethod(this, [this, message] { emitError(1, message); },
                                          Qt::QueuedConnection);
                continue;
            }

            const FfmpegDecoder::Meta meta = m_decoder.meta();
            const int producerChannels = qMax(1, m_outChannels.load(std::memory_order_relaxed));
            if (int(buffer.size()) < kMaxChunkFrames * producerChannels)
                buffer.resize(size_t(kMaxChunkFrames) * size_t(producerChannels));
            beginGeneration(0);
            m_decodeEof.store(false, std::memory_order_relaxed);
            m_drained.store(false, std::memory_order_relaxed);
            m_decodeActive.store(true, std::memory_order_relaxed);
            QMetaObject::invokeMethod(this, [this, meta] { applyMeta(meta); }, Qt::QueuedConnection);
            continue;
        }

        const qint64 seek = m_seekRequest.exchange(-1, std::memory_order_relaxed);
        if (seek >= 0 && m_decodeActive.load(std::memory_order_relaxed)) {
            // 拖动进度条会连续产生请求：节流避免反复冲刷缓冲造成断续
            const qint64 since = steadyMs() - m_lastSeekDoneMs.load(std::memory_order_relaxed);
            if (since < kSeekThrottleMs && since >= 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(kSeekThrottleMs - since));

            const int channels = qMax(1, m_outChannels.load(std::memory_order_relaxed));
            const int rate = qMax(8000, m_outSampleRate.load(std::memory_order_relaxed));
            const int targetRate = decodeRateFor(rate);
            if (m_decoder.targetSampleRate() != targetRate)
                m_decoder.setTargetFormat(targetRate, channels);
            m_decodeRate.store(targetRate, std::memory_order_relaxed);
            m_decoder.seekTo(seek);
            beginGeneration(seek);
            m_lastSeekDoneMs.store(steadyMs(), std::memory_order_relaxed);
            m_decodeEof.store(false, std::memory_order_relaxed);
            continue;
        }

        if (!m_decodeActive.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        const int channels = qMax(1, m_outChannels.load(std::memory_order_relaxed));
        AudioRing &ring = m_rings[m_ringGen.load(std::memory_order_relaxed) & 1];
        if (ring.space() < kMaxChunkFrames * channels) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        const int read = m_decoder.readFrames(buffer.data(), kMaxChunkFrames);
        if (read > 0) {
            if (ring.write(buffer.data(), read * channels) > 0) {
                m_decodeEof.store(false, std::memory_order_relaxed);
                m_drained.store(false, std::memory_order_relaxed);
            }
        } else if (m_decoder.isOpen()) {
            m_decodeEof.store(true, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

// -------------------------------------------------------------------- DSP 接口

void AudioEngine::setEqEnabled(bool on)
{
    if (m_eq.enabled == on)
        return;
    m_eq.enabled = on;
    publishParams(true, true, false, false);
}

QVariantList AudioEngine::eqGains() const
{
    QVariantList list;
    list.reserve(AudioDsp::kBandCount);
    for (int i = 0; i < AudioDsp::kBandCount; ++i)
        list.append(m_eq.gains[i]);
    return list;
}

void AudioEngine::setEqGains(const QVariantList &gains)
{
    const int n = qMin(gains.size(), AudioDsp::kBandCount);
    bool changed = false;
    for (int i = 0; i < n; ++i) {
        const double value = qBound(-24.0, gains.at(i).toDouble(), 24.0);
        if (m_eq.gains[i] == value)
            continue;
        m_eq.gains[i] = value;
        changed = true;
    }
    if (changed)
        publishParams(true, true, false, false);
}

void AudioEngine::setEqBand(int index, qreal gainDb)
{
    if (index < 0 || index >= AudioDsp::kBandCount)
        return;
    const double value = qBound(-24.0, double(gainDb), 24.0);
    if (m_eq.gains[index] == value)
        return;
    m_eq.gains[index] = value;
    publishParams(true, true, false, false);
}

void AudioEngine::applyEqPreset(const QString &name)
{
    const QList<qreal> gains = AudioDsp::presetGains(name);
    for (int i = 0; i < AudioDsp::kBandCount && i < gains.size(); ++i)
        m_eq.gains[i] = gains.at(i);
    m_eq.enabled = true;
    publishParams(true, true, false, false);
}

QStringList AudioEngine::eqPresetNames() const { return AudioDsp::presetNames(); }
QStringList AudioEngine::eqBandLabels() const { return AudioDsp::bandLabels(); }

void AudioEngine::setPreampDb(qreal db)
{
    const double value = qBound(-24.0, double(db), 24.0);
    if (m_eq.preampDb == value)
        return;
    m_eq.preampDb = value;
    publishParams(true, true, false, false);
}

void AudioEngine::setAutoHeadroom(bool on)
{
    if (m_eq.autoHeadroom == on)
        return;
    m_eq.autoHeadroom = on;
    publishParams(true, true, false, false);
}

void AudioEngine::setBalance(qreal v)
{
    const double value = qBound(-1.0, double(v), 1.0);
    if (m_eq.balance == value)
        return;
    m_eq.balance = value;
    publishParams(false, true, false, false);
}

void AudioEngine::setMono(bool on)
{
    if (m_eq.mono == on)
        return;
    m_eq.mono = on;
    publishParams(false, true, false, false);
}

void AudioEngine::setStereoWidth(qreal v)
{
    const double value = qBound(0.0, double(v), 2.0);
    if (m_eq.stereoWidth == value)
        return;
    m_eq.stereoWidth = value;
    publishParams(false, true, false, false);
}

void AudioEngine::setSwapChannels(bool on)
{
    if (m_eq.swapChannels == on)
        return;
    m_eq.swapChannels = on;
    publishParams(false, true, false, false);
}

void AudioEngine::setChannelGainLeft(qreal db)
{
    const double value = qBound(-24.0, double(db), 24.0);
    if (m_eq.channelGain[0] == value)
        return;
    m_eq.channelGain[0] = value;
    publishParams(false, true, false, false);
}

void AudioEngine::setChannelGainRight(qreal db)
{
    const double value = qBound(-24.0, double(db), 24.0);
    if (m_eq.channelGain[1] == value)
        return;
    m_eq.channelGain[1] = value;
    publishParams(false, true, false, false);
}

void AudioEngine::setReplayGainMode(int mode)
{
    const int value = qBound(0, mode, 2);
    if (m_eq.rgMode == value)
        return;
    m_eq.rgMode = value;
    publishParams(false, false, true, false);
}

void AudioEngine::setReplayGainPreampDb(qreal db)
{
    const double value = qBound(-24.0, double(db), 24.0);
    if (m_eq.rgPreampDb == value)
        return;
    m_eq.rgPreampDb = value;
    publishParams(false, false, true, false);
}

void AudioEngine::setReplayGainPreventClip(bool on)
{
    if (m_eq.rgPreventClip == on)
        return;
    m_eq.rgPreventClip = on;
    publishParams(false, false, true, false);
}

void AudioEngine::setLimiterEnabled(bool on)
{
    if (m_eq.limiter == on)
        return;
    m_eq.limiter = on;
    publishParams(false, true, false, true);
}

void AudioEngine::setLimiterThresholdDb(qreal db)
{
    const double value = qBound(-24.0, double(db), 0.0);
    if (m_eq.limiterThresholdDb == value)
        return;
    m_eq.limiterThresholdDb = value;
    publishParams(false, true, false, true);
}

bool AudioEngine::dspActive() const
{
    if (!dspEnabled())
        return false;
    if (m_eq.enabled) {
        for (int i = 0; i < AudioDsp::kBandCount; ++i)
            if (m_eq.gains[i] != 0.0)
                return true;
    }
    return !qFuzzyIsNull(m_eq.preampDb) || m_eq.rgMode > 0 || m_eq.limiter
           || !qFuzzyIsNull(m_eq.balance) || m_eq.mono
           || !qFuzzyCompare(m_eq.stereoWidth, 1.0) || m_eq.swapChannels
           || !qFuzzyIsNull(m_eq.channelGain[0]) || !qFuzzyIsNull(m_eq.channelGain[1]);
}

qreal AudioEngine::effectiveGainDb() const
{
    double headroom = 0.0;
    if (m_eq.autoHeadroom && m_eq.enabled) {
        double maxGain = 0.0;
        for (int i = 0; i < AudioDsp::kBandCount; ++i)
            maxGain = qMax(maxGain, m_eq.gains[i]);
        headroom = qMax(0.0, maxGain);
    }
    double rg = 0.0;
    if (m_eq.rgMode > 0) {
        rg = (m_eq.rgMode == 2 ? m_rgAlbumDb.load(std::memory_order_relaxed)
                               : m_rgTrackDb.load(std::memory_order_relaxed)) + m_eq.rgPreampDb;
        const double peak = m_rgPeak.load(std::memory_order_relaxed);
        if (m_eq.rgPreventClip && peak > 0.0)
            rg = qMin(rg, -20.0 * std::log10(peak));
    }
    return m_eq.preampDb + rg - headroom;
}

void AudioEngine::publishParams(bool eqChangedFlag, bool dspChangedFlag, bool rgChangedFlag,
                                bool limiterChangedFlag)
{
    {
        QMutexLocker lock(&m_paramMutex);
        m_eqSnapshot = m_eq;
        m_paramsPending.store(true, std::memory_order_release);
    }
    if (eqChangedFlag)
        emit eqChanged();
    if (dspChangedFlag)
        emit dspChanged();
    if (rgChangedFlag)
        emit replayGainChanged();
    if (limiterChangedFlag)
        emit limiterChanged();
    scheduleSave();
}

void AudioEngine::setSpectrumSink(QObject *sink)
{
    // 订阅者先于引擎销毁时，音频线程必须立刻停止引用它
    if (m_spectrumObject)
        disconnect(m_spectrumObject, nullptr, this, nullptr);
    m_spectrumSink = dynamic_cast<AudioSpectrumSink *>(sink);
    m_spectrumObject = sink;
    if (m_spectrumObject) {
        connect(m_spectrumObject, &QObject::destroyed, this, [this] {
            m_spectrumSink = nullptr;
            m_spectrumObject = nullptr;
        });
    }
}

// -------------------------------------------------------------------- 配置持久化

void AudioEngine::loadSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("AudioDsp"));

    m_eq.enabled = settings.value(QStringLiteral("eqEnabled"), false).toBool();
    const QJsonArray gains = QJsonDocument::fromJson(
                                 settings.value(QStringLiteral("eqGains")).toByteArray())
                                 .array();
    for (int i = 0; i < AudioDsp::kBandCount && i < gains.size(); ++i)
        m_eq.gains[i] = qBound(-24.0, gains.at(i).toDouble(), 24.0);
    m_eq.q = qBound(0.2, settings.value(QStringLiteral("eqQ"), 1.41).toDouble(), 8.0);
    m_eq.preampDb = qBound(-24.0, settings.value(QStringLiteral("preampDb"), 0.0).toDouble(), 24.0);
    m_eq.autoHeadroom = settings.value(QStringLiteral("autoHeadroom"), true).toBool();
    m_eq.balance = qBound(-1.0, settings.value(QStringLiteral("balance"), 0.0).toDouble(), 1.0);
    m_eq.stereoWidth = qBound(0.0, settings.value(QStringLiteral("stereoWidth"), 1.0).toDouble(), 2.0);
    m_eq.mono = settings.value(QStringLiteral("mono"), false).toBool();
    m_eq.swapChannels = settings.value(QStringLiteral("swapChannels"), false).toBool();
    m_eq.channelGain[0] = qBound(-24.0, settings.value(QStringLiteral("gainLeft"), 0.0).toDouble(), 24.0);
    m_eq.channelGain[1] = qBound(-24.0, settings.value(QStringLiteral("gainRight"), 0.0).toDouble(), 24.0);
    m_eq.rgMode = qBound(0, settings.value(QStringLiteral("rgMode"), 0).toInt(), 2);
    m_eq.rgPreampDb = qBound(-24.0, settings.value(QStringLiteral("rgPreamp"), 0.0).toDouble(), 24.0);
    m_eq.rgPreventClip = settings.value(QStringLiteral("rgPreventClip"), true).toBool();
    m_eq.limiter = settings.value(QStringLiteral("limiter"), false).toBool();
    m_eq.limiterThresholdDb = qBound(-24.0, settings.value(QStringLiteral("limiterDb"), -0.5).toDouble(), 0.0);
    m_pitchSemitones = qBound(-12.0, settings.value(QStringLiteral("pitchSemitones"), 0.0).toDouble(), 12.0);
    m_pitchMilli.store(int(std::lround(std::pow(2.0, double(m_pitchSemitones) / 12.0) * 1000.0)),
                       std::memory_order_relaxed);
    m_eq.masterEnabled = settings.value(QStringLiteral("dspEnabled"), true).toBool();
    m_dspEnabled.store(m_eq.masterEnabled, std::memory_order_relaxed);
    m_requestedRate = qBound(0, settings.value(QStringLiteral("outputSampleRate"), 0).toInt(), 384000);
    {
        const int saved = settings.value(QStringLiteral("outputBufferMs"), 0).toInt();
        m_bufferMs = saved <= 0 ? kBufferMsDefault : qBound(kBufferMsMin, saved, kBufferMsMax);
    }

    settings.endGroup();
}

void AudioEngine::scheduleSave()
{
    if (m_saveTimer)
        m_saveTimer->start();
}

void AudioEngine::saveSettings()
{
    QJsonArray gains;
    for (int i = 0; i < AudioDsp::kBandCount; ++i)
        gains.append(m_eq.gains[i]);

    QSettings settings;
    settings.beginGroup(QStringLiteral("AudioDsp"));
    settings.setValue(QStringLiteral("eqEnabled"), m_eq.enabled);
    settings.setValue(QStringLiteral("eqGains"),
                      QJsonDocument(gains).toJson(QJsonDocument::Compact));
    settings.setValue(QStringLiteral("eqQ"), m_eq.q);
    settings.setValue(QStringLiteral("preampDb"), m_eq.preampDb);
    settings.setValue(QStringLiteral("autoHeadroom"), m_eq.autoHeadroom);
    settings.setValue(QStringLiteral("balance"), m_eq.balance);
    settings.setValue(QStringLiteral("stereoWidth"), m_eq.stereoWidth);
    settings.setValue(QStringLiteral("mono"), m_eq.mono);
    settings.setValue(QStringLiteral("swapChannels"), m_eq.swapChannels);
    settings.setValue(QStringLiteral("gainLeft"), m_eq.channelGain[0]);
    settings.setValue(QStringLiteral("gainRight"), m_eq.channelGain[1]);
    settings.setValue(QStringLiteral("rgMode"), m_eq.rgMode);
    settings.setValue(QStringLiteral("rgPreamp"), m_eq.rgPreampDb);
    settings.setValue(QStringLiteral("rgPreventClip"), m_eq.rgPreventClip);
    settings.setValue(QStringLiteral("limiter"), m_eq.limiter);
    settings.setValue(QStringLiteral("limiterDb"), m_eq.limiterThresholdDb);
    settings.setValue(QStringLiteral("dspEnabled"), dspEnabled());
    settings.setValue(QStringLiteral("outputSampleRate"), m_requestedRate);
    settings.setValue(QStringLiteral("outputBufferMs"), m_bufferMs);
    settings.setValue(QStringLiteral("pitchSemitones"), m_pitchSemitones);
    settings.endGroup();
}
