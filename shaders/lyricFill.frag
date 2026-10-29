#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    float itemWidthPx;      // 字宽
    float fillPx;           // 过渡位置
    float softnessPx;       // 过渡宽度
    float unsungAlpha;
    float progressOpacity;  // 过渡区显示程度
    vec4  progressColor;    // 过渡颜色
};

void main() {
    vec4 c = texture(source, qt_TexCoord0);

    if (c.a < 1.0 / 255.0) {
        fragColor = vec4(0.0);
        return;
    }

    float soft = max(softnessPx, 1.0);
    float m = smoothstep(fillPx - soft, fillPx, qt_TexCoord0.x * itemWidthPx);

    vec4 dimmed = c * unsungAlpha;

    vec4 progress = vec4(progressColor.rgb * progressColor.a, progressColor.a) * c.a;

    fragColor = mix(dimmed, progress, progressOpacity * (1.0 - m)) * qt_Opacity;
}