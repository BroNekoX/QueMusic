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
#include <QDateTime>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <QJsonValue>
#include <QRandomGenerator>
#include <QStringList>
#include <vector>

#include "ApiCommon.h"
#include "ApiHttp.h"

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

// "歌手 - 歌名"：歌手名可能含 '-'，只在第一个 '-' 处分割
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

// 部分歌单接口用 4294967295 表示播放量缺失，统一按 0 处理（否则卡片会显示成 429496 万）
qint64 playCount(const QJsonObject &o)
{
    const qint64 v = qint64(o.value(QStringLiteral("playcount")).toDouble());
    return v >= 4294967295 ? 0 : v;
}

// 歌单列表 → 统一字段（imgurl 的 {size} 占位符换成小图）。
// 歌单卡片的 duration 槽位存的是「曲目数」：网易云用 trackCount、B 站用 total，酷狗用 songcount。
// 接口不带这个字段时为 0，卡片会隐藏曲目数而不是显示「0 首」
QVariantList parsePlaylists(const QJsonArray &arr)
{
    QVariantList info;
    for (const QJsonValue &v : arr) {
        const QJsonObject s = v.toObject();
        QString cover = s.value(QStringLiteral("imgurl")).toString();
        if (cover.contains(QStringLiteral("{size}")))
            cover.replace(QStringLiteral("{size}"), QStringLiteral("64"));
        info << ApiCommon::song(
            s.value(QStringLiteral("specialname")).toString(),
            s.value(QStringLiteral("username")).toString(),
            cover,
            QString::number(s.value(QStringLiteral("specialid")).toVariant().toLongLong()),
            s.value(QStringLiteral("songcount")).toInt(),
            s.value(QStringLiteral("intro")).toString(),
            QString(), QString(), 0,
            playCount(s));
    }
    return info;
}

// 高清(320k) hash：pay_type_320 == 3 表示需付费，回退普通 hash；
// 没有 320hash 时用 trans_param.ogg_320_hash（部分曲目只提供 ogg）。
QString hqHashFrom(const QJsonObject &song, const QString &base)
{
    if (song.value(QStringLiteral("pay_type_320")).toInt() == 3)
        return base;
    const QString hq = song.value(QStringLiteral("320hash")).toString();
    if (!hq.isEmpty())
        return hq;
    const QString ogg = song.value(QStringLiteral("trans_param"))
                            .toObject()
                            .value(QStringLiteral("ogg_320_hash"))
                            .toString();
    return ogg.isEmpty() ? base : ogg;
}

// 无损(SQ) hash：pay_type_sq != 0 表示需付费，回退普通 hash
QString sqHashFrom(const QJsonObject &song, const QString &base)
{
    if (song.value(QStringLiteral("pay_type_sq")).toInt() != 0)
        return base;
    const QString sq = song.value(QStringLiteral("sqhash")).toString();
    return sq.isEmpty() ? base : sq;
}

// 酷狗 pay_type → 统一 paytype（取值约定见 ApiCommon::kPaytypePaid）。
// 实测 920 条搜索结果：pay_type 只会出现 0 / 1 / 3，其中 1 与 3 的 privilege 都是
// 8 或 10（1 从不与 privilege=0 同现），即**非 0 一律是「需要会员/付费」**。
// 旧实现把原始值直传，于是 pay_type=1 的会员曲目落进消费端不认的槽位、永不显示
// VIP 角标（pay_type=3 只是恰好等于约定值才碰巧正常）。
// 字段缺失时按免费算：宁可漏标，也不要把拿不到付费信息的曲目一律标成 VIP。
int paytypeFromKugou(const QJsonObject &song)
{
    return song.value(QStringLiteral("pay_type")).toInt() == 0 ? ApiCommon::kPaytypeFree
                                                              : ApiCommon::kPaytypePaid;
}

// 歌手名：优先 authors[0].author_name，取不到回退 singername。
// 榜单/漫游接口偶发不带 authors，而 QJsonArray::at(0) 对空数组是越界访问
// （release 下是未定义行为，Q_ASSERT 会被编译掉），必须先判空。
QString artistOfSong(const QJsonObject &song)
{
    const QJsonArray authors = song.value(QStringLiteral("authors")).toArray();
    if (!authors.isEmpty()) {
        const QString name =
            authors.at(0).toObject().value(QStringLiteral("author_name")).toString();
        if (!name.isEmpty())
            return name;
    }
    return song.value(QStringLiteral("singername")).toString();
}

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
    if (!out.isEmpty())
        return out;
    out = rawDeflateInflate(data);
    if (out.isEmpty())
        qWarning() << "[krc] 歌词解压失败";
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

// 酷狗签名：salt + 按 key 排序的 k=v 拼接 + salt 的 MD5，各端只差 salt
QByteArray kugouSignature(const QByteArray &salt, const QStringList &keyValues)
{
    QStringList sorted = keyValues;
    sorted.sort();
    return md5Hex(salt + sorted.join(QString()).toUtf8() + salt);
}
} // namespace

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
void KugouApi::getComments(const QString &hash, int page, int pageSize)
{
    // 评论网关要数字 mixsongid（播放信息接口返回的 album_audio_id），按安卓端规则签名
    const auto requestComments = [this, page, pageSize](const QString &mixId) {
        if (m_mid.isEmpty())
            m_mid = kugouMidFromGuid(randomGuid());
        QVariantMap p;
        p.insert(QStringLiteral("mixsongid"), mixId);
        p.insert(QStringLiteral("need_show_image"), QStringLiteral("1"));
        p.insert(QStringLiteral("p"), QString::number(page));
        p.insert(QStringLiteral("pagesize"), QString::number(pageSize));
        p.insert(QStringLiteral("show_classify"), QStringLiteral("1"));
        p.insert(QStringLiteral("show_hotword_list"), QStringLiteral("1"));
        p.insert(QStringLiteral("extdata"), QStringLiteral("0"));
        p.insert(QStringLiteral("code"), QStringLiteral("fc4be23b4e972707f36b8a828a93ba8a"));
        p.insert(QStringLiteral("dfid"), QStringLiteral("-"));
        p.insert(QStringLiteral("mid"), m_mid);
        p.insert(QStringLiteral("uuid"), QStringLiteral("-"));
        p.insert(QStringLiteral("appid"), QStringLiteral("1005"));
        p.insert(QStringLiteral("clientver"), QStringLiteral("20489"));
        p.insert(QStringLiteral("clienttime"), QString::number(QDateTime::currentSecsSinceEpoch()));
        p.insert(QStringLiteral("signature"), QString::fromLatin1(kugouAndroidSignature(p)));

        QUrlQuery query;
        for (auto it = p.cbegin(); it != p.cend(); ++it)
            query.addQueryItem(it.key(), it.value().toString());
        QUrl url(QStringLiteral("https://gateway.kugou.com/mcomment/v1/cmtlist"));
        url.setQuery(query);

        get(url.toString(), [this](const QJsonObject &json) {
            QVariantList info;
            for (const QJsonValue &value : json.value(QStringLiteral("list")).toArray()) {
                const QJsonObject c = value.toObject();
                info << ApiCommon::comment(
                    c.value(QStringLiteral("user_name")).toString(),
                    c.value(QStringLiteral("user_pic")).toString(),
                    c.value(QStringLiteral("content")).toString(),
                    QDateTime::fromString(c.value(QStringLiteral("addtime")).toString(),
                                          QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                        .toSecsSinceEpoch(),
                    c.value(QStringLiteral("like")).toObject().value(QStringLiteral("count")).toInt(),
                    c.value(QStringLiteral("reply_num")).toInt());
            }
            emit resultReady(QStringLiteral("getComments"), ApiCommon::listResult(info), Source);
        });
    };

    // 翻页时直接用缓存，省掉一次 getSongInfo 往返
    const QString cached = m_mixIdCache.value(hash);
    if (!cached.isEmpty()) {
        requestComments(cached);
        return;
    }
    get(QStringLiteral("https://m.kugou.com/app/i/getSongInfo.php?cmd=playInfo&hash=") + hash,
        [this, hash, requestComments](const QJsonObject &meta) {
            const QString mixId = meta.value(QStringLiteral("album_audio_id")).toVariant().toString();
            if (mixId.isEmpty() || mixId == QLatin1String("0")) {
                emit resultReady(QStringLiteral("getComments"), ApiCommon::listResult({}), Source);
                return;
            }
            m_mixIdCache.insert(hash, mixId);
            requestComments(mixId);
        });
}

void KugouApi::get(const QString &url, const Callback &cb)
{
    ApiHttp::get(m_nam, this, QUrl(url), kUa, "https://www.kugou.com/", m_cookie.toUtf8(),
                 [cb](const QByteArray &body) {
                     // 部分接口的 JSON 被 KG_TAG 注释包裹，先剥掉再解析
                     QByteArray data = body;
                     data.replace("<!--KG_TAG_RES_START-->", "").replace("<!--KG_TAG_RES_END-->", "");
                     cb(QJsonDocument::fromJson(data).object());
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
                const QString hash = s.value(QStringLiteral("hash")).toString();
                const QString hq = hqHashFrom(s, hash);
                const QString sq = sqHashFrom(s, hash);
                info << ApiCommon::song(
                    s.value(QStringLiteral("songname")).toString(),
                    s.value(QStringLiteral("singername")).toString(),
                    tp.value(QStringLiteral("union_cover")).toString(),
                    hash,
                    s.value(QStringLiteral("duration")).toInt(),
                    s.value(QStringLiteral("album_name")).toString(),
                    hq, sq,
                    paytypeFromKugou(s));
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
                    playCount(s));
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
                const QString title = titleFromFilename(filename, s.value(QStringLiteral("songname")).toString());
                const QString artist = artistFromFilename(filename);
                const QJsonObject tp = s.value(QStringLiteral("trans_param")).toObject();
                const QString hash = s.value(QStringLiteral("hash")).toString();
                const QString hq = hqHashFrom(s, hash);
                const QString sq = sqHashFrom(s, hash);
                info << ApiCommon::song(
                    title.isEmpty() ? s.value(QStringLiteral("songname")).toString() : title,
                    artist.isEmpty() ? s.value(QStringLiteral("singername")).toString() : artist,
                    tp.value(QStringLiteral("union_cover")).toString(),
                    s.value(QStringLiteral("hash")).toString(),
                    s.value(QStringLiteral("duration")).toInt(),
                    s.value(QStringLiteral("album_name")).toString(),
                    hq, sq,
                    paytypeFromKugou(s));
            }
        }
        emit resultReady(QStringLiteral("searchSongs"),
                         ApiCommon::listResult(info), Source);
    });
}

// 歌单分类：category/list 给出 21 个真实分类
void KugouApi::getPlaylistMenu(int type)
{
    Q_UNUSED(type);
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/category/list"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("apiver"), QStringLiteral("2"));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("data")).toObject()
                                     .value(QStringLiteral("info")).toArray()) {
            const QJsonObject c = v.toObject();
            const QString id = QString::number(c.value(QStringLiteral("categoryid")).toInt());
            info << QVariantMap{
                {QStringLiteral("title"), c.value(QStringLiteral("categoryname")).toString()},
                {QStringLiteral("id"), id},
                {QStringLiteral("tagid"), id},
                {QStringLiteral("category"), id},
                {QStringLiteral("cover"), c.value(QStringLiteral("imgurl")).toString()},
            };
        }
        emit resultReady(QStringLiteral("getPlaylistMenu"),
                         ApiCommon::listResult(info), Source);
    });
}

// 分类歌单：sort=1 为热度排序，返回真实播放量；每个分类各自一套歌单、可持续翻页。
// withsong=1 才会带 songcount（曲目数），代价是每个歌单多回一小段歌曲预览（实测约 4 倍体积）
void KugouApi::getCategoryPlaylists(const QString &categoryid, int page, int pageSize)
{
    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/category/special"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("withsong"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("sort"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("ugc"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("categoryid"), categoryid);
    q.addQueryItem(QStringLiteral("page"), QString::number(qMax(page, 1)));
    q.addQueryItem(QStringLiteral("pagesize"), QString::number(qMax(pageSize, 1)));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        emit resultReady(QStringLiteral("getCategoryPlaylists"),
                         ApiCommon::listResult(parsePlaylists(
                             json.value(QStringLiteral("data")).toObject()
                                 .value(QStringLiteral("info")).toArray())),
                         Source);
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
        emit resultReady(QStringLiteral("getMusicPlaylists"),
                         ApiCommon::listResult(parsePlaylists(
                             json.value(QStringLiteral("data")).toObject()
                                 .value(QStringLiteral("info")).toArray())),
                         Source);
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
            const QString title = titleFromFilename(filename, filename);
            const QString artist = artistFromFilename(filename);
            const QString hash = s.value(QStringLiteral("hash")).toString();
            const QString hq = hqHashFrom(s, hash);
            const QString sq = sqHashFrom(s, hash);
            info << ApiCommon::song(
                title, artist,
                tp.value(QStringLiteral("union_cover")).toString(),
                hash,
                s.value(QStringLiteral("duration")).toInt(),
                s.value(QStringLiteral("album_name")).toString(),
                hq, sq,
                paytypeFromKugou(s));
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
            const QString hq = hqHashFrom(s, hash);
            const QString sq = sqHashFrom(s, hash);
            info << ApiCommon::song(
                s.value(QStringLiteral("songname")).toString(),
                s.value(QStringLiteral("singername")).toString(),
                tp.value(QStringLiteral("union_cover")).toString(),
                hash,
                s.value(QStringLiteral("duration")).toInt(),
                s.value(QStringLiteral("album_name")).toString(),
                hq, sq,
                paytypeFromKugou(s));
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

// 热门歌单：按热度搜歌单，结果自带真实播放量与歌曲数，与分类歌单不同源。
// 关键词逐页轮换，每个词 20 页（实测各词均 ≥22 页，不会翻空），共 160 页持续出新。
void KugouApi::getHotPlaylists(int page, int pageSize)
{
    static const QStringList kKeywords{
        QStringLiteral("热门"), QStringLiteral("古风"), QStringLiteral("粤语"),
        QStringLiteral("摇滚"), QStringLiteral("车载"), QStringLiteral("电音"),
        QStringLiteral("民谣"), QStringLiteral("爵士")};
    constexpr int kPagesPerKeyword = 20;
    const int index = qMax(page, 1) - 1;

    QUrl url(QStringLiteral("http://mobilecdnbj.kugou.com/api/v3/search/special"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("plat"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("keyword"), kKeywords.at(index % kKeywords.size()));
    q.addQueryItem(QStringLiteral("page"),
                   QString::number(index / kKeywords.size() % kPagesPerKeyword + 1));
    q.addQueryItem(QStringLiteral("pagesize"), QString::number(qMax(pageSize, 1)));
    url.setQuery(q);

    get(url.toString(), [this](const QJsonObject &json) {
        QVariantList info;
        for (const QJsonValue &v : json.value(QStringLiteral("data")).toObject()
                                     .value(QStringLiteral("info")).toArray()) {
            const QJsonObject s = v.toObject();
            const QString creator = s.value(QStringLiteral("nickname")).toString();
            info << ApiCommon::song(
                s.value(QStringLiteral("specialname")).toString(),
                creator,
                s.value(QStringLiteral("imgurl")).toString(),
                QString::number(s.value(QStringLiteral("specialid")).toVariant().toLongLong()),
                s.value(QStringLiteral("songcount")).toInt(),
                creator,
                QString(), QString(), 0,
                playCount(s));
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
            const QString hq = hqHashFrom(s, hash);
            const QString sq = sqHashFrom(s, hash);
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
                paytypeFromKugou(s));
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
            const QString artist = artistOfSong(s);
            const QString hash = s.value(QStringLiteral("hash")).toString();
            const QString hq = hqHashFrom(s, hash);
            const QString sq = sqHashFrom(s, hash);
            info << ApiCommon::song(
                title,
                artist,
                tp.value(QStringLiteral("union_cover")).toString(),
                s.value(QStringLiteral("hash")).toString(),
                s.value(QStringLiteral("duration")).toInt(),
                s.value(QStringLiteral("album_name")).toString(),
                hq, sq,
                paytypeFromKugou(s));
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
            const QString title = titleFromFilename(filename, filename);
            const QString artist = artistFromFilename(filename);
            const QString hash = s.value(QStringLiteral("hash")).toString();
            const QString hq = hqHashFrom(s, hash);
            const QString sq = sqHashFrom(s, hash);
            info << ApiCommon::song(
                title, artist,
                tp.value(QStringLiteral("union_cover")).toString(),
                hash,
                s.value(QStringLiteral("duration")).toInt(),
                s.value(QStringLiteral("album_name")).toString(),
                hq, sq,
                paytypeFromKugou(s));
        }
        emit resultReady(QStringLiteral("getSingerSongs"),
                         ApiCommon::listResult(info), Source);
    });
}

// 歌曲播放信息（type: 0 播放 / 1 下载）
// 酷狗音质由 hash 决定（普通/高清/无损是三个不同 hash），而免费接口 getSongInfo.php
// 只返回 128k 文件，所以地址用 trackercdn 按传入 hash 取，元信息仍取 getSongInfo.php，两者并行。
void KugouApi::getMusicInfo(const QString &hash, int type)
{
    if (hash.isEmpty()) {
        qWarning() << "[kugou] getMusicInfo: hash 为空";
        QVariantMap data;
        data.insert(QStringLiteral("hash"), hash);
        data.insert(QStringLiteral("type"), type);
        data.insert(QStringLiteral("errReason"), QStringLiteral("unavailable"));
        emit resultReady(QStringLiteral("getMusicInfo"), data, Source);
        return;
    }
    auto st = QSharedPointer<PlayRequest>::create();
    requestPlayMeta(hash, type, st);
    requestTrackerUrl(hash, type, st);
}

// 元信息（歌名/歌手/封面/时长）+ 128k 地址兜底
void KugouApi::requestPlayMeta(const QString &hash, int type, const QSharedPointer<PlayRequest> &st)
{
    QUrl url(QStringLiteral("https://m.kugou.com/app/i/getSongInfo.php?cmd=playInfo&hash=")
             + hash);
    get(url.toString(), [this, hash, type, st](const QJsonObject &json) {
        // 与 JS 版一致：兼容 {data:{...}} 和直接 {...} 两种返回结构
        QJsonObject d = json.value(QStringLiteral("data")).toObject();
        if (d.isEmpty())
            d = json;

        // backup_url 新版返回数组，旧版为字符串，统一兼容
        const QJsonValue bv = d.value(QStringLiteral("backup_url"));
        QString backupUrl;
        if (bv.isArray()) {
            const QJsonArray backupArr = bv.toArray();
            if (!backupArr.isEmpty())
                backupUrl = backupArr.first().toString();
        } else {
            backupUrl = bv.toString();
        }
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
        data.insert(QStringLiteral("songName"), songName.isEmpty() ? fileNameRaw : songName);
        data.insert(QStringLiteral("author_name"), d.value(QStringLiteral("author_name")).toString());
        data.insert(QStringLiteral("singer_img"), d.value(QStringLiteral("imgUrl")).toString());
        data.insert(QStringLiteral("album_img"), albumImg);
        data.insert(QStringLiteral("timeLength"),
                    d.value(QStringLiteral("timeLength")).toDouble()
                        ? d.value(QStringLiteral("timeLength")).toDouble()
                        : d.value(QStringLiteral("duration")).toDouble());
        data.insert(QStringLiteral("fileName"), fileName);

        st->meta = data;
        st->metaUrl = playUrl;
        st->metaBackupUrl = backupUrl;
        st->metaDone = true;
        finishPlayInfo(hash, type, st);
    });
}

// 按 hash 精确取址（trackercdn v2）：status=1 有权，status=2 表示该音质需会员/无版权
void KugouApi::requestTrackerUrl(const QString &hash, int type, const QSharedPointer<PlayRequest> &st)
{
    const QString key = QString::fromLatin1(
        QCryptographicHash::hash((hash + QStringLiteral("kgcloudv2")).toUtf8(),
                                 QCryptographicHash::Md5).toHex());
    QUrl url(QStringLiteral("https://trackercdn.kugou.com/i/v2/"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("key"), key);
    q.addQueryItem(QStringLiteral("hash"), hash);
    q.addQueryItem(QStringLiteral("br"), QStringLiteral("hq")); // 音质由 hash 决定，br 无实际作用
    q.addQueryItem(QStringLiteral("appid"), QStringLiteral("1005"));
    q.addQueryItem(QStringLiteral("pid"), QStringLiteral("2"));
    q.addQueryItem(QStringLiteral("cmd"), QStringLiteral("25"));
    q.addQueryItem(QStringLiteral("behavior"), QStringLiteral("play"));
    url.setQuery(q);

    get(url.toString(), [this, hash, type, st](const QJsonObject &json) {
        const int status = json.value(QStringLiteral("status")).toInt();
        const QJsonValue uv = json.value(QStringLiteral("url"));
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
            st->trackerExt = json.value(QStringLiteral("extName")).toString();
            st->trackerRate = json.value(QStringLiteral("bitRate")).toInt();
        }
        qDebug() << "[kugou] trackercdn 取址 hash:" << hash.left(8)
                 << "status:" << status << "bitRate:" << st->trackerRate
                 << "ext:" << st->trackerExt;
        st->trackerDone = true;
        finishPlayInfo(hash, type, st);
    });
}

// 两路返回后合并：优先 trackercdn（精确音质）→ 回退 128k → 再回退登录签名接口
void KugouApi::finishPlayInfo(const QString &hash, int type, const QSharedPointer<PlayRequest> &st)
{
    if (!st->metaDone || !st->trackerDone)
        return;

    QVariantMap data = st->meta;
    const QString fallbackUrl = st->metaUrl.isEmpty() ? st->metaBackupUrl : st->metaUrl;
    QString playUrl = st->trackerUrl.isEmpty() ? fallbackUrl : st->trackerUrl;

    // 文件名后缀跟随实际音质（无损存 .flac，而不是 .mp3）
    if (!st->trackerUrl.isEmpty() && !st->trackerExt.isEmpty()) {
        QString fileName = data.value(QStringLiteral("fileName")).toString();
        const int dot = fileName.lastIndexOf(QLatin1Char('.'));
        if (dot > 0)
            fileName = fileName.left(dot) + QLatin1Char('.') + st->trackerExt;
        data.insert(QStringLiteral("fileName"), fileName);
    }
    data.insert(QStringLiteral("url"), playUrl);
    data.insert(QStringLiteral("backup_url"), fallbackUrl);
    data.insert(QStringLiteral("hash"), hash);
    data.insert(QStringLiteral("type"), type);

    if (playUrl.isEmpty()) {
        // trackercdn status=2 说明该曲对该账号无权限（会员/购买），用于给出准确提示
        const QString reason = st->trackerStatus == 2 ? QStringLiteral("vip")
                                                     : QStringLiteral("unavailable");
        if (!cookieValue(QStringLiteral("token")).isEmpty()) {
            qDebug() << "[kugou] 免费通道无地址，转登录签名接口重试 hash:" << hash.left(8);
            requestSignedPlayInfo(hash, type, reason);
            return;
        }
        data.insert(QStringLiteral("errReason"), reason);
        qWarning() << "[kugou] 未登录且无可用地址（VIP/付费曲目需要会员或已购买）";
    }
    emit resultReady(QStringLiteral("getMusicInfo"), data, Source);
}

QString KugouApi::cookieValue(const QString &key) const
{
    const QStringList pairs = m_cookie.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &p : pairs) {
        const int i = p.indexOf(QLatin1Char('='));
        if (i > 0 && p.left(i).trimmed() == key)
            return p.mid(i + 1).trimmed();
    }
    return QString();
}

// wwwapi 播放接口（web 签名 + 登录 token），有会员/已购买的账号可取到完整播放地址
void KugouApi::requestSignedPlayInfo(const QString &hash, int type, const QString &reason)
{
    const QString mid = m_mid.isEmpty() ? kugouMidFromGuid(randomGuid()) : m_mid;
    QJsonObject params;
    params.insert(QStringLiteral("appid"), 1014);
    params.insert(QStringLiteral("area_code"), 1);
    params.insert(QStringLiteral("clienttime"),
                  QString::number(QDateTime::currentMSecsSinceEpoch()));
    params.insert(QStringLiteral("clientver"), 20000);
    params.insert(QStringLiteral("dfid"), m_dfid.isEmpty() ? QStringLiteral("-") : m_dfid);
    params.insert(QStringLiteral("hash"), hash);
    params.insert(QStringLiteral("mid"), mid);
    params.insert(QStringLiteral("page"), 1);
    params.insert(QStringLiteral("platid"), 4);
    params.insert(QStringLiteral("srcappid"), 2919);
    params.insert(QStringLiteral("token"), cookieValue(QStringLiteral("token")));
    params.insert(QStringLiteral("type"), 1);
    params.insert(QStringLiteral("userid"),
                  cookieValue(QStringLiteral("userid")).isEmpty()
                      ? QStringLiteral("0")
                      : cookieValue(QStringLiteral("userid")));
    params.insert(QStringLiteral("uuid"), mid);
    params.insert(QStringLiteral("signature"),
                  QString::fromLatin1(kugouWebSignature(params)));

    QUrlQuery q;
    const QStringList keys = params.keys();
    for (const QString &k : keys)
        q.addQueryItem(k, params.value(k).toVariant().toString());
    QUrl url(QStringLiteral("https://wwwapi.kugou.com/play/songinfo"));
    url.setQuery(q);

    get(url.toString(), [this, hash, type, reason](const QJsonObject &json) {
        const QJsonObject d = json.value(QStringLiteral("data")).toObject();
        const QString playUrl = d.value(QStringLiteral("play_url")).toString();
        const QJsonArray backupArr = d.value(QStringLiteral("backup_url")).toArray();
        const QString backup = backupArr.isEmpty() ? QString() : backupArr.first().toString();
        qDebug() << "[kugou] 签名播放接口 url:" << (playUrl.isEmpty() ? backup : playUrl)
                 << "err:" << json.value(QStringLiteral("err_code"));
        if (playUrl.isEmpty() && backup.isEmpty()) {
            // 无权限/风控：必须回传结果，否则上层 loadState 一直为 true（界面卡在加载中）
            QVariantMap fail;
            fail.insert(QStringLiteral("url"), QString());
            fail.insert(QStringLiteral("songName"), d.value(QStringLiteral("song_name")).toString());
            fail.insert(QStringLiteral("author_name"),
                        d.value(QStringLiteral("author_name")).toString());
            fail.insert(QStringLiteral("album_img"), d.value(QStringLiteral("img")).toString());
            fail.insert(QStringLiteral("timeLength"),
                        int(d.value(QStringLiteral("timelength")).toDouble() / 1000));
            fail.insert(QStringLiteral("hash"), hash);
            fail.insert(QStringLiteral("type"), type);
            fail.insert(QStringLiteral("errReason"),
                        reason.isEmpty() ? QStringLiteral("unavailable") : reason);
            emit resultReady(QStringLiteral("getMusicInfo"), fail, Source);
            return;
        }

        const QString songName = d.value(QStringLiteral("song_name")).toString();
        QString fileName = d.value(QStringLiteral("audio_name")).toString();
        if (fileName.isEmpty())
            fileName = songName + QStringLiteral(".mp3");
        const int tl = int(d.value(QStringLiteral("timelength")).toDouble() / 1000);

        QVariantMap data;
        data.insert(QStringLiteral("backup_url"), playUrl.isEmpty() ? backup : playUrl);
        data.insert(QStringLiteral("url"), playUrl.isEmpty() ? backup : playUrl);
        data.insert(QStringLiteral("songName"), songName);
        data.insert(QStringLiteral("author_name"), d.value(QStringLiteral("author_name")).toString());
        data.insert(QStringLiteral("singer_img"), d.value(QStringLiteral("img")).toString());
        data.insert(QStringLiteral("album_img"), d.value(QStringLiteral("img")).toString());
        data.insert(QStringLiteral("timeLength"), tl);
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
            const QString artist = artistOfSong(s);
            const QString hq = hqHashFrom(s, hash);
            const QString sq = sqHashFrom(s, hash);
            info << ApiCommon::song(
                title,
                artist,
                transparam.value(QStringLiteral("union_cover")).toString(),
                hash,
                s.value(QStringLiteral("duration")).toInt(),
                s.value(QStringLiteral("album_name")).toString(),
                hq, sq,
                paytypeFromKugou(s));
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
            const QString artist = artistOfSong(s);
            const QString hq = hqHashFrom(s, hash);
            const QString sq = sqHashFrom(s, hash);
            info << ApiCommon::song(
                title,
                artist,
                transparam.value(QStringLiteral("union_cover")).toString(),
                hash,
                s.value(QStringLiteral("duration")).toInt(),
                s.value(QStringLiteral("album_name")).toString(),
                hq, sq,
                paytypeFromKugou(s));
        }
        emit resultReady(QStringLiteral("getPersonalRadar"),
                         ApiCommon::listResult(info), Source);
    });
}
