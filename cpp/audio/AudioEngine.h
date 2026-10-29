// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

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
#include <mutex>
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

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged FINAL)

    Q_PROPERTY(QVariantMap sourceHeaders READ sourceHeaders WRITE setSourceHeaders FINAL)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged FINAL)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged FINAL)
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged FINAL)
    Q_PROPERTY(MediaStatus mediaStatus READ mediaStatus NOTIFY mediaStatusChanged FINAL)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged FINAL)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged FINAL)
    Q_PROPERTY(bool pitchCompensation READ pitchCompensation WRITE setPitchCompensation NOTIFY pitchCompensationChanged FINAL)
    Q_PROPERTY(qreal pitchSemitones READ pitchSemitones WRITE setPitchSemitones NOTIFY pitchChanged FINAL)
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged FINAL)

    Q_PROPERTY(QString urlStr READ urlStr WRITE setUrlStr NOTIFY urlStrChanged FINAL)
    Q_PROPERTY(QString noTitle READ noTitle WRITE setNoTitle NOTIFY noTitleChanged FINAL)
    Q_PROPERTY(QString album READ album WRITE setAlbum NOTIFY albumChanged FINAL)
    Q_PROPERTY(bool onMedia READ onMedia NOTIFY mediaStatusChanged FINAL)

    Q_PROPERTY(QString title READ title NOTIFY metaDataChanged FINAL)
    Q_PROPERTY(QString tagTitle READ tagTitle NOTIFY metaDataChanged FINAL)
    Q_PROPERTY(QString artist READ artist NOTIFY metaDataChanged FINAL)
    Q_PROPERTY(QString albumTitle READ albumTitle NOTIFY metaDataChanged FINAL)
    Q_PROPERTY(QString mediaType READ mediaType NOTIFY metaDataChanged FINAL)
    Q_PROPERTY(QString mediaDate READ mediaDate NOTIFY metaDataChanged FINAL)
    Q_PROPERTY(int bitRate READ bitRate NOTIFY metaDataChanged FINAL)
    Q_PROPERTY(int sourceSampleRate READ sourceSampleRate NOTIFY metaDataChanged FINAL)
    Q_PROPERTY(int sourceChannels READ sourceChannels NOTIFY metaDataChanged FINAL)
    Q_PROPERTY(QString codecName READ codecName NOTIFY metaDataChanged FINAL)

    Q_PROPERTY(int outputSampleRate READ outputSampleRate WRITE setOutputSampleRate NOTIFY outputFormatChanged FINAL)
    Q_PROPERTY(int decodeSampleRate READ decodeSampleRate NOTIFY outputFormatChanged FINAL)
    Q_PROPERTY(int bufferMs READ bufferMs WRITE setBufferMs NOTIFY outputFormatChanged FINAL)
    Q_PROPERTY(QString outputFormatName READ outputFormatName NOTIFY outputFormatChanged FINAL)
    Q_PROPERTY(bool dspEnabled READ dspEnabled WRITE setDspEnabled NOTIFY dspChanged FINAL)
    Q_PROPERTY(bool dspActive READ dspActive NOTIFY dspChanged FINAL)
    Q_PROPERTY(qreal effectiveGainDb READ effectiveGainDb NOTIFY dspChanged FINAL)
    Q_PROPERTY(qreal eqQ READ eqQ WRITE setEqQ NOTIFY eqChanged FINAL)

    Q_PROPERTY(bool eqEnabled READ eqEnabled WRITE setEqEnabled NOTIFY eqChanged FINAL)
    Q_PROPERTY(QVariantList eqGains READ eqGains WRITE setEqGains NOTIFY eqChanged FINAL)
    Q_PROPERTY(qreal preampDb READ preampDb WRITE setPreampDb NOTIFY eqChanged FINAL)
    Q_PROPERTY(bool autoHeadroom READ autoHeadroom WRITE setAutoHeadroom NOTIFY eqChanged FINAL)
    Q_PROPERTY(qreal balance READ balance WRITE setBalance NOTIFY dspChanged FINAL)
    Q_PROPERTY(bool mono READ mono WRITE setMono NOTIFY dspChanged FINAL)
    Q_PROPERTY(qreal stereoWidth READ stereoWidth WRITE setStereoWidth NOTIFY dspChanged FINAL)
    Q_PROPERTY(bool swapChannels READ swapChannels WRITE setSwapChannels NOTIFY dspChanged FINAL)
    Q_PROPERTY(qreal channelGainLeft READ channelGainLeft WRITE setChannelGainLeft NOTIFY dspChanged FINAL)
    Q_PROPERTY(qreal channelGainRight READ channelGainRight WRITE setChannelGainRight NOTIFY dspChanged FINAL)
    Q_PROPERTY(int replayGainMode READ replayGainMode WRITE setReplayGainMode NOTIFY replayGainChanged FINAL)
    Q_PROPERTY(qreal replayGainPreampDb READ replayGainPreampDb WRITE setReplayGainPreampDb NOTIFY replayGainChanged FINAL)
    Q_PROPERTY(bool replayGainPreventClip READ replayGainPreventClip WRITE setReplayGainPreventClip NOTIFY replayGainChanged FINAL)
    Q_PROPERTY(bool limiterEnabled READ limiterEnabled WRITE setLimiterEnabled NOTIFY limiterChanged FINAL)
    Q_PROPERTY(qreal limiterThresholdDb READ limiterThresholdDb WRITE setLimiterThresholdDb NOTIFY limiterChanged FINAL)

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

    qreal pitchSemitones() const { return m_pitchSemitones; }
    void setPitchSemitones(qreal semitones);

    QString deviceId() const { return m_deviceId; }
    void setDeviceId(const QString &id);

    QString urlStr() const { return m_urlStr; }
    void setUrlStr(const QString &url);
    QString noTitle() const { return m_noTitle; }
    void setNoTitle(const QString &name);
    QString album() const { return m_album; }
    void setAlbum(const QString &name);

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

    Q_INVOKABLE void fadeTo(qreal target, int ms);
    Q_INVOKABLE void fadeOut(int ms);
    Q_INVOKABLE void fadeIn(int ms);

    Q_INVOKABLE void fadeInOnNextAudio(int ms);
    Q_INVOKABLE void setEqBand(int index, qreal gainDb);
    Q_INVOKABLE void applyEqPreset(const QString &name);
    Q_INVOKABLE QStringList eqPresetNames() const;
    Q_INVOKABLE QStringList eqBandLabels() const;

    Q_INVOKABLE void setSpectrumSink(QObject *sink);

    void prepareNextSource(const QUrl &url, const QVariantMap &headers = {});
    void clearNextSource();
    void setTrackAdvancedCallback(std::function<void()> callback);

    void adoptSource(const QUrl &url);

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
    void albumChanged();
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

    std::shared_ptr<AudioSpectrumSinkHandle> acquireSpectrumHandle();
    void loadSettings();
    void saveSettings();
    void scheduleSave();

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
    QVariantMap m_sourceHeaders;
    QVariantMap m_pendingHeaders;
    QUrl m_nextUrl;
    QVariantMap m_nextHeaders;
    std::atomic<bool> m_nextReady{false};
    std::atomic<bool> m_autoSwitchPending{false};
    std::function<void()> m_advanceCallback;
    std::mutex m_callbackMutex;
    qint64 m_durationMs = 0;
    PlaybackState m_playbackState = StoppedState;
    MediaStatus m_mediaStatus = NoMedia;
    FfmpegDecoder::Meta m_meta;

    qreal m_volume = 1.0;
    qreal m_playbackRate = 1.0;
    QString m_deviceId;
    QString m_urlStr;
    QString m_noTitle;
    QString m_album;
    std::atomic<bool> m_pitchCompensation{false};
    std::atomic<int> m_rateMilli{1000};
    std::atomic<int> m_stretchSpeedMilli{1000};
    std::atomic<int> m_pitchMilli{1000};
    qreal m_pitchSemitones = 0.0;
    std::atomic<double> m_rgTrackDb{0.0};
    std::atomic<double> m_rgAlbumDb{0.0};
    std::atomic<double> m_rgPeak{0.0};
    qint64 m_lastEmittedPosition = -1;

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
    int m_bufferMs = 0;

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

    std::atomic<qint64> m_pendingSeekMs{-1};
    std::atomic<qint64> m_lastSeekDoneMs{0};
    std::atomic<bool> m_decodeActive{false};

    std::atomic<bool> m_audioFrozen{false};
    std::atomic<bool> m_decodeParked{false};

    QTimer *m_pollTimer = nullptr;
    QTimer *m_saveTimer = nullptr;

    QMutex m_spectrumMutex;
    std::shared_ptr<AudioSpectrumSinkHandle> m_spectrumHandle;
    qint64 m_lastSinkErrorMs = 0;
};
