// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef PLAYBACKCONTROLLER_H
#define PLAYBACKCONTROLLER_H

#include "audio/AudioEngine.h"
#include "QueueModel.h"

#include <QAbstractListModel>
#include <QMutex>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>
#include <QVector>
#include <QtQmlIntegration/qqmlintegration.h>

#include <atomic>

class MusicApiService;
class QQmlEngine;
class QJSEngine;

class PlaybackHistoryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        ArtistRole,
        PathRole,
        SourceRole,
        CoverRole,
        DurationRole,
        TimeRole
    };

    explicit PlaybackHistoryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE void append(const QVariantMap &item);
    Q_INVOKABLE void remove(int index, int count = 1);
    Q_INVOKABLE void clear();

    void setItems(const QVector<QVariantMap> &items);
    void pushFront(const QVariantMap &item);
    QVector<QVariantMap> items() const { return m_items; }

signals:
    void countChanged();

private:
    QVector<QVariantMap> m_items;
};

class PlaybackController : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Playback)
    QML_SINGLETON

    Q_PROPERTY(AudioEngine *player READ player CONSTANT FINAL)
    Q_PROPERTY(QueueModel *queue READ queue CONSTANT FINAL)
    Q_PROPERTY(PlaybackHistoryModel *history READ history CONSTANT FINAL)
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
    Q_PROPERTY(int autoSkipCount READ autoSkipCount WRITE setAutoSkipCount NOTIFY autoSkipCountChanged FINAL)
    Q_PROPERTY(QString localLyricsRequestPath READ localLyricsRequestPath WRITE setLocalLyricsRequestPath NOTIFY localLyricsRequestPathChanged FINAL)
    Q_PROPERTY(QString musicTitle READ musicTitle WRITE setMusicTitle NOTIFY musicTitleChanged FINAL)
    Q_PROPERTY(QString musicArtist READ musicArtist WRITE setMusicArtist NOTIFY musicArtistChanged FINAL)
    Q_PROPERTY(QString musicHash READ musicHash WRITE setMusicHash NOTIFY musicHashChanged FINAL)
    Q_PROPERTY(int musicSource READ musicSource WRITE setMusicSource NOTIFY musicSourceChanged FINAL)
    Q_PROPERTY(int kSourceWebDav READ kSourceWebDav CONSTANT FINAL)

public:
    explicit PlaybackController(QObject *parent);
    ~PlaybackController() override;

    static PlaybackController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static void notifyApiReady(MusicApiService *api);

    AudioEngine *player() { return &m_engine; }
    QueueModel *queue() { return &m_queue; }
    PlaybackHistoryModel *history() { return &m_history; }

    int count() const { return m_queue.count(); }
    int cycleIndex() const { return m_cycleIndex.load(std::memory_order_relaxed); }
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
    int autoSkipCount() const { return m_autoSkipCount; }
    void setAutoSkipCount(int count);

    QString localLyricsRequestPath() const { return m_localLyricsRequestPath; }
    void setLocalLyricsRequestPath(const QString &path);
    QString musicTitle() const { return m_musicTitle; }
    void setMusicTitle(const QString &title);
    QString musicArtist() const { return m_musicArtist; }
    void setMusicArtist(const QString &artist);
    QString musicHash() const { return m_musicHash; }
    void setMusicHash(const QString &hash);
    int musicSource() const { return m_musicSource; }
    void setMusicSource(int source);
    int kSourceWebDav() const { return 3; }

    Q_INVOKABLE void goTo(int index);
    Q_INVOKABLE void next(bool random = false);
    Q_INVOKABLE void previous();
    Q_INVOKABLE bool enqueue(const QVariantMap &item);
    Q_INVOKABLE void playNext(const QVariantMap &item);
    Q_INVOKABLE void playItem(const QVariantMap &item);
    Q_INVOKABLE void startTrack(int index);
    Q_INVOKABLE void playLocalSong(const QString &path, const QString &name);
    Q_INVOKABLE void playWebDav(const QString &url, const QString &name, const QString &artist);
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
    Q_INVOKABLE void sleepEnd();
    Q_INVOKABLE void saveQueue();
    Q_INVOKABLE void restoreSession();
    Q_INVOKABLE void loadHistory();
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE void flush();
    Q_INVOKABLE void noteNowPlaying();
    Q_INVOKABLE QString sourceText(int source) const;
    Q_INVOKABLE QString fmt(qint64 ms) const;

signals:
    void countChanged();
    void cycleIndexChanged();
    void volumeChanged();
    void volumeStepChanged();
    void mutedChanged();
    void sleepChanged();
    void abChanged();
    void autoSkipCountChanged();
    void localLyricsRequestPathChanged();
    void musicTitleChanged();
    void musicArtistChanged();
    void musicHashChanged();
    void musicSourceChanged();
    void warned(const QString &text, int type);

private:
    void bindApi();
    void loadSettings();
    void refreshRuntimeSettings();
    void loadQueue();
    void startSource(const QueueTrack &track);
    void startLocal(const QString &path, const QString &name);
    void setEngineSource(const QUrl &url);
    void prepareNext();
    void prepareNextLocked();
    void clearPrefetchLocked();
    void syncSnapshot();
    void applyAutoAdvance(int index, const QUrl &url, const QString &title,
                          const QString &artist, const QString &cover, int duration,
                          const QString &expectedPath);
    void autoAdvanceFromAudioThread();
    int nextIndexForCycle();
    int nextIndexForCycleLocked(bool consume);
    int randomIndexLocked(bool consume);
    void notePlayed(int index);
    void pushHistory(const QVariantMap &item);
    void writeHistory();
    void updateVolume();
    void onQueueContentsChanged();
    void onEngineMediaStatusChanged();
    void onEnginePlaybackStateChanged();
    void onEnginePositionChanged();
    void onEngineError(int code, const QString &message);
    void onEngineSourceChanged();
    void onUrlplay(const QString &playurl, const QString &title, const QString &artist,
                   const QString &cover, const QString &solve, const QString &hash, int source);
    void onApiWarned(const QString &text, int type);
    void onPlayUrlReady(const QString &hash, const QString &url, const QString &title,
                        const QString &artist, const QString &cover, int duration, int source);
    void onPlayFailed(const QString &hash, int source);

    AudioEngine m_engine;
    QueueModel m_queue;
    PlaybackHistoryModel m_history;
    QPointer<MusicApiService> m_api;
    inline static PlaybackController *s_instance = nullptr;

    QTimer m_sleepTimer;
    QTimer m_pauseTimer;
    QTimer m_saveTimer;

    QString m_musicTitle = QStringLiteral("QueMusic");
    QString m_musicArtist;
    QString m_musicHash;
    int m_musicSource = -1;
    QString m_localLyricsRequestPath;
    int m_autoSkipCount = 0;

    std::atomic<int> m_cycleIndex{0};
    bool m_muted = false;
    qreal m_volume = 0.6;
    int m_volumeStepPercent = 5;
    bool m_fadeEnabled = true;
    int m_fadeMs = 200;
    int m_fadeInMs = 220;
    int m_fadeOutMs = 120;

    int m_sleepMode = 0;
    int m_sleepRemain = 0;
    int m_abA = -1;
    int m_abB = -1;

    mutable QMutex m_stateMutex;
    QVector<QueueTrack> m_snapshot;
    int m_snapshotIndex = -1;
    int m_preparedNextIndex = -1;
    QUrl m_preparedNextUrl;
    int m_prefetchIndex = -1;
    QString m_prefetchTitle;
    QString m_prefetchArtist;
    QString m_prefetchCover;
    int m_prefetchDuration = 0;
    QVector<int> m_bag;
    QVector<int> m_recent;
    std::atomic_bool m_shuttingDown{false};
    int m_shuffleAvoid = 2;
    int m_historyLimit = 200;
};

#endif