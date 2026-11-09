// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QCoreApplication>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QJSEngine;
class QQmlEngine;

// 应用设置单例：顶替原 components/Config.qml（pragma Singleton + Settings）。
// - 对 QML 仍以 "Config" 名字暴露（QML_NAMED_ELEMENT(Config)），QML 调用面零改动。
// - C++ 拥有设置：QML 写 → setter 写透 QSettings("Options/<键名>") → NOTIFY，
//   C++ 消费端（Playback / MusicApiService）connect 对应 changed 信号自动刷新。
// - INI 键名与原 Config.qml 属性名逐字一致（category "Options"，键=属性名），
//   老用户的设置文件不需要迁移。
// - playHistory / lastQueue / lastQueueIndex 三个历史双属主残留键不再声明，
//   它们实际由 Playback / PlaybackHistory（C++）独占读写。
//
// 属性用宏批量生成。注意：moc 无法解析宏展开内部的 public:/signals: 访问段关键字，
// 因此宏只生成 Q_PROPERTY + getter/setter 声明（或存储成员），NOTIFY 信号在类体
// 的 signals: 段逐一显式声明，访问段由类体上下文提供。
#define APPSETTINGS_BOOL(NAME, CAMEL, DEFAULT)                                  \
    Q_PROPERTY(bool NAME READ NAME WRITE set##CAMEL NOTIFY NAME##Changed FINAL) \
    bool NAME() const { return m_##NAME; }                                      \
    void set##CAMEL(bool value);

#define APPSETTINGS_INT(NAME, CAMEL, DEFAULT)                                  \
    Q_PROPERTY(int NAME READ NAME WRITE set##CAMEL NOTIFY NAME##Changed FINAL) \
    int NAME() const { return m_##NAME; }                                      \
    void set##CAMEL(int value);

// 字符串键默认值全部为空串
#define APPSETTINGS_STRING(NAME, CAMEL)                                             \
    Q_PROPERTY(QString NAME READ NAME WRITE set##CAMEL NOTIFY NAME##Changed FINAL)  \
    QString NAME() const { return m_##NAME; }                                       \
    void set##CAMEL(const QString &value);

// 原 list<string>：以 QVariantList 暴露，QML 侧 filter/splice 等用法不变
#define APPSETTINGS_LIST(NAME, CAMEL)                                                   \
    Q_PROPERTY(QVariantList NAME READ NAME WRITE set##CAMEL NOTIFY NAME##Changed FINAL) \
    QVariantList NAME() const { return m_##NAME; }                                      \
    void set##CAMEL(const QVariantList &value);

class AppSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    QML_NAMED_ELEMENT(Config)

signals:
    // ---- 全局 ----
    void closeToManageChanged();
    void rememberWindowChanged();
    void winXChanged();
    void winYChanged();
    void winWChanged();
    void winHChanged();
    void autoUpdateChanged();
    void searchListChanged();
    void openShortCutChanged();
    void picCacheChanged();
    // ---- 播放器 ----
    void soundQualityChanged();
    void autoPlayChanged();
    void playerRateIndexChanged();
    void volumeStepChanged();
    void recursiveChanged();
    // ---- 播放增强 ----
    void seekStepChanged();
    void fadeEnabledChanged();
    void fadeMsChanged();
    void sleepMinutesChanged();
    void historyLimitChanged();
    void shuffleAvoidChanged();
    void autoRestoreQueueChanged();
    // ---- 媒体 ----
    void downloadFolderChanged();
    void scanTimeChanged();
    void cacheUrlChanged();
    void dataUrlChanged();
    void metaDataSourceChanged();
    void mainMusicSourceChanged();
    void serverAgencyChanged();
    void cacheSizeChanged();
    void editSourceChanged();
    // ---- 高级 ----
    void noWindowKitChanged();
    void softwareRenderChanged();
    void gpuRenderModeChanged();
    void displayFpsChanged();
    void debugChanged();
    void timerAnimatorChanged();
    void qmlAnimatorChanged();
    void displayDebugControlChanged();
    // ---- 新增（纯 C++ 常驻层改造）----
    void lightweightAutoEnabledChanged();
    void lightweightDelaySecondsChanged();
    void desktopLyricsOnChanged();
    void desktopMiniPlayerOnChanged();

public:
    // QML 单例工厂。main.cpp 在 MusicApiService/Playback 构造之前提前调用，
    // 保证消费端能取到实例并 connect 信号；之后 QML 首次访问 Config 时复用同一实例。
    static AppSettings *create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine);

    // C++ 消费端入口；self-check 等未提前创建的场景返回 nullptr
    static AppSettings *instance() { return s_instance; }

    explicit AppSettings(QObject *parent);
    ~AppSettings() override;

    // ---- 全局 ----
    APPSETTINGS_BOOL(closeToManage, CloseToManage, false)        // 关闭则最小化到托盘
    APPSETTINGS_BOOL(rememberWindow, RememberWindow, false)      // 记住窗口位置和大小
    APPSETTINGS_INT(winX, WinX, 0)
    APPSETTINGS_INT(winY, WinY, 0)
    APPSETTINGS_INT(winW, WinW, 0)
    APPSETTINGS_INT(winH, WinH, 0)
    APPSETTINGS_BOOL(autoUpdate, AutoUpdate, false)
    APPSETTINGS_LIST(searchList, SearchList)                     // 搜索历史
    APPSETTINGS_BOOL(openShortCut, OpenShortCut, true)
    APPSETTINGS_BOOL(picCache, PicCache, false)

    // ---- 播放器 ----
    APPSETTINGS_INT(soundQuality, SoundQuality, 1)               // 音质档位
    APPSETTINGS_BOOL(autoPlay, AutoPlay, true)
    APPSETTINGS_INT(playerRateIndex, PlayerRateIndex, 2)         // 倍速预设
    APPSETTINGS_INT(volumeStep, VolumeStep, 5)                   // 音量步长(%)
    APPSETTINGS_BOOL(recursive, Recursive, true)                 // 扫描子目录

    // ---- 播放增强 ----
    APPSETTINGS_INT(seekStep, SeekStep, 5)                       // 精确跳转步长（秒）
    APPSETTINGS_BOOL(fadeEnabled, FadeEnabled, true)
    APPSETTINGS_INT(fadeMs, FadeMs, 200)
    APPSETTINGS_INT(sleepMinutes, SleepMinutes, 30)
    APPSETTINGS_INT(historyLimit, HistoryLimit, 200)
    APPSETTINGS_INT(shuffleAvoid, ShuffleAvoid, 2)               // 随机播放避开最近 N 首
    APPSETTINGS_BOOL(autoRestoreQueue, AutoRestoreQueue, true)

    // ---- 媒体 ----
    APPSETTINGS_STRING(downloadFolder, DownloadFolder)
    APPSETTINGS_INT(scanTime, ScanTime, 15)                      // 自动扫描更新间隔
    APPSETTINGS_STRING(cacheUrl, CacheUrl)
    APPSETTINGS_STRING(dataUrl, DataUrl)
    APPSETTINGS_INT(metaDataSource, MetaDataSource, 0)           // 0.默认 1.Metadata 2.NetWork
    APPSETTINGS_INT(mainMusicSource, MainMusicSource, 0)         // 默认歌曲源，详见 MusicApi
    APPSETTINGS_INT(serverAgency, ServerAgency, 0)
    APPSETTINGS_INT(cacheSize, CacheSize, 500)                   // 缓存上限(mb)
    APPSETTINGS_STRING(editSource, EditSource)

    // ---- 高级 ----
    APPSETTINGS_BOOL(noWindowKit, NoWindowKit, false)
    APPSETTINGS_BOOL(softwareRender, SoftwareRender, false)
    APPSETTINGS_INT(gpuRenderMode, GpuRenderMode, 0)             // 0.默认平台 1.OpenGL 2.Vulkan
    APPSETTINGS_BOOL(displayFps, DisplayFps, false)
    APPSETTINGS_BOOL(debug, Debug, false)
    APPSETTINGS_BOOL(timerAnimator, TimerAnimator, false)
    APPSETTINGS_BOOL(qmlAnimator, QmlAnimator, false)
    APPSETTINGS_BOOL(displayDebugControl, DisplayDebugControl, false)

    // ---- 新增（纯 C++ 常驻层改造）----
    APPSETTINGS_BOOL(lightweightAutoEnabled, LightweightAutoEnabled, true)  // 轻量模式自动进入开关
    APPSETTINGS_INT(lightweightDelaySeconds, LightweightDelaySeconds, 60)    // 轻量模式延迟秒数
    APPSETTINGS_BOOL(desktopLyricsOn, DesktopLyricsOn, false)                // 桌面歌词开关
    APPSETTINGS_BOOL(desktopMiniPlayerOn, DesktopMiniPlayerOn, false)        // 小窗播放器开关

private:
    // 存储成员：默认值与原 Config.qml 声明逐字一致
#define APPSETTINGS_MEMBER_BOOL(NAME, DEFAULT)   bool m_##NAME = DEFAULT;
#define APPSETTINGS_MEMBER_INT(NAME, DEFAULT)    int m_##NAME = DEFAULT;
#define APPSETTINGS_MEMBER_STRING(NAME)          QString m_##NAME;
#define APPSETTINGS_MEMBER_LIST(NAME)            QVariantList m_##NAME;

    APPSETTINGS_MEMBER_BOOL(closeToManage, false)
    APPSETTINGS_MEMBER_BOOL(rememberWindow, false)
    APPSETTINGS_MEMBER_INT(winX, 0)
    APPSETTINGS_MEMBER_INT(winY, 0)
    APPSETTINGS_MEMBER_INT(winW, 0)
    APPSETTINGS_MEMBER_INT(winH, 0)
    APPSETTINGS_MEMBER_BOOL(autoUpdate, false)
    APPSETTINGS_MEMBER_LIST(searchList)
    APPSETTINGS_MEMBER_BOOL(openShortCut, true)
    APPSETTINGS_MEMBER_BOOL(picCache, false)

    APPSETTINGS_MEMBER_INT(soundQuality, 1)
    APPSETTINGS_MEMBER_BOOL(autoPlay, true)
    APPSETTINGS_MEMBER_INT(playerRateIndex, 2)
    APPSETTINGS_MEMBER_INT(volumeStep, 5)
    APPSETTINGS_MEMBER_BOOL(recursive, true)

    APPSETTINGS_MEMBER_INT(seekStep, 5)
    APPSETTINGS_MEMBER_BOOL(fadeEnabled, true)
    APPSETTINGS_MEMBER_INT(fadeMs, 200)
    APPSETTINGS_MEMBER_INT(sleepMinutes, 30)
    APPSETTINGS_MEMBER_INT(historyLimit, 200)
    APPSETTINGS_MEMBER_INT(shuffleAvoid, 2)
    APPSETTINGS_MEMBER_BOOL(autoRestoreQueue, true)

    APPSETTINGS_MEMBER_STRING(downloadFolder)
    APPSETTINGS_MEMBER_INT(scanTime, 15)
    APPSETTINGS_MEMBER_STRING(cacheUrl)
    APPSETTINGS_MEMBER_STRING(dataUrl)
    APPSETTINGS_MEMBER_INT(metaDataSource, 0)
    APPSETTINGS_MEMBER_INT(mainMusicSource, 0)
    APPSETTINGS_MEMBER_INT(serverAgency, 0)
    APPSETTINGS_MEMBER_INT(cacheSize, 500)
    APPSETTINGS_MEMBER_STRING(editSource)

    APPSETTINGS_MEMBER_BOOL(noWindowKit, false)
    APPSETTINGS_MEMBER_BOOL(softwareRender, false)
    APPSETTINGS_MEMBER_INT(gpuRenderMode, 0)
    APPSETTINGS_MEMBER_BOOL(displayFps, false)
    APPSETTINGS_MEMBER_BOOL(debug, false)
    APPSETTINGS_MEMBER_BOOL(timerAnimator, false)
    APPSETTINGS_MEMBER_BOOL(qmlAnimator, false)
    APPSETTINGS_MEMBER_BOOL(displayDebugControl, false)

    APPSETTINGS_MEMBER_BOOL(lightweightAutoEnabled, true)
    APPSETTINGS_MEMBER_INT(lightweightDelaySeconds, 60)
    APPSETTINGS_MEMBER_BOOL(desktopLyricsOn, false)
    APPSETTINGS_MEMBER_BOOL(desktopMiniPlayerOn, false)

#undef APPSETTINGS_MEMBER_BOOL
#undef APPSETTINGS_MEMBER_INT
#undef APPSETTINGS_MEMBER_STRING
#undef APPSETTINGS_MEMBER_LIST

    // 写透 QSettings("Options/<键名>")；键名=QML 属性名，与原 Config.qml 一致
    void persist(const QString &key, const QVariant &value);

    static AppSettings *s_instance;
};

#undef APPSETTINGS_BOOL
#undef APPSETTINGS_INT
#undef APPSETTINGS_STRING
#undef APPSETTINGS_LIST

#endif // APPSETTINGS_H
