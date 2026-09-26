#version 450 core

// 歌词平面：真透视投影到场景中的一块平面上（与背景共用相机 ⇒ 同向旋转）。
// 关键两点：① 字形**固定 LOD 0 采样**（用 mipmap 会把字糊掉）；② 发光用**两圈 8 向明采样**
// 累加 alpha 得到锐利辉光（不依赖 mip 模糊，字形本身也加自发光）。
layout(location = 0) in vec2 qt_TexCoord0;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    vec4  uGlowColor;    // 溢光色（QML 已解析）
    vec2  uResolution;
    vec2  uMouse;
    float uTime;
    float uGlow;         // 溢光强度
    float uTilt;         // 跟随鼠标倾斜幅度
    float uLineV;        // 当前行在纹理中的 v（定位"整行发光"作用范围）
    float uSweep;        // 当前行发光强度
    float uBeatGlow;     // 溢光随鼓点
    float uLevel;
    float uLyricScale;
    float uLyricY;
    float uLyricZ;
    float uCamDist;
    float uCamSens;
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

void main() {
    vec2 sp = (vec2(qt_TexCoord0.x, 1.0 - qt_TexCoord0.y) * uResolution * 2.0 - uResolution)
              / max(uResolution.y, 1.0);

    // 与背景同一套相机
    float yaw = uMouse.x * 1.2 * uCamSens;
    float pitch = clamp(0.30 + uMouse.y * 0.28 * uCamSens, 0.05, 0.75);
    float bob = sin(uTime * 0.4) * 0.25 * uFloat;
    vec3 ta = vec3(0.0, 0.5 + 0.8 * uLevel + bob, 0.0);
    vec3 ro = ta + vec3(cos(yaw), tan(pitch), sin(yaw)) * max(5.0, uCamDist);
    mat3 cb = lookAt(ro, ta);
    vec3 rd = normalize(cb * vec3(sp, 1.9));

    float pd = 6.0 + 2.4 * uLyricZ;
    vec3 pc = ro + cb[2] * pd;
    pc.y += bob * 0.6 + uLyricY * 2.2;
    // 倾斜必须加在"相机坐标系"（cb[0]=右 / cb[1]=上）而不是世界坐标：
    // 世界坐标下相机转到另一半时，同一个偏移会变成反向 ⇒ 左右半边转起来方向相反
    vec3 toCam = normalize(ro - pc);
    vec3 pn = normalize(toCam
                        + cb[0] * (uMouse.x * (0.30 + 0.60 * uTilt))
                        + cb[1] * (0.12 + uMouse.y * (0.20 + 0.30 * uTilt)));
    vec3 pr = normalize(cb[0] - pn * dot(cb[0], pn));
    vec3 pu = normalize(cb[1] - pn * dot(cb[1], pn));
    float scale = clamp(uLyricScale, 0.4, 2.0);
    float halfH = pd / 1.9 / scale;
    float halfW = halfH * uResolution.x / max(uResolution.y, 1.0);

    vec4 outCol = vec4(0.0);
    float den = dot(pn, rd);
    if (den < -1e-4) {
        float tp = dot(pc - ro, pn) / den;
        if (tp > 0.5) {
            vec3 hp = ro + rd * tp - pc;
            vec2 tuv = vec2(dot(hp, pr) / (2.0 * halfW) + 0.5, 0.5 - dot(hp, pu) / (2.0 * halfH));
            if (tuv.x > -0.08 && tuv.x < 1.08 && tuv.y > -0.08 && tuv.y < 1.08) {
                vec2 texel = 1.0 / max(uResolution, vec2(1.0));
                vec4 tx = textureLod(uLyrics, tuv, 0.0);          // 清晰：固定 LOD 0
                float a = tx.a;

                // 两圈 8 向辉光：近圈紧（贴着字形）、远圈散（大范围晕）
                float gNear = 0.0;
                float gFar = 0.0;
                for (int i = 0; i < 8; ++i) {
                    gNear += textureLod(uLyrics, tuv + kDirs[i] * texel * 2.5, 0.0).a;
                    gFar  += textureLod(uLyrics, tuv + kDirs[i] * texel * 11.0, 0.0).a;
                }
                // "字体发光"：整行按字形发光（没有滚动条/扫过带，就是这行字自己在亮）
                float curLine = exp(-abs(tuv.y - uLineV) * 30.0);            // 当前行遮罩
                float glowAmt = uGlow * (0.40 + 1.10 * uLevel * uBeatGlow)
                                * (1.0 + 2.6 * curLine * uSweep);

                vec3 col = tx.rgb;
                col += tx.rgb * a * (0.18 + 1.05 * glowAmt * curLine);        // 字形自发光（当前行最强）
                col += mix(tx.rgb, vec3(1.0), 0.35) * a * curLine * uSweep * (0.25 + 0.50 * uLevel);

                float halo = gNear * 0.070 + gFar * 0.038;

                // 上下渐隐：纹理边缘在 3D 倾斜下会被拉伸，直接隐去
                float edgeFade = smoothstep(0.0, 0.12, tuv.y) * smoothstep(1.0, 0.88, tuv.y);
                outCol = vec4((col + uGlowColor.rgb * halo * glowAmt) * edgeFade,
                              clamp((a + halo * glowAmt * 0.85) * edgeFade, 0.0, 1.0));
            }
        }
    }
    fragColor = outCol * qt_Opacity;
}
