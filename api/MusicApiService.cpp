// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "MusicApiService.h"

#include <QDebug>
#include <QQmlEngine>
#include <QJSEngine>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QtConcurrent>

#include "../cpp/AccountManager.h"
#include "../cpp/LocalLyricsReader.h"

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
// 未完成请求的兜底时限：超过这么久没有任何平台响应，就强制结束加载态
constexpr int kRequestWatchdogMs = 20000;

// 取多个候选字段中第一个非空字符串（模拟 JS 的 a || b || c || ""）
QString firstNonEmpty(const QVariantMap &m, std::initializer_list<const char *> keys)
{
    for (const char *k : keys) {
        const QVariant v = m.value(QLatin1String(k));
        if (v.isValid() && !v.toString().isEmpty())
            return v.toString();
    }
    return QString();
}

// TagLib 字符串统一转 UTF-8，避免中文歌词/标题乱码
QString tagString(const TagLib::String &value)
{
    return QString::fromUtf8(value.toCString(true));
}

// file:// URL 与纯路径统一归一成磁盘路径
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

// MP3(APIC) / FLAC(Picture) / MP4(covr)，其余容器忽略
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
} // namespace

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
    // 平台结果直接在本类处理（填模型 / 属性 / 发信号）
    connect(&m_netease, &NeteaseCloudApi::resultReady, this, &MusicApiService::handleResult);
    connect(&m_kugou, &KugouApi::resultReady, this, &MusicApiService::handleResult);
    connect(&m_bilibili, &BilibiliApi::resultReady, this, &MusicApiService::handleResult);

    // 音质初值：QML 侧还有 Binding 同步，这里读 ini 只为保证 Binding 生效前也正确
    QSettings opt(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                      + QStringLiteral("/BroNekoX/QueMusic.ini"),
                  QSettings::IniFormat);
    m_soundQuality = opt.value(QStringLiteral("Options/soundQuality"), 1).toInt();

    m_altsSaveTimer.setSingleShot(true);
    connect(&m_altsSaveTimer, &QTimer::timeout, this, &MusicApiService::saveQualityCache);
    loadQualityCache();

    // 计数兜底：任一平台「有请求不回结果」时，未完成计数会一直大于 0，
    // 加载动画就再也停不下来。20 秒没有任何响应即视为链路异常，强制销账
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

// 记录「同一首歌的不同音质 hash」（酷狗把普通/高清/无损做成三个不同 hash，
// 搜索、歌单、榜单结果里都带：hash / hashhq / hashsq）
void MusicApiService::rememberHashes(const QVariantList &items)
{
    if (m_alts.size() > 5000) // 只做加速用，不做无界增长
        return;
    bool changed = false;
    for (const QVariant &v : items) {
        const QVariantMap it = v.toMap();
        const QString base = it.value(QStringLiteral("hash")).toString();
        if (base.isEmpty())
            continue;
        const QString hq = it.value(QStringLiteral("hashhq")).toString();
        const QString sq = it.value(QStringLiteral("hashsq")).toString();
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
    m_altsSaveTimer.start(1500); // 合并连续列表解析，避免频繁写盘
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

// 音质设置 → 实际请求的 hash（0 标准 128k / 1 高清 320k / 2+ 无损 flac）
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
    // B 站无需登录：只同步音质（决定取哪条 DASH 音频流）
    if (source == kSourceBilibili) {
        m_bilibili.setQuality(m_soundQuality);
        return;
    }
    if (!m_account)
        return;
    if (source == kSourceNetease)
        // 带客户端身份（稳定 deviceId + os=pc）：未登录也要带，避免被网易云判为异常环境
        m_netease.setCookie(m_account->neteaseApiCookie());
    else if (source == kSourceKugou) {
        m_kugou.setCookie(m_account->kugouCookie());
        m_kugou.setDeviceInfo(m_account->kugouMid(), m_account->kugouDfid());
    }
}

// 内部变量名加前缀：避免调用点传入与外层同名变量时被自身初始化式遮蔽（曾误传 s 导致读到未初始化值）
#define DISPATCH(source, expr)                                              \
    do {                                                                    \
        const int quemusicSource = resolve(source);                         \
        syncSource(quemusicSource);                                         \
        switch (quemusicSource) {                                           \
        case kSourceNetease:  m_netease.expr; break;                        \
        case kSourceKugou:    m_kugou.expr; break;                          \
        case kSourceBilibili: m_bilibili.expr; break;                       \
        default:                                                            \
            qWarning() << "[api] 未实现的平台:" << quemusicSource;            \
        }                                                                   \
    } while (0)

// 统一请求入口（请求开始置 loadState=true）
void MusicApiService::searchSongs(const QString &keyword, int type, int page, int pageSize,
                                  int source)
{
    beginRequest();
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

// 分类页的歌单（酷狗走 category/special，与首页热门分类卡片的 tag 体系不同源）
void MusicApiService::getCategoryPlaylists(const QString &id, int page, int pageSize, int source)
{
    setLoadState(true);
    const int resolved = resolve(source);
    syncSource(resolved);
    if (resolved == kSourceKugou)
        m_kugou.getCategoryPlaylists(id, page, pageSize);
    else if (resolved == kSourceNetease)
        m_netease.getMusicPlaylists(id, page, pageSize);   // 菜单里的 id 即分类名(cat)
    else
        m_bilibili.getMusicPlaylists(id, page, pageSize);
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

// 同时拉取酷狗 + 网易云的榜单列表，合并进 toplistList（每条带 source）
void MusicApiService::getAllToplists()
{
    m_toplistList.clear();
    // 两个平台各算一个未完成请求：任何一个先回来都不会提前结束加载动画
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
    // 酷狗同一首歌按音质是不同 hash：按设置换成高清/无损 hash，没有就回退原 hash
    const int resolvedSource = resolve(source);
    const QString playHash = (resolvedSource == kSourceKugou) ? resolveQualityHash(hash) : hash;
    // type=0 是「播放」：每次点击都开一个新代次，旧代次的响应一律丢弃。
    // type=1 是下载，不影响当前播放，不参与代次竞争
    if (type == 0) {
        ++m_playGeneration;
        m_playHash = playHash;
        m_playSource = resolvedSource;
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
    m_comments.setItems(QVariantList()); // 先清空：切歌时不要残留上一首的评论
    DISPATCH(source, getComments(hash, qMax(1, page), qBound(1, pageSize, 50)));
}

void MusicApiService::getLyricInfo(const QString &hash, int duration, int source)
{
    // 歌词请求不单独改 loadState（由 getMusicInfo 链路管理）
    DISPATCH(source, getLyricInfo(hash, duration));
}

// 本地音乐（无歌词）时调用：覆盖在线歌词残留，显示占位歌词
void MusicApiService::setLocalLyrics()
{
    QVariantMap line;
    line.insert(QStringLiteral("time"), 0);
    line.insert(QStringLiteral("text"), QStringLiteral("纯音乐，请欣赏"));
    QVariantList placeholder;
    placeholder << line;
    setLyricsData(placeholder);
    setLyricsTranslate(QVariantList()); // 清空翻译，避免残留
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

                if (generation != m_localLyricsGeneration) // 期间又切了歌，丢弃
                    return;

                const QVariantList lyrics = result.value(QStringLiteral("lyrics")).toList();
                if (result.value(QStringLiteral("found")).toBool() && !lyrics.isEmpty()) {
                    emit localLyricsReady(filePath, lyrics,
                                          result.value(QStringLiteral("translate")).toList());
                    // 逐音节重建的内嵌 SYLT 没有翻译：顺手在线取一份（拿到会覆盖，失败保留这份）
                    if (result.value(QStringLiteral("source")).toString()
                        == QLatin1String("embedded-sylt"))
                        findLocalLyrics(filePath, title, artist, duration);
                } else if (allowOnlineSearch) {
                    findLocalLyrics(filePath, title, artist, duration);
                }
                // 其余情况保留当前歌词（如 .json 自带），不覆盖
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

    // Search only songs so the result can be resolved to a hash and then sent
    // through the existing platform-specific lyric endpoint.
    const QString keyword = request.artist.isEmpty()
        ? request.title
        : request.title + QLatin1Char(' ') + request.artist;
    searchSongs(keyword, 0, 1, 10, resolvedSource);
}

// B 站稿件没有字幕：复用「本地歌曲在线搜词」的同一条链路，
// 按标题/歌手在回退平台（默认酷狗）搜歌，命中后取该平台歌词覆盖占位歌词。
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

// 读取本地音频同目录同名 .json 元数据；不存在时回退读取音频内嵌 TAG 元数据
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

    // 无 .json 时读取音频内嵌 TAG，本地歌曲同样能拿到标题/歌手/歌词元数据
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
    m_coverHintCache.insert(path, hint);
    return hint;
}

// 只读表头，毫秒级，可同步调用
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

    // LYRICS 会映射到各容器对应字段（USLT / Vorbis LYRICS / ©lyr）。
    // setProperties 会删掉表里没有的既有属性，所以必须先取出全部现有属性再替换。
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

// 同目录 .lrc 优先，其次内嵌
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

// setter
void MusicApiService::setAllPlaylistMenu(const QVariant &v)
{
    if (m_allPlaylistMenu == v)
        return;
    m_allPlaylistMenu = v;
    emit allPlaylistMenuChanged();
}

void MusicApiService::setLyricsData(const QVariant &v)
{
    if (m_lyricsData == v)
        return;
    m_lyricsData = v;
    emit lyricsDataChanged();
}

void MusicApiService::setLyricsTranslate(const QVariant &v)
{
    if (m_lyricsTranslate == v)
        return;
    m_lyricsTranslate = v;
    emit lyricsTranslateChanged();
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

// 未完成请求计数：只有计数归零才结束加载态，
// 否则「列表页请求 + 播放请求」并发时，先回来的那个会把动画提前关掉
void MusicApiService::beginRequest()
{
    ++m_pendingRequests;
    // 有响应回来会重启，等价于「距上次响应超过 kRequestWatchdogMs 就强制销账」
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

// 过期判断：响应的 hash / 平台与当前代次不一致时，说明它是上一首歌的响应。
// hash 为空（接口没回 hash 的错误响应）不参与判断，避免把真实错误提示吞掉
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

// 字段归一化：统一字段兜底 + 旧字段名别名（兼容旧 QML 页面）
QVariantMap MusicApiService::normalizeItem(const QVariantMap &raw)
{
    QVariantMap item = raw;
    const auto has = [&item](const char *k) {
        return item.contains(QLatin1String(k)) && item.value(QLatin1String(k)).isValid();
    };
    const auto val = [&item](const char *k) { return item.value(QLatin1String(k)); };

    if (!has("title"))
        item.insert(QStringLiteral("title"), firstNonEmpty(item, {"specialname", "songname"}));
    if (!has("artist"))
        item.insert(QStringLiteral("artist"), firstNonEmpty(item, {"username", "singername", "author_name"}));
    if (!has("cover"))
        item.insert(QStringLiteral("cover"), firstNonEmpty(item, {"imgurl", "album_img"}));
    if (!has("hash"))
        item.insert(QStringLiteral("hash"), firstNonEmpty(item, {"specialid", "albumid", "id"}));
    if (!has("duration"))
        item.insert(QStringLiteral("duration"), 0);
    if (!has("album"))
        item.insert(QStringLiteral("album"), firstNonEmpty(item, {"intro", "album_name"}));
    if (!has("playcount"))
        item.insert(QStringLiteral("playcount"), 0);
    if (!has("paytype"))
        item.insert(QStringLiteral("paytype"), 0);
    if (!has("hashhq"))
        item.insert(QStringLiteral("hashhq"), val("hash"));
    if (!has("hashsq"))
        item.insert(QStringLiteral("hashsq"), val("hash"));

    // 旧字段名别名
    if (!has("specialname")) item.insert(QStringLiteral("specialname"), val("title"));
    if (!has("username"))   item.insert(QStringLiteral("username"),   val("artist"));
    if (!has("imgurl"))     item.insert(QStringLiteral("imgurl"),     val("cover"));
    if (!has("songname"))   item.insert(QStringLiteral("songname"),   val("title"));
    if (!has("singername")) item.insert(QStringLiteral("singername"), val("artist"));
    if (!has("intro"))      item.insert(QStringLiteral("intro"),      val("album"));
    if (!has("album_name")) item.insert(QStringLiteral("album_name"), val("album"));
    if (!has("specialid"))  item.insert(QStringLiteral("specialid"),  val("hash"));
    if (!has("albumid"))    item.insert(QStringLiteral("albumid"),    val("hash"));
    return item;
}

QVariantList MusicApiService::normalizeList(const QVariant &v)
{
    QVariantList out;
    const QVariantList list = v.toList();
    for (const QVariant &it : list)
        out << normalizeItem(it.toMap());
    return out;
}

// 平台结果统一处理（填模型 / 属性 / 发信号）
void MusicApiService::handleResult(const QString &action, const QVariant &data, int source)
{
    // 响应已到达就先销账：本函数中间有提前 return 的分支，
    // 放在出口处会漏减，导致加载动画永远不结束
    endRequest();

    const QVariantMap d = data.toMap();
    // 平台失败与「接口成功的合法空数据」必须区分：B 站失败时会回
    // { info: [], error: <QNetworkReply::NetworkError> }，带 error 的响应
    // 不能再当成「没有数据/没有歌词」使用，否则网络故障会被显示成「纯音乐，请欣赏」
    const bool requestFailed = d.contains(QStringLiteral("error"));
    if (requestFailed)
        qWarning() << "[api] 请求失败:" << action
                   << "error:" << d.value(QStringLiteral("error")).toInt()
                   << "source:" << source;
    const QVariant info = d.contains(QStringLiteral("info"))
                              ? d.value(QStringLiteral("info"))
                              : data;

    // 评论：直接灌评论模型，不参与歌曲 hash 等后续处理
    if (action == QLatin1String("getComments")) {
        m_comments.setItems(requestFailed ? QVariantList() : info.toList());
        return;
    }

    // 列表结果里带 hashhq/hashsq：记下来，播放时才能按音质设置升级 hash
    if (info.typeId() == QMetaType::QVariantList)
        rememberHashes(info.toList());

    if (action == QLatin1String("searchSongs")) {
        const bool localLookup = m_localLyricsSearches.contains(source);
        const bool onlineLookup = m_biliFallbackSearches.contains(source);
        if (localLookup || onlineLookup) {
            const LocalLyricsRequest request = localLookup
                ? m_localLyricsSearches.take(source)
                : m_biliFallbackSearches.take(source);
            const QVariantList songs = normalizeList(info);
            QVariantMap best;
            int bestScore = -1;
            for (const QVariant &value : songs) {
                const QVariantMap candidate = value.toMap();
                const QString candidateTitle = candidate.value(QStringLiteral("title")).toString();
                const QString candidateArtist = candidate.value(QStringLiteral("artist")).toString();
                int score = 0;
                if (candidateTitle.compare(request.title, Qt::CaseInsensitive) == 0)
                    score += 4;
                else if (candidateTitle.contains(request.title, Qt::CaseInsensitive)
                         || request.title.contains(candidateTitle, Qt::CaseInsensitive))
                    score += 2;
                if (!request.artist.isEmpty()
                    && (candidateArtist.contains(request.artist, Qt::CaseInsensitive)
                        || request.artist.contains(candidateArtist, Qt::CaseInsensitive)))
                    score += 2;
                if (!candidate.value(QStringLiteral("hash")).toString().isEmpty())
                    score += 1;
                if (score > bestScore) {
                    bestScore = score;
                    best = candidate;
                }
            }

            const QString hash = best.value(QStringLiteral("hash")).toString();
            if (hash.isEmpty()) {
                // 请求失败不是「没搜到」：不要把失败说成没有歌词
                if (localLookup && !requestFailed)
                    emit localLyricsFailed(request.filePath);
            } else if (localLookup) {
                m_pendingLocalLyrics.insert(hash, request);
                getLyricInfo(hash, request.duration, source);
            } else { // 回退歌词：直接走该平台歌词接口，结果覆盖占位歌词
                getLyricInfo(hash, request.duration, source);
            }
        } else {
            m_searchSongsResults.append(normalizeList(info));
        }
    } else if (action == QLatin1String("getPlaylistMenu")) {
        // 歌单分类：map 数组（QML 侧 text: modelData.title / [i].id 访问）
        QVariantList menu;
        for (const QVariant &v : info.toList()) {
            const QVariantMap it = normalizeItem(v.toMap());
            QVariantMap s;
            s.insert(QStringLiteral("title"), it.value(QStringLiteral("title")));
            s.insert(QStringLiteral("category"), it.value(QStringLiteral("category")));
            s.insert(QStringLiteral("id"), it.value(QStringLiteral("id")));
            menu << s;
        }
        setAllPlaylistMenu(menu);
    } else if (action == QLatin1String("getMusicPlaylists")) {
        m_musicPlaylists.append(normalizeList(info));
    } else if (action == QLatin1String("getCategoryPlaylists")) {
        m_musicPlaylists.append(normalizeList(info));
    } else if (action == QLatin1String("getPlaylistSongs")) {
        m_playlistSong.append(normalizeList(info));
    } else if (action == QLatin1String("getRecommendSongs")) {
        m_recommendSongs.append(normalizeList(info));
    } else if (action == QLatin1String("getHotPlaylistMenu")) {
        m_getHotlistMenu.clear();
        m_getHotlistMenu.append(normalizeList(info));
    } else if (action == QLatin1String("getHotPlaylists")) {
        m_hotPlayLists.append(normalizeList(info));
    } else if (action == QLatin1String("getNewSongs")) {
        m_newSongs.append(normalizeList(info));
    } else if (action == QLatin1String("getMusicToplist")) {
        // 榜单歌曲 → 填 playlistSong（供 playListSongsWindow 展示）
        m_playlistSong.append(normalizeList(info));
    } else if (action == QLatin1String("getAllToplist")) {
        // 单平台榜单列表：保留旧模型 + 累积到双平台模型（注入 source 供点击强制指定平台）
        m_musicToplist.clear();
        m_musicToplist.append(normalizeList(info));
        QVariantList list = normalizeList(info);
        for (QVariant &it : list) {
            QVariantMap m = it.toMap();
            m.insert(QStringLiteral("source"), source);
            it = m;
        }
        m_toplistList.append(list);
    } else if (action == QLatin1String("getHotSingers")) {
        // 分页累积：第一页由 UI 侧 clear，后续页直接 append
        m_singerList.append(normalizeList(info));
    } else if (action == QLatin1String("getSingerCategory")) {
        m_singerList.append(normalizeList(info));
    } else if (action == QLatin1String("getSingerSongs")) {
        m_playlistSong.append(normalizeList(info));
    } else if (action == QLatin1String("getPersonalFm")) {
        // 分页累积：第一页由 UI 侧 clear，后续页直接 append
        const QVariantList items = info.toList();
        // 空列表只有在「请求成功」时才等于「未登录」，失败时别误导用户去登录
        if (items.isEmpty() && !requestFailed) // 网易云 personal_fm 未登录返回空
            emit warned(QStringLiteral("私人漫游需要先在设置中登录账号"), 2);
        else
            m_personalFm.append(normalizeList(items));
    } else if (action == QLatin1String("getPersonalRadar")) {
        const QVariantList items = info.toList();
        if (items.isEmpty() && !requestFailed) // 网易云 recommend_songs 未登录返回空
            emit warned(QStringLiteral("私人雷达需要先在设置中登录账号"), 2);
        else
            m_personalRadar.append(normalizeList(items));
    } else if (action == QLatin1String("getMusicInfo")) {
        handleMusicInfo(d, source);
    } else if (action == QLatin1String("getLyricInfo")) {
        const QString lyricHash = d.value(QStringLiteral("hash")).toString();
        // 播放链路发出的歌词请求：代次不符说明当前已经在放下一首，丢弃
        if (m_playLyricGenerations.contains(lyricHash)) {
            const int generation = m_playLyricGenerations.take(lyricHash);
            if (generation != m_playGeneration) {
                qDebug() << "[api] 丢弃过期歌词响应 hash:" << lyricHash.left(8);
                return;
            }
        }
        // 请求失败 ≠ 没有歌词：失败时保留当前歌词并提示重试，
        // 否则「网络挂了/接口异常」会被显示成「纯音乐，请欣赏」
        if (requestFailed) {
            emit warned(QStringLiteral("歌词加载失败，请检查网络后重试"), 0);
            // 本地歌词的等待者也要销账，否则这个文件之后再也不会重新匹配
            if (!lyricHash.isEmpty() && m_pendingLocalLyrics.contains(lyricHash))
                emit localLyricsFailed(m_pendingLocalLyrics.take(lyricHash).filePath);
        } else {
            QVariantList onlineLyrics = d.value(QStringLiteral("info")).toList();
            // B 站稿件没有字幕：复用在线搜词链路补词（结果回来后覆盖下面的占位歌词）
            const bool biliNoSubtitle = source == kSourceBilibili && !lyricHash.isEmpty()
                                        && lyricHash == m_biliTrack.hash;
            if (biliNoSubtitle && onlineLyrics.isEmpty())
                findOnlineLyrics(m_biliTrack.title, m_biliTrack.artist, m_biliTrack.duration);

            // 只有「请求成功且确实没有歌词」才显示纯音乐占位
            if (onlineLyrics.isEmpty()) {
                QVariantMap line;
                line.insert(QStringLiteral("time"), 0);
                line.insert(QStringLiteral("text"), QStringLiteral("纯音乐，请欣赏"));
                onlineLyrics << line;
            }
            setLyricsData(onlineLyrics);
            setLyricsTranslate(d.value(QStringLiteral("translate")));

            if (!lyricHash.isEmpty() && m_pendingLocalLyrics.contains(lyricHash)) {
                const LocalLyricsRequest request = m_pendingLocalLyrics.take(lyricHash);
                const QVariantList lyrics = d.value(QStringLiteral("info")).toList();
                if (lyrics.isEmpty())
                    emit localLyricsFailed(request.filePath);
                else
                    emit localLyricsReady(request.filePath, lyrics,
                                          d.value(QStringLiteral("translate")).toList());
            }
        }

        // 歌词返回后发起下载
        const QString hash = lyricHash;
        if (!hash.isEmpty() && m_pendingDownloads.contains(hash)) {
            QVariantMap meta = m_pendingDownloads.take(hash);
            meta.insert(QStringLiteral("lyrics"), d.value(QStringLiteral("info")));
            meta.insert(QStringLiteral("translate"), d.value(QStringLiteral("translate")));
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
    // 接口回传的是实际请求的（可能是高清）hash；队列/收藏统一用普通 hash 当身份，
    // 否则同一首歌会因音质不同在队列里出现多条。
    const QString playHash = d.value(QStringLiteral("hash")).toString();
    // 快速连续切歌：B 的响应先回来并已起播，A 的响应后到。
    // 过期响应一旦走下去，会把音源切回 A，并按 A 的 hash 改队列索引
    if (type == 0 && isStalePlayResponse(playHash, source)) {
        qDebug() << "[api] 丢弃过期播放响应 hash:" << playHash.left(8)
                 << "当前代次 hash:" << m_playHash.left(8);
        return;
    }
    const QString identityHash = m_baseOf.value(playHash, playHash);
    if (playUrl.isEmpty()) { // 无可用地址：按原因提示（接口层已保证一定会回结果，这里必须给出提示）
        // 网络失败（超时/断网/解析失败）与「接口成功但确实没有地址」是两回事：
        // 前者提示检查网络，后者才是版权/会员问题
        const bool networkFailed = d.contains(QStringLiteral("error"));
        const QString reason = d.value(QStringLiteral("errReason")).toString();
        const bool kugouNotLoggedIn = source == kSourceKugou && m_account
                                      && !m_account->isKugouLoggedIn();
        QString msg;
        if (networkFailed)
            msg = QStringLiteral("网络请求失败，请检查网络后重试");
        else if (reason == QLatin1String("vip"))
            msg = kugouNotLoggedIn ? QStringLiteral("该歌曲需要 VIP 会员，请先登录酷狗账号")
                                   : QStringLiteral("该歌曲需要 VIP 会员或购买后才能播放");
        else if (kugouNotLoggedIn)
            msg = QStringLiteral("该歌曲暂无可用播放地址，可登录酷狗账号后重试");
        else
            msg = QStringLiteral("该歌曲受版权或会员限制，暂时无法获取播放地址");
        qDebug() << "[api] 无法播放:" << msg << "hash:" << playHash.left(8);
        emit warned(msg, 2);
        return;
    }
    // B 站稿件没有字幕时，要靠标题/歌手去其他平台搜词
    if (source == kSourceBilibili) {
        m_biliTrack.hash = playHash;
        m_biliTrack.title = d.value(QStringLiteral("songName")).toString();
        m_biliTrack.artist = d.value(QStringLiteral("author_name")).toString();
        m_biliTrack.duration = int(d.value(QStringLiteral("timeLength")).toDouble()); // 秒
    }
    if (type == 0) { // 播放
        QString cover = d.value(QStringLiteral("album_img")).toString();
        if (cover.contains(QLatin1String("{size}")))
            cover.replace(QLatin1String("{size}"), QLatin1String("512"));
        QString solve = d.value(QStringLiteral("album_img")).toString();
        if (solve.contains(QLatin1String("{size}")))
            solve.replace(QLatin1String("{size}"), QLatin1String("128"));
        // 秒 → 毫秒
        const double timeLength = d.value(QStringLiteral("timeLength")).toDouble();
        const int time = int(timeLength * (timeLength < 1000 ? 1000 : 1));
        emit urlplay(playUrl,
                     d.value(QStringLiteral("songName")).toString(),
                     d.value(QStringLiteral("author_name")).toString(),
                     cover, solve,
                     identityHash,
                     source);
        // 直接请求歌词（用实际请求的 hash，高清 hash 同样能取到歌词）
        // 记下这个歌词请求属于哪一代：歌词响应回来时要再校验一次代次
        if (m_playLyricGenerations.size() > 32) {
            for (auto it = m_playLyricGenerations.begin(); it != m_playLyricGenerations.end(); ) {
                if (it.value() != m_playGeneration)
                    it = m_playLyricGenerations.erase(it);
                else
                    ++it;
            }
        }
        m_playLyricGenerations.insert(playHash, m_playGeneration);
        getLyricInfo(playHash, time, source);
    } else if (type == 1) { // 下载
        // 秒 → 毫秒
        const double timeLength = d.value(QStringLiteral("timeLength")).toDouble();
        const int time = int(timeLength * (timeLength < 1000 ? 1000 : 1));

        QString cover = d.value(QStringLiteral("album_img")).toString();
        if (cover.contains(QLatin1String("{size}")))
            cover.replace(QLatin1String("{size}"), QLatin1String("512"));

        const QString hash = d.value(QStringLiteral("hash")).toString();

        QVariantMap meta;
        meta.insert(QStringLiteral("title"), d.value(QStringLiteral("songName")).toString());
        meta.insert(QStringLiteral("artist"), d.value(QStringLiteral("author_name")).toString());
        meta.insert(QStringLiteral("cover"), cover);
        meta.insert(QStringLiteral("duration"), int(timeLength)); // 秒
        meta.insert(QStringLiteral("hash"), hash);
        meta.insert(QStringLiteral("url"), playUrl);
        meta.insert(QStringLiteral("fileName"), d.value(QStringLiteral("fileName")).toString());

        // 先取歌词再下载
        m_pendingDownloads.insert(hash, meta);
        getLyricInfo(hash, time, source);
    }
}
