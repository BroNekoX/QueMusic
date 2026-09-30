// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 歌词界面插件：目录扫描 / 安装 / 删除见 PluginStore，这里只保留「同时只用其中一个」的单选状态。
// 内置界面与已安装插件合并成同一列表交给宿主与设置页使用。
#pragma once

#include "plugins/PluginStore.h"

#include <QQmlEngine>

class LyricsPluginStore : public PluginStore
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LyricsPlugins)
    QML_SINGLETON
    Q_PROPERTY(QString selectedId READ selectedId WRITE setSelectedId NOTIFY selectedIdChanged)
    Q_PROPERTY(QString source READ source NOTIFY selectedIdChanged)          // 当前入口，绑给 Loader.source

public:
    explicit LyricsPluginStore(QObject *parent = nullptr);
    static LyricsPluginStore *create(QQmlEngine *, QJSEngine *) { return new LyricsPluginStore(); }

    QString selectedId() const { return m_selectedId; }
    void setSelectedId(const QString &id);
    QString source() const { return m_source; }      // 缓存的当前入口

    Q_INVOKABLE void rescan() override;

signals:
    void selectedIdChanged();

protected:
    QVariantList builtinPlugins() const override;

private:
    void refreshSource();

    QString m_selectedId;
    QString m_source;
};
