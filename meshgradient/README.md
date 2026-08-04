# quemusic_meshgradient

独立 Mesh Gradient 动态背景渲染组件，供 QueMusic 播放器使用。

## 许可证

**GNU Affero General Public License v3.0 (AGPL-3.0)**

本组件是 **AMLL (Apple Music Like Lyrics)** 的 Mesh Gradient 背景渲染器的
C++/Qt 移植与改写版本，衍生自以下 AGPL-3.0 原始实现：

- 原始项目：https://github.com/amll-dev/applemusic-like-lyrics
- 许可证全文：本目录 `LICENSE`（或官方 https://www.gnu.org/licenses/agpl-3.0.txt）

### 移植来源文件

| 本组件文件 | AMLL 原始文件 |
|------------|---------------|
| `MeshGradientItem.cpp` / `.h` | `src/renderer/mesh/MeshGradientRenderer.ts`、`bhpmesh.ts`、`cp-presets.ts`、`cp-generate.ts` |
| `shaders/meshgradient.vert` | `src/renderer/mesh/mesh.vert.glsl` |
| `shaders/meshgradient.frag` | `src/renderer/mesh/mesh.frag.glsl` |

### 主要修改内容

1. **语言/框架移植**：TypeScript + WebGL → C++17 + Qt Quick Scene Graph
   自定义材质（`QSGMaterial` + `QSGGeometryNode`），适配 Qt 6 RHI
   （D3D11 / OpenGL / Metal / Vulkan）。
2. **着色器改写**：GLSL 440（Vulkan 风格），uniform 改为 `std140` 布局块，
   经 `qsb` 编译为 `.qsb` 资源。
3. **封面处理**：网络/本地封面加载、32×32 缩放、对比度/饱和度/亮度调整、
   模糊全部以 Qt (`QImage` / `QtConcurrent`) 重写。
4. **动画驱动**：`QTimer`（16ms）替代 `requestAnimationFrame`。
5. **适配修复**：
   - 纹理 `Linear` 过滤 + `ClampToEdge` 环绕：修复放大马赛克与镜像渐变错乱；
   - 补充 `commitTextureOperations()` 确保纹理上传 GPU；
   - 移除对音频频谱的硬依赖，音量仅作可选平滑参数；
   - 控制点切线强度 0.55，避免 Hermite 过冲导致 patch 翻转。

## 构建

独立库 target：`quemusic_meshgradient`（STATIC）。

QueMusic 主项目通过 `add_subdirectory(meshgradient)` 引入并链接：

```cmake
add_subdirectory(meshgradient)
target_link_libraries(QueMusic PRIVATE quemusic_meshgradient)
```

着色器（`shaders/*.vert/.frag`）由主项目 `qt6_add_shaders` 编译为
`:/shaders/meshgradient/shaders/meshgradient.*.qsb` 资源。

## 使用

```qml
import MeshGradientItem 1.0

MeshGradientItem {
    anchors.fill: parent
    coverUrl: mainMedia.urlStr
    flowSpeed: 1.0
    animating: true
    subDivisions: 50
}
```
