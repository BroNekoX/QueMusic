#version 450 core

// 预设 1：光栅地平（波面网格，线条锐利自发光）。背景着色器之一。
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

float hash13(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453);
}

mat3 lookAt(vec3 ro, vec3 ta) {
    vec3 cw = normalize(ta - ro);
    vec3 cu = normalize(cross(cw, vec3(0.0, 1.0, 0.0)));
    return mat3(cu, cross(cu, cw), cw);
}

// 波面高度：三段频谱各控制一片区域
float waveAt(vec3 p) {
    float band = p.x < -3.0 ? uLow : (p.x < 3.0 ? uMid : uAir);
    return 0.22 * sin(p.x * 0.55 + uTime * 0.5) * sin(p.z * 0.45 - uTime * 0.35)
           + band * 0.85;
}

void main() {
    vec2 uv = (vec2(qt_TexCoord0.x, 1.0 - qt_TexCoord0.y) * uResolution * 2.0 - uResolution)
             / max(uResolution.y, 1.0);

    float yaw = uMouse.x * 1.2 * uCamSens;
    float pitch = clamp(0.30 + uMouse.y * 0.28 * uCamSens, 0.05, 0.75);
    vec3 ta = vec3(0.0, 0.8 + 0.8 * uLevel * uAudio, 0.0);
    vec3 ro = ta + vec3(cos(yaw), tan(pitch), sin(yaw)) * max(5.0, uCamDist);
    vec3 rd = normalize(lookAt(ro, ta) * vec3(uv, 1.9));

    vec3 col = vec3(0.0);
    float t = 0.0;
    for (int i = 0; i < 40; ++i) {
        vec3 p = ro + rd * t;
        float d = p.y - waveAt(p);
        if (d < 0.004 * max(t, 1.0)) {
            // 网格线：到最近格线的距离 ⇒ pow 提锐利度（不糊）
            vec2 g = abs(fract(p.xz / (1.6 * uField)) - 0.5) * 2.0;
            float line = 1.0 - min(g.x, g.y);
            line = pow(clamp(line, 0.0, 1.0), 14.0);
            vec3 c = mix(uColor2.rgb, uColor.rgb, 0.5 + 0.5 * sin(p.z * 0.3));
            col = c * line * (0.55 + 1.30 * uLevel * uAudio);
            col *= exp(-max(t - 6.0, 0.0) * 0.10);          // 远处淡出
            break;
        }
        t += max(0.05, d * 0.5);
        if (t > 46.0) break;
    }
    // 地平线辉光
    col += mix(uColor.rgb, uColor2.rgb, 0.5) * exp(-abs(uv.y) * 9.0) * (0.10 + 0.55 * uLevel * uAudio);
    col *= uBright;

    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(lum), col, clamp(uSaturate, 1.0, 2.2));
    col += vec3(max(0.0, 0.012 - lum));
    col = col / (1.0 + col) * 1.6;
    col *= clamp(1.0 - 0.32 * dot(uv, uv), 0.3, 1.0);
    fragColor = vec4(pow(max(col, 0.0), vec3(0.4545)), 1.0) * qt_Opacity;
}
