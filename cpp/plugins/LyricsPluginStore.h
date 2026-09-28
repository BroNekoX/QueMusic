// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 歌词界面插件：每个插件一个目录（info.json + 入口 QML + 预览图 + 可选着色器），
// 目录名即插件 id。内置界面与已安装插件合并成同一列表交给宿主与设置页使用。
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QVariantList>
#include <QtQml/qqml.h>

class LyricsPluginStore : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LyricsPlugins)
    QML_SINGLETON
    Q_PROPERTY(QVariantList plugins READ plugins NOTIFY pluginsChanged)      // 内置 + 已安装
    Q_PROPERTY(QString dir READ dir NOTIFY pluginsChanged)                   // 安装目录
    Q_PROPERTY(QString selectedId READ selectedId WRITE setSelectedId NOTIFY selectedIdChanged)
    Q_PROPERTY(QString source READ source NOTIFY selectedIdChanged)          // 当前入口，绑给 Loader.source

public:
    explicit LyricsPluginStore(QObject *parent = nullptr);
    static LyricsPluginStore *create(QQmlEngine *, QJSEngine *) { return new LyricsPluginStore(); }

    const QVariantList &plugins() const { return m_plugins; }
    QString dir() const;
    QString selectedId() const { return m_selectedId; }
    void setSelectedId(const QString &id);
    QString source() const { return m_source; }      // 缓存的当前入口

    Q_INVOKABLE void rescan();
    // 安装 / 覆盖更新；返回错误文本，空串表示成功
    Q_INVOKABLE QString install(const QString &folder);
    Q_INVOKABLE QString remove(const QString &id);
    // id 为空表示打开插件根目录
    Q_INVOKABLE void reveal(const QString &id);

signals:
    void pluginsChanged();
    void selectedIdChanged();

private:
    void load();
    void refreshSource();
    QString writableRoot() const;

    QVariantList m_plugins;
    QString m_selectedId;
    QString m_source;
};
