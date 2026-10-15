#version 450 core

// 点云舞台（顶点着色器）：每个点的三维位置由"点序号 + 预设"算出，相机与背景、歌词共用一套。
layout(location = 0) in vec3 aId;        // (点序号, 角序号 0..5, 柔边标记)

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4 uColor;        // 主色
    vec4 uColor2;       // 副色
    vec2 uSize;         // item 像素尺寸（+8 字节 std140 填充）
    vec2 uPad;
    vec4 uCam;          // x=theta y=phi z=radius w=focal
    vec4 uCtrl;         // x=targetY y=time z=preset w=audioGain
    vec4 uAudio;        // low mid high air
    vec4 uMisc;         // x=level y=count z=density w=备用
};

layout(location = 0) out vec4 vColor;
layout(location = 1) out vec2 vCorner;
layout(location = 2) out float vSoft;

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

// 各预设的三维分布；ro = 相机世界坐标（预设 5 需要）
vec3 pointPos(float i, float n, vec3 ro) {
    vec3 h = hash31(i * 0.7919);
    float t = uCtrl.y;
    float aud = uMisc.x * uCtrl.w;

    if (uCtrl.z < 0.5) {                       // 0 星尘：环形体积云
        float ang = h.x * 6.2831853 + t * 0.07;
        float rad = 1.0 + 4.4 * pow(h.y, 0.6);
        float yy = (h.z - 0.5) * 5.0 + sin(t * 0.35 + i * 0.013) * 0.35;
        return vec3(cos(ang) * rad, yy, sin(ang) * rad);
    } else if (uCtrl.z < 1.5) {                // 1 光隧：沿轴流动的圆管
        float ang = h.x * 6.2831853;
        float zz = fract(h.y + t * (0.045 + 0.09 * aud)) * 18.0 - 9.0;
        float rad = 2.5 + 0.40 * sin(ang * 4.0 + zz * 0.7 + t * 0.9);
        return vec3(cos(ang) * rad, sin(ang) * rad * 0.85, zz);
    } else if (uCtrl.z < 2.5) {                // 2 星环：球面主体 + 赤道细环
        if (h.z > 0.62) {
            float ang = h.x * 6.2831853 + t * 0.28;
            float rr = 3.9 + 0.55 * sin(ang * 6.0 + t * 1.2);
            return vec3(cos(ang) * rr, (h.y - 0.5) * 0.32, sin(ang) * rr);
        }
        float th = h.x * 6.2831853 + t * 0.10;
        float ph = acos(clamp(h.y * 2.0 - 1.0, -1.0, 1.0));
        float rr = 2.7 * (1.0 + 0.07 * aud);
        return vec3(rr * sin(ph) * cos(th), rr * cos(ph), rr * sin(ph) * sin(th));
    } else if (uCtrl.z < 3.5) {                // 3 环绕星尘：实体舞台周围点缀
        float ang = h.x * 6.2831853 + t * 0.05;
        float ph = acos(clamp(h.y * 2.0 - 1.0, -1.0, 1.0));
        float rr = 6.2 + 4.2 * h.z + 0.5 * sin(t * 0.5 + ang * 3.0);
        return vec3(rr * sin(ph) * cos(ang), rr * cos(ph) * 0.60, rr * sin(ph) * sin(ang));
    } else if (uCtrl.z < 4.5) {                // 4 舞台星屑：歌词周围一层薄星屑
        // 前后都铺 ⇒ 总有点落在歌词前面，歌词就"在粒子里面"
        float ang = h.x * 6.2831853;
        float rad = 0.55 + 3.70 * pow(h.y, 0.72);
        float zz = (h.z - 0.5) * 6.5;
        float yy = (h.x - 0.5) * 5.0 + sin(t * 0.30 + i * 0.07) * 0.14;
        return vec3(cos(ang) * rad, yy + 0.40, zz);
    }
    // 5 前景浮尘：相机附近的大颗柔点
    vec3 dir = normalize(vec3(h.x * 2.0 - 1.0, h.y * 2.0 - 1.0, h.z * 2.0 - 1.0));
    float dist = 0.55 + 1.60 * fract(h.z * 7.7);
    return ro + dir * dist;
}

void main() {
    float i = aId.x;
    vec3 ta = vec3(0.0, uCtrl.x + 0.8 * uMisc.x * uCtrl.w, 0.0);
    vec3 ro = ta + uCam.z * vec3(cos(uCam.y) * cos(uCam.x),
                                 sin(uCam.y),
                                 cos(uCam.y) * sin(uCam.x));

    vec3 cw = normalize(ta - ro);
    vec3 cu = normalize(cross(cw, vec3(0.0, 1.0, 0.0)));
    vec3 cv = cross(cu, cw);

    vec3 p = pointPos(i, uMisc.y, ro);
    vec3 rel = p - ro;
    float depth = dot(rel, cw);

    if (depth < 0.25) {                        // 相机背后 ⇒ 剔到屏幕外
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        vColor = vec4(0.0);
        vCorner = vec2(2.0);
        vSoft = 0.0;
        return;
    }

    vec2 ndc = vec2(dot(rel, cu), dot(rel, cv)) * (max(uCam.w, 0.6) / depth);
    vec2 px = vec2(ndc.x * 0.5 * uSize.y + 0.5 * uSize.x,
                   -ndc.y * 0.5 * uSize.y + 0.5 * uSize.y);
    // 透视尺寸衰减：基准半径 ÷ 深度，并夹到 0.9~5.5 px ⇒ 小、清、不糊
    bool dust = uCtrl.z > 4.5;
    float base = dust ? (0.006 + 0.010 * hash11(i * 1.37))
                      : (0.028 + 0.022 * hash11(i * 1.37));
    vec2 off = cornerOffset(aId.y);
    float rad = clamp(base * uSize.y / max(depth, 0.6), 0.9, dust ? 26.0 : 5.5);
    px += off * rad;

    gl_Position = qt_Matrix * vec4(px, 0.0, 1.0);
    vCorner = off;
    vSoft = aId.z;
    float atten = 1.0 / (1.0 + 0.020 * depth * depth);
    vec3 col = mix(uColor.rgb, uColor2.rgb, hash11(i * 3.71));
    float a = atten * (0.35 + 1.10 * uMisc.x * uCtrl.w) * uMisc.z * (dust ? 0.28 : 1.0);
    vColor = vec4(col, a);
}