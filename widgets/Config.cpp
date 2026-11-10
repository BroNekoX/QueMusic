#include "Config.h"

#include <QQmlEngine>

Config *Config::instance()
{
    static Config *instance = nullptr;
    if (!instance) {
        instance = new Config();
        QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
    }
    return instance;
}

Config *Config::create(QQmlEngine *, QJSEngine *)
{
    return instance();
}

Config::RenderOptions Config::readRenderOptions(const QString &configDir)
{
    const SettingsFile file(configDir + QStringLiteral("/BroNekoX/QueMusic.ini"));
    RenderOptions options;
    options.gpuRenderMode = file.value(QStringLiteral("Options"), QStringLiteral("gpuRenderMode"), 0).toInt();
    options.timerAnimator = file.value(QStringLiteral("Options"), QStringLiteral("timerAnimator"), false).toBool();
    options.qmlAnimator = file.value(QStringLiteral("Options"), QStringLiteral("qmlAnimator"), false).toBool();
    return options;
}

Config::Config(QObject *parent)
    : QObject(parent)
{
#define QUE_MUSIC_LOAD(TYPE, NAME, DEFAULT) m_##NAME = AppSettings::get<TYPE>(QStringLiteral("Options"), #NAME, DEFAULT);
    QUE_MUSIC_OPTIONS(QUE_MUSIC_LOAD)
#undef QUE_MUSIC_LOAD

    // 兼容旧版 LastMedia 记录；没有 hash 就不迁移，省去无意义的读取
    if (m_lastSongHash.isEmpty()) {
        const QString oldHash = AppSettings::get<QString>(QStringLiteral("LastMedia"), "hash", QString());
        if (!oldHash.isEmpty()) {
            m_lastSongName = AppSettings::get<QString>(QStringLiteral("LastMedia"), "name", QString());
            m_lastSongArtist = AppSettings::get<QString>(QStringLiteral("LastMedia"), "artist", QString());
            m_lastSongCover = AppSettings::get<QString>(QStringLiteral("LastMedia"), "cover", QString());
            m_lastSongHash = oldHash;
            m_lastSongSource = AppSettings::get<int>(QStringLiteral("LastMedia"), "source", -1);
        }
    }
}
