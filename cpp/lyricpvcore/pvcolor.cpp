// SPDX-License-Identifier: MIT
#include "pvcolor.h"

#include <QtGlobal>
#include <cmath>

namespace PvColor {
namespace {

// 与 JS 版一样有界: 键里带的是 *连续* 的 alpha (每次淡入淡出都变), 不设上限
// 就是每秒几千条的内存泄漏。
constexpr int kCacheMax = 4096;
QHash<QString, QString> g_cache;          // 空 value 表示"解析为透明", 与 JS 缓存 null 一致
quint64 g_hits = 0;
quint64 g_misses = 0;

inline int toInt32(double v)
{
    // JS ToInt32: NaN/±Inf -> 0, 其余向零截断并按 2^32 环绕
    if (!std::isfinite(v))
        return 0;
    const double t = std::trunc(v);
    constexpr double two32 = 4294967296.0;
    double m = std::fmod(t, two32);
    if (m < 0)
        m += two32;
    if (m >= 2147483648.0)
        m -= two32;
    return static_cast<int>(m);
}

inline double jsRound(double v) { return std::floor(v + 0.5); }

inline int clampByte(double v)
{
    if (!(v > 0))
        return 0;
    if (v > 255)
        return 255;
    return static_cast<int>(v);
}

inline int clampAlphaByte(double v)
{
    if (!(v > 0))
        return 0;
    if (v >= 1)
        return 255;
    return static_cast<int>(jsRound(v * 255.0));
}

inline int hexVal(QChar c)
{
    const ushort u = c.unicode();
    if (u >= '0' && u <= '9')
        return u - '0';
    if (u >= 'a' && u <= 'f')
        return u - 'a' + 10;
    if (u >= 'A' && u <= 'F')
        return u - 'A' + 10;
    return -1;
}

/// 同 JS 的 `n.toString(16)`, n >= 0 的整数
QString toHex(qint64 n)
{
    return QString::number(n, 16);
}

/// 同 JS `String(x).padStart(2,'0')` 里的补零
QString pad2(const QString &s)
{
    return s.size() >= 2 ? s : QString(2 - s.size(), QChar(u'0')) + s;
}

/// 同 JS `J.hex`: 忽略大小写之外的一切, 只取前 6 位十六进制
void hexChannels(const QString &h, int *r, int *g, int *b)
{
    QString s = h;
    s.remove(QLatin1Char('#'));                 // JS 的 String.replace 只删第一个
    if (s.size() == 3) {
        QString d;
        d.reserve(6);
        for (QChar c : s) { d += c; d += c; }
        s = d;
    }
    const int n = parseIntHex(s.left(6), 6);
    *r = (n >> 16) & 255;
    *g = (n >> 8) & 255;
    *b = n & 255;
}

} // namespace

int parseIntHex(const QString &s, int maxDigits)
{
    int v = 0;
    int taken = 0;
    for (QChar c : s) {
        if (taken >= maxDigits)
            break;
        const int d = hexVal(c);
        if (d < 0)
            break;
        v = v * 16 + d;
        ++taken;
    }
    return v;                                    // 一位都没吃到就是 0, 等价于 NaN || 0
}

double parseFloatOr0(const QString &s)
{
    bool ok = false;
    const double v = s.trimmed().toDouble(&ok);
    return ok ? v : 0.0;
}

Rgba parseCss(const QString &c, double alpha)
{
    Rgba out;
    out.a = alpha;
    const QString s = c.trimmed();
    if (!s.isEmpty() && s.at(0) == QLatin1Char('#')) {
        QString h = s.mid(1);
        if (h.size() == 3 || h.size() == 4) {
            QString d;
            d.reserve(h.size() * 2);
            for (QChar ch : h) { d += ch; d += ch; }
            h = d;
        }
        if (h.size() >= 6) {
            out.r = parseIntHex(h.mid(0, 2), 2);
            out.g = parseIntHex(h.mid(2, 2), 2);
            out.b = parseIntHex(h.mid(4, 2), 2);
            if (h.size() >= 8)
                out.a *= parseIntHex(h.mid(6, 2), 2) / 255.0;
        }
    } else {
        // 同 /rgba?\(([^)]+)\)/i
        const int open = s.indexOf(QLatin1Char('('));
        const int close = s.indexOf(QLatin1Char(')'), open + 1);
        const QString head = open > 0 ? s.left(open).trimmed().toLower() : QString();
        if (open >= 0 && close > open && (head == QLatin1String("rgb") || head == QLatin1String("rgba"))) {
            const QStringList p = s.mid(open + 1, close - open - 1).split(QLatin1Char(','));
            if (p.size() > 0) out.r = parseFloatOr0(p.at(0));
            if (p.size() > 1) out.g = parseFloatOr0(p.at(1));
            if (p.size() > 2) out.b = parseFloatOr0(p.at(2));
            if (p.size() > 3) out.a *= parseFloatOr0(p.at(3));
        } else if (s.compare(QLatin1String("transparent"), Qt::CaseInsensitive) == 0) {
            out.a = 0;
        }
    }
    return out;
}

QString normalize(const QString &c, double alpha)
{
    if (alpha <= 0.002)
        return QString();

    // 键只需要区分 (颜色, 量化后的 alpha); 与 JS 版用同样的量化, 保证缓存行为一致
    const QString key = c + QLatin1Char('|')
            + (alpha >= 0.999 ? QStringLiteral("1") : QString::number(alpha, 'f', 2));
    const auto it = g_cache.constFind(key);
    if (it != g_cache.constEnd()) {
        ++g_hits;
        return it.value();
    }
    ++g_misses;

    Rgba p = parseCss(c, alpha);
    if (p.a <= 0.002) {
        if (g_cache.size() >= kCacheMax)
            g_cache.clear();
        g_cache.insert(key, QString());          // 空串 = JS 里的 null
        return QString();
    }
    const int rgb = ((toInt32(p.r) & 255) << 16) | ((toInt32(p.g) & 255) << 8) | (toInt32(p.b) & 255);
    QString v = QChar(u'#');
    v += pad2(toHex(clampAlphaByte(p.a)));
    v += QStringLiteral("%1").arg(rgb, 6, 16, QChar(u'0'));
    if (g_cache.size() >= kCacheMax)
        g_cache.clear();
    g_cache.insert(key, v);
    return v;
}

void clearCache()
{
    g_cache.clear();
    g_hits = g_misses = 0;
}

qint64 cacheSize() { return g_cache.size(); }

QString mix(const QString &h1, const QString &h2, double t)
{
    int a0, a1, a2, b0, b1, b2;
    hexChannels(h1, &a0, &a1, &a2);
    hexChannels(h2, &b0, &b1, &b2);
    const double l0 = a0 + (b0 - a0) * t;
    const double l1 = a1 + (b1 - a1) * t;
    const double l2 = a2 + (b2 - a2) * t;
    QString s = QChar(u'#');
    s += pad2(toHex(static_cast<qint64>(jsRound(l0))));
    s += pad2(toHex(static_cast<qint64>(jsRound(l1))));
    s += pad2(toHex(static_cast<qint64>(jsRound(l2))));
    return s;
}

double luminance(const QString &h)
{
    int r, g, b;
    hexChannels(h, &r, &g, &b);
    return (0.2126 * r + 0.7152 * g + 0.0722 * b) / 255.0;
}

QString withAlpha(const QString &argb, double mul)
{
    if (argb.size() < 9 || argb.at(0) != QLatin1Char('#') || mul >= 0.999)
        return argb;
    const double a = parseIntHex(argb.mid(1, 2), 2) / 255.0 * mul;
    return QString(QChar(u'#')) + pad2(toHex(clampAlphaByte(a))) + argb.mid(3);
}

} // namespace PvColor