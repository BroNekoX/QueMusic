// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#include "plugins/PluginStore.h"

#include "plugins/ZipReader.h"

#include "AppSettings.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>

#include <utility>

namespace {

constexpr int kApiVersion = 1;
const QString kDefaultLyrics = QStringLiteral("builtin.default");

QString pluginDataRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QVariantMap makeBuiltin(const QString &id, const QString &name, const QString &source)
{
    return {
        {QStringLiteral("id"), QStringLiteral("builtin.") + id},
        {QStringLiteral("name"), name},
        {QStringLiteral("author"), QStringLiteral("QueMusic")},
        {QStringLiteral("version"), QString()},
        {QStringLiteral("description"), QStringLiteral("内置歌词界面")},
        {QStringLiteral("type"), QStringLiteral("lyrics")},
        {QStringLiteral("source"), source},
        {QStringLiteral("preview"), QString()},
        {QStringLiteral("path"), QString()},
        {QStringLiteral("builtin"), true},
    };
}

QVariantList builtinLyrics()
{
    return {
        makeBuiltin(QStringLiteral("default"), QStringLiteral("默认"), QStringLiteral("qrc:/QueMusic/layout/MainLyric.qml")),
        makeBuiltin(QStringLiteral("free"), QStringLiteral("自由"), QStringLiteral("qrc:/QueMusic/lyricsui/LyricsFree.qml")),
        makeBuiltin(QStringLiteral("3d(Beta)"), QStringLiteral("3D(Beta)"), QStringLiteral("qrc:/QueMusic/lyricsui/Lyrics3D.qml")),
    };
}

QVariantMap readPlugin(const QString &path, const QString &fallbackType)
{
    QFile file(path + QStringLiteral("/info.json"));
    if (!file.open(QIODevice::ReadOnly))
        return {};

    const QJsonObject info = QJsonDocument::fromJson(file.readAll()).object();
    if (info.value(QStringLiteral("apiVersion")).toInt(1) != kApiVersion)
        return {};

    const QString entry = QDir(path).absoluteFilePath(
        info.value(QStringLiteral("entry")).toString(QStringLiteral("plugin.qml")));
    if (!QFileInfo::exists(entry))
        return {};

    QString type = info.value(QStringLiteral("type")).toString().toLower();
    if (type == QStringLiteral("music") || type == QStringLiteral("sources"))
        type = QStringLiteral("source");
    if (type != QStringLiteral("lyrics") && type != QStringLiteral("function")
        && type != QStringLiteral("source"))
        type = fallbackType;

    const QString preview = QDir(path).absoluteFilePath(
        info.value(QStringLiteral("preview")).toString(QStringLiteral("info.png")));
    const QString id = QFileInfo(path).fileName();

    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), info.value(QStringLiteral("name")).toString(id)},
        {QStringLiteral("author"), info.value(QStringLiteral("author")).toString()},
        {QStringLiteral("version"), info.value(QStringLiteral("version")).toString()},
        {QStringLiteral("description"), info.value(QStringLiteral("description")).toString()},
        {QStringLiteral("type"), type},
        {QStringLiteral("source"), QUrl::fromLocalFile(entry).toString()},
        {QStringLiteral("preview"), QFileInfo::exists(preview) ? QUrl::fromLocalFile(preview).toString() : QString()},
        {QStringLiteral("path"), path},
        {QStringLiteral("builtin"), false},
    };
}

bool copyFolder(const QString &from, const QString &to)
{
    if (!QDir().mkpath(to))
        return false;
    QDirIterator it(from, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString src = it.next();
        const QString dst = to + QLatin1Char('/') + QDir(from).relativeFilePath(src);
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

PluginStore::PluginStore(QObject *parent)
    : QObject(parent)
{
    load();
    m_lyricsId = AppSettings::value(QStringLiteral("Plugins"), QStringLiteral("lyrics"), kDefaultLyrics).toString();
    rebuild();
}

PluginStore *PluginStore::create(QQmlEngine *, QJSEngine *)
{
    return instance();
}

PluginStore *PluginStore::instance()
{
    static PluginStore *const store = new PluginStore();
    return store;
}

QString PluginStore::dir() const
{
    return QDir::toNativeSeparators(
        pluginDataRoot() + QStringLiteral("/plugins"));
}

void PluginStore::load()
{
    m_all.clear();

    const QString userRoot = pluginDataRoot()
                             + QStringLiteral("/plugins");
    const QString exeRoot = QCoreApplication::applicationDirPath() + QStringLiteral("/plugins");

    // 兼容旧目录：plugins/<id>、plugins/lyrics/<id>、plugins/function/<id>
    struct Root { QString path; QString type; };
    const Root roots[] = {
        {userRoot, QStringLiteral("function")},
        {userRoot + QStringLiteral("/lyrics"), QStringLiteral("lyrics")},
        {userRoot + QStringLiteral("/function"), QStringLiteral("function")},
        {exeRoot, QStringLiteral("function")},
        {exeRoot + QStringLiteral("/lyrics"), QStringLiteral("lyrics")},
        {exeRoot + QStringLiteral("/function"), QStringLiteral("function")},
    };

    QStringList seen;
    for (const Root &root : roots) {
        const QFileInfoList dirs = QDir(root.path).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &entry : dirs) {
            const QString folder = entry.fileName();
            if (folder == QStringLiteral("lyrics") || folder == QStringLiteral("function"))
                continue;
            const QVariantMap plugin = readPlugin(entry.absoluteFilePath(), root.type);
            if (plugin.isEmpty())
                continue;
            const QString id = plugin.value(QStringLiteral("id")).toString();
            if (seen.contains(id))
                continue;
            seen << id;
            m_all << plugin;
        }
    }
}

void PluginStore::rebuild()
{
    m_functions.clear();
    m_lyrics = builtinLyrics();

    for (const QVariant &item : std::as_const(m_all)) {
        const QString type = item.toMap().value(QStringLiteral("type")).toString();
        if (type == QStringLiteral("lyrics"))
            m_lyrics << item;
        else if (type != QStringLiteral("source"))
            m_functions << item;
    }

    m_enabled.clear();
    m_active.clear();
    for (const QVariant &item : std::as_const(m_functions)) {
        const QString id = item.toMap().value(QStringLiteral("id")).toString();
        if (!AppSettings::value(QStringLiteral("Plugins/function"), id, true).toBool())
            continue;
        m_enabled << id;
        m_active << item;
    }

    if (!find(m_lyricsId, nullptr))
        m_lyricsId = kDefaultLyrics;
    refreshLyrics();
}

void PluginStore::refreshLyrics()
{
    QVariantMap plugin;
    m_lyricsSource = find(m_lyricsId, &plugin) ? plugin.value(QStringLiteral("source")).toString()
                                               : QString();
}

bool PluginStore::find(const QString &id, QVariantMap *info) const
{
    for (const QVariantList *list : {&m_lyrics, &m_functions}) {
        for (const QVariant &item : *list) {
            const QVariantMap plugin = item.toMap();
            if (plugin.value(QStringLiteral("id")).toString() != id)
                continue;
            if (info)
                *info = plugin;
            return true;
        }
    }
    return false;
}

void PluginStore::rescan()
{
    load();
    rebuild();
    emit changed();
    emit enabledChanged();
    emit lyricsChanged();
}

void PluginStore::setEnabled(const QString &id, bool on)
{
    if (id.isEmpty() || m_enabled.contains(id) == on || !find(id, nullptr))
        return;
    AppSettings::setValue(QStringLiteral("Plugins/function"), id, on);
    AppSettings::sync();
    rebuild();
    emit enabledChanged();
}

void PluginStore::selectLyrics(const QString &id)
{
    if (id == m_lyricsId || !find(id, nullptr))
        return;
    m_lyricsId = id;
    refreshLyrics();
    AppSettings::setValue(QStringLiteral("Plugins"), QStringLiteral("lyrics"), id);
    AppSettings::sync();
    emit lyricsChanged();
}

QString PluginStore::install(const QString &folder)
{
    const QString from = QUrl::fromUserInput(folder).toLocalFile();
    if (!QFileInfo(from).isDir())
        return QStringLiteral("请选择插件文件夹");
    if (!QFileInfo::exists(from + QStringLiteral("/info.json")))
        return QStringLiteral("文件夹里没有 info.json，不是插件");

    const QString to = pluginDataRoot()
                       + QStringLiteral("/plugins/") + QFileInfo(from).fileName();
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

QString PluginStore::installZip(const QString &path)
{
    QString local = path;
    const QUrl url(path);
    if (url.isLocalFile())
        local = url.toLocalFile();
    QFile file(local);
    if (!file.open(QIODevice::ReadOnly))
        return QStringLiteral("无法打开插件包");
    return installArchive(file.readAll(), QFileInfo(local).completeBaseName());
}

QString PluginStore::installArchive(const QByteArray &archive, const QString &idHint)
{
    if (archive.isEmpty())
        return QStringLiteral("插件包为空");

    const QString scratchRoot = pluginDataRoot() + QStringLiteral("/cache/import");
    QDir().mkpath(scratchRoot);
    QTemporaryDir temp(scratchRoot + QStringLiteral("/run-XXXXXX"));
    if (!temp.isValid())
        return QStringLiteral("无法创建临时目录");

    const QString raw = temp.filePath(QStringLiteral("raw"));
    QDir().mkpath(raw);
    QString error;
    if (!ZipReader::extract(archive, raw, &error))
        return QStringLiteral("解压失败：%1").arg(error);

    QString source = raw;
    if (!QFileInfo::exists(raw + QStringLiteral("/info.json"))) {
        source.clear();
        const QStringList dirs = QDir(raw).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &dir : dirs) {
            if (QFileInfo::exists(raw + QLatin1Char('/') + dir + QStringLiteral("/info.json"))) {
                source = raw + QLatin1Char('/') + dir;
                break;
            }
        }
    }
    if (source.isEmpty() || !QFileInfo::exists(source + QStringLiteral("/info.json")))
        return QStringLiteral("插件包里没有 info.json");

    QString id = idHint;
    id.remove(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")));
    if (id.isEmpty() && source != raw)
        id = QFileInfo(source).fileName();
    if (id.isEmpty())
        return QStringLiteral("插件 id 无效");

    const QString staged = temp.filePath(QStringLiteral("pkg/") + id);
    QDir().mkpath(staged);
    const QDir from(source);
    const QStringList entries = from.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : entries) {
        const QString src = from.filePath(entry);
        const QString dst = staged + QLatin1Char('/') + entry;
        if (!QDir().rename(src, dst) && !QFile::rename(src, dst))
            return QStringLiteral("移动插件文件失败：%1").arg(entry);
    }
    return install(staged);
}

QString PluginStore::remove(const QString &id)
{
    QVariantMap plugin;
    if (!find(id, &plugin))
        return QStringLiteral("插件不存在");
    if (plugin.value(QStringLiteral("builtin")).toBool())
        return QStringLiteral("内置插件不能删除");
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
    const QString root = pluginDataRoot()
                         + QStringLiteral("/plugins");
    QDir().mkpath(root);
    QDesktopServices::openUrl(QUrl::fromLocalFile(root));
}