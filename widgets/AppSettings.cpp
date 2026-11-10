#include "AppSettings.h"

QSettings &AppSettings::settings()
{
    static QSettings *instance = nullptr;
    if (!instance)
        instance = new QSettings();
    return *instance;
}

QVariant AppSettings::value(const QString &group, const QString &key, const QVariant &def)
{
    return settings().value(group + QLatin1Char('/') + key, def);
}

void AppSettings::setValue(const QString &group, const QString &key, const QVariant &value)
{
    settings().setValue(group + QLatin1Char('/') + key, value);
}

void AppSettings::remove(const QString &group, const QString &key)
{
    settings().remove(group + QLatin1Char('/') + key);
}

void AppSettings::sync()
{
    settings().sync();
}

SettingsFile::SettingsFile(const QString &path)
    : m_settings(path, QSettings::IniFormat)
{
}

QVariant SettingsFile::value(const QString &group, const QString &key, const QVariant &def) const
{
    return m_settings.value(group + QLatin1Char('/') + key, def);
}

void SettingsFile::setValue(const QString &group, const QString &key, const QVariant &value)
{
    m_settings.setValue(group + QLatin1Char('/') + key, value);
}

void SettingsFile::remove(const QString &group, const QString &key)
{
    m_settings.remove(group + QLatin1Char('/') + key);
}

void SettingsFile::sync()
{
    m_settings.sync();
}
