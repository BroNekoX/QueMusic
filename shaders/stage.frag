#version 450 core

// 点云舞台：柔边精灵，输出 alpha=0 + 预乘颜色 ⇒ 预乘混合下等效加色发光
layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vCorner;
layout(location = 2) in float vSoft;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
};

layout(location = 0) out vec4 fragColor;

void main() {
    float d2 = dot(vCorner, vCorner);
    if (d2 > 1.0) discard;
    float d = sqrt(d2);
    // 硬核 + 极淡外晕；近处浮尘（vSoft=1）柔成散景，其它预设是干净的小亮点
    float core = 1.0 - smoothstep(mix(0.34, 0.00, vSoft), mix(0.47, 0.92, vSoft), d);
    float halo = pow(max(0.0, 1.0 - d), mix(5.0, 1.6, vSoft)) * mix(0.16, 0.55, vSoft);
    float a = (core * 1.25 + halo) * vColor.a;
    fragColor = vec4(vColor.rgb * a, 0.0) * qt_Opacity;
}
