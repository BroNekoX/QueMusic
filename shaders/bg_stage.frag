#version 450 core

// 预设：舞台 —— 一块被照亮的白色地面（中心亮、四周渐黑）+ 圆台 + 若干细丝圆环。
// 地面解析求交，台面与丝环球步进；丝环是"细丝"量级（半径 0.02~0.05），随音频变亮抖动。
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

const float kFloorY = -2.05;      // 地面高度（世界 Y）
const float kFloorR = 8.6;        // 地面照亮半径：之外全黑
const int   kThreads = 6;         // 丝条圆环数量

mat3 lookAt(vec3 ro, vec3 ta) {
    vec3 cw = normalize(ta - ro);
    vec3 cu = normalize(cross(cw, vec3(0.0, 1.0, 0.0)));
    return mat3(cu, cross(cu, cw), cw);
}

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

float hash11(float k) {
    return fract(sin(k * 127.1) * 43758.5453);
}

// 丝环参数：每像素先做一次包围球预筛，只有可能命中的环才参与步进（否则每步都算 6 个环太贵）
vec3 gRingC[kThreads];
float gRingR[kThreads];
float gRingT[kThreads];
bool gRingOn[kThreads];

void prepThreads(vec3 ro, vec3 rd) {
    for (int i = 0; i < kThreads; ++i) {
        float fi = float(i);
        float h1 = hash11(fi + 1.7), h2 = hash11(fi * 3.1 + 5.3), h3 = hash11(fi * 7.7 + 9.1);
        float R = (0.50 + h1 * 1.35) * uField;
        float rr = 0.018 + h2 * 0.024;
        float ang = uTime * (0.25 + h3 * 0.35) + h1 * 6.2832;
        float yy = kFloorY + 0.95 + h2 * 1.75 + 0.16 * sin(uTime * (0.6 + h3) + fi * 2.1);
        float rad = 1.0 + h3 * 2.3;
        vec3 c = vec3(cos(ang) * rad, yy, sin(ang) * rad * 0.85);
        gRingC[i] = c;
        gRingR[i] = R;
        gRingT[i] = rr * (1.0 + 0.35 * uAir) + 0.02;
        // 光线到包围球的最近距离
        vec3 oc = c - ro;
        float b = dot(oc, rd);
        float bsq = (R + gRingT[i]) * (R + gRingT[i]) - (dot(oc, oc) - b * b);
        gRingOn[i] = (b > 0.0) && (bsq > 0.0);
    }
}

// 台面 + 可能命中的丝环
float mapStage(vec3 p, out float ring) {
    float d = sdCylinder(p - vec3(0.0, kFloorY + 0.34, 0.0), 1.90 * uField, 0.34);
    d = min(d, sdCylinder(p - vec3(0.0, kFloorY + 0.62, 0.0), 1.35 * uField, 0.14));
    ring = 0.0;

    for (int i = 0; i < kThreads; ++i) {
        if (!gRingOn[i])
            continue;
        float fi = float(i);
        float h1 = hash11(fi + 1.7), h3 = hash11(fi * 7.7 + 9.1);
        float ang = uTime * (0.25 + h3 * 0.35) + h1 * 6.2832;
        vec3 q = p - gRingC[i];
        q.xz = rot2(q.xz, -ang * 1.6 + fi);
        q.yz = rot2(q.yz, (h1 - 0.5) * 1.1 + 0.18 * sin(uTime * 0.5 + fi));
        q.xy = rot2(q.xy, (h3 - 0.5) * 0.9);
        float dt = sdTorus(q, gRingR[i], gRingT[i]);
        if (dt < d) {
            d = dt;
            ring = 1.0;
        }
    }
    return d;
}

// 4 抽样四面体梯度：比 6 抽样中心差分便宜三分之一
vec3 stageNormal(vec3 p) {
    const vec2 k = vec2(1.0, -1.0);
    float ring;
    float d0 = mapStage(p, ring);
    float dx = mapStage(p + k.xyy * 0.0035, ring);
    float dy = mapStage(p + k.yxy * 0.0035, ring);
    float dz = mapStage(p + k.yyx * 0.0035, ring);
    return normalize(vec3(dx - d0, dy - d0, dz - d0));
}

void main() {
    vec2 sp = (vec2(qt_TexCoord0.x, 1.0 - qt_TexCoord0.y) * uResolution * 2.0 - uResolution)
              / max(uResolution.y, 1.0);

    vec3 ta = vec3(0.0, uCamTargetY + 0.55 * uLevel, 0.0);
    vec3 ro = ta + uCamRadius * vec3(cos(uCamPhi) * cos(uCamTheta),
                                     sin(uCamPhi),
                                     cos(uCamPhi) * sin(uCamTheta));
    mat3 cb = lookAt(ro, ta);
    vec3 rd = normalize(cb * vec3(sp, max(uFocal, 0.6)));

    vec3 body = vec3(0.014, 0.022, 0.036) + uColor.rgb * 0.020;
    vec3 accent = mix(uColor2.rgb, vec3(1.0), 0.25);
    vec3 white = mix(vec3(1.0), uColor.rgb, 0.20);

    vec3 col = vec3(0.0);
    float alpha = 0.0;

    // ══ ① 地面：解析求交（便宜，先算；同时用它截断后面的步进）══
    float tFloor = 1e3;
    if (rd.y < -1e-4) {
        float tf = (kFloorY - ro.y) / rd.y;
        if (tf > 0.3)
            tFloor = tf;
    }

    // ══ ② 台面 + 丝条环：球步进（包围球预筛 + 地面截断）══
    float tHit = 1e3;
    float ringHit = 0.0;
    vec3 nHit = vec3(0.0);
    prepThreads(ro, rd);
    {
        vec3 c = vec3(0.0, kFloorY + 1.1, 0.0);
        float br = 4.2 * uField;
        vec3 oc = ro - c;
        float bq = dot(oc, rd);
        float bh = bq * bq - (dot(oc, oc) - br * br);
        if (bh > 0.0) {
            float t0 = max(-bq - sqrt(bh), 0.30);
            float t1 = min(-bq + sqrt(bh), tFloor);
            float t = t0;
            for (int k = 0; k < 48; ++k) {
                if (t > t1)
                    break;
                vec3 p = ro + rd * t;
                float ring;
                float d = mapStage(p, ring);
                if (d < 0.0018) {
                    tHit = t;
                    ringHit = ring;
                    nHit = stageNormal(p);
                    break;
                }
                t += max(d * 0.90, 0.003);
            }
        }
    }
    float tMax = min(tFloor, tHit);

    bool floorNear = tFloor < tHit;
    if (!floorNear && tHit < 1e3) {
        vec3 ph = ro + rd * tHit;
        vec3 zone = mix(uColor.rgb, accent, hash11(floor(ph.x * 3.0) + floor(ph.z * 3.0)));
        if (ringHit > 0.5) {
            // 丝条：自发光细线 + 一点芯亮
            float glow = 0.75 + 0.85 * uAir + 0.45 * clamp(uHigh * 1.2 + uLevel * 0.5, 0.0, 1.0);
            col = mix(accent, white, 0.35) * glow * uBright;
            col += white * 0.22 * uBright;
            alpha = 0.95;
        } else {
            // 台面：深色金属 + 顶面刻线 + 边沿发光
            float up = clamp(nHit.y, 0.0, 1.0);
            float rim = 1.0 - up;
            col = body + zone * (0.10 + 0.75 * rim * rim) * uBright;
            col += white * pow(rim, 3.0) * 0.35 * uBright;
            alpha = 1.0;
        }
    } else if (tFloor < 1e3) {
        vec3 ph = ro + rd * tFloor;
        float r = length(ph.xz);
        // 被灯照亮的地面：中心白 → 四周黑
        float lit = pow(clamp(1.0 - r / (kFloorR * uField), 0.0, 1.0), 2.1);
        lit *= 0.50 + 0.50 * uLevel;
        col = mix(vec3(0.010, 0.016, 0.028), white, lit);
        col += white * pow(clamp(1.0 - r / 2.2, 0.0, 1.0), 3.0) * 0.18;        // 台下的热区
        col += accent * exp(-r * r * 0.12) * 0.12 * uBright;                    // 回光
        alpha = clamp(lit * 1.8 + 0.06, 0.0, 1.0);
    }

    // ══ ③ 天空：竖直渐变 + 地平线辉光（地面之外的暗场）══
    if (alpha < 0.999) {
        float hUp = rd.y * 0.5 + 0.5;
        vec3 sky = mix(vec3(0.022, 0.030, 0.048) * uBright,
                       vec3(0.005, 0.008, 0.016) * uBright, smoothstep(0.22, 0.95, hUp));
        sky += uColor.rgb * pow(max(0.0, 1.0 - abs(rd.y) * 2.6), 4.0) * 0.10 * uBright;
        col = mix(sky, col, alpha);
        alpha = 1.0;
    }

    fragColor = vec4(col * qt_Opacity, alpha * qt_Opacity);
}