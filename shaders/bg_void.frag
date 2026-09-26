#version 450 core

// 点云预设的底色：近黑 + 极淡星云（给点云做纵深对比，不抢主体）
layout(location = 0) in vec2 qt_TexCoord0;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4  uColor;
    vec4  uColor2;
    vec2  uResolution;
    vec2  uMouse;
    float uTime;
    float uLow;
    float uMid;
    float uHigh;
    float uAir;
    float uLevel;
    float uBright;
    float uSaturate;
    float uAudio;
    float uField;
    float uCamDist;
    float uCamSens;
};

layout(location = 0) out vec4 fragColor;

void main() {
    vec2 uv = (vec2(qt_TexCoord0.x, 1.0 - qt_TexCoord0.y) * uResolution * 2.0 - uResolution)
             / max(uResolution.y, 1.0);
    float n = 0.5 + 0.5 * sin(uv.x * 1.6 + uTime * 0.06) * sin(uv.y * 1.2 - uTime * 0.05);
    vec3 col = mix(uColor.rgb, uColor2.rgb, 0.5) * n * 0.11 * (0.4 + 0.9 * uLevel * uAudio) * uBright;
    col *= clamp(1.0 - 0.35 * dot(uv, uv), 0.25, 1.0);
    fragColor = vec4(pow(max(col, 0.0), vec3(0.4545)), 1.0) * qt_Opacity;
}
