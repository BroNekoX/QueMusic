// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2022-2024 AMLL Contributors
//
// 本着色器改写自 AMLL (Apple Music Like Lyrics) 的 mesh.frag.glsl：
//   https://github.com/amll-dev/applemusic-like-lyrics
//
// 主要修改内容：
//   - 语法升级为 GLSL 440（Vulkan 风格），供 qsb 编译为 Qt RHI 着色器
//   - uniform 改为 std140 布局块（qt_Matrix/qt_Opacity + 自定义 uniform）
//   - 采样器绑定号改为 binding = 1（binding 0 为 uniform 块）
//   - 移除原版的音量频谱 UV 扰动，保留旋转/暗角/抖动，由 Qt 端传入
//     u_sinAngle/u_cosAngle/u_volume/u_alpha/u_time
//
// 依据 GNU Affero General Public License v3.0 发布，全文见：
//   https://www.gnu.org/licenses/agpl-3.0.txt

#version 440

layout(location = 0) in vec3 v_color;
layout(location = 1) in vec2 v_uv;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D u_texture;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    // 自定义 Uniforms
    float u_aspect;
    float u_volume;
    float u_alpha;
    float u_sinAngle;
    float u_cosAngle;
    float u_time;
} ubuf;

// 预计算常量
const float INV_255 = 1.0 / 255.0;
const float HALF_INV_255 = 0.5 / 255.0;
const float GRADIENT_NOISE_A = 52.9829189;
const vec2 GRADIENT_NOISE_B = vec2(0.06711056, 0.00583715);

float gradientNoise(in vec2 uv) {
    return fract(GRADIENT_NOISE_A * fract(dot(uv, GRADIENT_NOISE_B)));
}

void main() {
    float volumeEffect = ubuf.u_volume * 2.0;

    float dither = INV_255 * gradientNoise(gl_FragCoord.xy) - HALF_INV_255;

    vec2 centeredUV = v_uv - vec2(0.5);

    vec2 rotatedUV = vec2(
        ubuf.u_cosAngle * centeredUV.x - ubuf.u_sinAngle * centeredUV.y,
        ubuf.u_sinAngle * centeredUV.x + ubuf.u_cosAngle * centeredUV.y
    );

    vec2 finalUV = rotatedUV * max(0.001, 1.0 - volumeEffect) + vec2(0.5);

    // UV 已贴几何（uvX/uvY 由顶点位置映射），方向由几何决定，无需手动翻转
    vec4 result = texture(u_texture, finalUV);

    float alphaVolumeFactor = ubuf.u_alpha * max(0.5, 1.0 - ubuf.u_volume * 0.5);
    result.rgb *= v_color * alphaVolumeFactor;
    result.a *= alphaVolumeFactor;

    result.rgb += vec3(dither);

    float dist = distance(v_uv, vec2(0.5));
    float vignette = smoothstep(0.8, 0.3, dist);
    float mask = 0.6 + vignette * 0.4;
    result.rgb *= mask;

    fragColor = result * ubuf.qt_Opacity;
}
