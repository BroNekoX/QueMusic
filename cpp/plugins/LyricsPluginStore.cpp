// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#include "plugins/LyricsPluginStore.h"

#include <QSettings>

namespace {

// 内置歌词界面（不可卸载）
QVariantList builtins()
{
    const auto make = [](const QString &id, const QString &name, const QString &source,
                         const QString &description = QStringLiteral("应用内置歌词界面")) {
        return QVariantMap{
            {QStringLiteral("id"), QStringLiteral("builtin.") + id},
            {QStringLiteral("name"), name},
            {QStringLiteral("author"), QStringLiteral("QueMusic")},
            {QStringLiteral("description"), description},
            {QStringLiteral("source"), source},
            {QStringLiteral("builtin"), true},
            {QStringLiteral("version"), QStringLiteral("")},
            {QStringLiteral("preview"), QStringLiteral("")}
        };
    };
    return {
        make(QStringLiteral("default"), QStringLiteral("默认"), QStringLiteral("qrc:/QueMusic/layout/MainLyric.qml")),
        make(QStringLiteral("free"), QStringLiteral("自由"), QStringLiteral("qrc:/QueMusic/lyricsui/LyricsFree.qml")),
        make(QStringLiteral("3d(Beta)"), QStringLiteral("3D(Beta)"), QStringLiteral("qrc:/QueMusic/lyricsui/Lyrics3D.qml")),
        // JIZURA 自动编排：源码在 plugins/lyrics/jizura/，同时被 CMake 收进本模块的
        // QML_FILES，由 qmlcachegen 预编译成字节码，因此加载是瞬时的
        // （同一份文件也可以作为外置插件安装，见该目录 README）
        make(QStringLiteral("jizura"), QStringLiteral("JIZURA 文字PV"),
             QStringLiteral("qrc:/QueMusic/plugins/lyrics/jizura/jizura.qml"),
             QStringLiteral("按歌词自动编排的矢量 PV")) };
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
