// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "MusicApiService.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QtConcurrent>

#include <algorithm>

#include "../cpp/AccountManager.h"
#include "../cpp/LocalLyricsReader.h"

#include "ApiCommon.h"

#include <attachedpictureframe.h>
#include <audioproperties.h>
#include <fileref.h>
#include <flacfile.h>
#include <flacpicture.h>
#include <id3v2tag.h>
#include <mp4coverart.h>
#include <mp4file.h>
#include <mp4tag.h>
#include <mpegfile.h>
#include <tag.h>
#include <tbytevector.h>
#include <tfile.h>
#include <tpropertymap.h>
#include <tstring.h>

namespace {
constexpr int kSourceKugou = 0;
constexpr int kSourceNetease = 1;
constexpr int kSourceBilibili = 2;
constexpr int kRequestWatchdogMs = 20000;
constexpr int kLyricTickMs = 320;

QString firstNonEmpty(const QVariantMap &map, std::initializer_list<const char *> keys)
{
    for (const char *key : keys) {
        const QVariant value = map.value(QLatin1String(key));
        if (value.isValid() && !value.toString().isEmpty())
            return value.toString();
    }
    return QString();
}

QString tagString(const TagLib::String &value)
{
    return QString::fromUtf8(value.toCString(true));
}

QString localPath(const QString &filePath)
{
    const QString path = QUrl::fromUserInput(filePath).toLocalFile();
    return path.isEmpty() ? filePath : path;
}

TagLib::ByteVector toByteVector(const QByteArray &data)
{
    return TagLib::ByteVector(data.constData(), size_t(data.size()));
}

TagLib::String toTagString(const QString &text)
{
    return TagLib::String(text.toUtf8().constData(), TagLib::String::UTF8);
}

QString imageMime(const QByteArray &data)
{
    if (data.startsWith(QByteArrayLiteral("\x89PNG")))
        return QStringLiteral("image/png");
    if (data.startsWith(QByteArrayLiteral("GIF8")))
        return QStringLiteral("image/gif");
    if (data.startsWith(QByteArrayLiteral("BM")))
        return QStringLiteral("image/bmp");
    return QStringLiteral("image/jpeg");
}

bool embedCover(TagLib::File *file, const QByteArray &data)
{
    const QString mime = imageMime(data);
    if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(file)) {
        TagLib::ID3v2::Tag *tag = mpeg->ID3v2Tag(true);
        if (tag == nullptr)
            return false;
        tag->removeFrames("APIC");
        auto *frame = new TagLib::ID3v2::AttachedPictureFrame;
        frame->setMimeType(toTagString(mime));
        frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
        frame->setPicture(toByteVector(data));
        tag->addFrame(frame);
        return true;
    }
    if (auto *flac = dynamic_cast<TagLib::FLAC::File *>(file)) {
        flac->removePictures();
        auto *picture = new TagLib::FLAC::Picture;
        picture->setMimeType(toTagString(mime));
        picture->setType(TagLib::FLAC::Picture::FrontCover);
        picture->setData(toByteVector(data));
        flac->addPicture(picture);
        return true;
    }
    if (auto *mp4 = dynamic_cast<TagLib::MP4::File *>(file)) {
        TagLib::MP4::Tag *tag = mp4->tag();
        if (tag == nullptr)
            return false;
        auto format = TagLib::MP4::CoverArt::JPEG;
        if (mime == QLatin1String("image/png"))
            format = TagLib::MP4::CoverArt::PNG;
        else if (mime == QLatin1String("image/gif"))
            format = TagLib::MP4::CoverArt::GIF;
        else if (mime == QLatin1String("image/bmp"))
            format = TagLib::MP4::CoverArt::BMP;
        TagLib::MP4::CoverArtList covers;
        covers.append(TagLib::MP4::CoverArt(format, toByteVector(data)));
        tag->setItem("covr", covers);
        return true;
    }
    return false;
}
}

AccountManager *MusicApiService::s_accountManager = nullptr;

MusicApiService *MusicApiService::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    auto *service = new MusicApiService(qmlEngine);
    service->setAccountManager(s_accountManager);
    return service;
}

void MusicApiService::setSharedAccountManager(AccountManager *am)
{
    s_accountManager = am;
}

MusicApiService::MusicApiService(QObject *parent)
    : QObject(parent)
{
    connect(&m_netease, &NeteaseCloudApi::resultReady, this, &MusicApiService::handleResult);
    connect(&m_kugou, &KugouApi::resultReady, this, &MusicApiService::handleResult);
    connect(&m_bilibili, &BilibiliApi::resultReady, this, &MusicApiService::handleResult);

    const QString sourcesDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                               + QStringLiteral("/BroNekoX/QueMusic/sources");
    QDir().mkpath(sourcesDir);
    m_scripts.setPluginDir(sourcesDir);
    connect(&m_scripts, &ScriptSourceHost::resultReady, this, &MusicApiService::handleResult);
    connect(&m_scripts, &ScriptSourceHost::pluginsChanged, this, &MusicApiService::rebuildSources);
    rebuildSources();
    m_scripts.reload();

    QSettings opt(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                      + QStringLiteral("/BroNekoX/QueMusic.ini"),
                  QSettings::IniFormat);
    m_soundQuality = opt.value(QStringLiteral("Options/soundQuality"), 1).toInt();

    m_altsSaveTimer.setSingleShot(true);
    connect(&m_altsSaveTimer, &QTimer::timeout, this, &MusicApiService::saveQualityCache);
    loadQualityCache();

    m_requestWatchdog.setSingleShot(true);
    m_requestWatchdog.setInterval(kRequestWatchdogMs);
    connect(&m_requestWatchdog, &QTimer::timeout, this, [this] {
        if (m_pendingRequests == 0)
            return;
        qWarning() << "[api]" << m_pendingRequests
                   << "个请求超过" << kRequestWatchdogMs / 1000 << "秒没有回结果，强制结束加载态";
        m_pendingRequests = 0;
        setLoadState(false);
    });

    m_lyricTimer.setInterval(kLyricTickMs);
    m_lyricTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_lyricTimer, &QTimer::timeout, this, &MusicApiService::advanceLyricIndex);
}

MusicApiService::~MusicApiService()
{
    if (m_altsDirty)
        saveQualityCache();
}

void MusicApiService::setAccountManager(AccountManager *am)
{
    m_account = am;
}

int MusicApiService::songSource() const
{
    return m_source;
}

void MusicApiService::setSongSource(int source)
{
    if (m_source == source)
        return;
    m_source = source;
    emit songSourceChanged();
}

int MusicApiService::resolve(int source) const
{
    return source < 0 ? m_source : source;
}

int MusicApiService::soundQuality() const
{
    return m_soundQuality;
}

void MusicApiService::setSoundQuality(int q)
{
    if (m_soundQuality == q)
        return;
    m_soundQuality = q;
    emit soundQualityChanged();
}

QVariantList MusicApiService::sources() const
{
    return m_sourceList;
}

void MusicApiService::rebuildSources()
{
    QVariantList list{
        QVariantMap{{QStringLiteral("id"), kSourceKugou},
                    {QStringLiteral("name"), QStringLiteral("酷狗音乐")},
                    {QStringLiteral("plugin"), false}},
        QVariantMap{{QStringLiteral("id"), kSourceNetease},
                    {QStringLiteral("name"), QStringLiteral("网易云音乐")},
                    {QStringLiteral("plugin"), false}},
        QVariantMap{{QStringLiteral("id"), kSourceBilibili},
                    {QStringLiteral("name"), QStringLiteral("哔哩哔哩")},
                    {QStringLiteral("plugin"), false}}
    };
    list += m_scripts.plugins();
    m_sourceList = list;
    emit sourcesChanged();
}

QStringList MusicApiService::sourceNames() const
{
    QStringList names;
    names.reserve(m_sourceList.size());
    for (const QVariant &entry : m_sourceList)
        names.append(entry.toMap().value(QStringLiteral("name")).toString());
    return names;
}

QVariantList MusicApiService::sourceIds() const
{
    QVariantList ids;
    ids.reserve(m_sourceList.size());
    for (const QVariant &entry : m_sourceList)
        ids.append(entry.toMap().value(QStringLiteral("id")).toInt());
    return ids;
}

int MusicApiService::sourceIdAt(int index) const
{
    if (index < 0 || index >= m_sourceList.size())
        return m_source;
    return m_sourceList.at(index).toMap().value(QStringLiteral("id")).toInt();
}

void MusicApiService::reloadSources()
{
    m_scripts.reload();
}

void MusicApiService::rememberHashes(const QVariantList &items)
{
    if (m_alts.size() > 5000)
        return;
    bool changed = false;
    for (const QVariant &value : items) {
        const QVariantMap item = value.toMap();
        const QString base = item.value(QStringLiteral("hash")).toString();
        if (base.isEmpty())
            continue;
        const QString hq = item.value(QStringLiteral("hashhq")).toString();
        const QString sq = item.value(QStringLiteral("hashsq")).toString();
        QualityAlts alts = m_alts.value(base);
        if (!hq.isEmpty() && hq != base && alts.hq != hq) {
            alts.hq = hq;
            m_baseOf.insert(hq, base);
            changed = true;
        }
        if (!sq.isEmpty() && sq != base && alts.sq != sq) {
            alts.sq = sq;
            m_baseOf.insert(sq, base);
            changed = true;
        }
        if (!alts.hq.isEmpty() || !alts.sq.isEmpty())
            m_alts.insert(base, alts);
    }
    if (changed)
        scheduleSaveQualityCache();
}

QString MusicApiService::qualityCachePath() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                        + QStringLiteral("/BroNekoX");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/QueMusicQuality.json");
}

void MusicApiService::loadQualityCache()
{
    QFile f(qualityCachePath());
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    const auto load = [this](const QJsonObject &obj, bool hq) {
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            const QString base = it.key();
            const QString alt = it.value().toString();
            if (base.isEmpty() || alt.isEmpty() || alt == base)
                continue;
            QualityAlts alts = m_alts.value(base);
            (hq ? alts.hq : alts.sq) = alt;
            m_alts.insert(base, alts);
            m_baseOf.insert(alt, base);
        }
    };
    load(root.value(QStringLiteral("hq")).toObject(), true);
    load(root.value(QStringLiteral("sq")).toObject(), false);
}

void MusicApiService::scheduleSaveQualityCache()
{
    m_altsDirty = true;
    m_altsSaveTimer.start(1500);
}

void MusicApiService::saveQualityCache()
{
    QJsonObject hq;
    QJsonObject sq;
    for (auto it = m_alts.constBegin(); it != m_alts.constEnd(); ++it) {
        if (!it.value().hq.isEmpty())
            hq.insert(it.key(), it.value().hq);
        if (!it.value().sq.isEmpty())
            sq.insert(it.key(), it.value().sq);
    }
    QJsonObject root;
    root.insert(QStringLiteral("hq"), hq);
    root.insert(QStringLiteral("sq"), sq);

    QSaveFile f(qualityCachePath());
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    if (f.commit())
        m_altsDirty = false;
}

QString MusicApiService::resolveQualityHash(const QString &hash) const
{
    const QString base = m_baseOf.value(hash, hash);
    const QualityAlts alts = m_alts.value(base);
    if (m_soundQuality >= 2 && !alts.sq.isEmpty())
        return alts.sq;
    if (m_soundQuality == 1 && !alts.hq.isEmpty())
        return alts.hq;
    return base;
}

void MusicApiService::syncSource(int source)
{
    if (source == kSourceBilibili) {
        m_bilibili.setQuality(m_soundQuality);
        return;
    }
    if (source == kSourceNetease) {
        if (m_account)
            m_netease.setCookie(m_account->neteaseApiCookie());
        m_netease.setQuality(m_soundQuality);
    } else if (source == kSourceKugou) {
        if (m_account) {
            m_kugou.setCookie(m_account->kugouCookie());
            m_kugou.setDeviceInfo(m_account->kugouMid(), m_account->kugouDfid());
        }
        m_kugou.setQuality(m_soundQuality);
    }
}

#define DISPATCH(source, expr)                                              \
    do {                                                                    \
        const int quemusicSource = resolve(source);                         \
        syncSource(quemusicSource);                                         \
        switch (quemusicSource) {                                           \
        case kSourceNetease:  m_netease.expr; break;                        \
        case kSourceKugou:    m_kugou.expr; break;                          \
        case kSourceBilibili: m_bilibili.expr; break;                       \
        default:                                                            \
            qWarning() << "[api] 音源不支持该功能:" << quemusicSource;        \
            endRequest();                                                    \
        }                                                                   \
    } while (0)

void MusicApiService::searchSongs(const QString &keyword, int type, int page, int pageSize,
                                  int source)
{
    beginRequest();
    const int resolved = resolve(source);
    if (m_scripts.isPlugin(resolved)) {
        m_scripts.request(resolved, QStringLiteral("searchSongs"),
                          QVariantMap{{QStringLiteral("keyword"), keyword},
                                      {QStringLiteral("type"), type},
                                      {QStringLiteral("page"), page},
                                      {QStringLiteral("pageSize"), pageSize}});
        return;
    }
    DISPATCH(source, searchSongs(keyword, type, page, pageSize));
}

void MusicApiService::getPlaylistMenu(int type, int source)
{
    beginRequest();
    DISPATCH(source, getPlaylistMenu(type));
}

void MusicApiService::getMusicPlaylists(const QString &tagid, int page, int pageSize,
                                        int source)
{
    beginRequest();
    DISPATCH(source, getMusicPlaylists(tagid, page, pageSize));
}

void MusicApiService::getCategoryPlaylists(const QString &id, int page, int pageSize, int source)
{
    beginRequest();
    const int resolved = resolve(source);
    if (m_scripts.isPlugin(resolved)) {
        m_scripts.request(resolved, QStringLiteral("getCategoryPlaylists"),
                          QVariantMap{{QStringLiteral("id"), id},
                                      {QStringLiteral("page"), page},
                                      {QStringLiteral("pageSize"), pageSize}});
        return;
    }

    syncSource(resolved);
    if (resolved == kSourceKugou) {
        m_kugou.getCategoryPlaylists(id, page, pageSize);
    } else if (resolved == kSourceNetease) {
        m_netease.getMusicPlaylists(id, page, pageSize);
    } else if (resolved == kSourceBilibili) {
        m_bilibili.getMusicPlaylists(id, page, pageSize);
    } else {
        qWarning() << "[api] 音源不支持该功能:" << resolved;
        endRequest();
    }
}

void MusicApiService::getPlaylistSongs(const QString &listid, int page, int pageSize,
                                       int source)
{
    beginRequest();
    DISPATCH(source, getPlaylistSongs(listid, page, pageSize));
}

void MusicApiService::getRecommendSongs(int page, int pageSize, int source)
{
    beginRequest();
    DISPATCH(source, getRecommendSongs(page, pageSize));
}

void MusicApiService::getHotPlaylistMenu(int type, int source)
{
    beginRequest();
    DISPATCH(source, getHotPlaylistMenu(type));
}

void MusicApiService::getHotPlaylists(int page, int pageSize, int source)
{
    beginRequest();
    DISPATCH(source, getHotPlaylists(page, pageSize));
}

void MusicApiService::getNewSongs(int type, int page, int pageSize, int source)
{
    beginRequest();
    DISPATCH(source, getNewSongs(type, page, pageSize));
}

void MusicApiService::getAllToplist(int source)
{
    m_toplistList.clear();
    beginRequest();
    DISPATCH(source, getAllToplist());
}

void MusicApiService::getAllToplists()
{
    m_toplistList.clear();
    beginRequest();
    syncSource(kSourceKugou);
    m_kugou.getAllToplist();
    beginRequest();
    syncSource(kSourceNetease);
    m_netease.getAllToplist();
}

void MusicApiService::getMusicToplist(int page, int pageSize, int rankid, int source)
{
    beginRequest();
    DISPATCH(source, getMusicToplist(page, pageSize, rankid));
}

void MusicApiService::getHotSingers(int page, int pageSize, int source)
{
    beginRequest();
    DISPATCH(source, getHotSingers(page, pageSize));
}

void MusicApiService::getSingerCategory(int area, int page, int pageSize, int source)
{
    beginRequest();
    DISPATCH(source, getSingerCategory(area, page, pageSize));
}

void MusicApiService::getSingerSongs(const QString &singerid, int page, int pageSize,
                                     int source)
{
    beginRequest();
    DISPATCH(source, getSingerSongs(singerid, page, pageSize));
}

void MusicApiService::getMusicInfo(const QString &hash, int type, int source)
{
    beginRequest();
    const int resolvedSource = resolve(source);
    const QString playHash = (resolvedSource == kSourceKugou) ? resolveQualityHash(hash) : hash;
    if (type == 0) {
        ++m_playGeneration;
        m_playHash = playHash;
        m_playSource = resolvedSource;
    }
    if (m_scripts.isPlugin(resolvedSource)) {
        m_scripts.request(resolvedSource, QStringLiteral("getMusicInfo"),
                          QVariantMap{{QStringLiteral("hash"), playHash},
                                      {QStringLiteral("id"), playHash},
                                      {QStringLiteral("type"), type}});
        return;
    }
    DISPATCH(source, getMusicInfo(playHash, type));
}

void MusicApiService::getPersonalFm(int page, int pageSize, int source)
{
    beginRequest();
    DISPATCH(source, getPersonalFm(page, pageSize));
}

void MusicApiService::getPersonalRadar(int page, int pageSize, int source)
{
    beginRequest();
    DISPATCH(source, getPersonalRadar(page, pageSize));
}

void MusicApiService::getComments(const QString &hash, int page, int pageSize, int source)
{
    if (hash.isEmpty())
        return;
    beginRequest();
    DISPATCH(source, getComments(hash, qMax(1, page), qBound(1, pageSize, 50)));
}

void MusicApiService::getLyricInfo(const QString &hash, int duration, int source)
{
    const int resolved = resolve(source);
    if (m_scripts.isPlugin(resolved)) {
        m_scripts.request(resolved, QStringLiteral("getLyricInfo"),
                          QVariantMap{{QStringLiteral("hash"), hash},
                                      {QStringLiteral("duration"), duration}});
        return;
    }
    DISPATCH(source, getLyricInfo(hash, duration));
}

void MusicApiService::setLocalLyrics()
{
    setLyricsData({QVariantMap{
        {QStringLiteral("time"), 0},
        {QStringLiteral("text"), QStringLiteral("纯音乐，请欣赏")}
    }});
    setLyricsTranslate({});
}

void MusicApiService::readLocalLyricsAsync(const QString &filePath, const QString &title,
                                           const QString &artist, int duration,
                                           bool allowOnlineSearch)
{
    if (filePath.trimmed().isEmpty()) {
        emit localLyricsFailed(filePath);
        return;
    }

    const int generation = ++m_localLyricsGeneration;

    auto *watcher = new QFutureWatcher<QVariantMap>(this);
    connect(watcher, &QFutureWatcher<QVariantMap>::finished, this,
            [this, watcher, generation, filePath, title, artist, duration, allowOnlineSearch]() {
                const QVariantMap result = watcher->result();
                watcher->deleteLater();

                if (generation != m_localLyricsGeneration)
                    return;

                const QVariantList lyrics = result.value(QStringLiteral("lyrics")).toList();
                if (result.value(QStringLiteral("found")).toBool() && !lyrics.isEmpty()) {
                    emit localLyricsReady(filePath, lyrics,
                                          result.value(QStringLiteral("translate")).toList());
                    if (result.value(QStringLiteral("source")).toString()
                        == QLatin1String("embedded-sylt")) {
                        findLocalLyrics(filePath, title, artist, duration);
                    }
                } else if (allowOnlineSearch) {
                    findLocalLyrics(filePath, title, artist, duration);
                }
            });
    watcher->setFuture(QtConcurrent::run(&LocalLyricsReader::read, filePath));
}

void MusicApiService::findLocalLyrics(const QString &filePath, const QString &title,
                                      const QString &artist, int duration, int source)
{
    const int resolvedSource = resolve(source);
    const QString cleanTitle = title.trimmed();
    if (filePath.trimmed().isEmpty() || cleanTitle.isEmpty()) {
        emit localLyricsFailed(filePath);
        return;
    }

    LocalLyricsRequest request;
    request.filePath = filePath;
    request.title = cleanTitle;
    request.artist = artist.trimmed();
    request.duration = duration;
    m_localLyricsSearches.insert(resolvedSource, request);

    const QString keyword = request.artist.isEmpty()
        ? request.title
        : request.title + QLatin1Char(' ') + request.artist;
    searchSongs(keyword, 0, 1, 10, resolvedSource);
}

void MusicApiService::findOnlineLyrics(const QString &title, const QString &artist,
                                       int duration, int source)
{
    const QString cleanTitle = title.trimmed();
    if (cleanTitle.isEmpty())
        return;
    LocalLyricsRequest request;
    request.title = cleanTitle;
    request.artist = artist.trimmed();
    request.duration = duration;
    m_biliFallbackSearches.insert(source, request);
    const QString keyword = request.artist.isEmpty()
        ? request.title
        : request.title + QLatin1Char(' ') + request.artist;
    searchSongs(keyword, 0, 1, 10, source);
}

QVariantMap MusicApiService::readLocalMetadataBlocking(const QString &filePath)
{
    QVariantMap meta;
    if (filePath.isEmpty())
        return meta;

    const QString path = localPath(filePath);
    const QFileInfo fi(path);
    const QString jsonPath = fi.absolutePath() + QLatin1Char('/')
                             + fi.completeBaseName() + QStringLiteral(".json");
    if (QFileInfo::exists(jsonPath)) {
        QFile f(jsonPath);
        if (f.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isObject())
                return doc.object().toVariantMap();
        }
    }

    const QByteArray encodedPath = QFile::encodeName(path);
    TagLib::FileRef ref(encodedPath.constData(), false);
    if (ref.isNull() || ref.file() == nullptr)
        return meta;

    const TagLib::Tag *tag = ref.tag();
    if (tag) {
        if (!tag->title().isEmpty())
            meta.insert(QStringLiteral("title"), tagString(tag->title()));
        if (!tag->artist().isEmpty())
            meta.insert(QStringLiteral("artist"), tagString(tag->artist()));
        if (!tag->album().isEmpty())
            meta.insert(QStringLiteral("album"), tagString(tag->album()));
    }

    const QVariantMap lyricMap = LocalLyricsReader::read(path);
    if (lyricMap.value(QStringLiteral("found")).toBool()) {
        meta.insert(QStringLiteral("lyrics"), lyricMap.value(QStringLiteral("lyrics")));
        meta.insert(QStringLiteral("translate"), lyricMap.value(QStringLiteral("translate")));
    }

    return meta;
}

void MusicApiService::readLocalMetadataAsync(const QString &filePath)
{
    if (filePath.isEmpty()) {
        emit localMetadataReady(filePath, {});
        return;
    }
    auto *watcher = new QFutureWatcher<QVariantMap>(this);
    connect(watcher, &QFutureWatcher<QVariantMap>::finished, this, [this, watcher, filePath]() {
        watcher->deleteLater();
        emit localMetadataReady(filePath, watcher->result());
    });
    watcher->setFuture(QtConcurrent::run(&MusicApiService::readLocalMetadataBlocking, filePath));
}

QString MusicApiService::readLocalCoverHint(const QString &filePath)
{
    if (filePath.isEmpty())
        return QString();
    const QString path = localPath(filePath);
    if (auto it = m_coverHintCache.constFind(path); it != m_coverHintCache.constEnd())
        return it.value();

    const QFileInfo fi(path);
    const QString jsonPath = fi.absolutePath() + QLatin1Char('/')
                             + fi.completeBaseName() + QStringLiteral(".json");
    QString hint;
    QFile f(jsonPath);
    if (f.open(QIODevice::ReadOnly)) {
        const QVariantMap meta = QJsonDocument::fromJson(f.readAll()).object().toVariantMap();
        hint = meta.value(QStringLiteral("cover")).toString();
    }
    if (m_coverHintCache.size() > 8192)
        m_coverHintCache.clear();
    m_coverHintCache.insert(path, hint);
    return hint;
}

QVariantMap MusicApiService::readLocalAudioInfo(const QString &filePath)
{
    QVariantMap info;
    const QString path = localPath(filePath);
    if (path.isEmpty())
        return info;

    const QFileInfo fi(path);
    info.insert(QStringLiteral("fileName"), fi.fileName());
    info.insert(QStringLiteral("size"), fi.size());

    const QByteArray encoded = QFile::encodeName(path);
    TagLib::FileRef ref(encoded.constData());
    info.insert(QStringLiteral("format"), fi.suffix().toUpper());
    if (ref.isNull() || ref.file() == nullptr)
        return info;

    if (const TagLib::Tag *tag = ref.tag()) {
        info.insert(QStringLiteral("title"), tagString(tag->title()));
        info.insert(QStringLiteral("artist"), tagString(tag->artist()));
        info.insert(QStringLiteral("album"), tagString(tag->album()));
        info.insert(QStringLiteral("genre"), tagString(tag->genre()));
        info.insert(QStringLiteral("year"), int(tag->year()));
        info.insert(QStringLiteral("track"), int(tag->track()));
    }
    if (const TagLib::AudioProperties *ap = ref.audioProperties()) {
        info.insert(QStringLiteral("duration"), ap->lengthInSeconds());
        info.insert(QStringLiteral("bitrate"), ap->bitrate());
        info.insert(QStringLiteral("sampleRate"), ap->sampleRate());
        info.insert(QStringLiteral("channels"), ap->channels());
    }
    return info;
}

bool MusicApiService::writeLocalMetadataBlocking(const QString &filePath, const QVariantMap &meta)
{
    const QString path = localPath(filePath);
    if (path.isEmpty() || !QFileInfo::exists(path))
        return false;

    const QByteArray encoded = QFile::encodeName(path);
    TagLib::FileRef ref(encoded.constData());
    if (ref.isNull() || ref.tag() == nullptr)
        return false;

    const auto tagged = [&meta](const char *key) {
        return toTagString(meta.value(QLatin1String(key)).toString());
    };
    TagLib::Tag *tag = ref.tag();
    tag->setTitle(tagged("title"));
    tag->setArtist(tagged("artist"));
    tag->setAlbum(tagged("album"));
    tag->setGenre(tagged("genre"));
    tag->setYear(meta.value(QStringLiteral("year")).toString().toUInt());
    tag->setTrack(meta.value(QStringLiteral("track")).toString().toUInt());

    const QString lyrics = meta.value(QStringLiteral("lyrics")).toString().trimmed();
    if (!lyrics.isEmpty()) {
        TagLib::PropertyMap props = ref.file()->properties();
        props.replace("LYRICS", TagLib::StringList(toTagString(lyrics)));
        ref.file()->setProperties(props);
    }

    const QString cover = meta.value(QStringLiteral("cover")).toString();
    if (!cover.isEmpty()) {
        QFile image(localPath(cover));
        if (image.open(QIODevice::ReadOnly))
            embedCover(ref.file(), image.readAll());
    }
    return ref.save();
}

QString MusicApiService::readLocalLyricsText(const QString &filePath)
{
    const QString path = localPath(filePath);
    if (path.isEmpty())
        return QString();

    const QFileInfo fi(path);
    QFile sidecar(fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName()
                  + QStringLiteral(".lrc"));
    if (sidecar.open(QIODevice::ReadOnly))
        return QString::fromUtf8(sidecar.readAll());

    const QByteArray encoded = QFile::encodeName(path);
    TagLib::FileRef ref(encoded.constData(), false);
    if (ref.isNull() || ref.file() == nullptr)
        return QString();

    TagLib::PropertyMap props = ref.file()->properties();
    const TagLib::StringList values = props.value("LYRICS");
    return values.isEmpty() ? QString() : tagString(values.front());
}

void MusicApiService::writeLocalMetadata(const QString &filePath, const QVariantMap &meta)
{
    auto *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, filePath]() {
        watcher->deleteLater();
        emit localMetadataSaved(filePath, watcher->result());
    });
    watcher->setFuture(
        QtConcurrent::run(&MusicApiService::writeLocalMetadataBlocking, filePath, meta));
}

void MusicApiService::setAllPlaylistMenu(const QVariant &v)
{
    if (m_allPlaylistMenu == v)
        return;
    m_allPlaylistMenu = v;
    emit allPlaylistMenuChanged();
}

void MusicApiService::setLyricsData(const QVariantList &v)
{
    if (m_lyricsData == v)
        return;
    m_lyricsData = v;
    rebuildLyricTimes(v);
    setLyricIndex(0);
    emit lyricsDataChanged();
}

void MusicApiService::setLyricsTranslate(const QVariantList &v)
{
    if (m_lyricsTranslate == v)
        return;
    m_lyricsTranslate = v;
    emit lyricsTranslateChanged();
}

void MusicApiService::rebuildLyricTimes(const QVariantList &list)
{
    m_lyricTimes.clear();
    m_lyricTimes.reserve(list.size());
    for (const QVariant &item : list)
        m_lyricTimes.append(item.toMap().value(QStringLiteral("time")).toLongLong());
}

void MusicApiService::setLyricIndex(int index)
{
    if (m_lyricIndex == index)
        return;
    m_lyricIndex = index;
    emit lyricIndexChanged();
}

void MusicApiService::setCommentsHasMore(bool hasMore)
{
    if (m_commentsHasMore == hasMore)
        return;
    m_commentsHasMore = hasMore;
    emit commentsHasMoreChanged();
}

void MusicApiService::advanceLyricIndex()
{
    if (m_lyricTimes.isEmpty()) {
        setLyricIndex(0);
        return;
    }

    const qlonglong position = m_lyricPositionMs + m_lyricOffsetMs + kLyricTickMs;
    const auto it = std::upper_bound(m_lyricTimes.cbegin(), m_lyricTimes.cend(), position);
    const int index = it == m_lyricTimes.cbegin() ? 0 : int(it - m_lyricTimes.cbegin()) - 1;
    setLyricIndex(index);
}

void MusicApiService::setLyricFollowActive(bool active)
{
    if (m_lyricFollowActive == active)
        return;
    m_lyricFollowActive = active;
    if (active) {
        m_lyricTimer.start();
        advanceLyricIndex();
    } else {
        m_lyricTimer.stop();
    }
    emit lyricFollowActiveChanged();
}

void MusicApiService::setLyricPositionMs(qlonglong ms)
{
    if (m_lyricPositionMs == ms)
        return;
    m_lyricPositionMs = ms;
    emit lyricPositionMsChanged();
}

void MusicApiService::setLyricOffsetMs(int ms)
{
    if (m_lyricOffsetMs == ms)
        return;
    m_lyricOffsetMs = ms;
    emit lyricOffsetMsChanged();
}

void MusicApiService::setLoadState(bool s)
{
    if (m_loadState == s)
        return;
    m_loadState = s;
    emit loadStateChanged();
    if (s)
        emit loaded();
    else
        emit finished();
}

void MusicApiService::beginRequest()
{
    ++m_pendingRequests;
    m_requestWatchdog.start();
    setLoadState(true);
}

void MusicApiService::endRequest()
{
    if (m_pendingRequests > 0)
        --m_pendingRequests;
    if (m_pendingRequests == 0) {
        m_requestWatchdog.stop();
        setLoadState(false);
    } else {
        m_requestWatchdog.start();
    }
}

bool MusicApiService::isStalePlayResponse(const QString &playHash, int source) const
{
    if (playHash.isEmpty() || m_playHash.isEmpty())
        return false;
    return playHash != m_playHash || source != m_playSource;
}

void MusicApiService::setGlobalid(const QVariant &v)
{
    if (m_globalid == v)
        return;
    m_globalid = v;
    emit globalidChanged();
}

void MusicApiService::setGlobaltagid(const QVariant &v)
{
    if (m_globaltagid == v)
        return;
    m_globaltagid = v;
    emit globaltagidChanged();
}

void MusicApiService::setGlobalinfo(const QVariant &v)
{
    if (m_globalinfo == v)
        return;
    m_globalinfo = v;
    emit globalinfoChanged();
}

void MusicApiService::setNowIndex(int v)
{
    if (m_nowIndex == v)
        return;
    m_nowIndex = v;
    emit nowIndexChanged();
}

QVariantMap MusicApiService::normalizeItem(const QVariantMap &raw)
{
    QVariantMap item = raw;
    if (item.isEmpty())
        return item;

    const auto has = [&item](const char *key) {
        const QVariant value = item.value(QLatin1String(key));
        return value.isValid() && !value.isNull();
    };
    const auto fill = [&item, &has](const char *key, const char *fallback) {
        if (!has(key) && has(fallback))
            item.insert(QLatin1String(key), item.value(QLatin1String(fallback)));
    };

    fill("title", "name");
    fill("artist", "singer");
    fill("cover", "pic");
    fill("hash", "id");

    if (!has("title"))     item.insert(QStringLiteral("title"), QString());
    if (!has("artist"))    item.insert(QStringLiteral("artist"), QString());
    if (!has("cover"))     item.insert(QStringLiteral("cover"), QString());
    if (!has("hash"))      item.insert(QStringLiteral("hash"), QString());
    if (!has("album"))     item.insert(QStringLiteral("album"), QString());
    if (!has("duration"))  item.insert(QStringLiteral("duration"), 0);
    if (!has("playcount")) item.insert(QStringLiteral("playcount"), 0);
    if (!has("paytype"))   item.insert(QStringLiteral("paytype"), ApiCommon::kPaytypeFree);
    if (!has("hashhq"))    item.insert(QStringLiteral("hashhq"), item.value(QStringLiteral("hash")));
    if (!has("hashsq"))    item.insert(QStringLiteral("hashsq"), item.value(QStringLiteral("hash")));
    return item;
}

QVariantList MusicApiService::normalizeList(const QVariant &value)
{
    const QVariantList list = value.toList();
    QVariantList result;
    result.reserve(list.size());
    for (const QVariant &entry : list)
        result.append(normalizeItem(entry.toMap()));
    return result;
}

QVariantList MusicApiService::songList(const QVariant &data)
{
    const QVariantList list = data.toList();
    QVariantList result;
    result.reserve(list.size());
    for (const QVariant &entry : list) {
        QVariantMap item = normalizeItem(entry.toMap());
        if (m_songFilter != 0) {
            const bool free = item.value(QStringLiteral("paytype")).toInt()
                              != ApiCommon::kPaytypePaid;
            if ((m_songFilter == 1 && !free) || (m_songFilter == 2 && free))
                continue;
        }
        result.append(item);
    }
    return result;
}

void MusicApiService::handleResult(const QString &action, const QVariant &data, int source)
{
    if (action != QLatin1String("getLyricInfo"))
        endRequest();

    const QVariantMap message = data.toMap();
    const bool requestFailed = message.contains(QStringLiteral("error"));
    if (requestFailed) {
        qWarning() << "[api] 请求失败:" << action
                   << "error:" << message.value(QStringLiteral("error")).toInt()
                   << "source:" << source;
    }
    const QVariant info = message.contains(QStringLiteral("info"))
                              ? message.value(QStringLiteral("info"))
                              : data;

    if (action == QLatin1String("getComments")) {
        m_comments.append(requestFailed ? QVariantList() : info.toList());
        setCommentsHasMore(!requestFailed
                           && message.value(QStringLiteral("hasMore")).toBool());
        return;
    }

    if (info.typeId() == QMetaType::QVariantList)
        rememberHashes(info.toList());

    if (action == QLatin1String("searchSongs")) {
        const bool localLookup = m_localLyricsSearches.contains(source);
        const bool onlineLookup = m_biliFallbackSearches.contains(source);
        if (!localLookup && !onlineLookup) {
            m_searchSongsResults.append(normalizeList(info));
            return;
        }

        const LocalLyricsRequest request = localLookup
            ? m_localLyricsSearches.take(source)
            : m_biliFallbackSearches.take(source);
        const QVariantList songs = normalizeList(info);
        QVariantMap best;
        int bestScore = -1;
        for (const QVariant &value : songs) {
            const QVariantMap candidate = value.toMap();
            const QString title = candidate.value(QStringLiteral("title")).toString();
            const QString artist = candidate.value(QStringLiteral("artist")).toString();
            int score = 0;
            if (title.compare(request.title, Qt::CaseInsensitive) == 0) {
                score += 4;
            } else if (title.contains(request.title, Qt::CaseInsensitive)
                       || request.title.contains(title, Qt::CaseInsensitive)) {
                score += 2;
            }
            if (!request.artist.isEmpty()
                && (artist.contains(request.artist, Qt::CaseInsensitive)
                    || request.artist.contains(artist, Qt::CaseInsensitive))) {
                score += 2;
            }
            if (!candidate.value(QStringLiteral("hash")).toString().isEmpty())
                score += 1;
            if (score > bestScore) {
                bestScore = score;
                best = candidate;
            }
        }

        const QString hash = best.value(QStringLiteral("hash")).toString();
        if (hash.isEmpty()) {
            if (localLookup && !requestFailed)
                emit localLyricsFailed(request.filePath);
            return;
        }
        if (localLookup)
            m_pendingLocalLyrics.insert(hash, request);
        getLyricInfo(hash, request.duration, source);
    } else if (action == QLatin1String("getPlaylistMenu")) {
        QVariantList menu;
        const QVariantList items = info.toList();
        menu.reserve(items.size());
        for (const QVariant &value : items) {
            const QVariantMap item = normalizeItem(value.toMap());
            menu.append(QVariantMap{
                {QStringLiteral("title"), item.value(QStringLiteral("title"))},
                {QStringLiteral("category"), item.value(QStringLiteral("category"))},
                {QStringLiteral("id"), item.value(QStringLiteral("id"))}
            });
        }
        setAllPlaylistMenu(menu);
    } else if (action == QLatin1String("getMusicPlaylists")
               || action == QLatin1String("getCategoryPlaylists")) {
        m_musicPlaylists.append(normalizeList(info));
    } else if (action == QLatin1String("getPlaylistSongs")
               || action == QLatin1String("getMusicToplist")
               || action == QLatin1String("getSingerSongs")) {
        m_playlistSong.append(songList(info));
    } else if (action == QLatin1String("getRecommendSongs")) {
        m_recommendSongs.append(normalizeList(info));
    } else if (action == QLatin1String("getHotPlaylistMenu")) {
        m_getHotlistMenu.clear();
        m_getHotlistMenu.append(normalizeList(info));
    } else if (action == QLatin1String("getHotPlaylists")) {
        m_hotPlayLists.append(normalizeList(info));
    } else if (action == QLatin1String("getNewSongs")) {
        m_newSongs.append(songList(info));
    } else if (action == QLatin1String("getAllToplist")) {
        const QVariantList normalized = normalizeList(info);
        m_musicToplist.clear();
        m_musicToplist.append(normalized);

        QVariantList withSource;
        withSource.reserve(normalized.size());
        for (const QVariant &value : normalized) {
            QVariantMap item = value.toMap();
            item.insert(QStringLiteral("source"), source);
            withSource.append(item);
        }
        m_toplistList.append(withSource);
    } else if (action == QLatin1String("getHotSingers")
               || action == QLatin1String("getSingerCategory")) {
        m_singerList.append(normalizeList(info));
    } else if (action == QLatin1String("getPersonalFm")
               || action == QLatin1String("getPersonalRadar")) {
        const bool fm = action == QLatin1String("getPersonalFm");
        const QVariantList items = info.toList();
        if (items.isEmpty() && !requestFailed) {
            emit warned(fm ? QStringLiteral("私人漫游需要先在设置中登录账号")
                           : QStringLiteral("私人雷达需要先在设置中登录账号"), 2);
        } else {
            (fm ? m_personalFm : m_personalRadar).append(normalizeList(items));
        }
    } else if (action == QLatin1String("getMusicInfo")) {
        handleMusicInfo(message, source);
    } else if (action == QLatin1String("getLyricInfo")) {
        const QString lyricHash = message.value(QStringLiteral("hash")).toString();
        if (m_playLyricGenerations.contains(lyricHash)) {
            const int generation = m_playLyricGenerations.take(lyricHash);
            if (generation != m_playGeneration) {
                qDebug() << "[api] 丢弃过期歌词响应 hash:" << lyricHash.left(8);
                return;
            }
        }

        if (requestFailed) {
            emit warned(QStringLiteral("歌词加载失败，请检查网络后重试"), 0);
            if (!lyricHash.isEmpty() && m_pendingLocalLyrics.contains(lyricHash))
                emit localLyricsFailed(m_pendingLocalLyrics.take(lyricHash).filePath);
        } else {
            QVariantList onlineLyrics = message.value(QStringLiteral("info")).toList();
            const bool biliNoSubtitle = source == kSourceBilibili && !lyricHash.isEmpty()
                                        && lyricHash == m_biliTrack.hash;
            if (biliNoSubtitle && onlineLyrics.isEmpty())
                findOnlineLyrics(m_biliTrack.title, m_biliTrack.artist, m_biliTrack.duration);

            if (onlineLyrics.isEmpty()) {
                onlineLyrics.append(QVariantMap{
                    {QStringLiteral("time"), 0},
                    {QStringLiteral("text"), QStringLiteral("纯音乐，请欣赏")}
                });
            }
            setLyricsData(onlineLyrics);
            setLyricsTranslate(message.value(QStringLiteral("translate")).toList());

            if (!lyricHash.isEmpty() && m_pendingLocalLyrics.contains(lyricHash)) {
                const LocalLyricsRequest request = m_pendingLocalLyrics.take(lyricHash);
                const QVariantList lyrics = message.value(QStringLiteral("info")).toList();
                if (lyrics.isEmpty()) {
                    emit localLyricsFailed(request.filePath);
                } else {
                    emit localLyricsReady(request.filePath, lyrics,
                                          message.value(QStringLiteral("translate")).toList());
                }
            }
        }

        if (!lyricHash.isEmpty() && m_pendingDownloads.contains(lyricHash)) {
            QVariantMap meta = m_pendingDownloads.take(lyricHash);
            meta.insert(QStringLiteral("lyrics"), message.value(QStringLiteral("info")));
            meta.insert(QStringLiteral("translate"),
                        message.value(QStringLiteral("translate")));
            m_downloader.addDownload(meta.value(QStringLiteral("url")).toString(),
                                     meta.value(QStringLiteral("fileName")).toString(),
                                     meta);
            emit warned(QStringLiteral("已添加下载: ")
                            + meta.value(QStringLiteral("fileName")).toString(), 1);
        }
    } else {
        qWarning() << "MusicApiService: 未知消息类型:" << action;
    }
}

void MusicApiService::handleMusicInfo(const QVariantMap &d, int source)
{
    const int type = d.value(QStringLiteral("type")).toInt();
    const QString playUrl = firstNonEmpty(d, {"url", "backup_url"});
    const QString playHash = d.value(QStringLiteral("hash")).toString();
    if (type == 0 && isStalePlayResponse(playHash, source)) {
        qDebug() << "[api] 丢弃过期播放响应 hash:" << playHash.left(8)
                 << "当前代次 hash:" << m_playHash.left(8);
        return;
    }

    if (playUrl.isEmpty()) {
        const bool networkFailed = d.contains(QStringLiteral("error"));
        const QString reason = d.value(QStringLiteral("errReason")).toString();
        const bool kugouNotLoggedIn = source == kSourceKugou && m_account
                                      && !m_account->isKugouLoggedIn();
        QString message;
        if (networkFailed) {
            message = QStringLiteral("网络请求失败，请检查网络后重试");
        } else if (reason == QLatin1String("vip")) {
            message = kugouNotLoggedIn ? QStringLiteral("该歌曲需要 VIP 会员，请先登录酷狗账号")
                                       : QStringLiteral("该歌曲需要 VIP 会员或购买后才能播放");
        } else if (kugouNotLoggedIn) {
            message = QStringLiteral("该歌曲暂无可用播放地址，可登录酷狗账号后重试");
        } else {
            message = QStringLiteral("该歌曲受版权或会员限制，暂时无法获取播放地址");
        }
        qDebug() << "[api] 无法播放:" << message << "hash:" << playHash.left(8);
        emit warned(message, 2);
        return;
    }

    const QString identityHash = m_baseOf.value(playHash, playHash);
    const QString songName = d.value(QStringLiteral("songName")).toString();
    const QString author = d.value(QStringLiteral("author_name")).toString();
    const double timeLength = d.value(QStringLiteral("timeLength")).toDouble();
    const int timeMs = int(timeLength * (timeLength < 1000 ? 1000 : 1));

    if (source == kSourceBilibili) {
        m_biliTrack.hash = playHash;
        m_biliTrack.title = songName;
        m_biliTrack.artist = author;
        m_biliTrack.duration = int(timeLength);
    }

    if (type == 0) {
        QString cover = d.value(QStringLiteral("album_img")).toString();
        cover.replace(QLatin1String("{size}"), QLatin1String("512"));
        QString solve = d.value(QStringLiteral("album_img")).toString();
        solve.replace(QLatin1String("{size}"), QLatin1String("128"));
        emit urlplay(playUrl, songName, author, cover, solve, identityHash, source);

        if (m_playLyricGenerations.size() > 32) {
            for (auto it = m_playLyricGenerations.begin(); it != m_playLyricGenerations.end();) {
                if (it.value() != m_playGeneration)
                    it = m_playLyricGenerations.erase(it);
                else
                    ++it;
            }
        }
        m_playLyricGenerations.insert(playHash, m_playGeneration);
        getLyricInfo(playHash, timeMs, source);
    } else if (type == 1) {
        QString cover = d.value(QStringLiteral("album_img")).toString();
        cover.replace(QLatin1String("{size}"), QLatin1String("512"));

        QVariantMap meta;
        meta.insert(QStringLiteral("title"), songName);
        meta.insert(QStringLiteral("artist"), author);
        meta.insert(QStringLiteral("cover"), cover);
        meta.insert(QStringLiteral("duration"), int(timeLength));
        meta.insert(QStringLiteral("hash"), playHash);
        meta.insert(QStringLiteral("url"), playUrl);
        meta.insert(QStringLiteral("fileName"), d.value(QStringLiteral("fileName")).toString());

        m_pendingDownloads.insert(playHash, meta);
        getLyricInfo(playHash, timeMs, source);
    }
}
