// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#include "plugins/PluginStore.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUrl>

namespace {

// 读取单个插件目录；缺 info.json / 入口不存在 / apiVersion 不匹配都跳过，避免装坏插件后崩启动
QVariantMap readPlugin(const QString &path, int apiVersion)
{
    QFile file(path + QStringLiteral("/info.json"));
    if (!file.open(QIODevice::ReadOnly))
        return {};

    const QJsonObject info = QJsonDocument::fromJson(file.readAll()).object();
    if (info.value(QStringLiteral("apiVersion")).toInt(1) != apiVersion)
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

PluginStore::PluginStore(QString kind, QString folder, QObject *parent)
    : QObject(parent)
    , m_kind(std::move(kind))
    , m_folder(std::move(folder))
{
}

QString PluginStore::writableRoot() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/plugins/") + m_folder;
}

QString PluginStore::dir() const
{
    return QDir::toNativeSeparators(writableRoot());
}

void PluginStore::load()
{
    QVariantList list = builtinPlugins();
    QStringList seen;
    seen.reserve(list.size());
    for (const QVariant &item : std::as_const(list))
        seen << item.toMap().value(QStringLiteral("id")).toString();

    // 用户目录优先；可执行文件旁的 plugins/<folder> 便于开发期直接放插件调试
    const QStringList roots{
        writableRoot(),
        QCoreApplication::applicationDirPath() + QStringLiteral("/plugins/") + m_folder
    };
    for (const QString &root : roots) {
        const QFileInfoList dirs = QDir(root).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &entry : dirs) {
            const QVariantMap plugin = readPlugin(entry.absoluteFilePath(), apiVersion());
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
}

// 插件数很少（十几个量级），线性查找比额外维护 id → 下标映射更省
bool PluginStore::find(const QString &id, QVariantMap *info) const
{
    for (const QVariant &item : m_plugins) {
        const QVariantMap plugin = item.toMap();
        if (plugin.value(QStringLiteral("id")).toString() != id)
            continue;
        if (info)
            *info = plugin;
        return true;
    }
    return false;
}

void PluginStore::rescan()
{
    load();
    emit pluginsChanged();
}

QString PluginStore::install(const QString &folder)
{
    const QString from = QUrl::fromUserInput(folder).toLocalFile();
    if (!QFileInfo(from).isDir())
        return QStringLiteral("请选择插件文件夹");
    if (!QFileInfo::exists(from + QStringLiteral("/info.json")))
        return QStringLiteral("文件夹里没有 info.json，不像是") + m_kind;

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

QString PluginStore::remove(const QString &id)
{
    QVariantMap plugin;
    if (!find(id, &plugin))
        return QStringLiteral("插件不存在");
    if (plugin.value(QStringLiteral("builtin")).toBool())
        return QStringLiteral("内置") + m_kind + QStringLiteral("不能删除");
    if (!QDir(plugin.value(QStringLiteral("path")).toString()).removeRecursively())
        return QStringLiteral("删除插件目录失败");
    rescan();
    return {};
}

void PluginStore::reveal(const QString &id)
{
    QVariantMap plugin;
    if (!id.isEmpty() && find(id, &plugin)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(plugin.value(QStringLiteral("path")).toString()));
        return;
    }
    const QString root = writableRoot();
    QDir().mkpath(root);
    QDesktopServices::openUrl(QUrl::fromLocalFile(root));
}
