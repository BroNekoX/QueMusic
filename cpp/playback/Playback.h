// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#pragma once

#include "CoverHelper.h"
#include "PlaybackCursor.h"
#include "PlaybackHistory.h"
#include "audio/AudioEngine.h"

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QtQmlIntegration/qqmlintegration.h>

#include <atomic>

class MusicApiService;
class QQmlEngine;
class QJSEngine;

class Playback : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(AudioEngine *player READ player CONSTANT FINAL)
    Q_PROPERTY(QueueModel *queue READ queue CONSTANT FINAL)
    Q_PROPERTY(PlaybackHistory *history READ history CONSTANT FINAL)
    Q_PROPERTY(CoverHelper *coverHelper READ coverHelper CONSTANT FINAL)
    Q_PROPERTY(bool inplay READ inplay NOTIFY inplayChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(int cycleIndex READ cycleIndex WRITE setCycleIndex NOTIFY cycleIndexChanged FINAL)
    Q_PROPERTY(QString cycleIcon READ cycleIcon NOTIFY cycleIndexChanged FINAL)
    Q_PROPERTY(QString cycleTip READ cycleTip NOTIFY cycleIndexChanged FINAL)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged FINAL)
    Q_PROPERTY(qreal outVolume READ outVolume NOTIFY volumeChanged FINAL)
    Q_PROPERTY(qreal volumeStep READ volumeStep NOTIFY volumeStepChanged FINAL)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged FINAL)
    Q_PROPERTY(int sleepMode READ sleepMode NOTIFY sleepChanged FINAL)
    Q_PROPERTY(int sleepRemain READ sleepRemain NOTIFY sleepChanged FINAL)
    Q_PROPERTY(QString sleepLabel READ sleepLabel NOTIFY sleepChanged FINAL)
    Q_PROPERTY(int abA READ abA NOTIFY abChanged FINAL)
    Q_PROPERTY(int abB READ abB NOTIFY abChanged FINAL)
    Q_PROPERTY(bool abArmed READ abArmed NOTIFY abChanged FINAL)
    Q_PROPERTY(QString localLyricsRequestPath READ localLyricsRequestPath NOTIFY localLyricsRequestPathChanged FINAL)
    Q_PROPERTY(QString musicTitle READ musicTitle NOTIFY musicTitleChanged FINAL)
    Q_PROPERTY(QString musicArtist READ musicArtist NOTIFY musicArtistChanged FINAL)
    Q_PROPERTY(QString musicHash READ musicHash NOTIFY musicHashChanged FINAL)
    Q_PROPERTY(int musicSource READ musicSource NOTIFY musicSourceChanged FINAL)
    Q_PROPERTY(int kSourceWebDav READ kSourceWebDav CONSTANT FINAL)

public:
    explicit Playback(QObject *parent);
    ~Playback() override;

    static Playback *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static void notifyApiReady(MusicApiService *api);

    AudioEngine *player() { return &m_engine; }
    QueueModel *queue() { return &m_queue; }
    PlaybackHistory *history() { return &m_history; }
    CoverHelper *coverHelper() { return m_cover; }

    int count() const { return m_queue.count(); }
    bool inplay() const { return m_inplay; }
    int cycleIndex() const { return m_cycle; }
    void setCycleIndex(int index);
    QString cycleIcon() const;
    QString cycleTip() const;

    qreal volume() const { return m_volume; }
    Q_INVOKABLE void setVolume(qreal value);
    qreal outVolume() const { return m_muted ? 0.0 : m_volume; }
    qreal volumeStep() const { return m_volumeStepPercent / 100.0; }
    Q_INVOKABLE void refreshSettings();
    bool muted() const { return m_muted; }
    void setMuted(bool muted);

    int sleepMode() const { return m_sleepMode; }
    int sleepRemain() const { return m_sleepRemain; }
    QString sleepLabel() const;
    int abA() const { return m_abA; }
    int abB() const { return m_abB; }
    bool abArmed() const { return m_abA >= 0 && m_abB > m_abA; }

    QString localLyricsRequestPath() const { return m_localLyricsRequestPath; }
    QString musicTitle() const { return m_title; }
    QString musicArtist() const { return m_artist; }
    QString musicHash() const { return m_hash; }
    int musicSource() const { return m_source; }
    int kSourceWebDav() const { return 3; }

    Q_INVOKABLE void playIndex(int index);
    Q_INVOKABLE void next(bool random = false);
    Q_INVOKABLE void previous();
    Q_INVOKABLE bool enqueue(const QString &name, const QString &path,
                             const QString &songer, int source);
    Q_INVOKABLE void playNext(const QString &name, const QString &path,
                              const QString &songer, int source);
    Q_INVOKABLE void playItem(const QString &name, const QString &path,
                              const QString &songer, int source);
    Q_INVOKABLE void playLocalSong(const QString &path, const QString &name);
    Q_INVOKABLE void togglePlay();
    Q_INVOKABLE void stepVolume(qreal delta);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void seekBy(qint64 ms);
    Q_INVOKABLE void seekBack();
    Q_INVOKABLE void seekForward();
    Q_INVOKABLE void setAbPoint(int which);
    Q_INVOKABLE void clearAb();
    Q_INVOKABLE void armSleep(int mode, int minutes);
    Q_INVOKABLE void stopSleep();
    Q_INVOKABLE void saveQueue();
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE void flush();
    Q_INVOKABLE QString sourceText(int source) const;
    Q_INVOKABLE QString fmt(qint64 ms) const;

signals:
    void countChanged();
    void inplayChanged();
    void cycleIndexChanged();
    void volumeChanged();
    void volumeStepChanged();
    void mutedChanged();
    void sleepChanged();
    void abChanged();
    void localLyricsRequestPathChanged();
    void musicTitleChanged();
    void musicArtistChanged();
    void musicHashChanged();
    void musicSourceChanged();
    void warned(const QString &text, int type);

private:
    struct Ready {
        int index = -1;
        QueueTrack track;
        QUrl url;
        QString title;
        QString artist;
        QString cover;
        int duration = 0;
    };

    void bindApi();
    void bindWebDav();
    void loadSettings();
    void refreshRuntimeSettings();
    void openLocal(const QueueTrack &track, const QString &localPath);
    void openOnline(const QueueTrack &track);
    void openWebDav(const QueueTrack &track);
    void prepareNext();
    void onTrackAdvanced();
    void adoptAuto(int index, const QString &path);
    void recordHistory();
    void loadQueue();
    void sleepEnd();
    int rowFor(int index, const QString &path) const;
    void setMusicHash(const QString &hash);
    void setMusicSource(int source);
    void setLocalLyricsRequestPath(const QString &path);
    void setMusicTitle(const QString &title);
    void setMusicArtist(const QString &artist);
    void setInplay(bool inplay);
    void updateVolume();
    void onQueueChanged();
    void onEngineMediaStatusChanged();
    void onEnginePositionChanged();
    void onEngineError(int code, const QString &message);
    void onEngineMetaDataChanged();
    void onLocalMetadataReady(const QString &filePath, const QVariantMap &meta);
    void onLocalLyricsReady(const QString &filePath, const QVariantList &lyrics,
                            const QVariantList &translate);
    void onLocalLyricsFailed(const QString &filePath);
    void onLocalCoverReady(const QString &filePath, const QString &coverUrl);
    void onWebDavCached(const QString &url, const QString &localPath);
    void onUrlplay(const QString &playurl, const QString &title, const QString &artist,
                   const QString &cover, const QString &solve, const QString &hash, int source);
    void onPlayUrlReady(const QString &hash, const QString &url, const QString &title,
                        const QString &artist, const QString &cover, int duration, int source);
    void onPlayFailed(const QString &hash, int source);

    AudioEngine m_engine;
    QueueModel m_queue;
    PlaybackHistory m_history;
    PlaybackCursor m_cursor;
    CoverHelper *m_cover = nullptr;
    QPointer<MusicApiService> m_api;
    inline static Playback *s_instance = nullptr;

    QTimer m_sleepTimer;
    QTimer m_pauseTimer;
    QTimer m_saveTimer;

    QueueTrack m_current;
    Ready m_next;
    std::atomic<int> m_audioIndex{-1};
    bool m_appending = false;
    bool m_onlinePending = false;
    bool m_webDavBound = false;
    bool m_queueLoaded = false;

    QString m_title = QStringLiteral("QueMusic");
    QString m_artist;
    QString m_hash;
    int m_source = -1;
    QString m_localLyricsRequestPath;
    QString m_jsonCover;
    int m_autoSkipCount = 0;

    int m_cycle = 0;
    bool m_inplay = false;
    bool m_muted = false;
    qreal m_volume = 0.6;
    int m_volumeStepPercent = 5;
    bool m_fadeEnabled = true;
    int m_fadeMs = 200;
    int m_shuffleAvoid = 2;
    int m_historyLimit = 200;

    int m_sleepMode = 0;
    int m_sleepRemain = 0;
    int m_abA = -1;
    int m_abB = -1;
};