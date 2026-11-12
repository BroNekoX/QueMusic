// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqml.h>

// 一个插件 = 一目录 + info.json；type 区分 function（可多选）与 lyrics（单选）
class PluginStore : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Plugins)
    QML_SINGLETON

    Q_PROPERTY(QVariantList functions READ functions NOTIFY changed FINAL)
    Q_PROPERTY(QVariantList activeFunctions READ activeFunctions NOTIFY enabledChanged FINAL)
    Q_PROPERTY(QVariantList lyrics READ lyrics NOTIFY changed FINAL)
    Q_PROPERTY(QStringList enabled READ enabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(QString lyricsId READ lyricsId NOTIFY lyricsChanged FINAL)
    Q_PROPERTY(QString lyricsSource READ lyricsSource NOTIFY lyricsChanged FINAL)
    Q_PROPERTY(QString dir READ dir NOTIFY changed FINAL)

public:
    explicit PluginStore(QObject *parent = nullptr);
    static PluginStore *create(QQmlEngine *, QJSEngine *) { return new PluginStore(); }

    QVariantList functions() const { return m_functions; }
    QVariantList activeFunctions() const { return m_active; }
    QVariantList lyrics() const { return m_lyrics; }
    QStringList enabled() const { return m_enabled; }
    QString lyricsId() const { return m_lyricsId; }
    QString lyricsSource() const { return m_lyricsSource; }
    QString dir() const;

    Q_INVOKABLE void rescan();
    Q_INVOKABLE void setEnabled(const QString &id, bool on);
    Q_INVOKABLE void selectLyrics(const QString &id);
    Q_INVOKABLE QString install(const QString &folder);
    Q_INVOKABLE QString remove(const QString &id);
    Q_INVOKABLE void reveal(const QString &id);

signals:
    void changed();
    void enabledChanged();
    void lyricsChanged();

private:
    void load();
    void rebuild();
    void refreshLyrics();
    bool find(const QString &id, QVariantMap *info) const;

    QVariantList m_all;
    QVariantList m_functions;
    QVariantList m_active;
    QVariantList m_lyrics;
    QStringList m_enabled;
    QString m_lyricsId;
    QString m_lyricsSource;
};