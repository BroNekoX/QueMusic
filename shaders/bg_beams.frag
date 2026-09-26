#version 450 core

// 预设 5：光棱（多层竖光柱，边缘锐利 + 上升流动）。背景着色器之一。
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

vec2 hash23(vec3 c) {
    float h = hash13(c);
    return fract(vec2(h, h * 7.31));
}

void main() {
    vec2 uv = (vec2(qt_TexCoord0.x, 1.0 - qt_TexCoord0.y) * uResolution * 2.0 - uResolution)
             / max(uResolution.y, 1.0);
    float aspect = uResolution.x / max(uResolution.y, 1.0);
    vec2 sp = vec2(uv.x * aspect, uv.y);

    vec3 col = vec3(0.0);
    float rise = 0.30 + 0.70 * uLevel * uAudio;

    // 4 层光柱：越远越窄越暗
    for (int L = 0; L < 4; ++L) {
        float fl = float(L);
        float lane = 3.0 + fl * 2.2;
        vec2 q = vec2(sp.x * lane + uMouse.x * (0.8 + fl * 0.9),
                      sp.y * (0.55 + fl * 0.25) - uTime * rise * (0.35 + 0.30 * fl));
        vec2 ip = floor(q);
        vec2 fp = fract(q) - 0.5;
        float h = hash13(vec3(ip, fl * 4.7));
        if (h > 0.52) {
            vec2 jit = (hash23(vec3(ip, fl * 2.3)) - 0.5) * 0.5;
            float bx = abs(fp.x - jit.x);
            float by = fp.y - jit.y;
            // 锐利竖条：横向快速衰减 + 纵向缓慢衰减
            float beam = pow(max(0.0, 1.0 - bx * 5.5), 3.0) * exp(-abs(by) * 2.2);
            float len = 0.35 + 0.65 * fract(h * 7.7);
            vec3 pc = mix(uColor.rgb, uColor2.rgb, fract(fl * 0.29 + h * 0.6));
            col += pc * beam * len * (0.35 + 0.85 * uLevel * uAudio) * (1.0 - fl * 0.14);
            // 顶端亮点
            col += mix(pc, vec3(1.0), 0.7) * beam * exp(-abs(by) * 26.0)
                   * (0.45 + 1.0 * uLevel * uAudio);
        }
    }

    // 底部辉光（让光柱像从下面射出）
    col += mix(uColor.rgb, uColor2.rgb, 0.5) * exp(-max(uv.y + 0.85, 0.0) * 3.2) * 0.20;
    col *= uBright * 1.35;

    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(lum), col, clamp(uSaturate, 1.0, 2.2));
    col += vec3(max(0.0, 0.012 - lum));
    col = col / (1.0 + col) * 1.65;
    col *= clamp(1.0 - 0.30 * dot(uv, uv), 0.3, 1.0);
    fragColor = vec4(pow(max(col, 0.0), vec3(0.4545)), 1.0) * qt_Opacity;
}
