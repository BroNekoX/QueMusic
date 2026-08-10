#version 450 core

// 歌词上下渐隐 + 渐进模糊（图层效果 layer.effect 用）
// 思路参考 ProgressiveBlur：高斯权重 + 半径随位置平滑映射（不割裂、无颗粒感）
// 2D 稀疏高斯（隔 2px 采样）：X、Y 双方向模糊 → 散焦效果，采样量比满核少 ~70%
// 顶部/底部模糊距离分别自定义（blurTop / blurBottom）
// 用法：给歌词 Item 设置 layer.enabled: true + layer.effect: ShaderEffect
// 采样器固定名 source（Qt layer 效果约定），binding = 1

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
    float y = uv.y;   // 0=顶部 1=底部

    // ===== 1. 渐进半径映射（顶/底分别控制）=====
    //    1-smoothstep 让半径连续平滑渐变 → 模糊区无割裂
    float blurK;
    if (y < 0.5) {
        // 上半区：到顶部的距离 y（0=顶 → 1-smoothstep 顶部最强）
        blurK = 1.0 - smoothstep(0.0, blurTop, y);
    } else {
        // 下半区：到底部的距离 1-y（1=底 → 1-smoothstep 底部最强）
        blurK = 1.0 - smoothstep(0.0, blurBottom, 1.0 - y);
    }
    // sigma 取 blurRadius 的一半，让覆盖范围约 ±2σ（截断处权重已很小，无台阶）
    float sigma = blurK * blurRadius * 0.5;

    // ===== 2. 2D 稀疏高斯（垂直 9 tap × 水平 5 tap，隔 2px 采样）=====
    //    模糊是平滑函数，隔 2px 采样 + 高斯权重 → 视觉几乎无差别，采样量大降
    //    覆盖范围不变：垂直 ±8px、水平 ±4px
    //    中间 sigma 小 → 直接返回原纹理（省采样，性能关键）
    vec4 col;
    if (sigma < 0.7) {
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

    // ===== 3. 上下渐隐（alpha 淡出，顶/底分别控制）=====
    float fade = smoothstep(0.0, fadeTop, y)
               * smoothstep(0.0, fadeBottom, 1.0 - y);

    fragColor = col * fade * qt_Opacity;
}
