// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 播放引擎：FFmpeg 解码线程 → 无锁环形缓冲 → 音频回调线程（DSP + 设备输出）
#pragma once

#include "AudioDsp.h"
#include "AudioRing.h"
#include "AudioSpectrumSink.h"
#include "FfmpegDecoder.h"
#include "PitchShifter.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QMutex>
#include <QThread>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

#include <atomic>
#include <cmath>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

class QAudioSink;
class QIODevice;
class QTimer;
class EngineIoDevice;

class AudioEngine : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    // 远端源的额外请求头（如 WebDAV 的 Authorization），在 setSource 之前写入
    Q_PROPERTY(QVariantMap sourceHeaders READ sourceHeaders WRITE setSourceHeaders)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(MediaStatus mediaStatus READ mediaStatus NOTIFY mediaStatusChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(bool pitchCompensation READ pitchCompensation WRITE setPitchCompensation NOTIFY pitchCompensationChanged)
    Q_PROPERTY(qreal pitchSemitones READ pitchSemitones WRITE setPitchSemitones NOTIFY pitchChanged)
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)
    // 展示用信息：封面 URL 与列表显示名由 QML 侧注入（引擎不关心来源）；onMedia 派生自 mediaStatus
    Q_PROPERTY(QString urlStr READ urlStr WRITE setUrlStr NOTIFY urlStrChanged)
    Q_PROPERTY(QString noTitle READ noTitle WRITE setNoTitle NOTIFY noTitleChanged)
    Q_PROPERTY(bool onMedia READ onMedia NOTIFY mediaStatusChanged)

    Q_PROPERTY(QString title READ title NOTIFY metaDataChanged)
    Q_PROPERTY(QString tagTitle READ tagTitle NOTIFY metaDataChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY metaDataChanged)
    Q_PROPERTY(QString albumTitle READ albumTitle NOTIFY metaDataChanged)
    Q_PROPERTY(QString mediaType READ mediaType NOTIFY metaDataChanged)
    Q_PROPERTY(QString mediaDate READ mediaDate NOTIFY metaDataChanged)
    Q_PROPERTY(int bitRate READ bitRate NOTIFY metaDataChanged)
    Q_PROPERTY(int sourceSampleRate READ sourceSampleRate NOTIFY metaDataChanged)
    Q_PROPERTY(int sourceChannels READ sourceChannels NOTIFY metaDataChanged)
    Q_PROPERTY(QString codecName READ codecName NOTIFY metaDataChanged)

    Q_PROPERTY(int outputSampleRate READ outputSampleRate WRITE setOutputSampleRate NOTIFY outputFormatChanged)
    Q_PROPERTY(int decodeSampleRate READ decodeSampleRate NOTIFY outputFormatChanged)
    Q_PROPERTY(int bufferMs READ bufferMs WRITE setBufferMs NOTIFY outputFormatChanged)
    Q_PROPERTY(QString outputFormatName READ outputFormatName NOTIFY outputFormatChanged)
    Q_PROPERTY(bool dspEnabled READ dspEnabled WRITE setDspEnabled NOTIFY dspChanged)
    Q_PROPERTY(bool dspActive READ dspActive NOTIFY dspChanged)
    Q_PROPERTY(qreal effectiveGainDb READ effectiveGainDb NOTIFY dspChanged)
    Q_PROPERTY(qreal eqQ READ eqQ WRITE setEqQ NOTIFY eqChanged)

    Q_PROPERTY(bool eqEnabled READ eqEnabled WRITE setEqEnabled NOTIFY eqChanged)
    Q_PROPERTY(QVariantList eqGains READ eqGains WRITE setEqGains NOTIFY eqChanged)
    Q_PROPERTY(qreal preampDb READ preampDb WRITE setPreampDb NOTIFY eqChanged)
    Q_PROPERTY(bool autoHeadroom READ autoHeadroom WRITE setAutoHeadroom NOTIFY eqChanged)
    Q_PROPERTY(qreal balance READ balance WRITE setBalance NOTIFY dspChanged)
    Q_PROPERTY(bool mono READ mono WRITE setMono NOTIFY dspChanged)
    Q_PROPERTY(qreal stereoWidth READ stereoWidth WRITE setStereoWidth NOTIFY dspChanged)
    Q_PROPERTY(bool swapChannels READ swapChannels WRITE setSwapChannels NOTIFY dspChanged)
    Q_PROPERTY(qreal channelGainLeft READ channelGainLeft WRITE setChannelGainLeft NOTIFY dspChanged)
    Q_PROPERTY(qreal channelGainRight READ channelGainRight WRITE setChannelGainRight NOTIFY dspChanged)
    Q_PROPERTY(int replayGainMode READ replayGainMode WRITE setReplayGainMode NOTIFY replayGainChanged)
    Q_PROPERTY(qreal replayGainPreampDb READ replayGainPreampDb WRITE setReplayGainPreampDb NOTIFY replayGainChanged)
    Q_PROPERTY(bool replayGainPreventClip READ replayGainPreventClip WRITE setReplayGainPreventClip NOTIFY replayGainChanged)
    Q_PROPERTY(bool limiterEnabled READ limiterEnabled WRITE setLimiterEnabled NOTIFY limiterChanged)
    Q_PROPERTY(qreal limiterThresholdDb READ limiterThresholdDb WRITE setLimiterThresholdDb NOTIFY limiterChanged)

public:
    enum PlaybackState { StoppedState = 0, PlayingState = 1, PausedState = 2 };
    Q_ENUM(PlaybackState)

    enum MediaStatus {
        NoMedia = 0, LoadingMedia = 1, LoadedMedia = 2, StalledMedia = 3,
        BufferingMedia = 4, BufferedMedia = 5, EndOfMedia = 6, InvalidMedia = 7
    };
    Q_ENUM(MediaStatus)

    explicit AudioEngine(QObject *parent = nullptr);
    ~AudioEngine() override;

    QUrl source() const { return m_source; }
    void setSource(const QUrl &url);

    QVariantMap sourceHeaders() const { return m_sourceHeaders; }
    void setSourceHeaders(const QVariantMap &headers) { m_sourceHeaders = headers; }

    qint64 position() const { return m_positionMs.load(std::memory_order_relaxed); }
    void setPosition(qint64 ms);

    qint64 duration() const { return m_durationMs; }
    bool playing() const { return m_playbackState == PlayingState; }
    PlaybackState playbackState() const { return m_playbackState; }
    MediaStatus mediaStatus() const { return m_mediaStatus; }

    qreal volume() const { return m_volume; }
    void setVolume(qreal v);

    qreal playbackRate() const { return m_playbackRate; }
    void setPlaybackRate(qreal rate);
    bool pitchCompensation() const { return m_pitchCompensation.load(std::memory_order_relaxed); }
    void setPitchCompensation(bool on);
    // 半音为单位，-12 ~ +12；与倍速互不干扰
    qreal pitchSemitones() const { return m_pitchSemitones; }
    void setPitchSemitones(qreal semitones);

    QString deviceId() const { return m_deviceId; }
    void setDeviceId(const QString &id);

    QString urlStr() const { return m_urlStr; }
    void setUrlStr(const QString &url);
    QString noTitle() const { return m_noTitle; }
    void setNoTitle(const QString &name);
    // 有媒体（含加载中/已加载/播放中/结束）：派生值，随 mediaStatusChanged 更新
    bool onMedia() const { return m_mediaStatus != NoMedia; }

    QString title() const { return m_meta.title; }
    QString tagTitle() const { return m_meta.tagTitle; }
    QString artist() const { return m_meta.artist; }
    QString albumTitle() const { return m_meta.album; }
    QString mediaType() const { return m_meta.codec; }
    QString mediaDate() const { return m_meta.date; }
    int bitRate() const { return m_meta.bitRate; }
    int sourceSampleRate() const { return m_meta.sampleRate; }
    int sourceChannels() const { return m_meta.channels; }
    QString codecName() const { return m_meta.codec; }

    int outputSampleRate() const { return m_outSampleRate.load(std::memory_order_relaxed); }
    void setOutputSampleRate(int rate);
    int decodeSampleRate() const { return m_decodeRate.load(std::memory_order_relaxed); }
    int bufferMs() const;
    void setBufferMs(int ms);
    QString outputFormatName() const;
    bool dspEnabled() const { return m_dspEnabled.load(std::memory_order_relaxed); }
    void setDspEnabled(bool on);
    bool dspActive() const;
    qreal eqQ() const { return m_eq.q; }
    void setEqQ(qreal q);

    bool eqEnabled() const { return m_eq.enabled; }
    void setEqEnabled(bool on);
    QVariantList eqGains() const;
    void setEqGains(const QVariantList &gains);
    qreal preampDb() const { return m_eq.preampDb; }
    void setPreampDb(qreal db);
    bool autoHeadroom() const { return m_eq.autoHeadroom; }
    void setAutoHeadroom(bool on);

    qreal balance() const { return m_eq.balance; }
    void setBalance(qreal v);
    bool mono() const { return m_eq.mono; }
    void setMono(bool on);
    qreal stereoWidth() const { return m_eq.stereoWidth; }
    void setStereoWidth(qreal v);
    bool swapChannels() const { return m_eq.swapChannels; }
    void setSwapChannels(bool on);
    qreal channelGainLeft() const { return m_eq.channelGain[0]; }
    qreal channelGainRight() const { return m_eq.channelGain[1]; }
    void setChannelGainLeft(qreal db);
    void setChannelGainRight(qreal db);

    int replayGainMode() const { return m_eq.rgMode; }
    void setReplayGainMode(int mode);
    qreal replayGainPreampDb() const { return m_eq.rgPreampDb; }
    void setReplayGainPreampDb(qreal db);
    bool replayGainPreventClip() const { return m_eq.rgPreventClip; }
    void setReplayGainPreventClip(bool on);

    bool limiterEnabled() const { return m_eq.limiter; }
    void setLimiterEnabled(bool on);
    qreal limiterThresholdDb() const { return m_eq.limiterThresholdDb; }
    void setLimiterThresholdDb(qreal db);

    qreal effectiveGainDb() const;

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void togglePause();
    // 淡变在音频线程逐样本完成，与 GUI 调度无关
    Q_INVOKABLE void fadeTo(qreal target, int ms);
    Q_INVOKABLE void fadeOut(int ms);
    Q_INVOKABLE void fadeIn(int ms);
    // 下一段音频真正出声时再淡入：切歌不靠 QML 计时，过渡自然
    Q_INVOKABLE void fadeInOnNextAudio(int ms);
    Q_INVOKABLE void setEqBand(int index, qreal gainDb);
    Q_INVOKABLE void applyEqPreset(const QString &name);
    Q_INVOKABLE QStringList eqPresetNames() const;
    Q_INVOKABLE QStringList eqBandLabels() const;
    // 订阅频谱：sink 需实现 AudioSpectrumSource，引擎只拿走共享句柄（见 AudioSpectrumSink.h）
    Q_INVOKABLE void setSpectrumSink(QObject *sink);

signals:
    void sourceChanged();
    void positionChanged();
    void durationChanged();
    void playingChanged();
    void playbackStateChanged();
    void mediaStatusChanged();
    void errorOccurred(int code, const QString &message);
    void metaDataChanged();
    void volumeChanged();
    void playbackRateChanged();
    void pitchCompensationChanged();
    void pitchChanged();
    void deviceIdChanged();
    void urlStrChanged();
    void noTitleChanged();
    void outputFormatChanged();
    void eqChanged();
    void dspChanged();
    void replayGainChanged();
    void limiterChanged();

private:
    friend class EngineIoDevice;

    struct EqParams
    {
        bool enabled = false;
        double gains[AudioDsp::kBandCount] = {0};
        double preampDb = 0.0;
        bool autoHeadroom = true;
        double balance = 0.0;
        double channelGain[2] = {0, 0};
        bool mono = false;
        double stereoWidth = 1.0;
        bool swapChannels = false;
        int rgMode = 0;
        double rgPreampDb = 0.0;
        bool rgPreventClip = true;
        bool limiter = false;
        double limiterThresholdDb = -0.5;
        double q = 1.41;
        bool masterEnabled = true;
    };

    qint64 render(char *data, qint64 maxlen);
    void decodeLoop();
    void applyMeta(const FfmpegDecoder::Meta &meta);
    void startThread();
    void stopThread();
    bool setupSink();
    void teardownSink();
    void openOnDecodeThread(const QUrl &url);
    void requestSeek(qint64 ms);
    void beginGeneration(qint64 baseMs);
    void applyPendingParams();
    void reconfigureOutput();
    int decodeRateFor(int deviceRate) const;
    void invokeOnOutput(std::function<void()> fn);
    void publishSpectrumHandle(std::shared_ptr<AudioSpectrumSinkHandle> handle);
    // 音频线程取句柄：抢不到锁就直接跳过本块，绝不阻塞音频回调
    std::shared_ptr<AudioSpectrumSinkHandle> acquireSpectrumHandle();
    void loadSettings();
    void saveSettings();
    void scheduleSave();
    // 把状态变更与信号发射统一拉回引擎所属线程：音频/解码线程绝不触碰 QML
    void postToSelf(std::function<void()> fn);
    void reportSinkError();
    void publishParams(bool eqChanged, bool dspChanged, bool rgChanged, bool limiterChanged);
    void setPlaybackState(PlaybackState state);
    void setMediaStatus(MediaStatus status);
    void emitError(int code, const QString &message);
    void pollState();
    QAudioDevice resolveDevice() const;
    void startSink();
    void stopSink();

    QUrl m_source;
    QVariantMap m_sourceHeaders;    // 远端源附加请求头（setSourceHeaders 写入）
    QVariantMap m_pendingHeaders;   // 与 m_pendingUrl 同锁保护
    qint64 m_durationMs = 0;
    PlaybackState m_playbackState = StoppedState;
    MediaStatus m_mediaStatus = NoMedia;
    FfmpegDecoder::Meta m_meta;

    qreal m_volume = 1.0;
    qreal m_playbackRate = 1.0;
    QString m_deviceId;
    QString m_urlStr;      // 封面 URL（QML 注入，仅供展示）
    QString m_noTitle;     // 列表显示名 / 文件名（QML 注入，仅供展示）
    std::atomic<bool> m_pitchCompensation{false};
    std::atomic<int> m_rateMilli{1000};
    std::atomic<int> m_stretchSpeedMilli{1000};
    std::atomic<int> m_pitchMilli{1000};
    qreal m_pitchSemitones = 0.0;
    std::atomic<double> m_rgTrackDb{0.0};
    std::atomic<double> m_rgAlbumDb{0.0};
    std::atomic<double> m_rgPeak{0.0};
    qint64 m_lastEmittedPosition = -1;

    // sink 与它的拉取定时器都归输出线程，GUI 卡顿影响不到音频
    QThread m_outputThread;
    QObject *m_outputContext = nullptr;
    QAudioSink *m_sink = nullptr;
    EngineIoDevice *m_io = nullptr;
    QAudioFormat m_format;
    std::atomic<int> m_outSampleRate{48000};
    std::atomic<int> m_outChannels{2};
    std::atomic<int> m_decodeRate{48000};
    int m_chunkFrames = 4096;
    int m_requestedRate = 0;
    int m_bufferMs = 0; // loadSettings() 会落定为 10~100 的有效值

    AudioRing m_rings[2];
    std::atomic<int> m_ringGen{0};
    std::atomic<double> m_ringBaseMs{0.0};
    int m_localGen = -1;
    double m_localMs = 0.0;
    std::atomic<qint64> m_positionMs{0};

    std::vector<float> m_scratch;
    std::vector<float> m_stretchIn;
    std::vector<float> m_stretchOut;
    PitchShifter m_stretch;

    AudioDsp m_dsp;
    EqParams m_eq;
    EqParams m_eqSnapshot;
    QMutex m_paramMutex;
    std::atomic<bool> m_paramsPending{false};
    std::atomic<bool> m_playingFlag{false};
    std::atomic<bool> m_dspEnabled{true};
    std::atomic<bool> m_sinkErrorFlag{false};
    std::atomic<double> m_volumeAtomic{1.0};
    std::atomic<int> m_fadeTargetMilli{1000};
    std::atomic<int> m_fadeFrames{0};
    std::atomic<int> m_fadeSeq{0};
    std::atomic<int> m_pendingFadeInFrames{0};
    int m_fadeLocalSeq = -1;
    int m_genFadeInFrames = 0;
    int m_genFrames = 0;

    FfmpegDecoder m_decoder;
    std::thread m_thread;
    std::atomic<bool> m_quit{false};
    std::atomic<bool> m_openRequest{false};
    QUrl m_pendingUrl;
    QMutex m_pendingUrlMutex;
    std::atomic<bool> m_decodeEof{false};
    std::atomic<bool> m_drained{false};
    std::atomic<qint64> m_seekRequest{-1};
    // seek 落地前禁止音频线程发布位置，否则进度条会先弹回旧值
    std::atomic<qint64> m_pendingSeekMs{-1};
    std::atomic<qint64> m_lastSeekDoneMs{0};
    std::atomic<bool> m_decodeActive{false};
    // 重建缓冲期间冻结音频/解码线程：缓冲区一被 resize，旧内存就释放，
    // 音频线程仍持有旧指针，会直接踩坏堆
    std::atomic<bool> m_audioFrozen{false};
    std::atomic<bool> m_decodeParked{false};

    QTimer *m_pollTimer = nullptr;
    QTimer *m_saveTimer = nullptr;
    // 频谱订阅：只保存共享句柄 + 保护它的锁，音频线程绝不直接引用 QObject
    QMutex m_spectrumMutex;
    std::shared_ptr<AudioSpectrumSinkHandle> m_spectrumHandle;
    qint64 m_lastSinkErrorMs = 0;
};
