#pragma once

#include <QSettings>
#include <QString>
#include <QVariant>

class AppSettings
{
public:
    static QVariant value(const QString &group, const QString &key, const QVariant &def = {});
    static void setValue(const QString &group, const QString &key, const QVariant &value);
    static void remove(const QString &group, const QString &key);
    static void sync();

    template <typename In>
    static In get(const QString &group, const char *key, const In &def)
    {
        return value(group, QString::fromLatin1(key), QVariant::fromValue(def)).template value<In>();
    }

    template <typename In>
    static void set(const QString &group, const char *key, const In &value)
    {
        setValue(group, QString::fromLatin1(key), QVariant::fromValue(value));
    }

private:
    static QSettings &settings();
};

class SettingsFile
{
public:
    explicit SettingsFile(const QString &path);
    QVariant value(const QString &group, const QString &key, const QVariant &def = {}) const;
    void setValue(const QString &group, const QString &key, const QVariant &value);
    void remove(const QString &group, const QString &key);
    void sync();

private:
    mutable QSettings m_settings;
};
