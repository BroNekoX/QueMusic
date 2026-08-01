#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float bars;          // 频段数，QML 传入 48.0
    float heightScale;   // 最大高度比例 0~1，QML 传入 0.9
    float blurRadius;    // 邻域融合半径（纹理坐标单位），QML 传入 1.5/48.0
} ubuf;

layout(binding = 1) uniform sampler2D spectrumTex;  // 48x1 的频谱纹理

void main()
{
    // 屏幕坐标：x 向右，y 向上为 0（qt_TexCoord0.y = 0 在顶部）
    // 我们要底部对齐，所以翻转 y
    float y = 1.0 - qt_TexCoord0.y;

    // 计算当前像素属于第几段频谱
    float seg = floor(qt_TexCoord0.x * ubuf.bars);
    float segX = (seg + 0.5) / ubuf.bars;   // 该段中心在纹理上的 u 坐标

    // —— 在 x 方向对相邻频段采样，做融合/模糊 ——
    float intensity = 0.0;
    float totalWeight = 0.0;
    for (int k = -2; k <= 2; ++k) {
        float neighborSeg = seg + float(k);
        if (neighborSeg < 0.0 || neighborSeg >= ubuf.bars)
            continue;
        float nx = (neighborSeg + 0.5) / ubuf.bars;
        // 距离越近权重越大
        float w = 1.0 - abs(float(k)) * 0.25;
        intensity += texture(spectrumTex, vec2(nx, 0.5)).r * w;
        totalWeight += w;
    }
    intensity /= max(totalWeight, 0.001);

    // 该频段对应的波形高度（底部对齐）
    float barHeight = intensity * ubuf.heightScale;

    // 用 smoothstep 做出柔和的上下边缘
    float edge = 0.015;  // 边缘柔和度
    float alpha = 1.0 - smoothstep(barHeight - edge, barHeight + edge, y);

    // 波形内部渐变着色：底部偏蓝，顶部偏青
    float t = clamp(y / max(barHeight, 0.001), 0.0, 1.0);
    vec3 color = mix(vec3(0.0, 0.35, 1.0),   // 底部：蓝色
                     vec3(0.0, 0.85, 1.0),   // 顶部：青色
                     t);
    // 顶部加一点高光
    color += vec3(0.2, 0.4, 0.5) * pow(t, 4.0);

    // 乘以 qt_Opacity 并输出
    fragColor = vec4(color * alpha, alpha) * ubuf.qt_Opacity;
}