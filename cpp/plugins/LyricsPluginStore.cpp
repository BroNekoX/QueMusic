// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#include "plugins/LyricsPluginStore.h"

#include <QSettings>

namespace {

// 内置歌词界面（不可卸载）
QVariantList builtins()
{
    const auto make = [](const QString &id, const QString &name, const QString &source) {
        return QVariantMap{
            {QStringLiteral("id"), QStringLiteral("builtin.") + id},
            {QStringLiteral("name"), name},
            {QStringLiteral("author"), QStringLiteral("QueMusic")},
            {QStringLiteral("description"), QStringLiteral("应用内置歌词界面")},
            {QStringLiteral("source"), source},
            {QStringLiteral("builtin"), true}
        };
    };
    return {
        make(QStringLiteral("default"), QStringLiteral("默认"), QStringLiteral("qrc:/QueMusic/layout/MainLyric.qml")),
        make(QStringLiteral("free"), QStringLiteral("自由"), QStringLiteral("qrc:/QueMusic/lyricsui/LyricsFree.qml")),
        make(QStringLiteral("3d"), QStringLiteral("3D"), QStringLiteral("qrc:/QueMusic/lyricsui/Lyrics3D.qml"))
    };
}

} // namespace

LyricsPluginStore::LyricsPluginStore(QObject *parent)
    : PluginStore(QStringLiteral("歌词界面插件"), QStringLiteral("lyrics"), parent)
{
    load();
    m_selectedId = QSettings().value(QStringLiteral("Plugins/lyrics"),
                                     QStringLiteral("builtin.default")).toString();
    refreshSource();
    if (!m_source.isEmpty())
        return;
    m_selectedId = QStringLiteral("builtin.default");   // 选中的插件被卸载过 ⇒ 回到内置默认
    refreshSource();
}

QVariantList LyricsPluginStore::builtinPlugins() const
{
    return builtins();
}

// 选中项的入口路径缓存下来：QML 绑定每次求值都会读它
void LyricsPluginStore::refreshSource()
{
    QVariantMap plugin;
    m_source = find(m_selectedId, &plugin) ? plugin.value(QStringLiteral("source")).toString() : QString();
}

void LyricsPluginStore::setSelectedId(const QString &id)
{
    if (id == m_selectedId)
        return;
    m_selectedId = id;
    refreshSource();
    QSettings().setValue(QStringLiteral("Plugins/lyrics"), id);
    emit selectedIdChanged();
}

void LyricsPluginStore::rescan()
{
    PluginStore::rescan();
    if (source().isEmpty())
        setSelectedId(QStringLiteral("builtin.default"));   // 选中的插件没了 ⇒ 回落到内置默认
}
