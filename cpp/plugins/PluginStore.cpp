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
#include <QSet>
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

QJsonObject readInfo(const QString &path)
{
    QFile file(path + QStringLiteral("/info.json"));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

QVariantMap readPlugin(const QString &path, const QString &fallbackType)
{
    const QJsonObject info = readInfo(path);
    if (info.isEmpty() || info.value(QStringLiteral("apiVersion")).toInt(1) != kApiVersion)
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
    const QDir base(from);
    while (it.hasNext()) {
        const QString src = it.next();
        const QString dst = to + QLatin1Char('/') + base.relativeFilePath(src);
        if (QFileInfo(src).isDir()) {
            if (!QDir().mkpath(dst))
                return false;
            continue;
        }
        if (!QDir().mkpath(QFileInfo(dst).absolutePath()))
            return false;
        if (QFile::exists(dst) && !QFile::remove(dst))
            return false;
        if (!QFile::copy(src, dst))
            return false;
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

QString PluginStore::pluginsRoot()
{
    return pluginDataRoot() + QStringLiteral("/plugins");
}

QString PluginStore::dir() const
{
    return QDir::toNativeSeparators(pluginsRoot());
}

QString PluginStore::sanitizeId(const QString &raw)
{
    QString id = raw.trimmed();
    id.remove(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")));
    while (id.startsWith(QLatin1Char('.')))
        id.remove(0, 1);
    if (id.size() > 64)
        id.truncate(64);
    return id;
}

void PluginStore::load()
{
    m_all.clear();

    const QString userRoot = pluginsRoot();
    const QString exeRoot = QCoreApplication::applicationDirPath() + QStringLiteral("/plugins");

    struct Root { QString path; QString type; };
    const Root roots[] = {
        {userRoot, QStringLiteral("function")},
        {userRoot + QStringLiteral("/lyrics"), QStringLiteral("lyrics")},
        {userRoot + QStringLiteral("/function"), QStringLiteral("function")},
        {exeRoot, QStringLiteral("function")},
        {exeRoot + QStringLiteral("/lyrics"), QStringLiteral("lyrics")},
        {exeRoot + QStringLiteral("/function"), QStringLiteral("function")},
    };

    QSet<QString> seen;
    for (const Root &root : roots) {
        const QFileInfoList dirs = QDir(root.path).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &entry : dirs) {
            const QString folder = entry.fileName();
            if (folder.startsWith(QLatin1Char('.')) || folder == QLatin1String("lyrics")
                || folder == QLatin1String("function"))
                continue;
            const QVariantMap plugin = readPlugin(entry.absoluteFilePath(), root.type);
            if (plugin.isEmpty())
                continue;
            const QString id = plugin.value(QStringLiteral("id")).toString();
            if (id.isEmpty() || seen.contains(id))
                continue;
            seen.insert(id);
            m_all << plugin;
        }
    }
}

void PluginStore::rebuild()
{
    m_lyrics = builtinLyrics();
    m_functions.clear();
    m_byId.clear();

    for (const QVariant &item : std::as_const(m_all)) {
        const QVariantMap plugin = item.toMap();
        const QString id = plugin.value(QStringLiteral("id")).toString();
        m_byId.insert(id, plugin);

        const QString type = plugin.value(QStringLiteral("type")).toString();
        if (type == QStringLiteral("lyrics"))
            m_lyrics << item;
        else if (type != QStringLiteral("source"))
            m_functions << item;
    }
    for (const QVariant &item : std::as_const(m_lyrics)) {
        const QVariantMap plugin = item.toMap();
        const QString id = plugin.value(QStringLiteral("id")).toString();
        if (!m_byId.contains(id))
            m_byId.insert(id, plugin);
    }

    if (!m_byId.contains(m_lyricsId))
        m_lyricsId = kDefaultLyrics;

    recomputeActive();
    refreshLyrics();
}

void PluginStore::recomputeActive()
{
    m_active.clear();
    m_enabled.clear();
    m_enabled.reserve(m_functions.size());
    for (const QVariant &item : std::as_const(m_functions)) {
        const QVariantMap plugin = item.toMap();
        const QString id = plugin.value(QStringLiteral("id")).toString();
        if (!AppSettings::value(QStringLiteral("Plugins/function"), id, true).toBool())
            continue;
        m_enabled << id;
        m_active << item;
    }
}

void PluginStore::refreshLyrics()
{
    m_lyricsSource = m_byId.value(m_lyricsId).value(QStringLiteral("source")).toString();
}

bool PluginStore::find(const QString &id, QVariantMap *info) const
{
    const auto it = m_byId.constFind(id);
    if (it == m_byId.constEnd())
        return false;
    if (info)
        *info = it.value();
    return true;
}

bool PluginStore::installed(const QString &id) const
{
    const auto it = m_byId.constFind(id);
    return it != m_byId.constEnd() && !it.value().value(QStringLiteral("builtin")).toBool();
}

QString PluginStore::versionOf(const QString &id) const
{
    return m_byId.value(id).value(QStringLiteral("version")).toString();
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
    if (id.isEmpty() || !m_byId.contains(id) || m_enabled.contains(id) == on)
        return;
    AppSettings::setValue(QStringLiteral("Plugins/function"), id, on);
    AppSettings::sync();
    recomputeActive();
    emit enabledChanged();
}

void PluginStore::selectLyrics(const QString &id)
{
    const QVariantMap plugin = m_byId.value(id);
    if (id == m_lyricsId || plugin.value(QStringLiteral("type")).toString() != QStringLiteral("lyrics"))
        return;
    m_lyricsId = id;
    refreshLyrics();
    AppSettings::setValue(QStringLiteral("Plugins"), QStringLiteral("lyrics"), id);
    AppSettings::sync();
    emit lyricsChanged();
}

QString PluginStore::installInto(const QString &folder, const QString &id)
{
    const QString root = pluginsRoot();
    if (!QDir().mkpath(root))
        return QStringLiteral("无法创建插件目录");
    const QString to = root + QLatin1Char('/') + id;
    if (QFileInfo::exists(to) && !QDir(to).removeRecursively())
        return QStringLiteral("无法覆盖同名插件，请先删除");
    if (!QDir().rename(folder, to))
        return QStringLiteral("无法写入插件目录");
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

    const QString root = pluginsRoot();
    if (!QDir().mkpath(root))
        return QStringLiteral("无法创建插件目录");

    QTemporaryDir temp(root + QStringLiteral("/.unpack-XXXXXX"));
    if (!temp.isValid())
        return QStringLiteral("无法创建临时目录");

    const QString raw = temp.filePath(QStringLiteral("pkg"));
    QDir().mkpath(raw);
    QString error;
    if (!ZipReader::extract(archive, raw, &error))
        return QStringLiteral("解压失败：%1").arg(error);

    QString source = raw;
    if (!QFileInfo::exists(raw + QStringLiteral("/info.json"))) {
        source.clear();
        const QFileInfoList dirs = QDir(raw).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &dir : dirs) {
            if (QFileInfo::exists(dir.absoluteFilePath() + QStringLiteral("/info.json"))) {
                source = dir.absoluteFilePath();
                break;
            }
        }
    }
    if (source.isEmpty() || !QFileInfo::exists(source + QStringLiteral("/info.json")))
        return QStringLiteral("插件包里没有 info.json");

    QString id = sanitizeId(idHint);
    if (id.isEmpty() && source != raw)
        id = sanitizeId(QFileInfo(source).fileName());
    if (id.isEmpty())
        id = sanitizeId(readInfo(source).value(QStringLiteral("id")).toString());
    if (id.isEmpty())
        return QStringLiteral("插件 id 无效");

    const QString stage = root + QStringLiteral("/.install-") + id;
    QDir(stage).removeRecursively();
    if (!copyFolder(source, stage)) {
        QDir(stage).removeRecursively();
        return QStringLiteral("复制插件文件失败");
    }

    const QString result = installInto(stage, id);
    if (!result.isEmpty()) {
        QDir(stage).removeRecursively();
        return result;
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
        return QStringLiteral("内置插件不能删除");

    const QString path = plugin.value(QStringLiteral("path")).toString();
    if (path.isEmpty() || !QDir(path).removeRecursively())
        return QStringLiteral("删除插件目录失败");

    if (m_lyricsId == id) {
        m_lyricsId = kDefaultLyrics;
        AppSettings::setValue(QStringLiteral("Plugins"), QStringLiteral("lyrics"), m_lyricsId);
    }
    AppSettings::remove(QStringLiteral("Plugins/function"), id);
    AppSettings::sync();
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
    const QString root = pluginsRoot();
    QDir().mkpath(root);
    QDesktopServices::openUrl(QUrl::fromLocalFile(root));
}
