#version 450 core

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float u_time;
    vec2 u_resolution;
    vec4 u_colors[4];
};

mat2 rotate2d(float a)
{
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

vec2 getPosition(int i, float t)
{
    float a = float(i) * 0.37;
    float b = 0.6 + fract(float(i) / 3.0) * 0.9;
    float c = 0.8 + fract(float(i + 1) / 4.0);
    return 0.5 + 0.5 * vec2(sin(t * b + a), cos(t * c + a * 1.5));
}

void main()
{
    // 官方默认：distortion 0.8 / swirl 0.1
    const float distortion = 0.8;
    const float swirl = 0.1;

    float aspect = u_resolution.x / max(u_resolution.y, 1.0);
    vec2 mid = vec2(0.5 * aspect, 0.5);
    vec2 uv = vec2(vUV.x * aspect, vUV.y);
    float t = 0.2 * (u_time + 41.5);

    float radius = smoothstep(0.0, 1.0, length(uv - mid));
    float center = 1.0 - radius;
    for (float i = 1.0; i <= 2.0; i++) {
        uv.x += distortion * center / i * sin(t + i * 0.4 * smoothstep(0.0, 1.0, uv.y))
                * cos(0.2 * t + i * 2.4 * smoothstep(0.0, 1.0, uv.y));
        uv.y += distortion * center / i * cos(t + i * 2.0 * smoothstep(0.0, 1.0, uv.x));
    }

    vec2 rotated = rotate2d(-3.0 * swirl * radius) * (uv - mid) + mid;

    // 8 个色斑：4 色各占两条独立轨迹，色块更小更碎
    vec3 color = vec3(0.0);
    float totalWeight = 0.0;
    for (int i = 0; i < 8; i++) {
        vec2 pos = getPosition(i < 4 ? i : i + 4, t);
        pos.x *= aspect;
        float weight = 1.0 / (pow(length(rotated - pos), 4.0) + 1e-3);
        color += u_colors[i % 4].rgb * u_colors[i % 4].a * weight;
        totalWeight += weight;
    }

    // 暗角
    const float dist = distance(vUV, vec2(0.5));
    color *= 0.65 + 0.35 * smoothstep(0.85, 0.25, dist);

    fragColor = vec4(color / max(totalWeight, 1e-4), 1.0) * qt_Opacity;
}
