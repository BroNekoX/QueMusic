// SPDX-License-Identifier: MIT
#include "pvgeometry.h"

#include <QtGlobal>
#include <algorithm>

namespace PvGeo {
namespace {

// QString 里缓存的 r2 结果没必要, 直接 append。用 QString::number 的 'f',2
// 再掐掉尾零, 等价于 JS 对"已经是 0.01 倍数"的 double 的最短往返打印。
inline void appendJsNumber(QString &s, double v)
{
    const double x = std::floor(v * 100.0 + 0.5) / 100.0;
    const auto xi = static_cast<qint32>(x); // JS 的 (x|0): 向零截断并按 int32 环绕
    if (static_cast<double>(xi) == x) {
        s += QString::number(xi);
        return;
    }
    QString t = QString::number(x, 'f', 2);
    if (t.contains(QLatin1Char('.'))) {
        while (t.endsWith(QLatin1Char('0')))
            t.chop(1);
        if (t.endsWith(QLatin1Char('.')))
            t.chop(1);
    }
    if (t == QLatin1String("-0"))
        t = QStringLiteral("0");
    s += t;
}

} // namespace

QString jsNumber(double v)
{
    QString s;
    s.reserve(12);
    appendJsNumber(s, v);
    return s;
}

QList<double> transformCommands(const QList<double> &commands, const Mat &m)
{
    QList<double> out;
    out.resize(commands.size());
    const double *d = commands.constData();
    double *o = out.data();
    const int n = commands.size();
    int i = 0;
    while (i < n) {
        const int code = static_cast<int>(d[i]);
        o[i] = d[i];
        ++i;
        const int args = argsOf(code);
        for (int j = 0; j < args; j += 2) {
            applyPoint(m, d[i + j], d[i + j + 1], &o[i + j], &o[i + j + 1]);
        }
        i += args;
    }
    return out;
}

std::array<double, 4> boundsOfCommands(const QList<double> &commands)
{
    std::array<double, 4> b { 0, 0, 0, 0 };
    const double *d = commands.constData();
    const int n = commands.size();
    int i = 0;
    bool first = true;
    while (i < n) {
        const int code = static_cast<int>(d[i]);
        ++i;
        const int args = argsOf(code);
        for (int j = 0; j < args; j += 2) {
            const double x = d[i + j];
            const double y = d[i + j + 1];
            if (first) {
                b = { x, y, x, y };
                first = false;
            } else {
                b[0] = std::min(b[0], x);
                b[1] = std::min(b[1], y);
                b[2] = std::max(b[2], x);
                b[3] = std::max(b[3], y);
            }
        }
        i += args;
    }
    // 控制点也算进去: 与原 JS 版本 (用录制点做包围盒) 保持一致
    return b;
}

QString svgFromCommands(const double *d, int n)
{
    QString s;
    s.reserve(n * 8 + 8); // 一个数最多 8 字符左右, 预分配避免反复扩容
    int i = 0;
    while (i < n) {
        const int code = static_cast<int>(d[i]);
        ++i;
        switch (code) {
        case MoveTo:
            s += QLatin1Char('M');
            appendJsNumber(s, d[i]);
            s += QLatin1Char(' ');
            appendJsNumber(s, d[i + 1]);
            i += 2;
            break;
        case LineTo:
            s += QLatin1Char('L');
            appendJsNumber(s, d[i]);
            s += QLatin1Char(' ');
            appendJsNumber(s, d[i + 1]);
            i += 2;
            break;
        case QuadTo:
            s += QLatin1Char('Q');
            appendJsNumber(s, d[i]);
            s += QLatin1Char(' ');
            appendJsNumber(s, d[i + 1]);
            s += QLatin1Char(' ');
            appendJsNumber(s, d[i + 2]);
            s += QLatin1Char(' ');
            appendJsNumber(s, d[i + 3]);
            i += 4;
            break;
        case CubicTo:
            s += QLatin1Char('C');
            for (int k = 0; k < 6; ++k) {
                if (k)
                    s += QLatin1Char(' ');
                appendJsNumber(s, d[i + k]);
            }
            i += 6;
            break;
        default:
            s += QLatin1Char('Z');
            break;
        }
    }
    return s;
}

QString svgFromCommands(const QList<double> &commands)
{
    return svgFromCommands(commands.constData(), commands.size());
}

QString svgFromCommandsTransformed(const double *d, int n, const Mat &m)
{
    QString s;
    s.reserve(n * 8 + 8);
    int i = 0;
    while (i < n) {
        const int code = static_cast<int>(d[i]);
        ++i;
        const int args = argsOf(code);
        if (args == 0) {
            s += QLatin1Char('Z');
            continue;
        }
        // 变换后的点先算出来, 再按指令码格式化成同样的布局
        double p[6];
        for (int j = 0; j < args; j += 2)
            applyPoint(m, d[i + j], d[i + j + 1], &p[j], &p[j + 1]);
        switch (code) {
        case MoveTo: s += QLatin1Char('M'); break;
        case LineTo: s += QLatin1Char('L'); break;
        case QuadTo: s += QLatin1Char('Q'); break;
        default:     s += QLatin1Char('C'); break;
        }
        for (int j = 0; j < args; ++j) {
            if (j)
                s += QLatin1Char(' ');
            appendJsNumber(s, p[j]);
        }
        i += args;
    }
    return s;
}

QString svgFromCommandsTransformed(const QList<double> &commands, const Mat &m)
{
    return svgFromCommandsTransformed(commands.constData(), commands.size(), m);
}

} // namespace PvGeo