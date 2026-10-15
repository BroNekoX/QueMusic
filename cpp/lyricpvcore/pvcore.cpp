// SPDX-License-Identifier: MIT
#include "pvcore.h"

#include "pvcolor.h"
#include "pvgeometry.h"

#include <QHash>
#include <QtGlobal>
#include <cmath>

namespace pvdetail {

/// SVG 记忆化的键: 几何 + 矩阵的 128 位指纹。
///
/// 用指纹而不是把几何本身存进键里, 是为了避免每帧复制一份路径数据。代价是理论上
/// 可能撞键 (两个 128 位指纹相同但几何不同), 概率量级 2^-128; 以 60fps x 300 条
/// 路径连续跑一小时 (约 6.5e7 次比较) 也只有 1e-31 量级, 可以忽略。
/// 每次生成都会和缓存里那份几何等价, 所以命中时复用字符串是等价的。
struct SvgKey {
    quint64 a = 0;
    quint64 b = 0;
    bool operator==(const SvgKey &o) const { return a == o.a && b == o.b; }
};

inline size_t qHash(const SvgKey &k, size_t seed = 0)
{
    // 显式限定到全局: 否则这个名字会遮蔽 Qt 自带的 quint64 版本
    return size_t(::qHash(k.a, seed) ^ (::qHash(k.b, seed) << 1));
}

} // namespace pvdetail

using pvdetail::SvgKey;

namespace {

constexpr int kSvgCacheMax = 8192;
QHash<SvgKey, QString> g_svgCache;

SvgKey fingerprint(const double *d, int n, const PvGeo::Mat &m, bool identity)
{
    quint64 h1 = 1469598103934665603ULL;                 // FNV-1a 64
    quint64 h2 = 0x9E3779B97F4A7C15ULL;                  // 另一条独立的混合
    const auto mixBytes = [&](const void *p, int bytes) {
        const auto *b = static_cast<const unsigned char *>(p);
        for (int i = 0; i < bytes; ++i) {
            h1 = (h1 ^ b[i]) * 1099511628211ULL;
            h2 = (h2 + b[i] + 0x165667B19E3779F9ULL) * 0x9E3779B97F4A7C15ULL;
        }
    };
    mixBytes(d, n * int(sizeof(double)));
    if (!identity)
        mixBytes(m.data(), int(sizeof(double)) * 6);
    return SvgKey { h1, h2 };
}

inline PvGeo::Mat matFrom(const QList<double> &v, int offset)
{
    PvGeo::Mat m = PvGeo::identity();
    if (offset >= 0 && offset + 6 <= v.size()) {
        for (int k = 0; k < 6; ++k)
            m[k] = v.at(offset + k);
    }
    return m;
}

} // namespace

LyricPvCore::LyricPvCore(QObject *parent)
    : QObject(parent)
{
}

void LyricPvCore::setSvgCacheEnabled(bool on)
{
    if (m_svgCacheEnabled == on)
        return;
    m_svgCacheEnabled = on;
    if (!on)
        g_svgCache.clear();
    Q_EMIT statsChanged();
}

QStringList LyricPvCore::pathsToSvg(const QList<double> &commands,
                                    const QList<int> &offsets,
                                    const QList<double> &matrices)
{
    const int n = qMax(0, offsets.size() - 1);
    QStringList out;
    out.reserve(n);
    const double *cd = commands.constData();
    const int total = commands.size();

    for (int i = 0; i < n; ++i) {
        const int begin = offsets.at(i);
        const int end = offsets.at(i + 1);
        if (begin < 0 || end > total || end <= begin) {
            out << QString();
            continue;
        }
        const int count = end - begin;
        const PvGeo::Mat m = matFrom(matrices, i * 6);
        const bool isIdentity = (m == PvGeo::identity());

        if (m_svgCacheEnabled) {
            const SvgKey key = fingerprint(cd + begin, count, m, isIdentity);
            const auto it = g_svgCache.constFind(key);
            if (it != g_svgCache.constEnd()) {
                ++m_svgHits;
                out << it.value();
                continue;
            }
            ++m_svgMisses;
            QString s = isIdentity ? PvGeo::svgFromCommands(cd + begin, count)
                                   : PvGeo::svgFromCommandsTransformed(cd + begin, count, m);
            if (g_svgCache.size() >= kSvgCacheMax)
                g_svgCache.clear();
            g_svgCache.insert(key, s);
            out << s;
        } else {
            out << (isIdentity ? PvGeo::svgFromCommands(cd + begin, count)
                               : PvGeo::svgFromCommandsTransformed(cd + begin, count, m));
        }
    }
    return out;
}

QString LyricPvCore::pathToSvg(const QList<double> &commands)
{
    return PvGeo::svgFromCommands(commands);
}

QString LyricPvCore::pathToSvgTransformed(const QList<double> &commands,
                                          double a, double b, double c,
                                          double d, double e, double f)
{
    return PvGeo::svgFromCommandsTransformed(commands, PvGeo::Mat { a, b, c, d, e, f });
}

QList<double> LyricPvCore::transformPath(const QList<double> &commands,
                                         double a, double b, double c,
                                         double d, double e, double f)
{
    return PvGeo::transformCommands(commands, PvGeo::Mat { a, b, c, d, e, f });
}

QList<double> LyricPvCore::pathBounds(const QList<double> &commands)
{
    const auto b = PvGeo::boundsOfCommands(commands);
    return QList<double> { b[0], b[1], b[2], b[3] };
}

QString LyricPvCore::normalizeColor(const QString &color, double alpha)
{
    return PvColor::normalize(color, alpha);
}

QString LyricPvCore::mixColors(const QString &a, const QString &b, double t)
{
    return PvColor::mix(a, b, t);
}

double LyricPvCore::luminance(const QString &color)
{
    return PvColor::luminance(color);
}

QString LyricPvCore::withAlpha(const QString &argb, double mul)
{
    return PvColor::withAlpha(argb, mul);
}

QList<double> LyricPvCore::matrixMultiply(const QList<double> &m, const QList<double> &n)
{
    const auto r = PvGeo::multiply(matFrom(m, 0), matFrom(n, 0));
    return QList<double> { r[0], r[1], r[2], r[3], r[4], r[5] };
}

QList<double> LyricPvCore::matrixApply(const QList<double> &m, double x, double y)
{
    double ox = 0;
    double oy = 0;
    PvGeo::applyPoint(matFrom(m, 0), x, y, &ox, &oy);
    return QList<double> { ox, oy };
}

double LyricPvCore::matrixUniformScale(const QList<double> &m)
{
    return PvGeo::uniformScale(matFrom(m, 0));
}

double LyricPvCore::ease(const QString &kind, double t)
{
    constexpr double c1 = 1.70158;
    constexpr double c3 = c1 + 1.0;
    const QString k = kind.trimmed().toLower();

    if (k == QLatin1String("linear"))
        return t;
    if (k == QLatin1String("inquad"))
        return t * t;
    if (k == QLatin1String("outquad"))
        return 1.0 - (1.0 - t) * (1.0 - t);
    if (k == QLatin1String("inoutquad"))
        return t < 0.5 ? 2.0 * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 2.0) / 2.0;
    if (k == QLatin1String("incubic"))
        return t * t * t;
    if (k == QLatin1String("outcubic"))
        return 1.0 - std::pow(1.0 - t, 3.0);
    if (k == QLatin1String("inoutcubic"))
        return t < 0.5 ? 4.0 * t * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 3.0) / 2.0;
    if (k == QLatin1String("inexpo"))
        return t <= 0.0 ? 0.0 : std::pow(2.0, 10.0 * t - 10.0);
    if (k == QLatin1String("outexpo"))
        return t >= 1.0 ? 1.0 : 1.0 - std::pow(2.0, -10.0 * t);
    if (k == QLatin1String("inback"))
        return c3 * t * t * t - c1 * t * t;
    if (k == QLatin1String("outback"))
        return 1.0 + c3 * std::pow(t - 1.0, 3.0) + c1 * std::pow(t - 1.0, 2.0);
    if (k == QLatin1String("insine"))
        return 1.0 - std::cos(t * M_PI / 2.0);
    if (k == QLatin1String("outsine"))
        return std::sin(t * M_PI / 2.0);
    if (k == QLatin1String("inoutsine"))
        return -(std::cos(M_PI * t) - 1.0) / 2.0;
    return t;
}

void LyricPvCore::resetCaches()
{
    g_svgCache.clear();
    PvColor::clearCache();
    m_svgHits = m_svgMisses = 0;
    Q_EMIT statsChanged();
}