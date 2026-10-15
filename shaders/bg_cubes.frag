#version 450 core

// 预设 0：方块地形 —— 一根根真盒子柱 + 浮空方块，高度由各频段各自的空间区域叠加。
// 顶面做描边高光、侧面做上亮下暗渐变（面法线来自真实的盒子求交），所以是"立柱"而不是"纸片"。
// 参考 sonic-topography / mineradio 的做法（同为"音域地形"这一视觉类型），本项目独立实现。
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
    // 地形专用
    float uBeat;         // 鼓点脉冲（浮空方块、涟漪、低频冲击）
    vec4  uRipA;         // 涟漪槽 A：xy = 世界 XZ，z = 触发时刻，w = 强度
    vec4  uRipB;
};

layout(location = 0) out vec4 fragColor;

// ── 地形常量 ──
const float kBaseY = -2.30;      // 地形基准面（世界 Y）—— 主要是往下放，横向居中在相机注视点上
const float kMaxH = 2.45;        // 柱子最高（世界单位）
const float kRMax = 10.6;        // 圆盘半径
const float kBoxFill = 0.85;     // 盒宽 / 格距，留缝才有"一根根"的分离感
const vec2 kCenter = vec2(0.0, 0.0);

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

vec2 hash22(float k) {
    return vec2(hash21(vec2(k, 1.7)), hash21(vec2(k, 9.3)));
}

float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash21(i), hash21(i + vec2(1.0, 0.0)), u.x),
               mix(hash21(i + vec2(0.0, 1.0)), hash21(i + vec2(1.0, 1.0)), u.x), u.y);
}

mat3 lookAt(vec3 ro, vec3 ta) {
    vec3 cw = normalize(ta - ro);
    vec3 cu = normalize(cross(cw, vec3(0.0, 1.0, 0.0)));
    return mat3(cu, cross(cu, cw), cw);
}

// 涟漪：返回 (抬升, 冷色强度, 白色强度)
vec3 rippleAt(vec2 q) {
    vec3 acc = vec3(0.0);
    for (int s = 0; s < 2; ++s) {
        vec4 rp = (s == 0) ? uRipA : uRipB;
        if (rp.w < 0.01)
            continue;
        float age = uTime - rp.z;
        if (age < 0.0 || age > 4.8)
            continue;
        float d = length(q - rp.xy) - age * 2.30;
        float prof = exp(-d * d / (0.95 * 0.95));
        float fade = exp(-age * 2.30 / 8.0) * (1.0 - smoothstep(2.10, 4.80, age));
        float p = prof * fade * rp.w;
        acc += vec3(p * 0.58, p * 1.30, 0.0);
    }
    return acc;
}

// 第 idx 格（格心 = idx * cell）的柱高。各频段各占一块空间区域，再乘圆盘衰减。
float columnHeight(vec2 idx, float cell, out float rndOut) {
    vec2 p = idx * cell;
    float r = length(p);
    float rnd = hash21(idx + 0.5);
    rndOut = rnd;
    if (r > kRMax)
        return 0.0;

    float fall = smoothstep(kRMax, kRMax * 0.42, r);
    float aud = max(uAudio, 0.0);

    // 次低音：中心穹顶（叠一层大尺度噪声，免得整片顶成一块白板）
    float dome = smoothstep(5.0, 0.0, r) * (0.55 + 0.45 * vnoise(p * 0.22));
    // 低音：噪声扭曲的环，按格随机开门 ⇒ 一片片柱子跳起来
    float nb = vnoise(p * 0.62 - vec2(0.0, uTime * 0.10)) * 2.0 - 1.0;
    float ring = smoothstep(8.2, 1.2, r + nb * 1.7);
    float gate = smoothstep(0.0, 1.0, rnd + clamp(uAudio, 0.0, 2.0) * 0.28);
    // 中低音：平滑噪声丘陵（同一片区域一起起伏 ⇒ 不是麻将牌）
    float hills = vnoise(p * 0.34 + vec2(uTime * 0.05, 0.0));
    // 中音：一条流动的河
    float flow = max(0.0, sin(p.x * 1.05 + p.y * 1.05 + vnoise(p * 0.5) * 2.4 - uTime * 2.2));
    // 高音：外围稀疏尖刺
    float spike = (rnd > 0.80) ? smoothstep(3.0, 8.0, r) * fract(rnd * 7.7) : 0.0;

    float sb = clamp(uLow * 0.30 + uBeat * 0.55, 0.0, 1.30);
    float ba = clamp(uLow * 0.85 + uBeat * 0.45, 0.0, 1.4);
    float lm = clamp(uMid * 0.55 + uLow * 0.25, 0.0, 1.2);
    float md = clamp(uMid * 0.95, 0.0, 1.2);
    float hm = clamp(uHigh * 0.75 + uMid * 0.20, 0.0, 1.2);

    float e = dome * 1.25 * sb
            + ring * 1.95 * ba * gate
            + hills * 0.85 * lm
            + flow * 1.25 * md
            + spike * 1.30 * hm;
    e = max(0.0, e * aud - 0.22);
    if (rnd > 0.988)
        e += 2.0 * uLevel * aud;

    // 空闲波浪：没有音频时也活着
    float idle = mix(vnoise(p * 0.30 + vec2(uTime * 0.05, 0.0)),
                     sin(p.x * 0.55 + p.y * 0.42 - uTime * 0.55) * 0.5 + 0.5, 0.45) * 0.16;
    e += idle;
    e += rippleAt(p).x * 0.85;                 // 涟漪抬高波前那一圈柱子

    // 软饱和：越接近上限越压，避免"响一点就整片顶成一堵墙"
    e = kMaxH * (1.0 - exp(-e * 0.80 / kMaxH));
    return e * fall;
}

// 浮空方块：与柱子同一套着色。全部落在板子后方（-X 侧），不挡歌词。
const int kFloatN = 12;

void floatBlock(int b, out vec3 c, out float s, out float rot) {
    vec2 h = hash22(float(b) * 3.77);
    float ang = uTime * 0.22 + h.x * 6.2832;
    float rad = 2.2 + h.y * 5.4;
    c = vec3(-1.6 - (0.35 + h.x * 0.85) * rad,
             kBaseY + 1.55 + h.x * 1.70 + sin(uTime * 0.55 + h.y * 6.2832) * 0.20 + uBeat * 0.75,
             cos(ang) * rad * 0.85);
    s = (0.110 + h.y * 0.155) * (0.55 + 0.75 * clamp(uBeat, 0.0, 1.0));
    rot = uTime * (0.16 + h.x * 0.22) + h.y * 6.2832;
}

// 顶面描边：5% 内边距的一圈亮线（局部 uv）
float topEdge(vec2 luv) {
    float e = smoothstep(0.075, 0.020, luv.x) + smoothstep(0.925, 0.980, luv.x)
            + smoothstep(0.075, 0.020, luv.y) + smoothstep(0.925, 0.980, luv.y);
    return min(e, 1.0);
}

void main() {
    vec2 sp = (vec2(qt_TexCoord0.x, 1.0 - qt_TexCoord0.y) * uResolution * 2.0 - uResolution)
              / max(uResolution.y, 1.0);

    float fs = clamp(uField, 0.45, 2.0);
    float cell = 0.300 / fs;                       // 格距（世界单位）
    float rmax = kRMax * fs;

    vec3 ta = vec3(0.0, uCamTargetY + 0.75 * uLevel, 0.0);
    vec3 ro = ta + uCamRadius * vec3(cos(uCamPhi) * cos(uCamTheta),
                                     sin(uCamPhi),
                                     cos(uCamPhi) * sin(uCamTheta));
    mat3 cb = lookAt(ro, ta);
    vec3 rd = normalize(cb * vec3(sp, max(uFocal, 0.6)));

    // ── 主题色：只用主题两色 + 白热，靠亮度分层出层次，不引入和主题打架的色相 ──
    vec3 base = uColor.rgb;                        // 主色
    vec3 acc = mix(uColor2.rgb, vec3(1.0), 0.10);  // 副色提亮
    vec3 hot = mix(acc, vec3(1.0), 0.30);          // 峰值白热
    vec3 body = vec3(0.012, 0.020, 0.034) + base * 0.022;
    const vec3 kLight = normalize(vec3(0.35, 0.80, 0.45));

    vec3 col = vec3(0.0);
    float alpha = 0.0;
    float tMax = 1e3;

    // ══ ① 浮空方块（先算，最近的一个会把地形步进截断）══
    float ft;
    vec3 fn;
    vec2 fuv;
    float fscale;
    int fHit = -1;
    for (int b = 0; b < kFloatN; ++b) {
        vec3 bc;
        float bs, brot;
        floatBlock(b, bc, bs, brot);
        float ca = cos(brot), sa = sin(brot);
        vec3 o = ro - bc;
        vec3 d = rd;
        o.xz = vec2(o.x * ca + o.z * sa, -o.x * sa + o.z * ca);
        d.xz = vec2(d.x * ca + d.z * sa, -d.x * sa + d.z * ca);
        vec3 inv = 1.0 / d;
        vec3 a = (-bs - o) * inv;
        vec3 k = (bs - o) * inv;
        vec3 lo = min(a, k), hi = max(a, k);
        float t0 = max(max(lo.x, lo.y), lo.z);
        float t1 = min(min(hi.x, hi.y), hi.z);
        if (t0 <= t1 && t1 > 0.4 && t0 < tMax) {
            float th = max(t0, 0.4);
            tMax = th;
            fHit = b;
            ft = th;
            fscale = bs;
            vec3 pn = o + d * th;
            vec3 an = abs(pn) / max(bs, 1e-4);
            fn = (an.x > an.y && an.x > an.z) ? vec3(sign(pn.x), 0.0, 0.0)
                 : (an.y > an.z) ? vec3(0.0, sign(pn.y), 0.0) : vec3(0.0, 0.0, sign(pn.z));
            fn.xz = vec2(fn.x * ca - fn.z * sa, fn.x * sa + fn.z * ca);   // 旋回世界坐标系，否则明暗是歪的
            fuv = (an.y > an.z && an.y > an.x) ? pn.xz : (an.x > an.z ? pn.zy : pn.xy);
            fuv = fuv / max(bs, 1e-4) * 0.5 + 0.5;
        }
    }

    // ══ ② 地形：XZ 用格索引空间的 DDA，逐格做真盒子求交（拿到面法线）══
    const float bh = 0.5 * kBoxFill;
    float tHit = 1e3;
    vec3 nHit = vec3(0.0);
    vec2 luv = vec2(0.0);
    float hHit = 0.0;
    float rndHit = 0.0;

    vec2 rod = (ro.xz - (kCenter + vec2(0.0, 0.0))) / cell;
    vec2 rdd = rd.xz / cell;
    // 世界坐标半径上限（原来误把格索引半径当世界半径 ⇒ 包围盒大 3 倍多，
    // 掠射光线会在 150 步内还没走到圆盘就退出 ⇒ 远处整片方块消失）
    float lim = kRMax * fs + cell;
    // 先把光线裁到地形包围盒内
    vec3 blo = vec3(kCenter.x - lim, kBaseY, kCenter.y - lim);
    vec3 bhi = vec3(kCenter.x + lim, kBaseY + kMaxH + 0.05, kCenter.y + lim);
    vec3 ia = (blo - ro) / rd, ib = (bhi - ro) / rd;
    vec3 l2 = min(ia, ib), h2 = max(ia, ib);
    float te = max(max(l2.x, l2.y), l2.z);
    float tx = min(min(h2.x, h2.y), h2.z);
    te = max(te, 0.35);
    if (te < tx && te < tMax) {
        vec3 ps = ro + rd * te;
        vec2 rid = (ps.xz - kCenter) / cell;
        int i = int(floor(rid.x)), j = int(floor(rid.y));
        int si = rdd.x > 0.0 ? 1 : -1;
        int sj = rdd.y > 0.0 ? 1 : -1;
        float t = te;
        for (int k = 0; k < 150; ++k) {
            if (t > tx || t > tMax)
                break;
            float rnd;
            // 盒子必须放在格子中心 [i+0.5, j+0.5]：DDA 的格子是 [i, i+1]，
            // 原来盒子放在 i 上等于偏了半格、一半落在上一格里 ⇒ 遮挡顺序错乱，方块看着被切
            vec2 cc = vec2(float(i) + 0.5, float(j) + 0.5);
            float h = columnHeight(cc, cell, rnd);
            if (h > 1e-3) {
                float y0 = kBaseY, y1 = kBaseY + h;
                // 注意 + te：rdd/rid 是相对步进入口的，盒子的 t 必须换算成绝对距离
                vec2 sa2 = (cc - bh - rid) / rdd + te;
                vec2 sb2 = (cc + bh - rid) / rdd + te;
                vec2 mn = min(sa2, sb2), mx = max(sa2, sb2);
                float x0 = max(mn.x, mn.y), x1 = min(mx.x, mx.y);
                float ya = (y0 - ro.y) / rd.y, yb = (y1 - ro.y) / rd.y;
                float ylo = min(ya, yb), yhi = max(ya, yb);
                float t0 = max(x0, ylo), t1 = min(x1, yhi);
                // t0 ≥ t：命中的必须是"前方"的面，否则会画到身后的盒子上
                if (t0 <= t1 && t0 >= t - 1e-4 && t1 > 0.35 && t0 < tMax) {
                    float th = max(t0, 0.35);
                    // 侧面还要分 X 面 / Z 面：原来一律当 X 面 ⇒ 明暗方向错乱，看着像被切
                    if (x0 > ylo) {
                        nHit = (mn.x > mn.y) ? vec3(rdd.x > 0.0 ? -1.0 : 1.0, 0.0, 0.0)
                                             : vec3(0.0, 0.0, rdd.y > 0.0 ? -1.0 : 1.0);
                    } else {
                        nHit = vec3(0.0, rd.y > 0.0 ? -1.0 : 1.0, 0.0);
                    }
                    tHit = th;
                    hHit = h;
                    rndHit = rnd;
                    vec3 ph = ro + rd * th;
                    vec2 f = (ph.xz - kCenter) / cell - cc;
                    // 盒内局部 uv ∈ [0,1]（原来多乘了 0.5 ⇒ 只到 [0.25,0.75]，顶面描边永远不亮）
                    luv = nHit.y > 0.5 ? (f / kBoxFill + 0.5)
                                       : (abs(nHit.x) > 0.5 ? vec2(f.y / kBoxFill + 0.5, 0.0)
                                                            : vec2(f.x / kBoxFill + 0.5, 0.0));
                    break;
                }
            }
            // 每步按当前格重算跨格时刻：增量累加会漂移，远处会漏格/穿格
            float tNextX = te + (float(i) + (si > 0 ? 1.0 : 0.0) - rid.x) / rdd.x;
            float tNextY = te + (float(j) + (sj > 0 ? 1.0 : 0.0) - rid.y) / rdd.y;
            if (tNextX < tNextY) {
                t = tNextX;
                i += si;
            } else {
                t = tNextY;
                j += sj;
            }
        }
    }

    // ══ ③ 着色 ══
    if (fHit >= 0 && ft <= tHit) {
        // 浮空方块：按"已抬升"处理 ⇒ 顶面整体发光 + 描边，与柱子同一套语言
        float rnd = hash21(vec2(float(fHit) * 5.31, 2.0));
        vec3 glow = mix(mix(base, acc, 0.45), mix(acc, hot, 0.5), 0.30 + fract(rnd * 11.0) * 0.55);
        float lum = 0.85 + 0.45 * max(dot(fn, kLight), 0.0);
        if (fn.y > 0.5) {
            col = body + glow * uBright * lum + mix(glow, hot, 0.50) * topEdge(fuv) * 1.05 * uBright;
        } else {
            float side = mix(0.30, 1.0, fuv.y);
            col = body * side + glow * uBright * lum * side * 1.15;
            col += glow * smoothstep(0.10, 0.0, 1.0 - fuv.y) * 0.55 * uBright;
        }
        alpha = 0.95;
    } else if (tHit < 1e3) {
        vec3 ph = ro + rd * tHit;
        vec2 q = ph.xz - kCenter;
        float r = length(q);
        float normE = clamp((hHit - 0.30) / 1.70, 0.0, 1.0);
        float relY = clamp((ph.y - kBaseY) / max(hHit, 1e-3), 0.0, 1.0);
        bool isTop = nHit.y > 0.5;

        // 区域配色（中心更亮更白热）+ 每格抖动
        float centerMix = 1.0 - clamp(r / (rmax * 0.95), 0.0, 1.0);
        vec3 glow = mix(mix(base, acc, 0.35), mix(acc, hot, 0.45),
                        clamp(centerMix * 0.85 + fract(rndHit * 11.0) * 0.20, 0.0, 1.0));
        float distFade = 1.0 - smoothstep(rmax * 0.75, rmax, r);
        vec3 rip = rippleAt(q);
        vec3 ripCol = mix(uColor2.rgb, vec3(1.0), 0.30);
        glow = mix(glow, ripCol, clamp(rip.y * 0.85, 0.0, 0.85));
        glow = mix(glow, vec3(1.0), clamp(rip.z, 0.0, 1.0));

        // 发光色本身随高度从底色渐亮 ⇒ 矮柱几乎只剩底色，高柱才亮（清晰的来源）
        vec3 lit = mix(body, glow, smoothstep(0.10, 0.72, normE));
        float lum = 0.72 + 0.55 * max(dot(nHit, kLight), 0.0);
        vec3 c;
        if (isTop) {
            float topI = smoothstep(0.10, 0.50, normE);
            c = body + lit * 0.72 * topI * uBright * lum;
            c += mix(lit, hot, 0.45) * topEdge(luv) * 1.05 * uBright * smoothstep(0.02, 0.40, normE);
            c += mix(lit, hot, 0.60) * pow(normE, 2.0) * 0.24 * uBright;
            // 稀疏闪点（高频）
            if (fract(rndHit * 53.0) > 0.982) {
                float tw = 0.5 + 0.5 * sin(uTime * 34.0 + rndHit * 90.0);
                c += mix(vec3(1.0), vec3(0.55, 1.0, 1.0), rndHit) * tw * uAir * 0.8;
            }
        } else {
            float side = smoothstep(0.42, 0.0, 1.0 - relY) * smoothstep(0.05, 0.55, normE);
            c = mix(body * mix(0.45, 1.0, relY), lit * 0.90 * uBright * lum, side * 1.30);
            c += lit * smoothstep(0.05, 0.0, 1.0 - relY) * smoothstep(0.10, 0.60, normE) * uBright;
        }
        // 涟漪自发光：波前本身是一圈亮环（不依赖柱子高度）
        c += ripCol * rip.y * 0.42;
        c += vec3(1.0) * rip.z * 0.85;

        // 空气雾 + 边缘消隐
        c = mix(c, mix(body, glow, 0.35) * 0.6, smoothstep(rmax * 0.60, rmax * 1.05, r) * 0.55);
        col = c;
        alpha = distFade;
    }

    // ══ ④ 天空：竖直渐变 + 地平线辉光 + 稀疏星点 ══
    if (alpha < 0.999) {
        float hUp = rd.y * 0.5 + 0.5;
        vec3 sky = mix(vec3(0.020, 0.030, 0.052) * uBright,
                       vec3(0.005, 0.008, 0.018) * uBright, smoothstep(0.24, 0.95, hUp));
        sky += uColor2.rgb * pow(max(0.0, 1.0 - abs(rd.y) * 2.2), 3.0) * 0.16 * uBright;
        sky += uColor.rgb * pow(max(0.0, 1.0 - abs(rd.y) * 3.4), 5.0) * 0.10 * uBright;
        vec2 sq = rd.xz / max(0.35, abs(rd.y) + 0.45) * 3.0;
        vec2 gi = floor(sq * 5.0), gf = fract(sq * 5.0) - 0.5;
        float sh = hash21(gi);
        if (sh > 0.88 && rd.y > -0.02) {
            vec2 off = (vec2(hash21(gi + 3.1), hash21(gi + 7.7)) - 0.5) * 0.6;
            float d = length(gf - off);
            sky += mix(uColor.rgb, vec3(0.85, 0.92, 1.0), 0.5)
                   * exp(-d * d * 90.0) * (sh - 0.88) * 9.0 * smoothstep(0.0, 0.25, rd.y);
        }
        col = mix(sky, col, alpha);
        alpha = 1.0;
    }

    fragColor = vec4(col * qt_Opacity, alpha * qt_Opacity);
}