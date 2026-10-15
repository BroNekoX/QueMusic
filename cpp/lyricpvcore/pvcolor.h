// SPDX-License-Identifier: MIT
//
// LyricPvCore — 颜色运算 (纯计算, 不依赖 QML)。
//
// 语义与 JIZURA for QueMusic 的 JS 版逐位对齐, 理由同 pvgeometry.h: 移植前后必须
// 产出完全一样的颜色字符串。用到的 JS 习惯在这里都能对上:
//   * `x & 255` 是 ToInt32 (向零截断 + int32 环绕), 不是 C++ 的取模;
//   * `Math.round` 是 floor(x+0.5);
//   * `parseInt(s,16) || 0` 只吃开头连续的十六进制位, 失败给 0;
//   * 一切都输出成 #AARRGGBB, QML 可以直接解析。
#pragma once

#include <QChar>
#include <QHash>
#include <QString>

namespace PvColor {

/// 解析结果: 0..255 的通道 + 0..1 的 alpha, 且已经乘过外部 alpha
struct Rgba {
    double r = 0;
    double g = 0;
    double b = 0;
    double a = 1;
};

/// 同 JS `parseInt(h, 16) || 0`: 只消费开头连续的十六进制位
int parseIntHex(const QString &s, int maxDigits);

/// 同 JS `parseFloat(s) || 0`
double parseFloatOr0(const QString &s);

/// `#rgb` / `#rgba` / `#rrggbb` / `#rrggbbaa` / `rgb(...)` / `rgba(...)` / `transparent`
Rgba parseCss(const QString &c, double alpha = 1.0);

/// 同 JS `normColor(c, a)`: 返回 `#AARRGGBB`, 透明时返回空字符串
QString normalize(const QString &c, double alpha = 1.0);

/// 清空内部缓存 (插件实例销毁时调用, 避免跨实例留着上一份 plan 的键)
void clearCache();
qint64 cacheSize();

/// 同 JS `J.mix`: 两色按 t 线性插值, 输出 `#rrggbb`
QString mix(const QString &h1, const QString &h2, double t);

/// 同 JS `J.lum`: Rec.709 相对亮度, 0..1
double luminance(const QString &h);

/// 同 JS `mulColorAlpha`: 把已有 `#AARRGGBB` 的 alpha 再乘一个系数
QString withAlpha(const QString &argb, double mul);

} // namespace PvColor