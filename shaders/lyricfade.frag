#version 450 core

layout(location = 0) in vec2 qt_TexCoord0;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    // 自定义 Uniforms（由 QML property 同名注入）
    float fadeTop;      // 顶部渐隐高度（占视口高度比例 0~1）
    float fadeBottom;   // 底部渐隐高度（占视口高度比例 0~1）
    float blurTop;      // 顶部渐进模糊距离（占视口高度比例 0~0.5）
    float blurBottom;   // 底部渐进模糊距离（占视口高度比例 0~0.5）
    float blurRadius;   // 边缘处最大模糊半径（像素，视觉强度）
    vec2  srcSize;      // 源纹理尺寸（像素）= 歌词 Item 宽高
};

layout(location = 0) out vec4 fragColor;

// 高斯权重：钟形曲线，中心强、边缘弱 → 柔和散焦
float gauss(float x, float sigma) {
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

void main() {
    vec2 uv = qt_TexCoord0;
    float y = uv.y;

    float kTop = 1.0 - smoothstep(0.0, max(blurTop,    1e-4), y);
    float kBot = 1.0 - smoothstep(0.0, max(blurBottom, 1e-4), 1.0 - y);
    float blurK = max(kTop, kBot);
    float sigma = blurK * blurRadius * 0.5;

    // 2D 稀疏高斯（垂直 9 tap × 水平 5 tap，隔 2px 采样）
    vec4 col;
    if (sigma < 0.5) {
        col = texture(source, uv);
    } else {
        col = vec4(0.0);
        float total = 0.0;
        for (int j = -4; j <= 4; ++j) {                 // 垂直（主渐进，隔 2px = ±8）
            float wy = gauss(float(j) * 2.0, sigma);
            for (int i = -2; i <= 2; ++i) {             // 水平（消除方向感，隔 2px = ±4）
                float wx = gauss(float(i) * 2.0, sigma);
                vec2 p = uv + vec2(float(i) * 2.0 / srcSize.x, float(j) * 2.0 / srcSize.y);
                p = clamp(p, 0.0, 1.0);                 // 边缘 clamp，防止越界
                col += texture(source, p) * (wx * wy);
                total += wx * wy;
            }
        }
        col /= total;
    }

    // 上下渐隐（alpha 淡出，顶/底分别控制）
    float fade = smoothstep(0.0, fadeTop, y)
               * smoothstep(0.0, fadeBottom, 1.0 - y);

    fragColor = col * fade * qt_Opacity;
}
