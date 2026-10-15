// SPDX-License-Identifier: MIT
//
// LyricPvCore — 显示列表几何: 路径指令的变换、包围盒与 SVG 生成。
//
// 这一层是纯计算, 不依赖 QML/QObject, 因此可以被任何"把 Canvas2D 调用流录制成
// 显示列表"的歌词 PV 引擎复用, 也可以单独做单元测试 (见 tests/tst_pvgeometry.cpp)。
//
// 指令格式 (与 JIZURA for QueMusic 的录制器一致, 一个扁平 double 数组):
//
//     code 0 = M  后接 2 个数 (x, y)
//     code 1 = L  后接 2 个数
//     code 2 = Q  后接 4 个数 (cx, cy, x, y)
//     code 3 = C  后接 6 个数 (c1x, c1y, c2x, c2y, x, y)
//     code 4 = Z  后接 0 个数
//
// 之所以用扁平数组而不是结构体: 录制阶段可以整段做仿射变换、裁剪、合并,
// 不必重新分配对象, 这也是原 JS 版本的关键设计。
#pragma once

#include <QList>
#include <QString>
#include <array>
#include <cmath>

namespace PvGeo {

// 指令码, 与录制器共享
enum Cmd : int { MoveTo = 0, LineTo = 1, QuadTo = 2, CubicTo = 3, Close = 4 };

/// 每个指令码携带的参数个数
inline constexpr int argsOf(int code)
{
    switch (code) {
    case MoveTo:
    case LineTo:  return 2;
    case QuadTo:  return 4;
    case CubicTo: return 6;
    default:      return 0;
    }
}

/// 2x3 仿射矩阵 [a, b, c, d, e, f], canvas 顺序 (与 Canvas2D 的 setTransform 相同)
using Mat = std::array<double, 6>;

inline constexpr Mat identity() { return { 1, 0, 0, 1, 0, 0 }; }

/// m * n (先做 n 再做 m)
inline Mat multiply(const Mat &m, const Mat &n)
{
    return { m[0] * n[0] + m[2] * n[1],
             m[1] * n[0] + m[3] * n[1],
             m[0] * n[2] + m[2] * n[3],
             m[1] * n[2] + m[3] * n[3],
             m[0] * n[4] + m[2] * n[5] + m[4],
             m[1] * n[4] + m[3] * n[5] + m[5] };
}

inline double determinant(const Mat &m) { return m[0] * m[3] - m[1] * m[2]; }

/// 平均缩放系数 (μ), 描边宽度/字号要按它换算
inline double uniformScale(const Mat &m)
{
    const double d = std::abs(determinant(m));
    return d > 1e-9 ? std::sqrt(d) : 1e-6;
}

inline void applyPoint(const Mat &m, double x, double y, double *outX, double *outY)
{
    *outX = m[0] * x + m[2] * y + m[4];
    *outY = m[1] * x + m[3] * y + m[5];
}

/// 逐点做仿射变换; 返回与输入等长的新数组
QList<double> transformCommands(const QList<double> &commands, const Mat &m);

/// 轴对齐包围盒 -> [x0, y0, x1, y1]; 空路径返回全 0
std::array<double, 4> boundsOfCommands(const QList<double> &commands);

/// JS `String(Math.round(v*100)/100)` 的等价实现
///
/// 必须逐位对齐: 录制结果要和原 JS 版本产出完全相同的 SVG 字符串, 否则同一个
/// 画面在移植前后会有像素级差异, 也无法用字符串比对做回归测试。
/// 注意两点: (1) JS 的 Math.round 是 floor(x+0.5) (向 +∞ 取整), C++ 的
/// std::round 是"远离零", 负数时不同; (2) JS `x === (x|0)` 里的 `|0` 是 int32
/// 截断, 所以整数判定要按 int32 语义走。
QString jsNumber(double v);

/// 把一条指令流生成 SVG path 字符串 (原地不做变换)
QString svgFromCommands(const QList<double> &commands);
QString svgFromCommands(const double *data, int count);

/// 变换 + 生成, 合成一步 (热路径, 避免中间数组)
QString svgFromCommandsTransformed(const double *data, int count, const Mat &m);
QString svgFromCommandsTransformed(const QList<double> &commands, const Mat &m);

} // namespace PvGeo