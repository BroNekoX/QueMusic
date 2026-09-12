#version 450 core

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D albumMap;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float u_time;
    vec2 u_resolution;
    vec4 u_colors[4];
};

void main()
{
    // 封面源缓慢旋转
    vec2 c = vUV - vec2(0.35);
    const float ang = -u_time * 0.24;
    vec2 uv = vec2(c.x * cos(ang) - c.y * sin(ang),
                   c.x * sin(ang) + c.y * cos(ang)) + vec2(0.5);

    vec4 color = texture(albumMap, uv);

    // 暗角
    const float dist = distance(vUV, vec2(0.5));
    color.rgb *= 0.65 + 0.35 * smoothstep(0.85, 0.25, dist);

    // 抖动消色带
    const float dither = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))))
                         / 255.0 - 0.5 / 255.0;
    color.rgb += dither;

    fragColor = color * qt_Opacity;
}
