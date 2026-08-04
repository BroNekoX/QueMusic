// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2022-2024 AMLL Contributors
//
// 本着色器改写自 AMLL (Apple Music Like Lyrics) 的 mesh.vert.glsl：
//   https://github.com/amll-dev/applemusic-like-lyrics
//
// 主要修改内容：
//   - 语法升级为 GLSL 440（Vulkan 风格），供 qsb 编译为 Qt RHI 着色器
//   - uniform 改为 std140 布局块（qt_Matrix/qt_Opacity + 自定义 uniform）
//   - 顶点坐标由 C++ 端预转为 item 本地坐标，经 qt_Matrix 映射到裁剪空间
//
// 依据 GNU Affero General Public License v3.0 发布，全文见：
//   https://www.gnu.org/licenses/agpl-3.0.txt

#version 440

layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec3 a_color;
layout(location = 2) in vec2 a_uv;

layout(location = 0) out vec3 v_color;
layout(location = 1) out vec2 v_uv;

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

void main() {
    v_color = a_color;
    v_uv = a_uv;
    // 几何顶点使用 item 本地坐标（由 C++ 端把 NDC 转换而来），
    // 乘上场景图矩阵 qt_Matrix 映射到裁剪空间（自动处理窗口宽高比/DPR/位移）
    gl_Position = ubuf.qt_Matrix * vec4(a_pos, 0.0, 1.0);
}
