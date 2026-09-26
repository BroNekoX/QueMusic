#version 450 core

// 预设 0：方块场 —— 参考图效果：方块按**同心圆波纹**规律跳动（幅度大）、场地收成圆丘（整体更小）、
// 位置偏画面下方；场周围有星尘，天空有多层圆环特效。
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

mat3 lookAt(vec3 ro, vec3 ta) {
    vec3 cw = normalize(ta - ro);
    vec3 cu = normalize(cross(cw, vec3(0.0, 1.0, 0.0)));
    return mat3(cu, cross(cu, cw), cw);
}

// 方块高度：**每格独立**按自己所属频段鼓动（不是同心圆 ⇒ 不会连成圆筒/一起升降），幅度拉高
float heightAt(vec2 p) {
    float cell = 0.72 * uField;
    vec2 ip = floor(p / cell);
    float r = length(p);
    float edge = 10.0 * uField;
    float mask = smoothstep(edge, edge * 0.55, r);
    if (mask < 0.003)
        return -2.5;                       // 场外比基线低 ⇒ 被雾吃掉，不出现平面感
    float pick = hash13(vec3(ip, 0.0));
    float amp = pick < 0.25 ? uLow : (pick < 0.5 ? uMid : (pick < 0.75 ? uHigh : uAir));
    float base = 0.04 + 0.12 * hash13(vec3(ip, 1.0));
    float h = base + 3.60 * pow(clamp(amp, 0.0, 1.0), 0.65)
              * (0.40 + 0.60 * hash13(vec3(ip, 2.0)));
    h *= 1.0 + 0.20 * exp(-r * r * 0.012);                          // 中心略高，但不形成圆壁
    return h * mask;
}

float march(vec3 ro, vec3 rd, out vec3 hit) {
    float t = 0.0;
    for (int i = 0; i < 72; ++i) {
        vec3 p = ro + rd * t;
        float d = p.y - heightAt(p.xz);
        if (d < 0.002 * max(t, 1.0)) { hit = p; return t; }
        t += max(0.025, d * 0.58);
        if (t > 44.0) break;
    }
    return -1.0;
}

vec3 normalAt(vec3 p) {
    const float e = 0.01;
    return normalize(vec3(heightAt(p.xz - vec2(e, 0.0)) - heightAt(p.xz + vec2(e, 0.0)),
                          2.0 * e,
                          heightAt(p.xz - vec2(0.0, e)) - heightAt(p.xz + vec2(0.0, e))));
}

void main() {
    vec2 uv = (vec2(qt_TexCoord0.x, 1.0 - qt_TexCoord0.y) * uResolution * 2.0 - uResolution)
             / max(uResolution.y, 1.0);

    // 视线更平 ⇒ 场地落在画面下方
    float yaw = uMouse.x * 1.2 * uCamSens;
    float pitch = clamp(0.20 + uMouse.y * 0.26 * uCamSens, 0.02, 0.60);
    vec3 ta = vec3(0.0, 0.9 + 0.7 * uLevel * uAudio, 0.0);
    vec3 ro = ta + vec3(cos(yaw), tan(pitch), sin(yaw)) * max(5.0, uCamDist);
    vec3 rd = normalize(lookAt(ro, ta) * vec3(uv, 1.9));

    vec3 col = vec3(0.0);

    // 天空：多层圆环特效（随音频呼吸）
    {
        vec2 c = uv - vec2(0.0, 0.18);
        float rr = length(c);
        for (int i = 0; i < 3; ++i) {
            float fi = float(i);
            float rad = 0.55 + fi * 0.48 + 0.07 * sin(uTime * 0.8 + fi * 2.1);
            float band = exp(-pow((rr - rad) / (0.045 + 0.02 * fi), 2.0));
            col += mix(uColor2.rgb, vec3(1.0), 0.30) * band * (0.10 + 0.50 * uLevel * uAudio);
        }
        // 场周围星尘（三层小而锐利的点）
        for (int L = 0; L < 3; ++L) {
            float fl = float(L);
            float scale = 20.0 / (0.5 + 0.8 * fl);
            vec2 q = uv * scale + vec2(uMouse.x * (1.0 + fl), -uMouse.y * (0.8 + fl))
                     + vec2(uTime * 0.012, -uTime * 0.008);
            vec2 ip = floor(q);
            vec2 fp = fract(q) - 0.5;
            float h = hash13(vec3(ip, fl * 3.7));
            if (h > 0.76) {
                vec2 off = (hash23(vec3(ip, fl * 5.3)) - 0.5) * 0.7;
                float d = length(fp - off);
                float radp = 0.05 + 0.04 * fract(h * 9.1);
                col += mix(uColor.rgb, vec3(1.0), 0.40)
                       * pow(max(0.0, 1.0 - d / radp), 4.0)
                       * (0.35 + 0.75 * uLevel * uAudio) * (1.0 - fl * 0.20);
            }
        }
    }

    // 方块场
    vec3 p;
    float t = march(ro, rd, p);
    if (t > 0.0) {
        vec3 n = normalAt(p);
        vec3 l = normalize(vec3(0.5, 0.8, 0.3));
        float dif = clamp(dot(n, l), 0.0, 1.0);
        vec2 ip = floor(p.xz / (0.72 * uField));
        vec3 base = mix(uColor.rgb, uColor2.rgb, hash13(vec3(ip, 3.0)));
        col = base * (0.05 + 0.62 * dif);
        // 顶面棱线自发光（随波纹一起跳）
        col += mix(base, vec3(1.0), 0.35) * pow(dif, 8.0) * (0.35 + 1.40 * uLevel * uAudio);
        col += uColor2.rgb * 0.12 * smoothstep(0.6, 1.8, p.y);
        col *= 1.0 - smoothstep(16.0, 34.0, t);
    }
    col *= uBright;

    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(lum), col, clamp(uSaturate, 1.0, 2.2));
    col += vec3(max(0.0, 0.012 - lum));
    col = col / (1.0 + col) * 1.6;
    col *= clamp(1.0 - 0.32 * dot(uv, uv), 0.3, 1.0);
    fragColor = vec4(pow(max(col, 0.0), vec3(0.4545)), 1.0) * qt_Opacity;
}
