// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "KugouApi.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <QJsonValue>
#include <QRandomGenerator>
#include <QStringList>
#include <vector>

#include "ApiCommon.h"

// zlib 手动声明
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
// 酷狗 KRC 歌词 XOR 密钥（16 字节）
const unsigned char kKrcKey[16] = {64, 71, 97, 119, 94, 50, 116, 71,
                                   81, 54, 49, 45, 206, 210, 110, 105};

// 裸 zlib 流解压（等价 Python 的 zlib.decompress）
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
        if (ret == Z_BUF_ERROR || ret == Z_MEM_ERROR) { // 缓冲区不够，翻倍重试
            destLen = qMax(destLen * 2, destLen + 1024);
            continue;
        }
        qWarning() << "[krc] zlib uncompress 失败, code:" << ret;
        return {};
    }
    return {};
}

// raw-deflate（RFC1951，无 zlib 头）→ 补 zlib 头 + adler32 → uncompress
QByteArray rawDeflateInflate(const QByteArray &raw)
{
    QByteArray zlib;
    zlib.append(char(0x78));
    zlib.append(char(0x9C));
    zlib.append(raw);
    quint32 a = 1, b = 0; // adler32
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

// 先试裸 zlib，失败再试 raw-deflate 补头
QByteArray inflateSmart(const QByteArray &data)
{
    QByteArray out = zlibInflate(data);
    if (!out.isEmpty()) {
        qDebug() << "[krc] 裸 zlib 解压成功, 长度:" << out.size();
        return out;
    }
    qDebug() << "[krc] 裸 zlib 失败，尝试 raw-deflate 补头...";
    out = rawDeflateInflate(data);
    if (!out.isEmpty())
        qDebug() << "[krc] raw-deflate 补头解压成功, 长度:" << out.size();
    else
        qWarning() << "[krc] raw-deflate 补头也失败！";
    return out;
}
} // namespace

namespace {
// 简易大整数，仅用于将 MD5 摘要转为十进制字符串（生成 kugou mid）
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
        return s.isEmpty() ? QStringLiteral("0") : s;
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
} // namespace

QByteArray KugouApi::kugouWebSignature(const QJsonObject &params)
{
    const QByteArray salt = "NVPh5oo715z5DIWAeQlhMDsWXXQV4hwt";
    QStringList keyValues;
    for (const QString &k : params.keys()) {
        const QString v = QJsonValue(params.value(k)).toVariant().toString();
        keyValues << (k + QLatin1Char('=') + v);
    }
    keyValues.sort();
    const QByteArray paramsStr = keyValues.join(QString()).toUtf8();
    return md5Hex(salt + paramsStr + salt);
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
    const QString p3 = QStringLiteral("4") + rndHex(3);
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
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
}

// 通用 GET 请求（带 UA / Cookie，清理 KG_TAG 包裹后解析 JSON）
void KugouApi::get(const QString &url, const Callback &cb)
{
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", kUa);
    req.setRawHeader("Accept-Encoding", "identity");
    req.setRawHeader("Referer", "https://www.kugou.com/");
    if (!m_cookie.isEmpty())
        req.setRawHeader("Cookie", m_cookie.toUtf8());

    qDebug() << "正在请求酷狗api：" << url;
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, cb] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[kugou] 请求失败:" << reply->errorString()
                       << reply->url().toString();
            cb(QJsonObject()); // 错误也回调，保证 QML 侧 loadState 能复位
            return;
        }
        QByteArray body = reply->readAll();
        body.replace("<!--KG_TAG_RES_START-->", "").replace("<!--KG_TAG_RES_END-->", "");
        const QJsonObject obj = QJsonDocument::fromJson(body).object();
        if (obj.isEmpty())
            qWarning() << "[kugou] JSON 解析为空, url:" << reply->url().toString()
                       << "body:" << QString::fromUtf8(body).left(200);
        cb(obj);
    });
}

// KRC 歌词解码（替代 pako.mjs：Base64 → 跳4 → XOR → 跳2 → raw inflate）
QString KugouApi::decodeKrc(const QByteArray &base64)
{
    QByteArray bytes = QByteArray::fromBase64(base64);
    if (bytes.size() <= 4) {
        qWarning() << "[krc] 数据太短，无法解码";
        return {};
    }
    bytes = bytes.mid(4); // 跳过前 4 字节（krc1 magic 头）

    // XOR 解密（酷狗固定 16 字节密钥）
    QByteArray decrypted(bytes.size(), Qt::Uninitialized);
    for (int i = 0; i < bytes.size(); ++i)
        decrypted[i] = char(uchar(bytes.at(i)) ^ kKrcKey[i % 16]);

    // 偏移4/2 兼容旧格式（带版本标记）
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
    static const QRegularExpression lineRe(QStringLiteral("^\\[(\\d+),(\\d+)\\](.*)$"));
    static const QRegularExpression wordRe(QStringLiteral("<(\\d+),(\\d+),\\d+>([^<]*)"));

    const QStringList lines = krc.split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty())
            continue; // 空行跳过
        const QRegularExpressionMatch m = lineRe.match(line);
        if (!m.hasMatch())
            continue;

        QString rawContent = m.captured(3).trimmed();

        // 判断是否为对唱（整行被全角括号包裹）
        const bool isOther = rawContent.endsWith("）");

        // 字级：<偏移,持续,0>文本
        QVariantList words;
        QString fullText;
        QRegularExpressionMatchIterator it = wordRe.globalMatch(m.captured(3));
        while (it.hasNext()) {
            const QRegularExpressionMatch w = it.next();
            QString text = w.captured(3);
            if (text.isEmpty())
                continue;
            if(isOther) {
                text.remove("（");
                text.remove("）");
            }
            words << QVariantMap{
                {QStringLiteral("offset"), w.captured(1).toInt()},
                {QStringLiteral("duration"), w.captured(2).toInt()},
                {QStringLiteral("text"), text},
            };
            fullText += text;
        }
        if (words.isEmpty())
            continue;
        result << QVariantMap{
            {QStringLiteral("time"), m.captured(1).toInt()},
            {QStringLiteral("text"), fullText},
            {QStringLiteral("info"), words},
            {QStringLiteral("isOther"), isOther}
        };
    }
    std::sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("time")).toInt()
               < b.toMap().value(QStringLiteral("time")).toInt();
    });
    return result;
}

QVariantList KugouApi::krcTranslations(const QString &krc)
{
    static const QRegularExpression re(QStringLiteral("\\[language:([^\\]]+)\\]"));
    const QRegularExpressionMatch m = re.match(krc);
    if (!m.hasMatch())
        return {};

    const QJsonObject obj =
        QJsonDocument::fromJson(QByteArray::fromBase64(m.captured(1).toLatin1())).object();
    const QJsonArray content = obj.value(QStringLiteral("content")).toArray();
    if (content.isEmpty())
        return {};
    QJsonObject c = content.first().toObject();
    if (c.value(QStringLiteral("type")).toInt() != 1 && content.size() > 1)
        c = content.at(1).toObject();

    QVariantList result;
    for (const QJsonValue &v : c.value(QStringLiteral("lyricContent")).toArray()) {
        const QJsonArray item = v.toArray();
        // 与 oldjs extractTranslations 一致：纯空格行 → 空字符串。
        // 否则 QML 显示时空白行还会占位，造成"翻译区空一大片"。
        const QString text = item.isEmpty() ? QString() : item.first().toString();
        result << (text.trimmed().isEmpty() ? QString() : text);
    }
    return result;
}

// 搜索（type: 0 歌曲 / 1 歌单 / 2 专辑 / 3 歌词）
void KugouApi::searchSongs(const QString &keyword, int type, int page, int pageSize)
{
    QUrl url;
    switch (type) {
    case 0:
        url = QUrl(QStringLiteral("http://mobilecdn.kugou.com/api/v3/search/song"));
        break;
    case 1:
        url = QUrl(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/search/special"));
        break;
    case 2:
        url = QUrl(QStringLiteral("http://msearch.kugou.com/api/v3/search/album"));
        break;
    default:
        url = QUrl(QStringLiteral("http://mobileservice.kugou.com/api/v3/lyric/search"));
        break;
    }
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("pagesize"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    q.addQueryItem(QStringLiteral("keyword"), keyword);
    if (type == 1) {
        q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
        q.addQueryItem(QStringLiteral("highlight"), QStringLiteral("em"));
        q.addQueryItem(QStringLiteral("filter"), QStringLiteral("0"));
        q.addQueryItem(QStringLiteral("sver"), QStringLiteral("2"));
        q.addQueryItem(QStringLiteral("with_res_tag"), QStringLiteral("1"));
    } else if (type == 2) {
        q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
        q.addQueryItem(QStringLiteral("iscorrection"), QStringLiteral("1"));
        q.addQueryItem(QStringLiteral("highlight"), QStringLiteral("em"));
        q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
        q.addQueryItem(QStringLiteral("sver"), QStringLiteral("2"));
        q.addQueryItem(QStringLiteral("with_res_tag"), QStringLiteral("1"));
    } else if (type == 3) {
        q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
        q.addQueryItem(QStringLiteral("highlight"), QStringLiteral("1"));
        q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
        q.addQueryItem(QStringLiteral("area_code"), QStringLiteral("1"));
        q.addQueryItem(QStringLiteral("with_res_tag"), QStringLiteral("1"));
    } else {
        q.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    }
    url.setQuery(q);

    get(url.toString(), [this, type](const QJsonObject &json) {
        const QJsonArray arr = json.value(QStringLiteral("data")).toObject()
                                   .value(QStringLiteral("info")).toArray();
        QVariantList info;
        if (type == 0) { // 歌曲
            for (const QJsonValue &v : arr) {
                const QJsonObject s = v.toObject();
                const QJsonObject tp = s.value(QStringLiteral("trans_param")).toObject();
                const int pay320 = s.value(QStringLiteral("pay_type_320")).toInt();
                const QString hash = s.value(QStringLiteral("hash")).toString();
                const QString hq = pay320 != 3
                                       ? s.value(QStringLiteral("320hash")).toString()
                                             .isEmpty()
                                             ? tp.value(QStringLiteral("ogg_320_hash")).toString()
                                             : s.value(QStringLiteral("320hash")).toString()
                                       : hash;
                const QString sq = s.value(QStringLiteral("pay_type_sq")).toInt() == 0
                                       ? s.value(QStringLiteral("sqhash")).toString()
                                       : hash;
                info << ApiCommon::song(
                    s.value(QStringLiteral("songname")).toString(),
                    s.value(QStringLiteral("singername")).toString(),
                    tp.value(QStringLiteral("union_cover")).toString(),
                    hash,
                    s.value(QStringLiteral("duration")).toInt(),
                    s.value(QStringLiteral("album_name")).toString(),
                    hq, sq,
                    s.value(QStringLiteral("pay_type")).toInt(1));
            }
        } else if (type == 1) { // 歌单
            for (const QJsonValue &v : arr) {
                const QJsonObject s = v.toObject();
                info << ApiCommon::song(
                    s.value(QStringLiteral("specialname")).toString(),
                    s.value(QStringLiteral("nickname")).toString(),
                    s.value(QStringLiteral("imgurl")).toString(),
                    QString::number(s.value(QStringLiteral("specialid")).toVariant().toLongLong()),
                    s.value(QStringLiteral("songcount")).toInt(),
                    s.value(QStringLiteral("intro")).toString(),
                    QString(), QString(), 0,
                    qint64(s.value(QStringLiteral("playcount")).toDouble()));
            }
        } else if (type == 2) { // 专辑
            for (const QJsonValue &v : arr) {
                const QJsonObject a = v.toObject();
                info << ApiCommon::song(
                    a.value(QStringLiteral("albumname")).toString(),
                    a.value(QStringLiteral("singername")).toString(),
                    a.value(QStringLiteral("imgurl")).toString(),
                    QString::number(a.value(QStringLiteral("albumid")).toVariant().toLongLong()),
                    a.value(QStringLiteral("songcount")).toInt(),
                    a.value(QStringLiteral("albumname")).toString());
            }
        } else if (type == 3) { // 歌词搜索结果
            for (const QJsonValue &v : arr) {
                const QJsonObject s = v.toObject();
                const QString filename = s.value(QStringLiteral("filename")).toString();
                const QStringList parts = filename.split(QLatin1Char('-'));
                const QString title = parts.size() > 1 ? parts.at(1).trimmed() : s.value(QStringLiteral("songname")).toString();
                const QString artist = parts.size() > 1 ? parts.at(0).trimmed() : s.value(QStringLiteral("singername")).toString();
                const QJsonObject tp = s.value(QStringLiteral("trans_param")).toObject();
                const int pay320 = s.value(QStringLiteral("pay_type_320")).toInt();
                const QString hash = s.value(QStringLiteral("hash")).toString();
                const QString hq = pay320 != 3
                                       ? s.value(QStringLiteral("320hash")).toString()
                                                 .isEmpty()
                                             ? tp.value(QStringLiteral("ogg_320_hash")).toString()
                                             : s.value(QStringLiteral("320hash")).toString()
                                       : hash;
                const QString sq = s.value(QStringLiteral("pay_type_sq")).toInt() == 0
                                       ? s.value(QStringLiteral("sqhash")).toString()
                                       : hash;
                info << ApiCommon::song(
                    title.isEmpty() ? s.value(QStringLiteral("songname")).toString() : title,
                    artist.isEmpty() ? s.value(QStringLiteral("singername")).toString() : artist,
                    tp.value(QStringLiteral("union_cover")).toString(),
                    s.value(QStringLiteral("hash")).toString(),
                    s.value(QStringLiteral("duration")).toInt(),
                    s.value(QStringLiteral("album_name")).toString(),
                    hq, sq,
                    s.value(QStringLiteral("pay_type")).toInt(1));
            }
        }
        emit resultReady(QStringLiteral("searchSongs"),
                         ApiCommon::listResult(info), Source);
    });
}

// 歌单分类
void KugouApi::getPlaylistMenu(int type)
{
    Q_UNUSED(type);
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/tag/list"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("pid"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("apiver"), QStringLiteral("2"));
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("data")).toObject()
                                     .value(QStringLiteral("info")).toArray()) {
            const QJsonObject t = v.toObject();
            info << QVariantMap{
                {QStringLiteral("title"), t.value(QStringLiteral("name")).toString()},
                {QStringLiteral("id"), t.value(QStringLiteral("id")).toVariant().toLongLong()},
                {QStringLiteral("category"), t.value(QStringLiteral("id")).toString()},
            };
        }
        emit resultReady(QStringLiteral("getPlaylistMenu"),
                         ApiCommon::listResult(info), Source);
    });
}

// 分类信息（透传 data，并显式带上 special_tag_id 供上层拉取该分类歌单）
void KugouApi::getMenuInfo(const QString &id)
{
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/tag/info"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("apiver"), QStringLiteral("2"));
    q.addQueryItem(QStringLiteral("id"), id);
    url.setQuery(q);

    get(url.toString(), [this, id](const QJsonObject &json) {
        QJsonObject raw = json.value(QStringLiteral("data")).toObject();
        // 兜底：tag/info 返回可能不含 tagid 字段，强制注入请求的 id
        if (!raw.contains(QStringLiteral("special_tag_id")))
            raw.insert(QStringLiteral("special_tag_id"), id);
        emit resultReady(QStringLiteral("getMenuInfo"), raw.toVariantMap(), Source);
    });
}

// 分类下歌单列表
void KugouApi::getMusicPlaylists(const QString &tagid, int page, int pageSize)
{
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/tag/specialList"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("ugc"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("sort"), QStringLiteral("2"));
    q.addQueryItem(QStringLiteral("pagesize"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    q.addQueryItem(QStringLiteral("tagid"), tagid);
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("data")).toObject()
                                     .value(QStringLiteral("info")).toArray()) {
            const QJsonObject s = v.toObject();
            QString cover = s.value(QStringLiteral("imgurl")).toString();
            if (cover.contains(QStringLiteral("{size}")))
                cover.replace(QStringLiteral("{size}"), QStringLiteral("64"));
            info << ApiCommon::song(
                s.value(QStringLiteral("specialname")).toString(),
                s.value(QStringLiteral("username")).toString(),
                cover,
                QString::number(s.value(QStringLiteral("specialid")).toVariant().toLongLong()),
                s.value(QStringLiteral("slid")).toInt(),
                s.value(QStringLiteral("intro")).toString(),
                QString(), QString(), 0,
                qint64(s.value(QStringLiteral("playcount")).toDouble()));
        }
        emit resultReady(QStringLiteral("getMusicPlaylists"),
                         ApiCommon::listResult(info), Source);
    });
}

// 歌单内歌曲
void KugouApi::getPlaylistSongs(const QString &listid, int page, int pageSize)
{
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/special/song"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
    q.addQueryItem(QStringLiteral("with_res_tag"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("pagesize"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    q.addQueryItem(QStringLiteral("specialid"), listid);
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("data")).toObject()
                                     .value(QStringLiteral("info")).toArray()) {
            const QJsonObject s = v.toObject();
            const QJsonObject tp = s.value(QStringLiteral("trans_param")).toObject();
            // 文件名形如 "歌手 - 歌名"
            const QString filename = s.value(QStringLiteral("filename")).toString();
            const QStringList parts = filename.split(QLatin1Char('-'));
            const QString title = parts.size() > 1 ? parts.at(1).trimmed() : filename;
            const QString artist = parts.size() > 1 ? parts.at(0).trimmed() : QString();
            const QString hash = s.value(QStringLiteral("hash")).toString();
            const int pay320 = s.value(QStringLiteral("pay_type_320")).toInt();
            const QString hq = pay320 == 0 && !s.value(QStringLiteral("320hash")).toString().isEmpty()
                                   ? s.value(QStringLiteral("320hash")).toString()
                                   : hash;
            const QString sq = s.value(QStringLiteral("pay_type_sq")).toInt() == 0
                                   ? s.value(QStringLiteral("sqhash")).toString()
                                   : hash;
            info << ApiCommon::song(
                title, artist,
                tp.value(QStringLiteral("union_cover")).toString(),
                hash,
                s.value(QStringLiteral("duration")).toInt(),
                s.value(QStringLiteral("album_name")).toString(),
                hq, sq,
                s.value(QStringLiteral("pay_type")).toInt());
        }
        emit resultReady(QStringLiteral("getPlaylistSongs"),
                         ApiCommon::listResult(info), Source);
    });
}

// 推荐歌曲（新歌榜 type=1）
void KugouApi::getRecommendSongs(int page, int pageSize)
{
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/rank/newsong"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("with_cover"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("type"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("area_code"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("with_res_tag"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("pagesize"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("data")).toObject()
                                     .value(QStringLiteral("info")).toArray()) {
            const QJsonObject s = v.toObject();
            const QJsonObject tp = s.value(QStringLiteral("trans_param")).toObject();
            const QString hash = s.value(QStringLiteral("hash")).toString();
            const int pay320 = s.value(QStringLiteral("pay_type_320")).toInt();
            const QString hq = pay320 != 3
                                   ? s.value(QStringLiteral("320hash")).toString()
                                         .isEmpty()
                                         ? tp.value(QStringLiteral("ogg_320_hash")).toString()
                                         : s.value(QStringLiteral("320hash")).toString()
                                   : hash;
            const QString sq = s.value(QStringLiteral("pay_type_sq")).toInt() == 0
                                   ? s.value(QStringLiteral("sqhash")).toString()
                                   : hash;
            info << ApiCommon::song(
                s.value(QStringLiteral("songname")).toString(),
                s.value(QStringLiteral("singername")).toString(),
                tp.value(QStringLiteral("union_cover")).toString(),
                hash,
                s.value(QStringLiteral("duration")).toInt(),
                s.value(QStringLiteral("album_name")).toString(),
                hq, sq,
                s.value(QStringLiteral("pay_type")).toInt(1));
        }
        emit resultReady(QStringLiteral("getRecommendSongs"),
                         ApiCommon::listResult(info), Source);
    });
}

// 热门歌单分类
void KugouApi::getHotPlaylistMenu(int type)
{
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/tag/recommend"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("apiver"), QStringLiteral("2"));
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("showtype"), QString::number(type));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("data")).toObject()
                                     .value(QStringLiteral("info")).toArray()) {
            const QJsonObject t = v.toObject();
            info << QVariantMap{
                {QStringLiteral("title"), t.value(QStringLiteral("name")).toString()},
                {QStringLiteral("id"), t.value(QStringLiteral("id")).toVariant().toLongLong()},
                {QStringLiteral("tagid"), t.value(QStringLiteral("special_tag_id")).toVariant().toLongLong()},
                {QStringLiteral("cover"), t.value(QStringLiteral("bannerurl")).toString()},
            };
        }
        emit resultReady(QStringLiteral("getHotPlaylistMenu"),
                         ApiCommon::listResult(info), Source);
    });
}

// 热门歌单
void KugouApi::getHotPlaylists(int page, int pageSize)
{
    Q_UNUSED(pageSize);
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v5/special/recommend"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("recommend_expire"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("sign"), QStringLiteral("52186982747e1404d426fa3f2a1e8ee4"));
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("uid"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
    q.addQueryItem(QStringLiteral("area_code"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("appid"), QStringLiteral("1005"));
    q.addQueryItem(QStringLiteral("mid"), QStringLiteral("286974383886022203545511837994020015101"));
    q.addQueryItem(QStringLiteral("_t"), QStringLiteral("1545746286"));
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("data")).toObject()
                                     .value(QStringLiteral("list")).toArray()) {
            const QJsonObject s = v.toObject();
            info << ApiCommon::song(
                s.value(QStringLiteral("specialname")).toString(),
                s.value(QStringLiteral("nickname")).toString(),
                s.value(QStringLiteral("imgurl")).toString(),
                QString::number(s.value(QStringLiteral("specialid")).toVariant().toLongLong()),
                s.value(QStringLiteral("songcount")).toInt(),
                s.value(QStringLiteral("intro")).toString(),
                QString(), QString(), 0,
                qint64(s.value(QStringLiteral("playcount")).toDouble()));
        }
        emit resultReady(QStringLiteral("getHotPlaylists"),
                         ApiCommon::listResult(info), Source);
    });
}

// 新歌（type: 1 华语 / 2 欧美 / 3 日韩）
void KugouApi::getNewSongs(int type, int page, int pageSize)
{
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/rank/newsong"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("with_cover"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("area_code"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("with_res_tag"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("pagesize"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    q.addQueryItem(QStringLiteral("type"), QString::number(type));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("data")).toObject()
                                     .value(QStringLiteral("info")).toArray()) {
            const QJsonObject s = v.toObject();
            const QJsonArray authors = s.value(QStringLiteral("authors")).toArray();
            const QJsonObject tp = s.value(QStringLiteral("trans_param")).toObject();
            const QString hash = s.value(QStringLiteral("hash")).toString();
            const int pay320 = s.value(QStringLiteral("pay_type_320")).toInt();
            const QString hq = pay320 != 3
                                   ? s.value(QStringLiteral("320hash")).toString()
                                         .isEmpty()
                                         ? tp.value(QStringLiteral("ogg_320_hash")).toString()
                                         : s.value(QStringLiteral("320hash")).toString()
                                   : hash;
            const QString sq = s.value(QStringLiteral("pay_type_sq")).toInt() == 0
                                   ? s.value(QStringLiteral("sqhash")).toString()
                                   : hash;
            QString artist;
            if (authors.size() > 1)
                artist = authors.at(0).toObject().value(QStringLiteral("author_name")).toString()
                         + QLatin1Char(',')
                         + authors.at(1).toObject().value(QStringLiteral("author_name")).toString();
            else if (authors.size() == 1)
                artist = authors.at(0).toObject().value(QStringLiteral("author_name")).toString();
            info << ApiCommon::song(
                s.value(QStringLiteral("songname")).toString(),
                artist,
                tp.value(QStringLiteral("union_cover")).toString(),
                hash,
                s.value(QStringLiteral("duration")).toInt(),
                s.value(QStringLiteral("album_name")).toString(),
                hq, sq,
                s.value(QStringLiteral("pay_type")).toInt(1));
        }
        emit resultReady(QStringLiteral("getNewSongs"),
                         ApiCommon::listResult(info), Source);
    });
}

// 榜单列表
void KugouApi::getAllToplist()
{
    // 酷狗榜单列表（rank/list 动态接口，返回全部榜单 + 封面）
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/rank/list"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("withsong"), QStringLiteral("0"));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("data")).toObject()
                                     .value(QStringLiteral("info")).toArray()) {
            const QJsonObject t = v.toObject();
            const QJsonObject total = t.value(QStringLiteral("extra")).toObject().value(QStringLiteral("resp")).toObject();
            info << ApiCommon::song(
                t.value(QStringLiteral("rankname")).toString(),
                t.value(QStringLiteral("update_frequency")).toString(),
                t.value(QStringLiteral("imgurl")).toString(),
                QString::number(t.value(QStringLiteral("rankid")).toVariant().toLongLong()),
                total.value(QStringLiteral("all_total")).toInt(),
                t.value(QStringLiteral("intro")).toString());
        }
        // 接口异常时回退到内置热门榜单
        if (info.isEmpty()) {
            info << ApiCommon::song(QStringLiteral("酷狗飙升榜"), QString(), QString(),
                                    QStringLiteral("6666"));
            info << ApiCommon::song(QStringLiteral("酷狗TOP500"), QString(), QString(),
                                    QStringLiteral("8888"));
        }
        emit resultReady(QStringLiteral("getAllToplist"),
                         ApiCommon::listResult(info), Source);
    });
}

// 榜单歌曲
void KugouApi::getMusicToplist(int page, int pageSize, int rankid)
{
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/rank/song"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
    q.addQueryItem(QStringLiteral("ranktype"), QStringLiteral("2"));
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("pagesize"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    q.addQueryItem(QStringLiteral("area_code"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("rankid"), QString::number(rankid));
    q.addQueryItem(QStringLiteral("with_res_tag"), QStringLiteral("1"));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("data")).toObject()
                                     .value(QStringLiteral("info")).toArray()) {
            const QJsonObject s = v.toObject();
            const QJsonObject tp = s.value(QStringLiteral("trans_param")).toObject();
            const QString filename = s.value(QStringLiteral("filename")).toString();
            const QString title = s.value(QStringLiteral("songname")).toString();
            const QJsonObject author = s.value(QStringLiteral("authors")).toArray().at(0).toObject();
            const QString hash = s.value(QStringLiteral("hash")).toString();
            const int pay320 = s.value(QStringLiteral("pay_type_320")).toInt();
            const QString hq = pay320 == 0 && !s.value(QStringLiteral("320hash")).toString().isEmpty()
                                   ? s.value(QStringLiteral("320hash")).toString()
                                   : hash;
            const QString sq = s.value(QStringLiteral("pay_type_sq")).toInt() == 0
                                   ? s.value(QStringLiteral("sqhash")).toString()
                                   : hash;
            info << ApiCommon::song(
                title,
                author.value(QStringLiteral("author_name")).toString(),
                tp.value(QStringLiteral("union_cover")).toString(),
                s.value(QStringLiteral("hash")).toString(),
                s.value(QStringLiteral("duration")).toInt(),
                s.value(QStringLiteral("album_name")).toString(),
                hq, sq,
                s.value(QStringLiteral("pay_type")).toInt(1));
        }
        emit resultReady(QStringLiteral("getMusicToplist"),
                         ApiCommon::listResult(info), Source);
    });
}

// 热门歌手（歌手榜）
void KugouApi::getHotSingers(int page, int pageSize)
{
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/singer/rank"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("type"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    q.addQueryItem(QStringLiteral("pagesize"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        // 兼容 data.singers / data.info 两种结构
        QJsonArray arr = json.value(QStringLiteral("data")).toObject()
                             .value(QStringLiteral("singers")).toArray();
        if (arr.isEmpty())
            arr = json.value(QStringLiteral("data")).toObject()
                      .value(QStringLiteral("info")).toArray();
        for (const QJsonValue &v : arr) {
            const QJsonObject s = v.toObject();
            info << ApiCommon::song(
                s.value(QStringLiteral("singername")).toString(),
                QString(),
                s.value(QStringLiteral("imgurl")).toString(),
                QString::number(s.value(QStringLiteral("singerid")).toVariant().toLongLong()));
        }
        emit resultReady(QStringLiteral("getHotSingers"),
                         ApiCommon::listResult(info), Source);
    });
}

// 歌手分类（area: 1 华语 / 2 欧美 / 3 日本 / 4 韩国）
void KugouApi::getSingerCategory(int area, int page, int pageSize)
{
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/singer/list"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("type"), QString::number(area));
    q.addQueryItem(QStringLiteral("sex"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    q.addQueryItem(QStringLiteral("pagesize"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        // 兼容 data.singers / data.info 两种结构
        QJsonArray arr = json.value(QStringLiteral("data")).toObject()
                             .value(QStringLiteral("singers")).toArray();
        if (arr.isEmpty())
            arr = json.value(QStringLiteral("data")).toObject()
                      .value(QStringLiteral("info")).toArray();
        for (const QJsonValue &v : arr) {
            const QJsonObject s = v.toObject();
            info << ApiCommon::song(
                s.value(QStringLiteral("singername")).toString(),
                QString(),
                s.value(QStringLiteral("imgurl")).toString(),
                QString::number(s.value(QStringLiteral("singerid")).toVariant().toLongLong()));
        }
        emit resultReady(QStringLiteral("getSingerCategory"),
                         ApiCommon::listResult(info), Source);
    });
}

// 歌手歌曲
void KugouApi::getSingerSongs(const QString &singerid, int page, int pageSize)
{
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/singer/song"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("singerid"), singerid);
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    q.addQueryItem(QStringLiteral("pagesize"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        // 兼容 data.songs / data.info 两种结构
        QJsonArray arr = json.value(QStringLiteral("data")).toObject()
                             .value(QStringLiteral("songs")).toArray();
        if (arr.isEmpty())
            arr = json.value(QStringLiteral("data")).toObject()
                      .value(QStringLiteral("info")).toArray();
        for (const QJsonValue &v : arr) {
            const QJsonObject s = v.toObject();
            const QJsonObject tp = s.value(QStringLiteral("trans_param")).toObject();
            const QString filename = s.value(QStringLiteral("filename")).toString();
            const QStringList parts = filename.split(QLatin1Char('-'));
            const QString title = parts.size() > 1 ? parts.at(1).trimmed() : filename;
            const QString artist = parts.size() > 1 ? parts.at(0).trimmed() : QString();
            const QString hash = s.value(QStringLiteral("hash")).toString();
            const int pay320 = s.value(QStringLiteral("pay_type_320")).toInt();
            const QString hq = pay320 == 0 && !s.value(QStringLiteral("320hash")).toString().isEmpty()
                                   ? s.value(QStringLiteral("320hash")).toString()
                                   : hash;
            const QString sq = s.value(QStringLiteral("pay_type_sq")).toInt() == 0
                                   ? s.value(QStringLiteral("sqhash")).toString()
                                   : hash;
            info << ApiCommon::song(
                title, artist,
                tp.value(QStringLiteral("union_cover")).toString(),
                hash,
                s.value(QStringLiteral("duration")).toInt(),
                s.value(QStringLiteral("album_name")).toString(),
                hq, sq,
                s.value(QStringLiteral("pay_type")).toInt());
        }
        emit resultReady(QStringLiteral("getSingerSongs"),
                         ApiCommon::listResult(info), Source);
    });
}

// 歌曲播放信息（type: 0 播放 / 1 下载）
void KugouApi::getMusicInfo(const QString &hash, int type)
{
    QUrl url(QStringLiteral("https://m.kugou.com/app/i/getSongInfo.php?cmd=playInfo&hash=")
             + hash);
    get(url.toString(), [this, hash, type](const QJsonObject &json) {
        // 与 JS 版一致：兼容 {data:{...}} 和直接 {...} 两种返回结构
        QJsonObject d = json.value(QStringLiteral("data")).toObject();
        if (d.isEmpty())
            d = json;

        const QString backupUrl = d.value(QStringLiteral("backup_url")).toString();
        const QString playUrl = d.value(QStringLiteral("url")).toString();
        QString albumImg = d.value(QStringLiteral("trans_param")).toObject()
                               .value(QStringLiteral("union_cover")).toString();
        if (albumImg.isEmpty())
            albumImg = d.value(QStringLiteral("album_img")).toString();
        if (albumImg.isEmpty())
            albumImg = d.value(QStringLiteral("imgUrl")).toString();
        const QString songName = d.value(QStringLiteral("songName")).toString();
        const QString fileNameRaw = d.value(QStringLiteral("fileName")).toString();
        const QString ext = d.value(QStringLiteral("extName")).toString();
        QString fileName = fileNameRaw;
        if (!fileName.isEmpty() && !ext.isEmpty())
            fileName += QLatin1Char('.') + ext;
        if (fileName.isEmpty())
            fileName = songName + QStringLiteral(".mp3");

        QVariantMap data;
        data.insert(QStringLiteral("backup_url"), backupUrl.isEmpty() ? playUrl : backupUrl);
        data.insert(QStringLiteral("url"), playUrl.isEmpty() ? backupUrl : playUrl);
        data.insert(QStringLiteral("songName"), songName.isEmpty() ? fileNameRaw : songName);
        data.insert(QStringLiteral("author_name"), d.value(QStringLiteral("author_name")).toString());
        data.insert(QStringLiteral("singer_img"), d.value(QStringLiteral("imgUrl")).toString());
        data.insert(QStringLiteral("album_img"), albumImg);
        data.insert(QStringLiteral("timeLength"),
                    d.value(QStringLiteral("timeLength")).toDouble()
                        ? d.value(QStringLiteral("timeLength")).toDouble()
                        : d.value(QStringLiteral("duration")).toDouble());
        data.insert(QStringLiteral("fileName"), fileName);
        data.insert(QStringLiteral("hash"), hash);
        data.insert(QStringLiteral("type"), type);
        emit resultReady(QStringLiteral("getMusicInfo"), data, Source);
    });
}

// 歌词（两步：搜索候选 → 下载 KRC → 解码）
void KugouApi::getLyricInfo(const QString &hash, int duration)
{
    // 歌词搜索接口期望 duration 是"秒"。QML 侧可能传毫秒（如 245000），这里归一化成秒
    if (duration > 10000)
        duration = duration / 1000;

    QUrl url(QStringLiteral("http://lyrics.kugou.com/search"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("ver"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("man"), QStringLiteral("yes"));
    q.addQueryItem(QStringLiteral("client"), QStringLiteral("pc"));
    q.addQueryItem(QStringLiteral("duration"), QString::number(duration));
    q.addQueryItem(QStringLiteral("hash"), hash);
    url.setQuery(q);

    get(url.toString(), [this, hash](const QJsonObject &json) {
        const QJsonArray candidates = json.value(QStringLiteral("candidates")).toArray();
        if (candidates.isEmpty()) { // 无歌词
            qWarning() << "[lyric] 没有找到歌词候选（可能是歌曲没有歌词，或 hash/duration 不对）";
            QVariantMap empty;
            empty.insert(QStringLiteral("info"), QVariantList{});
            empty.insert(QStringLiteral("translate"), QVariantList{});
            empty.insert(QStringLiteral("hash"), hash);
            emit resultReady(QStringLiteral("getLyricInfo"), empty, Source);
            return;
        }
        const QJsonObject first = candidates.first().toObject();
        const QString id = QString::number(first.value(QStringLiteral("id")).toVariant().toLongLong());
        const QString accesskey = first.value(QStringLiteral("accesskey")).toString();

        QUrl url2(QStringLiteral("http://lyrics.kugou.com/download"));
        QUrlQuery q2;
        q2.addQueryItem(QStringLiteral("ver"), QStringLiteral("1"));
        q2.addQueryItem(QStringLiteral("client"), QStringLiteral("pc"));
        q2.addQueryItem(QStringLiteral("id"), id);
        q2.addQueryItem(QStringLiteral("accesskey"), accesskey);
        q2.addQueryItem(QStringLiteral("fmt"), QStringLiteral("krc"));
        q2.addQueryItem(QStringLiteral("charset"), QStringLiteral("utf8"));
        url2.setQuery(q2);

        get(url2.toString(), [this, hash](const QJsonObject &json2) {
            QVariantMap data;
            const QString content = json2.value(QStringLiteral("content")).toString();
            if (!content.isEmpty()) {
                const QString krc = decodeKrc(content.toLatin1());
                const QVariantList info = krcToLyrics(krc);
                const QVariantList trans = krcTranslations(krc);
                data.insert(QStringLiteral("info"), info);
                data.insert(QStringLiteral("translate"), trans);
            } else {
                qWarning() << "[lyric] content 为空，歌词下载失败";
                data.insert(QStringLiteral("info"), QVariantList{});
                data.insert(QStringLiteral("translate"), QVariantList{});
            }
            data.insert(QStringLiteral("hash"), hash);
            emit resultReady(QStringLiteral("getLyricInfo"), data, Source);
        });
    });
}

// 私人漫游/私人雷达：酷狗无 personal_fm/recommend_songs，用榜单等价实现。
// rankid 6666 = 飙升榜（最新潮流），8888 = TOP500（全网热门）。
void KugouApi::getPersonalFm(int page, int pageSize)
{
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/rank/song"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
    q.addQueryItem(QStringLiteral("ranktype"), QStringLiteral("2"));
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("pagesize"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("area_code"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    q.addQueryItem(QStringLiteral("rankid"), QStringLiteral("8888"));
    q.addQueryItem(QStringLiteral("with_res_tag"), QStringLiteral("1"));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("data")).toObject()
                                     .value(QStringLiteral("info")).toArray()) {
            const QJsonObject s = v.toObject();
            const QString filename = s.value(QStringLiteral("filename")).toString();
            const QString title = s.value(QStringLiteral("songname")).toString();
            const QJsonObject transparam = s.value(QStringLiteral("trans_param")).toObject();
            const QJsonObject tp = s.value(QStringLiteral("trans_param")).toObject();
            const QString hash = s.value(QStringLiteral("hash")).toString();
            const QJsonArray artistList = s.value("authors").toArray();
            const QJsonObject artistValue = artistList.at(0).toObject();
            const QString artist = artistValue.value(QStringLiteral("author_name")).toString();
            const int pay320 = s.value(QStringLiteral("pay_type_320")).toInt();
            const QString hq = pay320 != 3
                                   ? s.value(QStringLiteral("320hash")).toString()
                                             .isEmpty()
                                         ? tp.value(QStringLiteral("ogg_320_hash")).toString()
                                         : s.value(QStringLiteral("320hash")).toString()
                                   : hash;
            const QString sq = s.value(QStringLiteral("pay_type_sq")).toInt() == 0
                                   ? s.value(QStringLiteral("sqhash")).toString()
                                   : hash;
            info << ApiCommon::song(
                title,
                artist,
                transparam.value(QStringLiteral("union_cover")).toString(),
                hash,
                s.value(QStringLiteral("duration")).toInt(),
                s.value(QStringLiteral("album_name")).toString(),
                hq, sq,
                s.value(QStringLiteral("pay_type")).toInt(1));
        }
        emit resultReady(QStringLiteral("getPersonalFm"),
                         ApiCommon::listResult(info), Source);
    });
}

void KugouApi::getPersonalRadar(int page, int pageSize)
{
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/rank/song"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("version"), QStringLiteral("9108"));
    q.addQueryItem(QStringLiteral("ranktype"), QStringLiteral("2"));
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("pagesize"), QString::number(pageSize));
    q.addQueryItem(QStringLiteral("area_code"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    q.addQueryItem(QStringLiteral("rankid"), QStringLiteral("6666"));
    q.addQueryItem(QStringLiteral("with_res_tag"), QStringLiteral("1"));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("data")).toObject()
                                     .value(QStringLiteral("info")).toArray()) {
            const QJsonObject s = v.toObject();
            const QString filename = s.value(QStringLiteral("filename")).toString();
            const QString title = s.value(QStringLiteral("songname")).toString();
            const QJsonObject transparam = s.value(QStringLiteral("trans_param")).toObject();
            const QJsonObject tp = s.value(QStringLiteral("trans_param")).toObject();
            const QString hash = s.value(QStringLiteral("hash")).toString();
            const QJsonArray artistList = s.value("authors").toArray();
            const QJsonObject artistValue = artistList.at(0).toObject();
            const QString artist = artistValue.value(QStringLiteral("author_name")).toString();
            const int pay320 = s.value(QStringLiteral("pay_type_320")).toInt();
            const QString hq = pay320 != 3
                                   ? s.value(QStringLiteral("320hash")).toString()
                                             .isEmpty()
                                         ? tp.value(QStringLiteral("ogg_320_hash")).toString()
                                         : s.value(QStringLiteral("320hash")).toString()
                                   : hash;
            const QString sq = s.value(QStringLiteral("pay_type_sq")).toInt() == 0
                                   ? s.value(QStringLiteral("sqhash")).toString()
                                   : hash;
            info << ApiCommon::song(
                title,
                artist,
                transparam.value(QStringLiteral("union_cover")).toString(),
                hash,
                s.value(QStringLiteral("duration")).toInt(),
                s.value(QStringLiteral("album_name")).toString(),
                hq, sq,
                s.value(QStringLiteral("pay_type")).toInt(1));
        }
        emit resultReady(QStringLiteral("getPersonalRadar"),
                         ApiCommon::listResult(info), Source);
    });
}
