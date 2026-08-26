// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "MusicApiService.h"

#include <QDebug>
#include <QQmlEngine>
#include <QJSEngine>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

#include "../cpp/AccountManager.h"

namespace {
constexpr int kSourceKugou = 0;
constexpr int kSourceNetease = 1;

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

void MusicApiService::syncCookie(int source)
{
    if (!m_account)
        return;
    if (source == kSourceNetease)
        m_netease.setCookie(m_account->neteaseCookie());
    else if (source == kSourceKugou)
        m_kugou.setCookie(m_account->kugouCookie());
}

#define DISPATCH(source, expr)                       \
    do {                                              \
        const int s = resolve(source);                \
        syncCookie(s);                                \
        switch (s) {                                  \
        case kSourceNetease: m_netease.expr; break;   \
        case kSourceKugou:   m_kugou.expr; break;     \
        default:                                      \
            qWarning() << "[api] 未实现的平台:" << s;   \
        }                                             \
    } while (0)

// 统一请求入口（请求开始置 loadState=true）
void MusicApiService::searchSongs(const QString &keyword, int type, int page, int pageSize,
                                  int source)
{
    setLoadState(true);
    DISPATCH(source, searchSongs(keyword, type, page, pageSize));
}

void MusicApiService::getPlaylistMenu(int type, int source)
{
    setLoadState(true);
    DISPATCH(source, getPlaylistMenu(type));
}

void MusicApiService::getMenuInfo(const QString &id, int source)
{
    setLoadState(true);
    DISPATCH(source, getMenuInfo(id));
}

void MusicApiService::getMusicPlaylists(const QString &tagid, int page, int pageSize,
                                        int source)
{
    setLoadState(true);
    DISPATCH(source, getMusicPlaylists(tagid, page, pageSize));
}

void MusicApiService::getPlaylistSongs(const QString &listid, int page, int pageSize,
                                       int source)
{
    setLoadState(true);
    DISPATCH(source, getPlaylistSongs(listid, page, pageSize));
}

void MusicApiService::getRecommendSongs(int page, int pageSize, int source)
{
    setLoadState(true);
    DISPATCH(source, getRecommendSongs(page, pageSize));
}

void MusicApiService::getHotPlaylistMenu(int type, int source)
{
    setLoadState(true);
    DISPATCH(source, getHotPlaylistMenu(type));
}

void MusicApiService::getHotPlaylists(int page, int pageSize, int source)
{
    setLoadState(true);
    DISPATCH(source, getHotPlaylists(page, pageSize));
}

void MusicApiService::getNewSongs(int type, int page, int pageSize, int source)
{
    setLoadState(true);
    DISPATCH(source, getNewSongs(type, page, pageSize));
}

void MusicApiService::getAllToplist(int source)
{
    m_toplistList.clear();
    setLoadState(true);
    DISPATCH(source, getAllToplist());
}

// 同时拉取酷狗 + 网易云的榜单列表，合并进 toplistList（每条带 source）
void MusicApiService::getAllToplists()
{
    m_toplistList.clear();
    setLoadState(true);
    syncCookie(kSourceKugou);
    m_kugou.getAllToplist();
    syncCookie(kSourceNetease);
    m_netease.getAllToplist();
}

void MusicApiService::getMusicToplist(int page, int pageSize, int rankid, int source)
{
    setLoadState(true);
    DISPATCH(source, getMusicToplist(page, pageSize, rankid));
}

void MusicApiService::getHotSingers(int page, int pageSize, int source)
{
    setLoadState(true);
    DISPATCH(source, getHotSingers(page, pageSize));
}

void MusicApiService::getSingerCategory(int area, int page, int pageSize, int source)
{
    setLoadState(true);
    DISPATCH(source, getSingerCategory(area, page, pageSize));
}

void MusicApiService::getSingerSongs(const QString &singerid, int page, int pageSize,
                                     int source)
{
    setLoadState(true);
    DISPATCH(source, getSingerSongs(singerid, page, pageSize));
}

void MusicApiService::getMusicInfo(const QString &hash, int type, int source)
{
    setLoadState(true);
    DISPATCH(source, getMusicInfo(hash, type));
}

void MusicApiService::getPersonalFm(int page, int pageSize, int source)
{
    setLoadState(true);
    DISPATCH(source, getPersonalFm(page, pageSize));
}

void MusicApiService::getPersonalRadar(int page, int pageSize, int source)
{
    setLoadState(true);
    DISPATCH(source, getPersonalRadar(page, pageSize));
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

// 读取本地音频同目录同名 .json 元数据；不存在返回空 map
QVariantMap MusicApiService::readLocalMetadata(const QString &filePath)
{
    QVariantMap meta;
    if (filePath.isEmpty())
        return meta;

    QString localPath = QUrl::fromUserInput(filePath).toLocalFile();
    if (localPath.isEmpty())
        localPath = filePath;

    const QFileInfo fi(localPath);
    const QString jsonPath = fi.absolutePath() + QLatin1Char('/')
                             + fi.completeBaseName() + QStringLiteral(".json");
    if (!QFileInfo::exists(jsonPath))
        return meta;

    QFile f(jsonPath);
    if (!f.open(QIODevice::ReadOnly))
        return meta;

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return meta;

    return doc.object().toVariantMap();
}

// setter
void MusicApiService::setAllPlaylistMenu(const QVariant &v)
{
    if (m_allPlaylistMenu == v)
        return;
    m_allPlaylistMenu = v;
    emit allPlaylistMenuChanged();
}

void MusicApiService::setPlaylistmenuInfo(const QVariant &v)
{
    if (m_playlistmenuInfo == v)
        return;
    m_playlistmenuInfo = v;
    emit playlistmenuInfoChanged();
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
    const QVariantMap d = data.toMap();
    const QVariant info = d.contains(QStringLiteral("info"))
                              ? d.value(QStringLiteral("info"))
                              : data;

    if (action == QLatin1String("searchSongs")) {
        m_searchSongsResults.append(normalizeList(info));
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
    } else if (action == QLatin1String("getMenuInfo")) {
        setPlaylistmenuInfo(d);
        // 收到分类信息后自动拉取该分类下的歌单
        QVariant tid = d.value(QStringLiteral("special_tag_id"));
        if (!tid.isValid() || tid.toString().isEmpty())
            tid = d.value(QStringLiteral("tag_id"));
        if (!tid.isValid() || tid.toString().isEmpty())
            tid = d.value(QStringLiteral("tagid"));
        if (!tid.isValid() || tid.toString().isEmpty())
            return;
        // 网易云 top_playlist 的 cat 参数需要分类名（而非数字 id），
        // 从已缓存的分类列表 allPlaylistMenu 中按 id 反查分类名。
        QString playlistArg = tid.toString();
        if (source == 1) {
            const QVariantList menu = m_allPlaylistMenu.toList();
            for (const QVariant &v : menu) {
                const QVariantMap it = v.toMap();
                if (it.value(QStringLiteral("id")).toString() == tid.toString()) {
                    playlistArg = it.value(QStringLiteral("title")).toString();
                    break;
                }
            }
            if (playlistArg.isEmpty())
                playlistArg = tid.toString();
        }
        getMusicPlaylists(playlistArg, 1, 20, source);
    } else if (action == QLatin1String("getMusicPlaylists")) {
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
        m_playlistSong.clear();
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
        m_playlistSong.clear();
        m_playlistSong.append(normalizeList(info));
    } else if (action == QLatin1String("getPersonalFm")) {
        // 分页累积：第一页由 UI 侧 clear，后续页直接 append
        m_personalFm.append(normalizeList(info));
    } else if (action == QLatin1String("getPersonalRadar")) {
        m_personalRadar.append(normalizeList(info));
    } else if (action == QLatin1String("getMusicInfo")) {
        handleMusicInfo(d, source);
    } else if (action == QLatin1String("getLyricInfo")) {
        setLyricsData(d.value(QStringLiteral("info")));
        setLyricsTranslate(d.value(QStringLiteral("translate")));

        // 歌词返回后发起下载
        const QString hash = d.value(QStringLiteral("hash")).toString();
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
    setLoadState(false);
}

void MusicApiService::handleMusicInfo(const QVariantMap &d, int source)
{
    const int type = d.value(QStringLiteral("type")).toInt();
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
        emit urlplay(d.value(QStringLiteral("backup_url")).toString(),
                     d.value(QStringLiteral("songName")).toString(),
                     d.value(QStringLiteral("author_name")).toString(),
                     cover, solve,
                     d.value(QStringLiteral("hash")).toString(),
                     source);
        // 直接请求歌词
        getLyricInfo(d.value(QStringLiteral("hash")).toString(), time, source);
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
        meta.insert(QStringLiteral("url"), d.value(QStringLiteral("url")).toString());
        meta.insert(QStringLiteral("fileName"), d.value(QStringLiteral("fileName")).toString());

        // 先取歌词再下载
        m_pendingDownloads.insert(hash, meta);
        getLyricInfo(hash, time, source);
    }
}
