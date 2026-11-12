#pragma once

#include "AppSettings.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

class Config final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static Config *instance();
    static Config *create(QQmlEngine *, QJSEngine *);

    struct RenderOptions
    {
        int gpuRenderMode = 0;
        bool timerAnimator = false;
        bool qmlAnimator = false;
    };
    static RenderOptions readRenderOptions(const QString &configDir);

// 预处理设置项，让他编译生成设置项内容
#define QUE_MUSIC_OPTIONS(set) \
    set(bool, closeToManage, false) \
    set(bool, rememberWindow, false) \
    set(int, winX, 0) \
    set(int, winY, 0) \
    set(int, winW, 0) \
    set(int, winH, 0) \
    set(bool, autoUpdate, false) \
    set(QStringList, searchList, QStringList()) \
    set(bool, openShortCut, true) \
    set(bool, picCache, false) \
    set(int, soundQuality, 1) \
    set(bool, autoPlay, true) \
    set(int, playerRateIndex, 2) \
    set(int, volumeStep, 5) \
    set(bool, recursive, true) \
    set(int, seekStep, 5) \
    set(bool, fadeEnabled, true) \
    set(int, fadeMs, 200) \
    set(int, sleepMinutes, 30) \
    set(int, historyLimit, 200) \
    set(int, shuffleAvoid, 2) \
    set(bool, autoRestoreQueue, true) \
    set(QString, playHistory, QStringLiteral("[]")) \
    set(QString, lastQueue, QStringLiteral("[]")) \
    set(int, lastQueueIndex, -1) \
    set(QString, lastSongName, QString()) \
    set(QString, lastSongArtist, QString()) \
    set(QString, lastSongCover, QString()) \
    set(QString, lastSongHash, QString()) \
    set(int, lastSongSource, -1) \
    set(QString, downloadFolder, QString()) \
    set(int, scanTime, 15) \
    set(QString, cacheUrl, QString()) \
    set(QString, dataUrl, QString()) \
    set(int, metaDataSource, 0) \
    set(int, mainMusicSource, 0) \
    set(int, serverAgency, 0) \
    set(int, cacheSize, 500) \
    set(QString, editSource, QString()) \
    set(bool, noWindowKit, false) \
    set(bool, softwareRender, false) \
    set(int, gpuRenderMode, 0) \
    set(bool, displayFps, false) \
    set(bool, debug, false) \
    set(bool, timerAnimator, false) \
    set(bool, qmlAnimator, false) \
    set(bool, displayDebugControl, false) \
    set(qreal, musicVolume, 0.6) \
    set(bool, muted, false) \
    set(int, cycleIndex, 0) \
    set(bool, logEnabled, true) \
    set(int, logLevel, 1) \
    set(bool, isFirst, true)

#define QUE_MUSIC_OPTION(TYPE, NAME, DEFAULT) \
    Q_PROPERTY(TYPE NAME READ NAME WRITE set_##NAME NOTIFY NAME##Changed FINAL) \
public: \
    TYPE NAME() const { return m_##NAME; } \
    void set_##NAME(const TYPE &value) \
    { \
        if (m_##NAME == value) \
            return; \
        m_##NAME = value; \
        AppSettings::set(QStringLiteral("Options"), #NAME, value); \
        emit NAME##Changed(); \
    } \
private: \
    TYPE m_##NAME = DEFAULT;

    QUE_MUSIC_OPTIONS(QUE_MUSIC_OPTION)
#undef QUE_MUSIC_OPTION

signals:
#define QUE_MUSIC_SIGNAL(TYPE, NAME, DEFAULT) void NAME##Changed();
    QUE_MUSIC_OPTIONS(QUE_MUSIC_SIGNAL)
#undef QUE_MUSIC_SIGNAL

private:
    explicit Config(QObject *parent = nullptr);
};
