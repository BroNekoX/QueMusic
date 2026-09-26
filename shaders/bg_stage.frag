#version 450 core

// 预设：舞台 —— **实体 3D 几何**（射线步进的 SDF 场景，不是粒子拼的）：
// 圆形多层平台 + 底部基座 + 数个真实圆环（Torus），环随音频变粗/旋转；周围粒子由 LyricsStage 叠在上面。
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

vec3 hueShift(vec3 c, float a) {
    const vec3 k = vec3(0.57735027);
    float ca = cos(a);
    return c * ca + cross(k, c) * sin(a) + k * dot(k, c) * (1.0 - ca);
}

mat3 lookAt(vec3 ro, vec3 ta) {
    vec3 cw = normalize(ta - ro);
    vec3 cu = normalize(cross(cw, vec3(0.0, 1.0, 0.0)));
    return mat3(cu, cross(cu, cw), cw);
}

// SDF：有限圆柱（平台）、圆环、旋转
float sdCylinder(vec3 p, float r, float h) {
    vec2 d = vec2(length(p.xz) - r, abs(p.y) - h);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float sdTorus(vec3 p, float R, float r) {
    return length(vec2(length(p.xz) - R, p.y)) - r;
}

vec2 rot2(vec2 v, float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c) * v;
}

const float kRingY0 = 0.10;      // 环的高度基准

// 场景：平台（3 层叠盘）+ 3 个真实圆环
float map(vec3 p, out float ringDist) {
    // 圆形舞台：主台面 + 装饰边沿 + 基座
    float d = sdCylinder(p - vec3(0.0, -1.15, 0.0), 4.10 * uField, 0.30);
    d = min(d, sdCylinder(p - vec3(0.0, -1.55, 0.0), 4.75 * uField, 0.22));
    d = min(d, sdCylinder(p - vec3(0.0, -2.10, 0.0), 3.20 * uField, 0.45));

    ringDist = 1e9;
    for (int i = 0; i < 3; ++i) {
        float fi = float(i);
        float R = (2.30 + fi * 0.95) * uField;
        float yy = kRingY0 + fi * 0.85 + 0.10 * sin(uTime * 0.9 + fi * 2.0);
        vec3 q = p - vec3(0.0, yy, 0.0);
        q.xz = rot2(q.xz, uTime * (0.28 - 0.07 * fi) + fi * 1.7);       // 自转
        q.yz = rot2(q.yz, 0.30 * (fi - 1.0));                            // 轻微倾斜
        float ring = sdTorus(q, R, 0.085 + 0.045 * uLevel * uAudio);
        ringDist = min(ringDist, ring);
        d = min(d, ring);
    }
    return d;
}

vec3 normalAt(vec3 p) {
    const float e = 0.006;
    float dummy;
    vec3 n = vec3(map(p + vec3(e, 0.0, 0.0), dummy) - map(p - vec3(e, 0.0, 0.0), dummy),
                  map(p + vec3(0.0, e, 0.0), dummy) - map(p - vec3(0.0, e, 0.0), dummy),
                  map(p + vec3(0.0, 0.0, e), dummy) - map(p - vec3(0.0, 0.0, e), dummy));
    return normalize(n);
}

void main() {
    vec2 uv = (vec2(qt_TexCoord0.x, 1.0 - qt_TexCoord0.y) * uResolution * 2.0 - uResolution)
             / max(uResolution.y, 1.0);

    float yaw = uMouse.x * 1.2 * uCamSens;
    float pitch = clamp(0.26 + uMouse.y * 0.26 * uCamSens, 0.04, 0.70);
    vec3 ta = vec3(0.0, -0.15 + 0.55 * uLevel * uAudio, 0.0);
    vec3 ro = ta + vec3(cos(yaw), tan(pitch), sin(yaw)) * max(5.0, uCamDist);
    vec3 rd = normalize(lookAt(ro, ta) * vec3(uv, 1.9));

    vec3 col = vec3(0.0);
    // 舞台后方的巨大光环（背景装饰）
    {
        vec2 c = uv - vec2(0.0, 0.10);
        float rr = length(c);
        for (int i = 0; i < 2; ++i) {
            float fi = float(i);
            float rad = 0.75 + fi * 0.55 + 0.06 * sin(uTime * 0.9 + fi * 2.0);
            col += mix(uColor2.rgb, vec3(1.0), 0.35)
                   * exp(-pow((rr - rad) / 0.05, 2.0)) * (0.10 + 0.45 * uLevel * uAudio);
        }
    }

    // 射线步进
    float t = 0.0;
    float ringDist = 1e9;
    float hitT = -1.0;
    for (int i = 0; i < 90; ++i) {
        vec3 p = ro + rd * t;
        float rd2;
        float d = map(p, rd2);
        if (d < 0.0025 * max(t, 1.0)) { hitT = t; ringDist = rd2; break; }
        t += max(0.01, d * 0.85);
        if (t > 40.0) break;
    }

    if (hitT > 0.0) {
        vec3 p = ro + rd * hitT;
        vec3 n = normalAt(p);
        vec3 l = normalize(vec3(0.45, 0.80, 0.35));
        float dif = clamp(dot(n, l), 0.0, 1.0);
        float spe = pow(clamp(dot(normalize(l - rd), n), 0.0, 1.0), 42.0);
        float fres = pow(1.0 - clamp(dot(n, -rd), 0.0, 1.0), 3.0);

        bool isRing = (p.y - kRingY0) > -0.35;      // 环都在台面之上
        float rr = length(p.xz);
        if (isRing) {
            // 圆环：自发光 + 边缘菲涅尔，随音频更亮更粗
            vec3 rc = mix(uColor.rgb, hueShift(uColor2.rgb, 0.6), clamp((p.y + 0.2) * 0.5, 0.0, 1.0));
            col = rc * (0.55 + 1.60 * uLevel * uAudio) + vec3(0.6) * fres;
        } else {
            // 平台：深色金属 + 顶面同心刻线 + 边缘发光
            float groove = 0.5 + 0.5 * sin(rr * 7.0);
            vec3 base = mix(uColor2.rgb * 0.10, uColor.rgb * 0.16, 0.5);
            col = base * (0.20 + 0.85 * dif);
            if (n.y > 0.55)                                  // 顶面刻线
                col += mix(uColor.rgb, uColor2.rgb, 0.4) * groove * 0.18;
            col += mix(uColor.rgb, vec3(1.0), 0.4) * fres * (0.35 + 0.90 * uLevel * uAudio);
            col += vec3(0.5) * spe;
        }
        // 台面下方的辉光 + 远处淡出
        col += mix(uColor.rgb, uColor2.rgb, 0.5) * exp(-max(rr - 4.2 * uField, 0.0) * 0.5) * 0.06;
        col *= 1.0 - smoothstep(20.0, 38.0, hitT);
    }

    col *= uBright;

    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(lum), col, clamp(uSaturate, 1.0, 2.2));
    col += vec3(max(0.0, 0.012 - lum));
    col = col / (1.0 + col) * 1.6;
    col *= clamp(1.0 - 0.32 * dot(uv, uv), 0.3, 1.0);
    fragColor = vec4(pow(max(col, 0.0), vec3(0.4545)), 1.0) * qt_Opacity;
}
