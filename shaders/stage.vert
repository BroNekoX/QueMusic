#version 450 core

// 点云舞台（顶点着色器）：每个点在三维空间里的位置由"点序号 + 预设"算出，
// 与背景着色器共用同一套相机（yaw/pitch/lookAt + kFocal=1.9）⇒ 点的透视与方块场完全一致，
// 再按深度做尺寸衰减与亮度衰减 ⇒ 真实景深。参考 MineRadio 的点云思路，本项目原创实现。
layout(location = 0) in vec2 aId;        // (点序号, 角序号 0..5)

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4  uColor;
    vec4  uColor2;
    vec2  uSize;        // item 像素尺寸
    vec2  uMouse;
    float uTime;
    float uLow;
    float uMid;
    float uHigh;
    float uAir;
    float uLevel;
    float uPreset;      // 0星尘 1光隧 2星环 3舞台
    float uAudio;
    float uCount;
    float uCamDist;
    float uCamSens;
    float uDensity;
};

layout(location = 0) out vec4 vColor;
layout(location = 1) out vec2 vCorner;

const float kFocal = 1.9;

float hash11(float n) {
    return fract(sin(n) * 43758.5453123);
}

vec3 hash31(float n) {
    return vec3(hash11(n), hash11(n + 7.13), hash11(n + 19.77));
}

vec2 cornerOffset(float c) {
    int k = int(c + 0.5);
    if (k == 0) return vec2(-1.0, -1.0);
    if (k == 1) return vec2(1.0, -1.0);
    if (k == 2) return vec2(-1.0, 1.0);
    if (k == 3) return vec2(1.0, -1.0);
    if (k == 4) return vec2(1.0, 1.0);
    return vec2(-1.0, 1.0);
}

// 三维分布：每一预设一种形态，均由频谱驱动
vec3 pointPos(float i, float n) {
    vec3 h = hash31(i * 0.7919);
    float t = uTime;
    float aud = uLevel * uAudio;

    if (uPreset < 0.5) {                       // 星尘：环形体积云（缓慢自转）
        float ang = h.x * 6.2831853 + t * 0.07;
        float rad = 1.0 + 4.4 * pow(h.y, 0.6);
        float yy = (h.z - 0.5) * 5.0 + sin(t * 0.35 + i * 0.013) * 0.35;
        return vec3(cos(ang) * rad, yy, sin(ang) * rad);
    } else if (uPreset < 1.5) {                // 光隧：沿轴流动的圆管
        float ang = h.x * 6.2831853;
        float zz = fract(h.y + t * (0.045 + 0.09 * aud)) * 18.0 - 9.0;
        float rad = 2.5 + 0.40 * sin(ang * 4.0 + zz * 0.7 + t * 0.9);
        return vec3(cos(ang) * rad, sin(ang) * rad * 0.85, zz);
    } else if (uPreset < 2.5) {                // 星环：球面主体 + 赤道细环
        if (h.z > 0.62) {
            float ang = h.x * 6.2831853 + t * 0.28;
            float rr = 3.9 + 0.55 * sin(ang * 6.0 + t * 1.2);
            return vec3(cos(ang) * rr, (h.y - 0.5) * 0.32, sin(ang) * rr);
        }
        float th = h.x * 6.2831853 + t * 0.10;
        float ph = acos(clamp(h.y * 2.0 - 1.0, -1.0, 1.0));
        float rr = 2.7 * (1.0 + 0.07 * aud);
        return vec3(rr * sin(ph) * cos(th), rr * cos(ph), rr * sin(ph) * sin(th));
    }
    // 预设 3：环绕星尘 —— 给实体舞台（bg_stage.frag）做周围点缀，大半径球壳缓慢自转
    float ang = h.x * 6.2831853 + t * 0.05;
    float ph = acos(clamp(h.y * 2.0 - 1.0, -1.0, 1.0));
    float rr = 6.2 + 4.2 * h.z + 0.5 * sin(t * 0.5 + ang * 3.0);
    return vec3(rr * sin(ph) * cos(ang), rr * cos(ph) * 0.60, rr * sin(ph) * sin(ang));
}

void main() {
    float i = aId.x;
    float yaw = uMouse.x * 1.2 * uCamSens;
    float pitch = clamp(0.30 + uMouse.y * 0.28 * uCamSens, 0.05, 0.75);
    vec3 ta = vec3(0.0, 0.5 + 0.8 * uLevel * uAudio, 0.0);
    vec3 ro = ta + vec3(cos(yaw), tan(pitch), sin(yaw)) * max(5.0, uCamDist);

    vec3 cw = normalize(ta - ro);
    vec3 cu = normalize(cross(cw, vec3(0.0, 1.0, 0.0)));
    vec3 cv = cross(cu, cw);

    vec3 p = pointPos(i, uCount);
    vec3 rel = p - ro;
    float depth = dot(rel, cw);

    if (depth < 0.25) {                        // 相机背后 ⇒ 剔到屏幕外
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        vColor = vec4(0.0);
        vCorner = vec2(2.0);
        return;
    }

    vec2 ndc = vec2(dot(rel, cu), dot(rel, cv)) * (kFocal / depth);
    vec2 px = vec2(ndc.x * 0.5 * uSize.y + 0.5 * uSize.x,
                   -ndc.y * 0.5 * uSize.y + 0.5 * uSize.y);
    // 透视尺寸衰减：基准半径约为屏高的 5.5%~10% 除以深度（深度 9 时约 5~7 像素，近处更大）
    float rad = (0.055 + 0.045 * hash11(i * 1.37)) * uSize.y / max(depth, 0.6);
    vec2 off = cornerOffset(aId.y);
    px += off * rad;

    gl_Position = qt_Matrix * vec4(px, 0.0, 1.0);
    vCorner = off;
    float atten = 1.0 / (1.0 + 0.020 * depth * depth);
    vec3 col = mix(uColor.rgb, uColor2.rgb, hash11(i * 3.71));
    vColor = vec4(col, atten * (0.35 + 1.10 * uLevel * uAudio) * uDensity);
}
