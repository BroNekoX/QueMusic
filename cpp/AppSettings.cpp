// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "AppSettings.h"

#include <QMetaType>
#include <QQmlEngine>
#include <QSettings>
#include <QVariant>

AppSettings *AppSettings::s_instance = nullptr;

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
{
    // 启动时把 INI 里的存量键全部读进成员；缺省值即成员声明值（与原 Config.qml 一致）
    QSettings settings;
    settings.beginGroup(QStringLiteral("Options"));

#define APPSETTINGS_LOAD_BOOL(NAME) \
    m_##NAME = settings.value(QStringLiteral(#NAME), m_##NAME).toBool();
#define APPSETTINGS_LOAD_INT(NAME) \
    m_##NAME = settings.value(QStringLiteral(#NAME), m_##NAME).toInt();
#define APPSETTINGS_LOAD_STRING(NAME) \
    m_##NAME = settings.value(QStringLiteral(#NAME), m_##NAME).toString();

    // searchList 在 INI 里可能是：QStringList（"a, b"）、@Invalid()（空 list）、
    // 或单元素裸字符串（QSettings 对单元素 list 不加逗号分隔），统一归一化为 QVariantList。
    const QVariant listValue = settings.value(QStringLiteral("searchList"));
    const QMetaType::Type listType = static_cast<QMetaType::Type>(listValue.metaType().id());
    if (listType == QMetaType::QStringList || listType == QMetaType::QVariantList) {
        m_searchList = listValue.toList();
    } else if (const QString single = listValue.toString(); !single.isEmpty()) {
        m_searchList = QVariantList{single};
    }

    APPSETTINGS_LOAD_BOOL(closeToManage)
    APPSETTINGS_LOAD_BOOL(rememberWindow)
    APPSETTINGS_LOAD_INT(winX)
    APPSETTINGS_LOAD_INT(winY)
    APPSETTINGS_LOAD_INT(winW)
    APPSETTINGS_LOAD_INT(winH)
    APPSETTINGS_LOAD_BOOL(autoUpdate)
    APPSETTINGS_LOAD_BOOL(openShortCut)
    APPSETTINGS_LOAD_BOOL(picCache)

    APPSETTINGS_LOAD_INT(soundQuality)
    APPSETTINGS_LOAD_BOOL(autoPlay)
    APPSETTINGS_LOAD_INT(playerRateIndex)
    APPSETTINGS_LOAD_INT(volumeStep)
    APPSETTINGS_LOAD_BOOL(recursive)

    APPSETTINGS_LOAD_INT(seekStep)
    APPSETTINGS_LOAD_BOOL(fadeEnabled)
    APPSETTINGS_LOAD_INT(fadeMs)
    APPSETTINGS_LOAD_INT(sleepMinutes)
    APPSETTINGS_LOAD_INT(historyLimit)
    APPSETTINGS_LOAD_INT(shuffleAvoid)
    APPSETTINGS_LOAD_BOOL(autoRestoreQueue)

    APPSETTINGS_LOAD_STRING(downloadFolder)
    APPSETTINGS_LOAD_INT(scanTime)
    APPSETTINGS_LOAD_STRING(cacheUrl)
    APPSETTINGS_LOAD_STRING(dataUrl)
    APPSETTINGS_LOAD_INT(metaDataSource)
    APPSETTINGS_LOAD_INT(mainMusicSource)
    APPSETTINGS_LOAD_INT(serverAgency)
    APPSETTINGS_LOAD_INT(cacheSize)
    APPSETTINGS_LOAD_STRING(editSource)

    APPSETTINGS_LOAD_BOOL(noWindowKit)
    APPSETTINGS_LOAD_BOOL(softwareRender)
    APPSETTINGS_LOAD_INT(gpuRenderMode)
    APPSETTINGS_LOAD_BOOL(displayFps)
    APPSETTINGS_LOAD_BOOL(debug)
    APPSETTINGS_LOAD_BOOL(timerAnimator)
    APPSETTINGS_LOAD_BOOL(qmlAnimator)
    APPSETTINGS_LOAD_BOOL(displayDebugControl)

    settings.endGroup();

#undef APPSETTINGS_LOAD_BOOL
#undef APPSETTINGS_LOAD_INT
#undef APPSETTINGS_LOAD_STRING
}

AppSettings::~AppSettings()
{
    if (s_instance == this)
        s_instance = nullptr;
}

// 单例工厂：静态实例守卫 + C++ 所有权，QML 侧不可接管（与 LogManager 同一模式）
AppSettings *AppSettings::create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(scriptEngine)
    if (!s_instance) {
        s_instance = new AppSettings(QCoreApplication::instance());
        QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }
    return s_instance;
}

void AppSettings::persist(const QString &key, const QVariant &value)
{
    QSettings settings;
    settings.setValue(QStringLiteral("Options/") + key, value);
}

// setter 批量实现：赋值 → 写透 QSettings → NOTIFY
#define APPSETTINGS_SETTER_BOOL(NAME, CAMEL)             \
    void AppSettings::set##CAMEL(bool value)             \
    {                                                    \
        if (m_##NAME == value)                           \
            return;                                      \
        m_##NAME = value;                                \
        persist(QStringLiteral(#NAME), value);           \
        emit NAME##Changed();                            \
    }

#define APPSETTINGS_SETTER_INT(NAME, CAMEL)              \
    void AppSettings::set##CAMEL(int value)              \
    {                                                    \
        if (m_##NAME == value)                           \
            return;                                      \
        m_##NAME = value;                                \
        persist(QStringLiteral(#NAME), value);           \
        emit NAME##Changed();                            \
    }

#define APPSETTINGS_SETTER_STRING(NAME, CAMEL)           \
    void AppSettings::set##CAMEL(const QString &value)   \
    {                                                    \
        if (m_##NAME == value)                           \
            return;                                      \
        m_##NAME = value;                                \
        persist(QStringLiteral(#NAME), value);           \
        emit NAME##Changed();                            \
    }

#define APPSETTINGS_SETTER_LIST(NAME, CAMEL)                     \
    void AppSettings::set##CAMEL(const QVariantList &value)      \
    {                                                            \
        if (m_##NAME == value)                                   \
            return;                                              \
        m_##NAME = value;                                        \
        persist(QStringLiteral(#NAME), value);                   \
        emit NAME##Changed();                                    \
    }

// ---- 全局 ----
APPSETTINGS_SETTER_BOOL(closeToManage, CloseToManage)
APPSETTINGS_SETTER_BOOL(rememberWindow, RememberWindow)
APPSETTINGS_SETTER_INT(winX, WinX)
APPSETTINGS_SETTER_INT(winY, WinY)
APPSETTINGS_SETTER_INT(winW, WinW)
APPSETTINGS_SETTER_INT(winH, WinH)
APPSETTINGS_SETTER_BOOL(autoUpdate, AutoUpdate)
APPSETTINGS_SETTER_LIST(searchList, SearchList)
APPSETTINGS_SETTER_BOOL(openShortCut, OpenShortCut)
APPSETTINGS_SETTER_BOOL(picCache, PicCache)

// ---- 播放器 ----
APPSETTINGS_SETTER_INT(soundQuality, SoundQuality)
APPSETTINGS_SETTER_BOOL(autoPlay, AutoPlay)
APPSETTINGS_SETTER_INT(playerRateIndex, PlayerRateIndex)
APPSETTINGS_SETTER_INT(volumeStep, VolumeStep)
APPSETTINGS_SETTER_BOOL(recursive, Recursive)

// ---- 播放增强 ----
APPSETTINGS_SETTER_INT(seekStep, SeekStep)
APPSETTINGS_SETTER_BOOL(fadeEnabled, FadeEnabled)
APPSETTINGS_SETTER_INT(fadeMs, FadeMs)
APPSETTINGS_SETTER_INT(sleepMinutes, SleepMinutes)
APPSETTINGS_SETTER_INT(historyLimit, HistoryLimit)
APPSETTINGS_SETTER_INT(shuffleAvoid, ShuffleAvoid)
APPSETTINGS_SETTER_BOOL(autoRestoreQueue, AutoRestoreQueue)

// ---- 媒体 ----
APPSETTINGS_SETTER_STRING(downloadFolder, DownloadFolder)
APPSETTINGS_SETTER_INT(scanTime, ScanTime)
APPSETTINGS_SETTER_STRING(cacheUrl, CacheUrl)
APPSETTINGS_SETTER_STRING(dataUrl, DataUrl)
APPSETTINGS_SETTER_INT(metaDataSource, MetaDataSource)
APPSETTINGS_SETTER_INT(mainMusicSource, MainMusicSource)
APPSETTINGS_SETTER_INT(serverAgency, ServerAgency)
APPSETTINGS_SETTER_INT(cacheSize, CacheSize)
APPSETTINGS_SETTER_STRING(editSource, EditSource)

// ---- 高级 ----
APPSETTINGS_SETTER_BOOL(noWindowKit, NoWindowKit)
APPSETTINGS_SETTER_BOOL(softwareRender, SoftwareRender)
APPSETTINGS_SETTER_INT(gpuRenderMode, GpuRenderMode)
APPSETTINGS_SETTER_BOOL(displayFps, DisplayFps)
APPSETTINGS_SETTER_BOOL(debug, Debug)
APPSETTINGS_SETTER_BOOL(timerAnimator, TimerAnimator)
APPSETTINGS_SETTER_BOOL(qmlAnimator, QmlAnimator)
APPSETTINGS_SETTER_BOOL(displayDebugControl, DisplayDebugControl)

// ---- 新增（纯 C++ 常驻层改造）----
APPSETTINGS_SETTER_BOOL(lightweightAutoEnabled, LightweightAutoEnabled)
APPSETTINGS_SETTER_INT(lightweightDelaySeconds, LightweightDelaySeconds)
APPSETTINGS_SETTER_BOOL(desktopLyricsOn, DesktopLyricsOn)
APPSETTINGS_SETTER_BOOL(desktopMiniPlayerOn, DesktopMiniPlayerOn)

#undef APPSETTINGS_SETTER_BOOL
#undef APPSETTINGS_SETTER_INT
#undef APPSETTINGS_SETTER_STRING
#undef APPSETTINGS_SETTER_LIST
