#version 450 core

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec2 fragCoord;

layout(binding = 1) uniform sampler2D albumColorMap;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    // 自定义 Uniforms
    float time;
    vec2 resolution;
    vec2 albumColorMapRes;
};

layout(location = 0) out vec4 fragColor;

// 噪声算法
float rand(vec2 n) {
    return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 ip = floor(p);
    vec2 u = fract(p);
    u = u * u * (3.0 - 2.0 * u);

    float res = mix(
        mix(rand(ip), rand(ip + vec2(1.0, 0.0)), u.x),
        mix(rand(ip + vec2(0.0, 1.0)), rand(ip + vec2(1.0, 1.0)), u.x),
        u.y
    );
    return res * res;
}

const mat2 mtx = mat2(0.78, 0.63, -0.62, 0.77);

// 分数布朗运动
float fbm(vec2 p) {
    float f = 0.0;

    f += 0.046 * noise(p + time * 0.2) + time * 0.006;// 0.2 0.006
    p = mtx * p * 2.02;

    f += 0.032 * noise(p);
    p = mtx * p * 2.01;

    f += 0.170 * noise(p);
    p = mtx * p * 2.00;

    return f / 1.1;//0.969
}

float pattern(in vec2 p) {
    return fbm(p + fbm(p + fbm(p)));
}

// 噪声抖动
float gradientNoise(vec2 uv) {
    return fract(52.9829189 * fract(dot(uv, vec2(0.06711056, 0.00583715))));
}

void main() {
    // 归一化 UV
    vec2 uv = fragCoord / resolution.x;

    // 生成噪声值
    float shade = pattern(uv) * 4.0;

    vec4 color = texture(albumColorMap, vec2(fract(shade), 0.0));

    // --后处理效果
    vec2 center = vec2(0.5 * resolution.x / resolution.x, 0.5);
    // 暗角效果
    float dist = distance(uv, center);
    float vignette = 1.0 - smoothstep(0.3, 0.8, dist);
    color.rgb *= mix(0.6, 1.0, vignette);
    // 抖动效果
    float dither = (1.0 / 256.0) * gradientNoise(gl_FragCoord.xy) - (0.5 / 256.0);
    color.rgb += vec3(dither);

    //fragColor = vec4(color.rgb, 1.0) * qt_Opacity;
    fragColor = vec4(color.rgb, 1.0);
}