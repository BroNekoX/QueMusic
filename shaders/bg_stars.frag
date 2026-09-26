#version 450 core

// 预设：平面星空 —— 多层星点（鼠标视差）+ 一条银河带 + 轻微闪烁；不做相机环绕，纯粹平铺星空。
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

    // 银河带：一条斜向的弥散亮带（噪声状的稀疏星云）
    float band = exp(-pow((sp.y - 0.35 * sp.x + 0.15) / 0.42, 2.0));
    float grain = 0.5 + 0.5 * sin(sp.x * 7.3 + uTime * 0.05) * sin(sp.y * 6.1 - uTime * 0.04);
    col += mix(uColor.rgb, uColor2.rgb, 0.6) * band * grain * 0.10
           * (0.5 + 0.8 * uLevel * uAudio);

    // 5 层星点：越远越小越淡、视差越小
    for (int L = 0; L < 5; ++L) {
        float fl = float(L);
        float depth = 0.35 + 0.75 * fl;
        float scale = 30.0 / depth;
        // 视差量在"格子空间"里必须与 scale 无关，才会换算成"屏幕位移 ∝ depth"：
        // 屏幕位移 = 1.8 / scale = 0.06 * depth ⇒ 近层动得多、远层动得少
        vec2 q = sp * scale + vec2(uMouse.x, -uMouse.y) * 1.8
                 + uTime * vec2(0.10, -0.07);
        vec2 ip = floor(q);
        vec2 fp = fract(q) - 0.5;
        float h = hash13(vec3(ip, fl * 3.13));
        if (h > 0.78) {
            vec2 off = (hash23(vec3(ip, fl * 5.7)) - 0.5) * 0.72;
            float d = length(fp - off);
            float rad = 0.05 + 0.045 * fract(h * 9.13);
            float tw = 0.65 + 0.35 * sin(uTime * (1.3 + 2.0 * h) + h * 33.0);   // 闪烁
            col += mix(uColor2.rgb, vec3(1.0), 0.45 + 0.45 * fract(h * 3.1))
                   * pow(max(0.0, 1.0 - d / rad), 4.0)
                   * (0.45 + 0.85 * uLevel * uAudio) * tw * (1.0 - fl * 0.13);
            // 极少数大星带十字星芒
            if (h > 0.988) {
                float flare = pow(max(0.0, 1.0 - abs(fp.x - off.x) * 30.0), 3.0) * exp(-abs(fp.y - off.y) * 3.2)
                            + pow(max(0.0, 1.0 - abs(fp.y - off.y) * 30.0), 3.0) * exp(-abs(fp.x - off.x) * 3.2);
                col += vec3(1.0) * flare * (0.35 + 0.75 * uLevel * uAudio);
            }
        }
    }

    col *= uBright * 1.4;

    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(lum), col, clamp(uSaturate, 1.0, 2.2));
    col += vec3(max(0.0, 0.010 - lum));
    col = col / (1.0 + col) * 1.7;
    col *= clamp(1.0 - 0.26 * dot(uv, uv), 0.3, 1.0);
    fragColor = vec4(pow(max(col, 0.0), vec3(0.4545)), 1.0) * qt_Opacity;
}
