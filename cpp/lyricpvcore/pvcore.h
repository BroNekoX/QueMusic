// SPDX-License-Identifier: MIT
//
// LyricPvCore — 歌词 PV 的 C++ 计算内核。
// 每帧都要跑的路径拼串与颜色解析原占一帧四成以上耗时，搬到这里做；接口保持通用，
// 任何把 Canvas2D 录制为显示列表的 PV 都能复用。
// 线程约定：只允许在 QML GUI 线程调用（与调用方共享同一份有界缓存）。
#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class LyricPvCore : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    /// pathsToSvg 的记忆化命中/未命中计数, 用来判断缓存是否划算
    Q_PROPERTY(qint64 svgCacheHits READ svgCacheHits NOTIFY statsChanged)
    Q_PROPERTY(qint64 svgCacheMisses READ svgCacheMisses NOTIFY statsChanged)
    Q_PROPERTY(bool svgCacheEnabled READ svgCacheEnabled WRITE setSvgCacheEnabled NOTIFY statsChanged)

public:
    explicit LyricPvCore(QObject *parent = nullptr);

    // ---------------------------------------------------------------- 显示列表
    /// 批量把若干条指令流生成 SVG path —— 每帧只跨一次语言边界。
    ///
    /// `commands` 是所有路径首尾相接的扁平数组 (格式见 pvgeometry.h);
    /// `offsets` 长度 N+1, 第 i 条路径占 [offsets[i], offsets[i+1]);
    /// `matrices` 长度 6*N, 每条路径一个仿射矩阵 (不需要就填单位矩阵)。
    /// 返回 N 个 SVG 字符串, 顺序与 offsets 一致。
    Q_INVOKABLE QStringList pathsToSvg(const QList<double> &commands,
                                       const QList<int> &offsets,
                                       const QList<double> &matrices);

    Q_INVOKABLE QString pathToSvg(const QList<double> &commands);

    Q_INVOKABLE QString pathToSvgTransformed(const QList<double> &commands,
                                             double a, double b, double c,
                                             double d, double e, double f);

    /// 对整条指令流做仿射变换, 返回等长的新指令流
    Q_INVOKABLE QList<double> transformPath(const QList<double> &commands,
                                            double a, double b, double c,
                                            double d, double e, double f);

    /// 包围盒 [x0, y0, x1, y1] (控制点计入, 与 JS 版一致)
    Q_INVOKABLE QList<double> pathBounds(const QList<double> &commands);

    // ------------------------------------------------------------------ 颜色
    /// 同 JS normColor: 任意 CSS 颜色 -> `#AARRGGBB`; 透明返回空串
    Q_INVOKABLE QString normalizeColor(const QString &color, double alpha = 1.0);
    /// 两色按 t 线性插值 -> `#rrggbb`
    Q_INVOKABLE QString mixColors(const QString &a, const QString &b, double t);
    /// Rec.709 相对亮度 0..1
    Q_INVOKABLE double luminance(const QString &color);
    /// 把一个 `#AARRGGBB` 的 alpha 再乘一个系数
    Q_INVOKABLE QString withAlpha(const QString &argb, double mul);

    // ------------------------------------------------------------------ 矩阵
    Q_INVOKABLE QList<double> matrixMultiply(const QList<double> &m, const QList<double> &n);
    Q_INVOKABLE QList<double> matrixApply(const QList<double> &m, double x, double y);
    Q_INVOKABLE double matrixUniformScale(const QList<double> &m);

    // ------------------------------------------------------------ 其它通用件
    /// 缓动曲线: linear/quad/cubic/expo/back/sine 的 in/out/inOut 变体。
    /// 这是给别的 PV 作者复用的便利函数, 不在热路径上 (JS 里做几次乘法更快)。
    Q_INVOKABLE double ease(const QString &kind, double t);

    /// 清空所有内部缓存 (插件实例销毁时调用)
    Q_INVOKABLE void resetCaches();

    qint64 svgCacheHits() const { return m_svgHits; }
    qint64 svgCacheMisses() const { return m_svgMisses; }
    bool svgCacheEnabled() const { return m_svgCacheEnabled; }
    void setSvgCacheEnabled(bool on);

Q_SIGNALS:
    void statsChanged();

private:
    qint64 m_svgHits = 0;
    qint64 m_svgMisses = 0;
    bool m_svgCacheEnabled = true;
};