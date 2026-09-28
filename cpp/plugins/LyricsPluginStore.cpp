// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#include "plugins/LyricsPluginStore.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

namespace {

// 内置歌词界面（不可卸载）
QVariantList builtinPlugins()
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

// 读取单个插件目录；缺 info.json / 入口不存在 / apiVersion 不匹配都跳过，避免装坏插件后崩启动
QVariantMap readPlugin(const QString &path)
{
    QFile file(path + QStringLiteral("/info.json"));
    if (!file.open(QIODevice::ReadOnly))
        return {};

    const QJsonObject info = QJsonDocument::fromJson(file.readAll()).object();
    if (info.value(QStringLiteral("apiVersion")).toInt(1) != 1)
        return {};

    const QString entryPath = QDir(path).absoluteFilePath(
        info.value(QStringLiteral("entry")).toString(QStringLiteral("plugin.qml")));
    if (!QFileInfo::exists(entryPath))
        return {};

    const QString previewPath = QDir(path).absoluteFilePath(
        info.value(QStringLiteral("preview")).toString(QStringLiteral("info.png")));

    QVariantMap plugin;
    plugin.insert(QStringLiteral("id"), QFileInfo(path).fileName());
    plugin.insert(QStringLiteral("name"), info.value(QStringLiteral("name")).toString(QFileInfo(path).fileName()));
    plugin.insert(QStringLiteral("author"), info.value(QStringLiteral("author")).toString());
    plugin.insert(QStringLiteral("version"), info.value(QStringLiteral("version")).toString());
    plugin.insert(QStringLiteral("description"), info.value(QStringLiteral("description")).toString());
    plugin.insert(QStringLiteral("source"), QUrl::fromLocalFile(entryPath).toString());
    plugin.insert(QStringLiteral("preview"),
                  QFileInfo::exists(previewPath) ? QUrl::fromLocalFile(previewPath).toString() : QString());
    plugin.insert(QStringLiteral("path"), path);
    plugin.insert(QStringLiteral("builtin"), false);
    return plugin;
}

bool copyFolder(const QString &from, const QString &to)
{
    if (!QDir().mkpath(to))
        return false;
    QDirIterator it(from, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString src = it.next();
        const QString rel = QDir(from).relativeFilePath(src);
        const QString dst = to + QLatin1Char('/') + rel;
        if (QFileInfo(src).isDir()) {
            if (!QDir().mkpath(dst))
                return false;
        } else {
            QDir().mkpath(QFileInfo(dst).absolutePath());
            if (QFile::exists(dst))
                QFile::remove(dst);
            if (!QFile::copy(src, dst))
                return false;
        }
    }
    return true;
}

} // namespace

LyricsPluginStore::LyricsPluginStore(QObject *parent)
    : QObject(parent)
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

QString LyricsPluginStore::writableRoot() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/plugins/lyrics");
}

QString LyricsPluginStore::dir() const
{
    return QDir::toNativeSeparators(writableRoot());
}

void LyricsPluginStore::load()
{
    QVariantList list = builtinPlugins();
    QStringList seen;
    for (const QVariant &item : std::as_const(list))
        seen << item.toMap().value(QStringLiteral("id")).toString();

    // 用户目录优先；可执行文件旁的 plugins/lyrics 便于开发期直接放插件调试
    const QStringList roots{
        writableRoot(),
        QCoreApplication::applicationDirPath() + QStringLiteral("/plugins/lyrics")
    };
    for (const QString &root : roots) {
        const QFileInfoList dirs = QDir(root).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &entry : dirs) {
            const QVariantMap plugin = readPlugin(entry.absoluteFilePath());
            if (plugin.isEmpty())
                continue;
            const QString id = plugin.value(QStringLiteral("id")).toString();
            if (seen.contains(id))
                continue;
            seen << id;
            list.append(plugin);
        }
    }
    m_plugins = list;
    refreshSource();
}

// 选中项的入口路径缓存下来：QML 绑定每次求值都会读它
void LyricsPluginStore::refreshSource()
{
    m_source.clear();
    for (int i = 0; i < m_plugins.size(); ++i) {
        const QVariantMap plugin = m_plugins.at(i).toMap();
        if (plugin.value(QStringLiteral("id")).toString() == m_selectedId) {
            m_source = plugin.value(QStringLiteral("source")).toString();
            return;
        }
    }
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
    load();
    emit pluginsChanged();
    if (source().isEmpty())
        setSelectedId(QStringLiteral("builtin.default"));
}

QString LyricsPluginStore::install(const QString &folder)
{
    const QString from = QUrl::fromUserInput(folder).toLocalFile();
    if (!QFileInfo(from).isDir())
        return QStringLiteral("请选择插件文件夹");
    if (!QFileInfo::exists(from + QStringLiteral("/info.json")))
        return QStringLiteral("文件夹里没有 info.json，不像是歌词界面插件");

    const QString to = writableRoot() + QLatin1Char('/') + QFileInfo(from).fileName();
    if (QDir::cleanPath(from) != QDir::cleanPath(to)) {
        if (to.startsWith(QDir::cleanPath(from) + QLatin1Char('/')))
            return QStringLiteral("不能把插件装到它自己的子目录里");
        if (QFileInfo::exists(to) && !QDir(to).removeRecursively())
            return QStringLiteral("无法覆盖同名插件，请先删除");
        if (!copyFolder(from, to))
            return QStringLiteral("复制插件文件失败");
    }
    rescan();
    return {};
}

QString LyricsPluginStore::remove(const QString &id)
{
    QString path;
    bool builtin = false;
    for (int i = 0; i < m_plugins.size(); ++i) {
        const QVariantMap plugin = m_plugins.at(i).toMap();
        if (plugin.value(QStringLiteral("id")).toString() != id)
            continue;
        path = plugin.value(QStringLiteral("path")).toString();
        builtin = plugin.value(QStringLiteral("builtin")).toBool();
        break;
    }
    if (path.isNull() && !builtin)
        return QStringLiteral("插件不存在");
    if (builtin)
        return QStringLiteral("内置界面不能删除");
    if (!QDir(path).removeRecursively())
        return QStringLiteral("删除插件目录失败");
    if (m_selectedId == id)
        setSelectedId(QStringLiteral("builtin.default"));
    rescan();
    return {};
}

void LyricsPluginStore::reveal(const QString &id)
{
    for (int i = 0; i < m_plugins.size(); ++i) {
        const QVariantMap plugin = m_plugins.at(i).toMap();
        if (plugin.value(QStringLiteral("id")).toString() == id) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(plugin.value(QStringLiteral("path")).toString()));
            return;
        }
    }
    const QString root = writableRoot();
    QDir().mkpath(root);
    QDesktopServices::openUrl(QUrl::fromLocalFile(root));
}
