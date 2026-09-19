#version 450 core

// 卡片背景模糊（高质量版）：与 cardblur.frag 完全同一套算法，
// 唯一区别是 TAPS 13（169 次采样）。因为半径公式是 lod0 = log2(半径 / R)，
// R 从 3 变 6 后会自动：基底层级细一级 + 采样密一倍，而模糊半径保持一致。

layout(location = 0) in vec2 qt_TexCoord0;
layout(binding = 1) uniform sampler2D src;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float blur;         // 模糊强度 0~1
    float blurMax;      // 模糊半径上限（设备像素）
    float saturation;   // 饱和度倍数：1.0 = 原图
    float corner;       // 圆角半径（局部像素）
    vec2  cardSize;     // 卡片尺寸（局部像素）
    vec2  texSize;      // 抓取纹理尺寸（设备像素）
    float aa;           // 圆角抗锯齿宽度（局部像素）
    vec4  fillColor;    // 源透明/空缺处的补底色（只用 rgb）
};

layout(location = 0) out vec4 fragColor;

const int TAPS = 13;    // 每轴采样数（奇数）
const int R    = TAPS / 2;

float roundedBoxSDF(vec2 p, vec2 halfSize, float r) {
    vec2 q = abs(p) - halfSize + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

void main() {
    // ===== 1. 模糊 =====
    float radius = max(blurMax * blur, 1.0);
    float lod0   = clamp(log2(radius / float(R)), 0.0, 5.0);
    vec2  texel  = exp2(lod0) / texSize;

    vec4  col   = vec4(0.0);
    float total = 0.0;
    for (int j = -R; j <= R; ++j) {
        float wj = float(R + 1 - abs(j));
        for (int i = -R; i <= R; ++i) {
            float w = wj * float(R + 1 - abs(i));
            col   += textureLod(src, qt_TexCoord0 + vec2(float(i), float(j)) * texel, lod0) * w;
            total += w;
        }
    }
    col /= total;

    // ===== 2. 饱和度 =====
    float gray = dot(col.rgb, vec3(0.299, 0.587, 0.114));
    col.rgb = mix(vec3(gray), col.rgb, saturation);

    // ===== 3. 补底色 =====
    // 抓取纹理是预乘 alpha，所以"叠加一个不透明底色"就是这个式子。
    // 源不透明处 (1 - col.a) == 0，补色不参与 → 无副作用。
    col.rgb += fillColor.rgb * (1.0 - col.a);
    col.a = 1.0;

    // ===== 4. 圆角 =====
    vec2  p    = (qt_TexCoord0 - 0.5) * cardSize;
    float d    = roundedBoxSDF(p, cardSize * 0.5, corner);
    float mask = 1.0 - smoothstep(-aa, aa, d);

    fragColor = col * mask * qt_Opacity;
}
