#version 450 core

// 点云预设（星尘 / 光隧 / 星环）的底色：域扭曲的体积星云 + 中心辉光 + 低频压力波 + 星点 + 暗角。
// 目标是把点云放进"有纵深的暗场"里，而不是贴在黑底上。
layout(location = 0) in vec2 qt_TexCoord0;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4  uColor;
    vec4  uColor2;
    vec2  uResolution;
    // 共享相机
    float uCamTheta;
    float uCamPhi;
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
    float uCamRadius;
    float uFocal;
    float uCamTargetY;
};

layout(location = 0) out vec4 fragColor;

// 无 sin 的廉价哈希 / 值噪声，够细致又不吃性能
float h21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(h21(i), h21(i + vec2(1.0, 0.0)), f.x),
               mix(h21(i + vec2(0.0, 1.0)), h21(i + vec2(1.0, 1.0)), f.x), f.y);
}

float fbm(vec2 p) {
    float a = 0.5, s = 0.0;
    for (int k = 0; k < 4; ++k) {
        s += a * vnoise(p);
        p = p * 2.03 + 17.1;
        a *= 0.5;
    }
    return s;
}

void main() {
    vec2 uv = (vec2(qt_TexCoord0.x, 1.0 - qt_TexCoord0.y) * uResolution * 2.0 - uResolution)
              / max(uResolution.y, 1.0);
    float r = length(uv * vec2(0.78, 1.0));

    // ══ ① 体积星云：域扭曲的多倍频噪声，随视角平移（有视差的"远方"）══
    vec2 q = uv * (0.60 + 0.12 * uField) + vec2(uCamTheta * 0.40, (uCamPhi - 0.30) * 0.62);
    vec2 warp = vec2(fbm(q + uTime * 0.020), fbm(q + 4.7 - uTime * 0.015));
    float n = fbm(q * 1.75 + warp * 1.25 + vec2(0.0, uTime * 0.030));
    n = pow(clamp(n, 0.0, 1.0), 1.9);

    vec3 neb = mix(uColor.rgb, uColor2.rgb, clamp(0.30 + 0.55 * n, 0.0, 1.0));
    vec3 col = neb * n * 0.30 * (0.35 + 0.85 * uLevel * uAudio) * uBright;

    // ══ ② 中心辉光 + 低频压力波（鼓点扩散感）══
    float pulse = 0.5 + 0.5 * sin(r * 7.0 - uTime * 1.9);
    col += mix(uColor2.rgb, vec3(1.0), 0.30) * exp(-r * r * 0.55) * (0.09 + 0.26 * uLow) * uBright;
    col += uColor.rgb * pulse * exp(-r * 1.65) * 0.06 * (0.30 + uLow) * uBright;

    // ══ ③ 深空星点（背景层，比舞台点云更细更暗）══
    vec2 sq = uv * 150.0 + vec2(uCamTheta * 40.0, (uCamPhi - 0.30) * 40.0);
    vec2 si = floor(sq);
    float sh = h21(si);
    if (sh > 0.9955) {
        vec2 sf = fract(sq) - 0.5;
        float d = length(sf);
        col += mix(uColor2.rgb, vec3(1.0), 0.6) * pow(max(0.0, 1.0 - d * 2.4), 5.0) * 0.55 * uBright;
    }

    // ══ ④ 竖直渐变 + 暗角 ══
    col *= mix(0.60, 1.15, smoothstep(-0.55, 0.70, uv.y));
    col *= clamp(1.0 - 0.28 * dot(uv, uv), 0.25, 1.0);

    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(lum), col, clamp(uSaturate, 1.0, 2.0));
    fragColor = vec4(col, 1.0) * qt_Opacity;
}