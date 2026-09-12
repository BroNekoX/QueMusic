#version 450 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec2 vUV;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float u_time;
    vec2 u_resolution;
    vec4 u_colors[4];
};

void main()
{
    vUV = aTexCoord;
    gl_Position = qt_Matrix * vec4(aPosition, 0.0, 1.0);
}
