#version 450 core

// 点云舞台：圆形柔边精灵，输出 alpha=0 + 预乘颜色 ⇒ Qt 的预乘混合下等效"加色发光"
layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vCorner;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
};

layout(location = 0) out vec4 fragColor;

void main() {
    float d2 = dot(vCorner, vCorner);
    if (d2 > 1.0) discard;
    float d = sqrt(d2);
    // 硬核（接近实心的小圆盘，边缘快速收口 ⇒ 清晰）+ 极淡外晕（只提供一点体积感）
    float core = 1.0 - smoothstep(0.28, 0.42, d);
    float halo = pow(max(0.0, 1.0 - d), 3.5) * 0.28;
    float a = (core + halo) * vColor.a;
    fragColor = vec4(vColor.rgb * a, 0.0) * qt_Opacity;
}
