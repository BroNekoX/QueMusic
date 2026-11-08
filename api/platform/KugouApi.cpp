// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "KugouApi.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDateTime>
#include <QPointer>
#include <QRegularExpression>
#include <QThreadPool>
#include <QUrl>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <QJsonValue>
#include <QRandomGenerator>
#include <QStringList>
#include <vector>

#include "ApiCommon.h"
#include "ApiHttp.h"

extern "C" {
typedef unsigned char Bytef;
typedef unsigned long uLong;
typedef unsigned long uLongf;
int uncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
}
#ifndef Z_OK
#  define Z_OK 0
#endif
#ifndef Z_BUF_ERROR
#  define Z_BUF_ERROR (-5)
#endif
#ifndef Z_MEM_ERROR
#  define Z_MEM_ERROR (-4)
#endif

namespace {
const char *kUa = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                  "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";

const char *kAndroidUa = "Android15-1070-11083-46-0-DiscoveryDRADProtocol-wifi";

const unsigned char kKrcKey[16] = {64, 71, 97, 119, 94, 50, 116, 71,
                                   81, 54, 49, 45, 206, 210, 110, 105};

QString artistFromFilename(const QString &filename)
{
    const int i = filename.indexOf(QLatin1Char('-'));
    return i > 0 ? filename.left(i).trimmed() : QString();
}

QString titleFromFilename(const QString &filename, const QString &fallback = QString())
{
    const int i = filename.indexOf(QLatin1Char('-'));
    return i > 0 ? filename.mid(i + 1).trimmed() : fallback;
}

qint64 playCount(const QJsonObject &o)
{
    const qint64 v = qint64(o.value("playcount").toDouble());
    return v >= 4294967295 ? 0 : v;
}

QVariantList parsePlaylists(const QJsonArray &arr)
{
    QVariantList info;
    for (const QJsonValue &v : arr) {
        const QJsonObject s = v.toObject();
        QString cover = s.value("imgurl").toString();
        if (cover.contains("{size}"))
            cover.replace("{size}", "64");
        info << ApiCommon::song(
            s.value("specialname").toString(),
            s.value("username").toString(),
            cover,
            QString::number(s.value("specialid").toVariant().toLongLong()),
            s.value("songcount").toInt(),
            s.value("intro").toString(),
            QString(), QString(), 0,
            playCount(s));
    }
    return info;
}

QString hqHashFrom(const QJsonObject &song, const QString &base)
{
    if (song.value("pay_type_320").toInt() == 3)
        return base;
    const QString hq = song.value("320hash").toString();
    if (!hq.isEmpty())
        return hq;
    const QString ogg = song.value("trans_param")
                            .toObject()
                            .value("ogg_320_hash")
                            .toString();
    return ogg.isEmpty() ? base : ogg;
}

QString sqHashFrom(const QJsonObject &song, const QString &base)
{
    if (song.value("pay_type_sq").toInt() != 0)
        return base;
    const QString sq = song.value("sqhash").toString();
    return sq.isEmpty() ? base : sq;
}

int paytypeFromKugou(const QJsonObject &song)
{
    return song.value("pay_type").toInt() == 0 ? ApiCommon::kPaytypeFree
                                                              : ApiCommon::kPaytypePaid;
}

QString artistOfSong(const QJsonObject &song)
{
    const QJsonArray authors = song.value("authors").toArray();
    if (!authors.isEmpty()) {
        const QString name =
            authors.at(0).toObject().value("author_name").toString();
        if (!name.isEmpty())
            return name;
    }
    return song.value("singername").toString();
}

QString unionCover(const QJsonObject &song)
{
    return song.value("trans_param").toObject()
        .value("union_cover").toString();
}

// 列表接口的歌曲字段差异只剩 title / artist / cover，其余统一走这里。
QVariantMap makeSong(const QJsonObject &song, QString title, QString artist, QString cover)
{
    const QString hash = song.value("hash").toString();
    return ApiCommon::song(std::move(title), std::move(artist), std::move(cover), hash,
                           song.value("duration").toInt(),
                           song.value("album_name").toString(),
                           hqHashFrom(song, hash), sqHashFrom(song, hash),
                           paytypeFromKugou(song));
}

QByteArray zlibInflate(const QByteArray &data)
{
    if (data.isEmpty())
        return {};
    uLongf destLen = qMax<uLongf>(static_cast<uLongf>(data.size()) * 2, 1024);
    for (int attempt = 0; attempt < 5; ++attempt) {
        QByteArray out(int(destLen), Qt::Uninitialized);
        const int ret = uncompress(reinterpret_cast<Bytef *>(out.data()), &destLen,
                                   reinterpret_cast<const Bytef *>(data.constData()),
                                   static_cast<uLong>(data.size()));
        if (ret == Z_OK) {
            out.resize(int(destLen));
            return out;
        }
        if (ret == Z_BUF_ERROR || ret == Z_MEM_ERROR) {
            destLen = qMax(destLen * 2, destLen + 1024);
            continue;
        }
        qWarning() << "[krc] zlib uncompress 失败, code:" << ret;
        return {};
    }
    return {};
}

QByteArray rawDeflateInflate(const QByteArray &raw)
{
    QByteArray zlib;
    zlib.append(char(0x78));
    zlib.append(char(0x9C));
    zlib.append(raw);
    quint32 a = 1, b = 0;
    for (char c : raw) {
        a = (a + uchar(c)) % 65521;
        b = (b + a) % 65521;
    }
    const quint32 adler = (b << 16) | a;
    zlib.append(char((adler >> 24) & 0xFF));
    zlib.append(char((adler >> 16) & 0xFF));
    zlib.append(char((adler >> 8) & 0xFF));
    zlib.append(char(adler & 0xFF));
    return zlibInflate(zlib);
}

QByteArray inflateSmart(const QByteArray &data)
{
    QByteArray out = zlibInflate(data);
    if (!out.isEmpty())
        return out;
    out = rawDeflateInflate(data);
    if (out.isEmpty())
        qWarning() << "[krc] 歌词解压失败";
    return out;
}

QString playUrlOf(const QJsonObject &data, QString *backup)
{
    if (backup) {
        QString b = data.value("play_backup_url").toString();
        if (b.isEmpty()) {
            const QJsonValue bv = data.value("backup_url");
            b = bv.isArray() ? (bv.toArray().isEmpty() ? QString()
                                                       : bv.toArray().first().toString())
                             : bv.toString();
        }
        *backup = b;
    }
    return data.value("play_url").toString();
}
}

namespace {

class BigUint {
public:
    static BigUint fromBytes(const QByteArray &bytes)
    {
        BigUint v;
        v.digits.clear();
        v.digits.push_back(0);
        for (unsigned char c : bytes) {
            mulSmall(v, 256);
            addSmall(v, c);
        }
        return v;
    }

    QString toDecimalString() const
    {
        QString out;
        BigUint tmp = *this;
        while (!tmp.isZero()) {
            quint32 rem = 0;
            divSmall(tmp, 1000000000u, rem);
            out.prepend(QString::number(rem).rightJustified(9, QLatin1Char('0')));
        }
        QString s = out;
        s.remove(QRegularExpression("^0+(?=\\d)"));
        return s.isEmpty() ? "0" : s;
    }

private:
    std::vector<quint32> digits;

    static void mulSmall(BigUint &a, quint32 m)
    {
        quint64 carry = 0;
        for (size_t i = 0; i < a.digits.size(); ++i) {
            quint64 cur = (quint64)a.digits[i] * m + carry;
            a.digits[i] = (quint32)(cur & 0xFFFFFFFFu);
            carry = cur >> 32;
        }
        if (carry)
            a.digits.push_back((quint32)carry);
    }

    static void addSmall(BigUint &a, quint32 m)
    {
        quint64 carry = m;
        for (size_t i = 0; i < a.digits.size() && carry; ++i) {
            quint64 cur = (quint64)a.digits[i] + carry;
            a.digits[i] = (quint32)(cur & 0xFFFFFFFFu);
            carry = cur >> 32;
        }
        if (carry)
            a.digits.push_back((quint32)carry);
    }

    static void divSmall(BigUint &a, quint32 d, quint32 &rem)
    {
        quint64 r = 0;
        for (size_t i = a.digits.size(); i-- > 0; ) {
            quint64 cur = (r << 32) | a.digits[i];
            a.digits[i] = (quint32)(cur / d);
            r = cur % d;
        }
        while (!a.digits.empty() && a.digits.back() == 0)
            a.digits.pop_back();
        rem = (quint32)r;
    }

    bool isZero() const
    {
        return digits.empty() || (digits.size() == 1 && digits[0] == 0);
    }
};

QByteArray md5Hex(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}

QByteArray kugouSignature(const QByteArray &salt, const QStringList &keyValues)
{
    QStringList sorted = keyValues;
    sorted.sort();
    return md5Hex(salt + sorted.join(QString()).toUtf8() + salt);
}

QByteArray liteAndroidSignature(const QVariantMap &params)
{
    QStringList keyValues;
    keyValues.reserve(params.size());
    for (auto it = params.cbegin(); it != params.cend(); ++it)
        keyValues << (it.key() + QLatin1Char('=') + it.value().toString());
    return kugouSignature("LnT6xpN3khm36zse0QzvmgTZ3waWdRSA", keyValues);
}

QString liteSignKey(const QString &hash, const QString &appid, const QString &mid,
                    const QString &userid)
{
    return QString::fromLatin1(
        md5Hex((hash + "185672dd44712f60bb1736df5a377e82" + appid + mid + userid)
                   .toUtf8()));
}
}

QByteArray KugouApi::kugouWebSignature(const QJsonObject &params)
{
    QStringList keyValues;
    keyValues.reserve(params.size());
    for (const QString &key : params.keys())
        keyValues << (key + QLatin1Char('=') + QJsonValue(params.value(key)).toVariant().toString());
    return kugouSignature("NVPh5oo715z5DIWAeQlhMDsWXXQV4hwt", keyValues);
}

QByteArray KugouApi::kugouAndroidSignature(const QVariantMap &params)
{
    QStringList keyValues;
    keyValues.reserve(params.size());
    for (auto it = params.cbegin(); it != params.cend(); ++it)
        keyValues << (it.key() + QLatin1Char('=') + it.value().toString());
    return kugouSignature("OIlwieks28dk2k092lksi2UIkp", keyValues);
}

QString KugouApi::randomGuid()
{
    const char *hexChars = "0123456789abcdef";
    auto rndHex = [&](int len) {
        QString s;
        s.reserve(len);
        for (int i = 0; i < len; ++i)
            s.append(hexChars[QRandomGenerator::global()->bounded(16)]);
        return s;
    };
    const QString p3 = "4" + rndHex(3);
    const QString p4 = QString::number(8 + QRandomGenerator::global()->bounded(4)) + rndHex(3);
    return rndHex(8) + QLatin1Char('-') + rndHex(4) + QLatin1Char('-') + p3
           + QLatin1Char('-') + p4 + QLatin1Char('-') + rndHex(12);
}

QString KugouApi::kugouMidFromGuid(const QString &guid)
{
    const QByteArray digest = QCryptographicHash::hash(guid.toUtf8(), QCryptographicHash::Md5);
    const BigUint v = BigUint::fromBytes(digest);
    return v.toDecimalString();
}

KugouApi::KugouApi(QObject *parent)
    : MusicSource(ApiSource::Kugou, parent)
{
    m_nam = new QNetworkAccessManager(this);
}

void KugouApi::applyCredentials(const SourceCredentials &credentials)
{
    if (m_credentials == credentials)
        return;
    m_credentials = credentials;
    setQuality(credentials.quality);
    setDeviceInfo(credentials.deviceMid, credentials.deviceDfid);
    setCookie(credentials.cookie);
}

void KugouApi::onComments(const QString &hash, int page, int pageSize)
{
    const auto requestComments = [this, page, pageSize](const QString &mixId) {
        if (m_mid.isEmpty())
            m_mid = kugouMidFromGuid(randomGuid());
        QVariantMap p;
        p.insert("mixsongid", mixId);
        p.insert("need_show_image", "1");
        p.insert("p", QString::number(page));
        p.insert("pagesize", QString::number(pageSize));
        p.insert("show_classify", "1");
        p.insert("show_hotword_list", "1");
        p.insert("extdata", "0");
        p.insert("code", "fc4be23b4e972707f36b8a828a93ba8a");
        p.insert("dfid", "-");
        p.insert("mid", m_mid);
        p.insert("uuid", "-");
        p.insert("appid", "1005");
        p.insert("clientver", "20489");
        p.insert("clienttime", QString::number(QDateTime::currentSecsSinceEpoch()));
        p.insert("signature", QString::fromLatin1(kugouAndroidSignature(p)));

        QUrlQuery query;
        for (auto it = p.cbegin(); it != p.cend(); ++it)
            query.addQueryItem(it.key(), it.value().toString());
        QUrl url("https://gateway.kugou.com/mcomment/v1/cmtlist");
        url.setQuery(query);

        get(url.toString(), [this, pageSize](const QJsonObject &json) {
            QVariantList info;
            for (const QJsonValue &value : json.value("list").toArray()) {
                const QJsonObject c = value.toObject();
                info << ApiCommon::comment(
                    c.value("user_name").toString(),
                    c.value("user_pic").toString(),
                    c.value("content").toString(),
                    QDateTime::fromString(c.value("addtime").toString(),
                                          "yyyy-MM-dd HH:mm:ss")
                        .toSecsSinceEpoch(),
                    c.value("like").toObject().value("count").toInt(),
                    c.value("reply_num").toInt());
            }
            QVariantMap result = ApiCommon::listResult(info);

            result.insert("hasMore", info.size() >= pageSize);
            if (json.contains("error"))
                result.insert("error", json.value("error"));
            emit resultReady(ApiAction::Comments, result, Source);
        });
    };

    const QString cached = m_mixIdCache.value(hash);
    if (!cached.isEmpty()) {
        requestComments(cached);
        return;
    }
    get("https://m.kugou.com/app/i/getSongInfo.php?cmd=playInfo&hash=" + hash,
        [this, hash, requestComments](const QJsonObject &meta) {
            const QString mixId = meta.value("album_audio_id").toVariant().toString();
            if (mixId.isEmpty() || mixId == QLatin1String("0")) {
                emit resultReady(ApiAction::Comments, ApiCommon::listResult({}), Source);
                return;
            }
            if (m_mixIdCache.size() > 2000)
                m_mixIdCache.clear();
            m_mixIdCache.insert(hash, mixId);
            requestComments(mixId);
        });
}

void KugouApi::get(const QString &url, const Callback &cb)
{
    ApiHttp::get(m_nam, this, QUrl(url), kUa, "https://www.kugou.com/", m_cookie.toUtf8(),
                 [cb](const QByteArray &body, QNetworkReply::NetworkError error) {

                     QByteArray data = body;
                     data.replace("<!--KG_TAG_RES_START-->", "").replace("<!--KG_TAG_RES_END-->", "");
                     QJsonObject object = QJsonDocument::fromJson(data).object();
                     if (error != QNetworkReply::NoError)
                         object.insert("error", int(error));
                     cb(object);
                 });
}

void KugouApi::getLite(const QString &url, const Callback &cb)
{
    const QByteArray clienttime = QByteArray::number(QDateTime::currentSecsSinceEpoch());
    const QList<QPair<QByteArray, QByteArray>> headers{
        { QByteArrayLiteral("dfid"), m_dfid.toUtf8() },
        { QByteArrayLiteral("clienttime"), clienttime },
        { QByteArrayLiteral("mid"), m_mid.toUtf8() },
        { QByteArrayLiteral("kg-rc"), QByteArrayLiteral("1") },
        { QByteArrayLiteral("kg-thash"), QByteArrayLiteral("5d816a0") },
        { QByteArrayLiteral("kg-rec"), QByteArrayLiteral("1") },
        { QByteArrayLiteral("kg-rf"), QByteArrayLiteral("B9EDA08A64250DEFFBCADDEE00F8F25F") },
    };
    ApiHttp::get(m_nam, this, QUrl(url), kAndroidUa, "https://www.kugou.com/",
                 m_cookie.toUtf8(),
                 [cb](const QByteArray &body, QNetworkReply::NetworkError error) {
                     QJsonObject object = QJsonDocument::fromJson(body).object();
                     if (error != QNetworkReply::NoError)
                         object.insert("error", int(error));
                     cb(object);
                 },
                 headers);
}

QString KugouApi::decodeKrc(const QByteArray &base64)
{
    QByteArray bytes = QByteArray::fromBase64(base64);
    if (bytes.size() <= 4) {
        qWarning() << "[krc] 数据太短，无法解码";
        return {};
    }
    bytes = bytes.mid(4);

    QByteArray decrypted(bytes.size(), Qt::Uninitialized);
    for (int i = 0; i < bytes.size(); ++i)
        decrypted[i] = char(uchar(bytes.at(i)) ^ kKrcKey[i % 16]);

    const int offsets[] = {0, 4, 2};
    for (int skip : offsets) {
        if (decrypted.size() <= skip + 4)
            continue;
        const QByteArray inflated = inflateSmart(decrypted.mid(skip));
        if (!inflated.isEmpty())
            return QString::fromUtf8(inflated);
    }

    qWarning() << "[krc] 所有偏移都解压失败！";
    return {};
}

QVariantList KugouApi::krcToLyrics(const QString &krc)
{
    QVariantList result;
    static const QRegularExpression lineRe("^\\[(\\d+),(\\d+)\\](.*)$");
    static const QRegularExpression wordRe("<(\\d+),(\\d+),\\d+>([^<]*)");

    const QStringList lines = krc.split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty())
            continue;
        const QRegularExpressionMatch m = lineRe.match(line);
        if (!m.hasMatch())
            continue;

        QString rawContent = m.captured(3).trimmed();

        const bool isOther = rawContent.endsWith("）");

        QVariantList words;
        QString fullText;
        QRegularExpressionMatchIterator it = wordRe.globalMatch(m.captured(3));
        while (it.hasNext()) {
            const QRegularExpressionMatch w = it.next();
            QString text = w.captured(3);
            if (text.isEmpty())
                continue;
            if (isOther) {
                text.remove("（");
                text.remove("）");
            }
            words << QVariantMap{
                {"offset", w.captured(1).toInt()},
                {"duration", w.captured(2).toInt()},
                {"text", text},
            };
            fullText += text;
        }
        if (words.isEmpty())
            continue;
        result << QVariantMap{
            {"time", m.captured(1).toInt()},
            {"text", fullText},
            {"info", words},
            {"isOther", isOther}
        };
    }
    std::sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value("time").toInt()
               < b.toMap().value("time").toInt();
    });
    return result;
}

QVariantList KugouApi::krcTranslations(const QString &krc)
{
    static const QRegularExpression re("\\[language:([^\\]]+)\\]");
    const QRegularExpressionMatch m = re.match(krc);
    if (!m.hasMatch())
        return {};

    const QJsonObject obj =
        QJsonDocument::fromJson(QByteArray::fromBase64(m.captured(1).toLatin1())).object();
    const QJsonArray content = obj.value("content").toArray();
    if (content.isEmpty())
        return {};
    QJsonObject c = content.first().toObject();
    if (c.value("type").toInt() != 1 && content.size() > 1)
        c = content.at(1).toObject();

    QVariantList result;
    for (const QJsonValue &v : c.value("lyricContent").toArray()) {
        const QJsonArray item = v.toArray();

        const QString text = item.isEmpty() ? QString() : item.first().toString();
        result << (text.trimmed().isEmpty() ? QString() : text);
    }
    return result;
}

void KugouApi::onSearchSongs(const QString &keyword, int type, int page, int pageSize)
{
    performSearch(keyword, type, page, pageSize, ApiAction::SearchSongs);
}

void KugouApi::onSearchLyrics(const QString &keyword, int page, int pageSize)
{
    performSearch(keyword, 0, page, qMax(pageSize, 1), ApiAction::SearchLyrics);
}

void KugouApi::performSearch(const QString &keyword, int type, int page, int pageSize,
                             const QString &resultAction)
{
    QUrl url;
    switch (type) {
    case 0:
        url = QUrl("http://mobilecdn.kugou.com/api/v3/search/song");
        break;
    case 1:
        url = QUrl("http://mobilecdnbj.kugou.com/api/v3/search/special");
        break;
    case 2:
        url = QUrl("http://msearch.kugou.com/api/v3/search/album");
        break;
    default:
        url = QUrl("http://mobileservice.kugou.com/api/v3/lyric/search");
        break;
    }
    QUrlQuery q;
    q.addQueryItem("pagesize", QString::number(pageSize));
    q.addQueryItem("page", QString::number(page));
    q.addQueryItem("keyword", keyword);
    if (type == 1) {
        q.addQueryItem("version", "9108");
        q.addQueryItem("highlight", "em");
        q.addQueryItem("filter", "0");
        q.addQueryItem("sver", "2");
        q.addQueryItem("with_res_tag", "1");
    } else if (type == 2) {
        q.addQueryItem("version", "9108");
        q.addQueryItem("iscorrection", "1");
        q.addQueryItem("highlight", "em");
        q.addQueryItem("plat", "0");
        q.addQueryItem("sver", "2");
        q.addQueryItem("with_res_tag", "1");
    } else if (type == 3) {
        q.addQueryItem("version", "9108");
        q.addQueryItem("highlight", "1");
        q.addQueryItem("plat", "0");
        q.addQueryItem("area_code", "1");
        q.addQueryItem("with_res_tag", "1");
    } else {
        q.addQueryItem("format", "json");
    }
    url.setQuery(q);

    get(url.toString(), [this, type, resultAction](const QJsonObject &json) {
        const QJsonArray arr = json.value("data").toObject()
                                   .value("info").toArray();
        QVariantList info;
        if (type == 0) {
            for (const QJsonValue &v : arr) {
                const QJsonObject s = v.toObject();
                const QJsonObject tp = s.value("trans_param").toObject();
                info << makeSong(s, s.value("songname").toString(),
                                s.value("singername").toString(),
                                tp.value("union_cover").toString());
            }
        } else if (type == 1) {
            for (const QJsonValue &v : arr) {
                const QJsonObject s = v.toObject();
                info << ApiCommon::song(
                    s.value("specialname").toString(),
                    s.value("nickname").toString(),
                    s.value("imgurl").toString(),
                    QString::number(s.value("specialid").toVariant().toLongLong()),
                    s.value("songcount").toInt(),
                    s.value("intro").toString(),
                    QString(), QString(), 0,
                    playCount(s));
            }
        } else if (type == 2) {
            for (const QJsonValue &v : arr) {
                const QJsonObject a = v.toObject();
                info << ApiCommon::song(
                    a.value("albumname").toString(),
                    a.value("singername").toString(),
                    a.value("imgurl").toString(),
                    QString::number(a.value("albumid").toVariant().toLongLong()),
                    a.value("songcount").toInt(),
                    a.value("albumname").toString());
            }
        } else if (type == 3) {
            for (const QJsonValue &v : arr) {
                const QJsonObject s = v.toObject();
                const QString filename = s.value("filename").toString();
                const QString title = titleFromFilename(filename, s.value("songname").toString());
                const QString artist = artistFromFilename(filename);
                const QJsonObject tp = s.value("trans_param").toObject();
                info << makeSong(s,
                                title.isEmpty()
                                    ? s.value("songname").toString() : title,
                                artist.isEmpty()
                                    ? s.value("singername").toString() : artist,
                                tp.value("union_cover").toString());
            }
        }
        emit resultReady(resultAction, ApiCommon::listResult(info), Source);
    });
}

void KugouApi::onPlaylistMenu(int type)
{
    Q_UNUSED(type);
    QUrl url("http://mobilecdnbj.kugou.com/api/v3/category/list");
    QUrlQuery q;
    q.addQueryItem("plat", "0");
    q.addQueryItem("apiver", "2");
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value("data").toObject()
                                     .value("info").toArray()) {
            const QJsonObject c = v.toObject();
            const QString id = QString::number(c.value("categoryid").toInt());
            info << QVariantMap{
                {"title", c.value("categoryname").toString()},
                {"id", id},
                {"tagid", id},
                {"category", id},
                {"cover", c.value("imgurl").toString()},
            };
        }
        emit resultReady(ApiAction::PlaylistMenu,
                         ApiCommon::listResult(info), Source);
    });
}

void KugouApi::onCategoryPlaylists(const QString &categoryid, int page, int pageSize)
{
    QUrl url("http://mobilecdnbj.kugou.com/api/v3/category/special");
    QUrlQuery q;
    q.addQueryItem("withsong", "1");
    q.addQueryItem("sort", "1");
    q.addQueryItem("plat", "0");
    q.addQueryItem("ugc", "1");
    q.addQueryItem("categoryid", categoryid);
    q.addQueryItem("page", QString::number(qMax(page, 1)));
    q.addQueryItem("pagesize", QString::number(qMax(pageSize, 1)));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        emit resultReady(ApiAction::CategoryPlaylists,
                         ApiCommon::listResult(parsePlaylists(
                             json.value("data").toObject()
                                 .value("info").toArray())),
                         Source);
    });
}

void KugouApi::onMusicPlaylists(const QString &tagid, int page, int pageSize)
{
    QUrl url("http://mobilecdnbj.kugou.com/api/v3/tag/specialList");
    QUrlQuery q;
    q.addQueryItem("plat", "0");
    q.addQueryItem("ugc", "1");
    q.addQueryItem("sort", "2");
    q.addQueryItem("pagesize", QString::number(pageSize));
    q.addQueryItem("page", QString::number(page));
    q.addQueryItem("tagid", tagid);
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        emit resultReady(ApiAction::MusicPlaylists,
                         ApiCommon::listResult(parsePlaylists(
                             json.value("data").toObject()
                                 .value("info").toArray())),
                         Source);
    });
}

void KugouApi::onPlaylistSongs(const QString &listid, int page, int pageSize)
{
    QUrl url("http://mobilecdnbj.kugou.com/api/v3/special/song");
    QUrlQuery q;
    q.addQueryItem("plat", "0");
    q.addQueryItem("version", "9108");
    q.addQueryItem("with_res_tag", "1");
    q.addQueryItem("pagesize", QString::number(pageSize));
    q.addQueryItem("page", QString::number(page));
    q.addQueryItem("specialid", listid);
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value("data").toObject()
                                     .value("info").toArray()) {
            const QJsonObject s = v.toObject();
            const QJsonObject tp = s.value("trans_param").toObject();

            const QString filename = s.value("filename").toString();
            info << makeSong(s, titleFromFilename(filename, filename),
                             artistFromFilename(filename),
                             tp.value("union_cover").toString());
        }
        emit resultReady(ApiAction::PlaylistSongs,
                         ApiCommon::listResult(info), Source);
    });
}

void KugouApi::onRecommendSongs(int page, int pageSize)
{
    requestNewSongs(1, page, pageSize, ApiAction::RecommendSongs, false);
}

void KugouApi::onNewSongs(int type, int page, int pageSize)
{
    requestNewSongs(type, page, pageSize, ApiAction::NewSongs, true);
}

void KugouApi::requestNewSongs(int type, int page, int pageSize, const QString &action,
                               bool joinAuthors)
{
    QUrl url("http://mobilecdnbj.kugou.com/api/v3/rank/newsong");
    QUrlQuery q;
    q.addQueryItem("format", "json");
    q.addQueryItem("version", "9108");
    q.addQueryItem("plat", "0");
    q.addQueryItem("with_cover", "1");
    q.addQueryItem("area_code", "1");
    q.addQueryItem("with_res_tag", "1");
    q.addQueryItem("pagesize", QString::number(pageSize));
    q.addQueryItem("page", QString::number(page));
    q.addQueryItem("type", QString::number(type));
    url.setQuery(q);

    get(url.toString(), [this, action, joinAuthors](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value("data").toObject()
                                     .value("info").toArray()) {
            const QJsonObject s = v.toObject();
            QString artist;
            if (joinAuthors) {
                const QJsonArray authors = s.value("authors").toArray();
                if (authors.size() > 1) {
                    artist = authors.at(0).toObject()
                                 .value("author_name").toString()
                             + QLatin1Char(',')
                             + authors.at(1).toObject()
                                   .value("author_name").toString();
                } else if (authors.size() == 1) {
                    artist = authors.at(0).toObject()
                                 .value("author_name").toString();
                }
            }
            if (artist.isEmpty())
                artist = s.value("singername").toString();

            info << makeSong(s, s.value("songname").toString(), artist,
                             unionCover(s));
        }
        emit resultReady(action, ApiCommon::listResult(info), Source);
    });
}

void KugouApi::onHotPlaylistMenu(int type)
{
    QUrl url("http://mobilecdnbj.kugou.com/api/v3/tag/recommend");
    QUrlQuery q;
    q.addQueryItem("apiver", "2");
    q.addQueryItem("plat", "0");
    q.addQueryItem("showtype", QString::number(type));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value("data").toObject()
                                     .value("info").toArray()) {
            const QJsonObject t = v.toObject();
            info << QVariantMap{
                {"title", t.value("name").toString()},
                {"id", t.value("id").toVariant().toLongLong()},
                {"tagid", t.value("special_tag_id").toVariant().toLongLong()},
                {"cover", t.value("bannerurl").toString()},
            };
        }
        emit resultReady(ApiAction::HotPlaylistMenu,
                         ApiCommon::listResult(info), Source);
    });
}

void KugouApi::onHotPlaylists(int page, int pageSize)
{
    static const QStringList kKeywords{
        QStringLiteral("热门"), QStringLiteral("古风"), QStringLiteral("粤语"),
        QStringLiteral("摇滚"), QStringLiteral("车载"), QStringLiteral("电音"),
        QStringLiteral("民谣"), QStringLiteral("爵士")};
    constexpr int kPagesPerKeyword = 20;
    const int index = qMax(page, 1) - 1;

    QUrl url("http://mobilecdnbj.kugou.com/api/v3/search/special");
    QUrlQuery q;
    q.addQueryItem("plat", "0");
    q.addQueryItem("keyword", kKeywords.at(index % kKeywords.size()));
    q.addQueryItem("page",
                   QString::number(index / kKeywords.size() % kPagesPerKeyword + 1));
    q.addQueryItem("pagesize", QString::number(qMax(pageSize, 1)));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value("data").toObject()
                                     .value("info").toArray()) {
            const QJsonObject s = v.toObject();
            const QString creator = s.value("nickname").toString();
            info << ApiCommon::song(
                s.value("specialname").toString(),
                creator,
                s.value("imgurl").toString(),
                QString::number(s.value("specialid").toVariant().toLongLong()),
                s.value("songcount").toInt(),
                creator,
                QString(), QString(), 0,
                playCount(s));
        }
        emit resultReady(ApiAction::HotPlaylists,
                         ApiCommon::listResult(info), Source);
    });
}


void KugouApi::onAllToplist()
{

    QUrl url("http://mobilecdnbj.kugou.com/api/v3/rank/list");
    QUrlQuery q;
    q.addQueryItem("version", "9108");
    q.addQueryItem("plat", "0");
    q.addQueryItem("withsong", "0");
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value("data").toObject()
                                     .value("info").toArray()) {
            const QJsonObject t = v.toObject();
            const QJsonObject total = t.value("extra").toObject().value("resp").toObject();
            info << ApiCommon::song(
                t.value("rankname").toString(),
                t.value("update_frequency").toString(),
                t.value("imgurl").toString(),
                QString::number(t.value("rankid").toVariant().toLongLong()),
                total.value("all_total").toInt(),
                t.value("intro").toString());
        }

        if (info.isEmpty()) {
            info << ApiCommon::song(QStringLiteral("酷狗飙升榜"), QString(), QString(),
                                    "6666");
            info << ApiCommon::song(QStringLiteral("酷狗TOP500"), QString(), QString(),
                                    "8888");
        }
        emit resultReady(ApiAction::AllToplist,
                         ApiCommon::listResult(info), Source);
    });
}

void KugouApi::onMusicToplist(int page, int pageSize, int rankid)
{
    requestRankSongs(rankid, page, pageSize, ApiAction::MusicToplist);
}

void KugouApi::requestRankSongs(int rankid, int page, int pageSize, const QString &action)
{
    QUrl url("http://mobilecdnbj.kugou.com/api/v3/rank/song");
    QUrlQuery q;
    q.addQueryItem("version", "9108");
    q.addQueryItem("ranktype", "2");
    q.addQueryItem("plat", "0");
    q.addQueryItem("pagesize", QString::number(pageSize));
    q.addQueryItem("page", QString::number(page));
    q.addQueryItem("area_code", "1");
    q.addQueryItem("rankid", QString::number(rankid));
    q.addQueryItem("with_res_tag", "1");
    url.setQuery(q);

    get(url.toString(), [this, action](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value("data").toObject()
                                     .value("info").toArray()) {
            const QJsonObject s = v.toObject();
            info << makeSong(s, s.value("songname").toString(),
                             artistOfSong(s), unionCover(s));
        }
        emit resultReady(action, ApiCommon::listResult(info), Source);
    });
}

void KugouApi::onHotSingers(int page, int pageSize)
{
    QUrl url("http://mobilecdnbj.kugou.com/api/v3/singer/rank");
    QUrlQuery q;
    q.addQueryItem("type", "1");
    q.addQueryItem("page", QString::number(page));
    q.addQueryItem("pagesize", QString::number(pageSize));
    q.addQueryItem("version", "9108");
    q.addQueryItem("plat", "0");
    url.setQuery(q);
    requestSingerList(url, ApiAction::HotSingers);
}

void KugouApi::onSingerCategory(int area, int page, int pageSize)
{
    QUrl url("http://mobilecdnbj.kugou.com/api/v3/singer/list");
    QUrlQuery q;
    q.addQueryItem("type", QString::number(area));
    q.addQueryItem("sex", "0");
    q.addQueryItem("page", QString::number(page));
    q.addQueryItem("pagesize", QString::number(pageSize));
    q.addQueryItem("version", "9108");
    q.addQueryItem("plat", "0");
    url.setQuery(q);
    requestSingerList(url, ApiAction::SingerCategory);
}

void KugouApi::requestSingerList(const QUrl &url, const QString &action)
{
    get(url.toString(), [this, action](const QJsonObject &json) {
        QVariantList info;

        QJsonArray arr = json.value("data").toObject()
                             .value("singers").toArray();
        if (arr.isEmpty())
            arr = json.value("data").toObject()
                      .value("info").toArray();
        for (const QJsonValue &v : arr) {
            const QJsonObject s = v.toObject();
            info << ApiCommon::song(
                s.value("singername").toString(),
                QString(),
                s.value("imgurl").toString(),
                QString::number(s.value("singerid").toVariant().toLongLong()));
        }
        emit resultReady(action, ApiCommon::listResult(info), Source);
    });
}

void KugouApi::onSingerSongs(const QString &singerid, int page, int pageSize)
{
    QUrl url("http://mobilecdnbj.kugou.com/api/v3/singer/song");
    QUrlQuery q;
    q.addQueryItem("singerid", singerid);
    q.addQueryItem("page", QString::number(page));
    q.addQueryItem("pagesize", QString::number(pageSize));
    q.addQueryItem("version", "9108");
    q.addQueryItem("plat", "0");
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;

        QJsonArray arr = json.value("data").toObject()
                             .value("songs").toArray();
        if (arr.isEmpty())
            arr = json.value("data").toObject()
                      .value("info").toArray();
        for (const QJsonValue &v : arr) {
            const QJsonObject s = v.toObject();
            const QJsonObject tp = s.value("trans_param").toObject();
            const QString filename = s.value("filename").toString();
            info << makeSong(s, titleFromFilename(filename, filename),
                             artistFromFilename(filename),
                             tp.value("union_cover").toString());
        }
        emit resultReady(ApiAction::SingerSongs,
                         ApiCommon::listResult(info), Source);
    });
}

void KugouApi::onMusicInfo(const QString &hash, int type)
{
    if (hash.isEmpty()) {
        qWarning() << "[kugou] getMusicInfo: hash 为空";
        QVariantMap data;
        data.insert("hash", hash);
        data.insert("type", type);
        data.insert("errReason", "unavailable");
        emit resultReady(ApiAction::MusicInfo, data, Source);
        return;
    }
    auto st = QSharedPointer<PlayRequest>::create();
    requestPlayMeta(hash, type, st);
    requestTrackerUrl(hash, type, st);
}

void KugouApi::requestPlayMeta(const QString &hash, int type, const QSharedPointer<PlayRequest> &st)
{
    QUrl url("https://m.kugou.com/app/i/getSongInfo.php?cmd=playInfo&hash="
             + hash);
    get(url.toString(), [this, hash, type, st](const QJsonObject &json) {

        QJsonObject d = json.value("data").toObject();
        if (d.isEmpty())
            d = json;

        const QJsonValue bv = d.value("backup_url");
        QString backupUrl;
        if (bv.isArray()) {
            const QJsonArray backupArr = bv.toArray();
            if (!backupArr.isEmpty())
                backupUrl = backupArr.first().toString();
        } else {
            backupUrl = bv.toString();
        }
        const QString playUrl = d.value("url").toString();
        QString albumImg = d.value("trans_param").toObject()
                               .value("union_cover").toString();
        if (albumImg.isEmpty())
            albumImg = d.value("album_img").toString();
        if (albumImg.isEmpty())
            albumImg = d.value("imgUrl").toString();
        const QString songName = d.value("songName").toString();
        const QString fileNameRaw = d.value("fileName").toString();
        const QString ext = d.value("extName").toString();
        QString fileName = fileNameRaw;
        if (!fileName.isEmpty() && !ext.isEmpty())
            fileName += QLatin1Char('.') + ext;
        if (fileName.isEmpty())
            fileName = songName + ".mp3";

        QVariantMap data;
        data.insert("songName", songName.isEmpty() ? fileNameRaw : songName);
        data.insert("author_name", d.value("author_name").toString());
        data.insert("singer_img", d.value("imgUrl").toString());
        data.insert("album_img", albumImg);
        data.insert("timeLength",
                    d.value("timeLength").toDouble()
                        ? d.value("timeLength").toDouble()
                        : d.value("duration").toDouble());
        data.insert("fileName", fileName);

        if (json.contains("error"))
            data.insert("error", json.value("error"));

        st->meta = data;
        st->metaUrl = playUrl;
        st->metaBackupUrl = backupUrl;

        st->albumId = d.value("albumid").toVariant().toString();
        if (st->albumId.isEmpty())
            st->albumId = d.value("req_albumid").toVariant().toString();
        st->albumAudioId = d.value("album_audio_id").toVariant().toString();
        st->metaDone = true;
        finishPlayInfo(hash, type, st);
    });
}

void KugouApi::requestTrackerUrl(const QString &hash, int type, const QSharedPointer<PlayRequest> &st)
{
    const QString key = QString::fromLatin1(
        QCryptographicHash::hash((hash + "kgcloudv2").toUtf8(),
                                 QCryptographicHash::Md5).toHex());
    QUrl url("https://trackercdn.kugou.com/i/v2/");
    QUrlQuery q;
    q.addQueryItem("key", key);
    q.addQueryItem("hash", hash);
    q.addQueryItem("br", "hq");
    q.addQueryItem("appid", "1005");
    q.addQueryItem("pid", "2");
    q.addQueryItem("cmd", "25");
    q.addQueryItem("behavior", "play");
    url.setQuery(q);

    get(url.toString(), [this, hash, type, st](const QJsonObject &json) {
        const int status = json.value("status").toInt();
        const QJsonValue uv = json.value("url");
        QString u;
        if (uv.isArray()) {
            const QJsonArray arr = uv.toArray();
            if (!arr.isEmpty())
                u = arr.first().toString();
        } else {
            u = uv.toString();
        }
        st->trackerStatus = status;
        if (status == 1 && !u.isEmpty()) {
            st->trackerUrl = u;
            st->trackerExt = json.value("extName").toString();
            st->trackerRate = json.value("bitRate").toInt();
        }
        qDebug() << "[kugou] trackercdn 取址 hash:" << hash.left(8)
                 << "status:" << status << "bitRate:" << st->trackerRate
                 << "ext:" << st->trackerExt;
        st->trackerDone = true;
        finishPlayInfo(hash, type, st);
    });
}

void KugouApi::finishPlayInfo(const QString &hash, int type, const QSharedPointer<PlayRequest> &st)
{
    if (!st->metaDone || !st->trackerDone)
        return;

    const QString reason = st->trackerStatus == 2 ? "vip"
                                                 : "unavailable";
    if (cookieValue({"token", "t"}).isEmpty()) {
        finishWithFallback(hash, type, st, reason);
        return;
    }

    qInfo() << "[kugou] 先试 VIP 取址（概念版 → web 签名）hash:" << hash.left(8);
    requestLitePlayInfo(hash, type, st, reason);
}

void KugouApi::finishWithFallback(const QString &hash, int type,
                                  const QSharedPointer<PlayRequest> &st, const QString &reason,
                                  int transportError)
{
    QVariantMap data = st->meta;
    const QString fallbackUrl = st->metaUrl.isEmpty() ? st->metaBackupUrl : st->metaUrl;
    const QString freeUrl = st->trackerUrl.isEmpty() ? fallbackUrl : st->trackerUrl;

    if (!st->trackerUrl.isEmpty() && !st->trackerExt.isEmpty()) {
        QString fileName = data.value("fileName").toString();
        const int dot = fileName.lastIndexOf(QLatin1Char('.'));
        if (dot > 0)
            fileName = fileName.left(dot) + QLatin1Char('.') + st->trackerExt;
        data.insert("fileName", fileName);
    }
    data.insert("url", freeUrl);
    data.insert("backup_url", fallbackUrl);
    data.insert("hash", hash);
    data.insert("type", type);

    if (transportError != 0)
        data.insert("error", transportError);
    if (freeUrl.isEmpty()) {
        data.insert("errReason", reason);
        qWarning() << "[kugou] 所有取址通道均无地址（VIP/付费或版权限制）hash:" << hash.left(8);
    }
    emit resultReady(ApiAction::MusicInfo, data, Source);
}

QString KugouApi::cookieValue(const QStringList &keys) const
{
    const QStringList pairs = m_cookie.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &p : pairs) {
        const int i = p.indexOf(QLatin1Char('='));
        if (i <= 0)
            continue;
        const QString name = p.left(i).trimmed();
        for (const QString &key : keys) {
            if (name.compare(key, Qt::CaseInsensitive) == 0)
                return p.mid(i + 1).trimmed();
        }
    }
    return QString();
}

QJsonObject KugouApi::songInfoBaseParams() const
{

    const QString mid = m_mid.isEmpty() ? kugouMidFromGuid(randomGuid()) : m_mid;
    QJsonObject params;
    params.insert("appid", 1014);
    params.insert("platid", 4);
    params.insert("srcappid", 2919);
    params.insert("clientver", 20000);
    params.insert("clienttime",
                  QString::number(QDateTime::currentMSecsSinceEpoch()));
    params.insert("mid", mid);
    params.insert("uuid", mid);
    params.insert("dfid", m_dfid.isEmpty() ? "-" : m_dfid);
    params.insert("token",
                  cookieValue({"token", "t"}));
    const QString userid = cookieValue({"userid", "user_id",
                                        "KugooID"});
    params.insert("userid",
                  userid.isEmpty() ? "0" : userid);
    return params;
}

void KugouApi::requestSignedPlayInfo(const QString &hash, int type,
                                     const QSharedPointer<PlayRequest> &st, const QString &reason)
{
    QJsonObject params = songInfoBaseParams();
    params.insert("hash", hash);
    requestSongInfo(params, hash, type, st, reason);
}

void KugouApi::requestSongInfo(const QJsonObject &params, const QString &hash, int type,
                               const QSharedPointer<PlayRequest> &st, const QString &reason,
                               const QJsonObject &metaFallback)
{
    const bool secondStep = params.contains("encode_album_audio_id");
    QJsonObject signedParams = params;
    signedParams.insert("signature",
                        QString::fromLatin1(kugouWebSignature(params)));

    QUrlQuery q;
    const QStringList keys = signedParams.keys();
    for (const QString &k : keys)
        q.addQueryItem(k, signedParams.value(k).toVariant().toString());
    QUrl url("https://wwwapi.kugou.com/play/songinfo");
    url.setQuery(q);

    get(url.toString(),
        [this, hash, type, st, reason, secondStep, metaFallback](const QJsonObject &json) {
            const QJsonObject d = json.value("data").toObject();

            const auto pick = [&](const char *key) {
                const QString v = d.value(QLatin1String(key)).toString();
                return v.isEmpty() ? metaFallback.value(QLatin1String(key)).toString() : v;
            };
            double tl = d.value("timelength").toDouble();
            if (tl <= 0)
                tl = metaFallback.value("timelength").toDouble();
            const int timeLength = int(tl / 1000);

            QString backup;
            const QString playUrl = playUrlOf(d, &backup);
            if (playUrl.isEmpty() && backup.isEmpty()) {
                const QString encodeId =
                    d.value("encode_album_audio_id").toString();
                if (!secondStep && !encodeId.isEmpty()) {

                    qInfo() << "[kugou] songinfo 第一步返回 encode_album_audio_id，继续取址"
                            << hash.left(8);
                    QJsonObject next = songInfoBaseParams();
                    next.insert("encode_album_audio_id", encodeId);
                    requestSongInfo(next, hash, type, st, reason, d);
                    return;
                }
                qWarning() << "[kugou] 签名取址无播放地址 step:" << (secondStep ? 2 : 1)
                           << "err_code:" << json.value("err_code").toInt();
                finishWithFallback(hash, type, st, reason,
                                   json.value("error").toInt());
                return;
            }

            const QString songName = pick("song_name");
            QString fileName = pick("audio_name");
            if (fileName.isEmpty())
                fileName = songName + ".mp3";

            QVariantMap data;
            data.insert("backup_url", playUrl.isEmpty() ? backup : playUrl);
            data.insert("url", playUrl.isEmpty() ? backup : playUrl);
            data.insert("songName", songName);
            data.insert("author_name", pick("author_name"));
            data.insert("singer_img", pick("img"));
            data.insert("album_img", pick("img"));
            data.insert("timeLength", timeLength);
            data.insert("fileName", fileName);
            data.insert("hash", hash);
            data.insert("type", type);
            emit resultReady(ApiAction::MusicInfo, data, Source);
        });
}

QVariantMap KugouApi::liteBaseParams() const
{
    QVariantMap params;
    params.insert("dfid", m_dfid.isEmpty() ? "-" : m_dfid);
    params.insert("mid", m_mid);
    params.insert("uuid", "-");
    params.insert("appid", 3116);
    params.insert("clientver", 11440);
    params.insert("clienttime",
                  QString::number(QDateTime::currentSecsSinceEpoch()));
    params.insert("token",
                  cookieValue({"token", "t"}));
    const QString userid = cookieValue({"userid", "user_id",
                                        "KugooID"});
    params.insert("userid",
                  userid.isEmpty() ? "0" : userid);
    return params;
}

void KugouApi::requestLitePlayInfo(const QString &hash, int type,
                                   const QSharedPointer<PlayRequest> &st, const QString &reason)
{
    if (!m_liteUserAuth.isEmpty()) {
        requestLiteSongAuth(hash, type, st, m_liteUserAuth, reason);
        return;
    }

    QVariantMap params = liteBaseParams();
    params.insert("module_id", 51);
    params.insert("signature", QString::fromLatin1(liteAndroidSignature(params)));

    QUrlQuery q;
    for (auto it = params.cbegin(); it != params.cend(); ++it)
        q.addQueryItem(it.key(), it.value().toString());

    getLite("http://trackercdngz.kugou.com/v1/user_verify?" + q.toString(),
            [this, hash, type, st, reason](const QJsonObject &json) {
                const QString userAuth = json.value("data").toObject()
                                             .value("auth").toString();
                if (userAuth.isEmpty()) {
                    qWarning() << "[kugou] 概念版 user_verify 失败，落回 web 签名 status:"
                               << json.value("status").toInt()
                               << "err_code:" << json.value("error_code").toInt();
                    requestSignedPlayInfo(hash, type, st, reason);
                    return;
                }
                m_liteUserAuth = userAuth;
                qInfo() << "[kugou] 概念版用户级 auth 已获取，继续取歌曲授权" << hash.left(8);
                requestLiteSongAuth(hash, type, st, userAuth, reason);
            });
}

void KugouApi::requestLiteSongAuth(const QString &hash, int type,
                                   const QSharedPointer<PlayRequest> &st, const QString &userAuth,
                                   const QString &reason)
{
    QVariantMap params = liteBaseParams();
    params.insert("authorization", userAuth);
    params.insert("module_id", 51);
    params.insert("album_audio_id",
                  st->albumAudioId.isEmpty() ? 0 : st->albumAudioId.toLongLong());
    params.insert("clientver", 11561);
    params.insert("hash", hash.toLower());
    params.insert("signature", QString::fromLatin1(liteAndroidSignature(params)));

    QUrlQuery q;
    for (auto it = params.cbegin(); it != params.cend(); ++it)
        q.addQueryItem(it.key(), it.value().toString());

    getLite("http://trackercdngz.kugou.com/v1/authorization?" + q.toString(),
            [this, hash, type, st, reason](const QJsonObject &json) {
                const QJsonObject d = json.value("data").toObject();
                const QString auth = d.value("auth").toString();
                const QString openTime =
                    d.value("open_time").toVariant().toString();
                if (auth.isEmpty() || openTime.isEmpty()) {

                    m_liteUserAuth.clear();
                    qWarning() << "[kugou] 概念版歌曲授权失败，落回 web 签名 status:"
                               << json.value("status").toInt()
                               << "err_code:" << json.value("error_code").toInt();
                    requestSignedPlayInfo(hash, type, st, reason);
                    return;
                }
                qInfo() << "[kugou] 概念版歌曲授权已获取，换播放地址" << hash.left(8);
                requestLiteUrlStep(hash, type, st, auth, openTime, m_quality, reason);
            });
}

void KugouApi::requestLiteUrlStep(const QString &hash, int type,
                                  const QSharedPointer<PlayRequest> &st, const QString &auth,
                                  const QString &openTime, int quality, const QString &reason)
{
    const QString lower = hash.toLower();
    const QString userid = cookieValue({"userid", "user_id",
                                        "KugooID"});
    QVariantMap params = liteBaseParams();
    params.insert("album_id",
                  st->albumId.isEmpty() ? 0 : st->albumId.toLongLong());
    params.insert("area_code", 1);
    params.insert("module", QString());
    params.insert("hash", lower);
    params.insert("need_m", 0);
    params.insert("ssa_flag", "is_fromtrack");
    params.insert("version", 11430);
    params.insert("open_time", openTime);
    params.insert("ptype", 0);
    params.insert("need_ogg", 1);
    params.insert("page_id", 967177915);
    params.insert("auth", auth);
    params.insert("mtype", 0);
    params.insert("quality",
                  quality >= 2 ? "flac"
                               : (quality == 1 ? "320" : "128"));
    params.insert("album_audio_id",
                  st->albumAudioId.isEmpty() ? 0 : st->albumAudioId.toLongLong());
    params.insert("behavior", "play");
    params.insert("pid", 411);
    params.insert("module_id", 51);
    params.insert("cmd", 26);
    params.insert("ppage_id", "356753938,823673182,967485191");
    params.insert("clientver", 11561);
    params.insert("pidversion", 3001);
    params.insert("cdnBackup", 1);
    params.insert("key",
                  liteSignKey(lower, "3116", m_mid,
                              userid.isEmpty() ? "0" : userid));

    params.insert("signature", QString::fromLatin1(liteAndroidSignature(params)));

    QUrlQuery q;
    for (auto it = params.cbegin(); it != params.cend(); ++it)
        q.addQueryItem(it.key(), it.value().toString());

    getLite("https://gateway.kugou.com/tracker/v5/url?" + q.toString(),
            [this, hash, type, st, auth, openTime, quality, reason](const QJsonObject &json) {

                QString url;
                const auto takeUrl = [&url](const QJsonValue &value) {
                    if (value.isArray()) {
                        for (const QJsonValue &item : value.toArray()) {
                            if (!item.toString().isEmpty()) {
                                url = item.toString();
                                break;
                            }
                        }
                    } else if (!value.toString().isEmpty()) {
                        url = value.toString();
                    }
                };
                takeUrl(json.value("url"));
                if (url.isEmpty())
                    takeUrl(json.value("data").toObject()
                                .value("url"));

                if (url.isEmpty()) {
                    const int status = json.value("status").toInt();
                    if (quality > 0) {
                        qInfo() << "[kugou] 概念版拒绝该音质，降级重试 status:" << status;
                        requestLiteUrlStep(hash, type, st, auth, openTime, 0, reason);
                        return;
                    }
                    qWarning() << "[kugou] 概念版取址无地址 status:" << status
                               << "err_code:" << json.value("error_code").toInt();
                    requestSignedPlayInfo(hash, type, st, reason);
                    return;
                }

                const QJsonObject dd = json.value("data").toObject();
                QString ext = json.value("extName").toString();
                if (ext.isEmpty())
                    ext = dd.value("extName").toString();
                QVariantMap data = st->meta;
                QString fileName = data.value("fileName").toString();
                const int dot = fileName.lastIndexOf(QLatin1Char('.'));
                if (!ext.isEmpty() && dot > 0)
                    fileName = fileName.left(dot) + QLatin1Char('.') + ext;
                data.insert("fileName", fileName);
                data.insert("url", url);
                data.insert("backup_url", url);
                data.insert("hash", hash);
                data.insert("type", type);
                emit resultReady(ApiAction::MusicInfo, data, Source);
            });
}

void KugouApi::onLyricInfo(const QString &hash, int duration)
{
    if (duration > 10000)
        duration = duration / 1000;

    QUrl url("http://lyrics.kugou.com/search");
    QUrlQuery q;
    q.addQueryItem("ver", "1");
    q.addQueryItem("man", "yes");
    q.addQueryItem("client", "pc");
    q.addQueryItem("duration", QString::number(duration));
    q.addQueryItem("hash", hash);
    url.setQuery(q);

    get(url.toString(), [this, hash](const QJsonObject &json) {
        const QJsonArray candidates = json.value("candidates").toArray();
        if (candidates.isEmpty()) {
            qWarning() << "[lyric] 没有找到歌词候选（可能是歌曲没有歌词，或 hash/duration 不对）";
            QVariantMap empty;
            empty.insert("info", QVariantList{});
            empty.insert("translate", QVariantList{});
            empty.insert("hash", hash);

            if (json.contains("error"))
                empty.insert("error", json.value("error"));
            emit resultReady(ApiAction::LyricInfo, empty, Source);
            return;
        }
        const QJsonObject first = candidates.first().toObject();
        const QString id = QString::number(first.value("id").toVariant().toLongLong());
        const QString accesskey = first.value("accesskey").toString();

        QUrl url2("http://lyrics.kugou.com/download");
        QUrlQuery q2;
        q2.addQueryItem("ver", "1");
        q2.addQueryItem("client", "pc");
        q2.addQueryItem("id", id);
        q2.addQueryItem("accesskey", accesskey);
        q2.addQueryItem("fmt", "krc");
        q2.addQueryItem("charset", "utf8");
        url2.setQuery(q2);

        get(url2.toString(), [this, hash](const QJsonObject &json2) {
            const QString content = json2.value("content").toString();
            if (content.isEmpty()) {
                qWarning() << "[lyric] content 为空，歌词下载失败";
                QVariantMap data;
                data.insert("info", QVariantList{});
                data.insert("translate", QVariantList{});
                if (json2.contains("error"))
                    data.insert("error", json2.value("error"));
                data.insert("hash", hash);
                emit resultReady(ApiAction::LyricInfo, data, Source);
                return;
            }

            // KRC 解码 + 正则解析是纯 CPU 工作，放到线程池，避免卡 UI。
            const QByteArray encoded = content.toLatin1();
            QPointer<KugouApi> self(this);
            QThreadPool::globalInstance()->start([self, hash, encoded] {
                const QString krc = decodeKrc(encoded);
                QVariantMap data;
                data.insert("info", krcToLyrics(krc));
                data.insert("translate", krcTranslations(krc));
                data.insert("hash", hash);
                if (!self)
                    return;
                QMetaObject::invokeMethod(self.data(), [self, data] {
                    if (self)
                        emit self->resultReady(ApiAction::LyricInfo, data, Source);
                }, Qt::QueuedConnection);
            });
        });
    });
}

void KugouApi::onPersonalFm(int page, int pageSize)
{
    requestRankSongs(8888, page, pageSize, ApiAction::PersonalFm);
}

void KugouApi::onPersonalRadar(int page, int pageSize)
{
    requestRankSongs(6666, page, pageSize, ApiAction::PersonalRadar);
}
