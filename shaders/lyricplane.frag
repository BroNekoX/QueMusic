#version 450 core

// 歌词板：空间里的一块平板，固定在场景正前方（世界 +X），不跟随相机 ——
// 转到侧面看侧面，转到背面看背面（背面 u 镜像，仍然正着读）。
// 四层叠加：字形自发光 + 两圈辉光 + 太阳泛光 + 地面倒影；另有景深、雾、上下渐隐、暗底衬。
layout(location = 0) in vec2 qt_TexCoord0;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    vec4  uGlowColor;
    vec2  uResolution;
    float uTime;

    // 共享相机
    float uCamTheta;
    float uCamPhi;
    float uCamRadius;
    float uCamTargetY;
    float uFocal;        // 1/tan(fovY/2)

    // 歌词板（世界坐标，固定不动）
    float uHalfH;        // 板半高；常量 ⇒ 拉远相机时整块跟着变小
    float uPlaneDist;    // 沿 +X 往前放多远
    float uYOffset;      // 板中心高度
    float uFloorY;       // 地面高度（倒影用）

    // 观感
    float uGlow;
    float uSweep;
    float uBeatGlow;
    float uLevel;
    float uBeat;
    float uSun;          // 慢速副歌闸门
    float uReflect;
    float uReadab;
    float uFog;
    float uDof;
    float uFloat;
};

layout(binding = 1) uniform sampler2D uLyrics;
layout(location = 0) out vec4 fragColor;

const vec2 kDirs[8] = vec2[8](
    vec2(1.0, 0.0), vec2(0.7071, 0.7071), vec2(0.0, 1.0), vec2(-0.7071, 0.7071),
    vec2(-1.0, 0.0), vec2(-0.7071, -0.7071), vec2(0.0, -1.0), vec2(0.7071, -0.7071));

mat3 lookAt(vec3 ro, vec3 ta) {
    vec3 cw = normalize(ta - ro);
    vec3 cu = normalize(cross(cw, vec3(0.0, 1.0, 0.0)));
    return mat3(cu, cross(cu, cw), cw);
}

vec3 camEye(vec3 ta) {
    return ta + uCamRadius * vec3(cos(uCamPhi) * cos(uCamTheta),
                                  sin(uCamPhi),
                                  cos(uCamPhi) * sin(uCamTheta));
}

// 与平面求交：返回 uv、命中点相对板中心的 y、是否背面
bool hitBoard(vec3 ro, vec3 rd, vec3 B, out vec2 uv, out float y, out bool back) {
    const vec3 kN = vec3(1.0, 0.0, 0.0);        // 板法线：世界 +X
    float den = dot(rd, kN);
    uv = vec2(0.0);
    y = 0.0;
    back = den > 0.0;                            // 射线朝 +X ⇒ 看到背面
    if (abs(den) < 1e-5)
        return false;
    float t = dot(B - ro, kN) / den;
    if (t <= 0.5)
        return false;
    vec3 p = ro + rd * t - B;
    float halfH = max(uHalfH, 0.2);
    float halfW = halfH * (uResolution.x / max(uResolution.y, 1.0));
    // 屏幕右 = -Z ⇒ u 沿 -Z 增长；纹理 v 向下 ⇒ v 与 y 反号
    uv = vec2(0.5 - p.z / (2.0 * halfW), 0.5 - p.y / (2.0 * halfH));
    if (back)
        uv.x = 1.0 - uv.x;                       // 背面镜像
    y = p.y;
    return true;
}

void main() {
    vec2 sp = (vec2(qt_TexCoord0.x, 1.0 - qt_TexCoord0.y) * uResolution * 2.0 - uResolution)
              / max(uResolution.y, 1.0);

    float bob = sin(uTime * 0.4) * 0.22 * uFloat;
    vec3 ta = vec3(0.0, uCamTargetY + 0.75 * uLevel, 0.0);
    vec3 ro = camEye(ta);
    mat3 cb = lookAt(ro, ta);
    vec3 rd = normalize(cb * vec3(sp, max(uFocal, 0.6)));

    vec3 B = vec3(uPlaneDist, uYOffset + bob, 0.0);     // 板中心
    float halfH = max(uHalfH, 0.2);
    float halfW = halfH * (uResolution.x / max(uResolution.y, 1.0));

    vec3 col = vec3(0.0);
    float alpha = 0.0;

    // ① 板本体
    vec2 uv;
    float py;
    bool back;
    if (hitBoard(ro, rd, B, uv, py, back)) {
        if (uv.x > -0.09 && uv.x < 1.09 && uv.y > -0.09 && uv.y < 1.09) {
            vec2 texel = 1.0 / max(uResolution, vec2(1.0));
            float far = clamp(abs(py) / halfH, 0.0, 1.0);          // 离当前行的行距
            float lod = uDof * far * far * 3.0;                    // 景深（mip LOD）
            float depthFog = 1.0 - uFog * far * far;
            float edgeFade = smoothstep(-0.05, 0.09, uv.y) * smoothstep(1.05, 0.91, uv.y);

            vec4 tx = textureLod(uLyrics, uv, lod);
            float a = tx.a;

            // 两圈 8 向辉光：近圈贴字形、远圈大范围晕
            float gNear = 0.0, gFar = 0.0;
            for (int i = 0; i < 8; ++i) {
                gNear += textureLod(uLyrics, uv + kDirs[i] * texel * (2.2 + 2.0 * far), lod).a;
                gFar  += textureLod(uLyrics, uv + kDirs[i] * texel * (10.0 + 6.0 * far), lod).a;
            }
            float curLine = exp(-abs(py) * 8.0 / halfH);           // 当前行遮罩
            float glowAmt = uGlow * (0.40 + 1.05 * uLevel * uBeatGlow + 0.85 * uBeat)
                            * (1.0 + 2.4 * curLine * uSweep) * depthFog;

            vec3 c = tx.rgb;
            c += tx.rgb * a * (0.20 + 1.00 * glowAmt * curLine);
            c += mix(tx.rgb, vec3(1.0), 0.40) * a * curLine * uSweep * (0.25 + 0.55 * uLevel);
            c += mix(tx.rgb, vec3(1.0), 0.35) * a * uSun * 0.30;

            // 颜色值给到 >1，src-over 下才会当"光"而不是"蒙版"
            float halo = gNear * 0.070 + gFar * 0.036;
            float backLit = uReadab * smoothstep(0.02, 0.85, gNear) * (1.0 - a) * edgeFade;
            col += (c + uGlowColor.rgb * halo * glowAmt * 2.80
                    - vec3(0.05, 0.065, 0.08) * backLit) * edgeFade * depthFog;
            alpha = clamp(a * edgeFade + halo * glowAmt * 0.42 * edgeFade
                          + backLit * (1.0 - alpha), 0.0, 1.0);
        }
    }

    // ② 地面倒影：把射线关于地面镜像后与同一块板求交（只有下望时才可能看到地面）
    if (uReflect > 0.001 && rd.y < -0.02) {
        vec3 oR = vec3(ro.x, 2.0 * uFloorY - ro.y, ro.z);
        vec3 dR = vec3(rd.x, -rd.y, rd.z);
        vec2 uvR;
        float pyR;
        bool backR;
        if (hitBoard(oR, dR, B, uvR, pyR, backR)) {
            if (uvR.x > 0.0 && uvR.x < 1.0 && uvR.y > 0.0 && uvR.y < 1.0) {
                vec4 tx = textureLod(uLyrics, uvR, 2.4);
                float lift = max(pyR + uYOffset - uFloorY, 0.0);   // 离地越高倒影越淡
                float fade = uReflect * exp(-lift * 1.6) * 0.60;
                col += tx.rgb * tx.a * fade;
                alpha = clamp(alpha + tx.a * fade * 1.5, 0.0, 1.0);
            }
        }
    }

    // ③ 太阳式径向泛光：当前行背后一团暖光（慢速副歌驱动）
    // 它会和字形颜色相加，过强会把字糊成一片白
    {
        vec3 rel = B - ro;
        float zc = dot(rel, cb[2]);
        if (zc > 0.5) {
            vec2 ndc = vec2(dot(rel, cb[0]), dot(rel, cb[1])) * (max(uFocal, 0.6) / zc);
            float d2 = length(sp - ndc);
            float r = 0.20 + 0.13 * (0.5 + 0.5 * sin(uTime * 0.37)) + 0.15 * uSun + 0.05 * uBeat;
            float fall = exp(-pow(d2 / max(r, 0.05), 2.3));
            float amt = (0.05 + 0.22 * uSun + 0.10 * uLevel + 0.14 * uBeat) * uGlow;
            vec3 warm = mix(vec3(1.00, 0.90, 0.58), vec3(1.00, 0.96, 0.80), uSun);
            col += warm * fall * amt * 1.50;
            alpha = clamp(alpha + fall * amt * 0.85, 0.0, 1.0);
        }
    }

    fragColor = vec4(col, alpha) * qt_Opacity;
}