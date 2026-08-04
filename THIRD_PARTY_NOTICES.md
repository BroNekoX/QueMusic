# 第三方组件声明 / Third-Party Notices

本文件记录 QueMusic 项目中使用的第三方开源组件及其许可信息。
The following third-party components are used in QueMusic.

QueMusic 主体代码以 **Apache License 2.0** 授权（见根目录 `LICENSE`）。
以下标记为 AGPL-3.0 的组件为独立衍生作品，依据各自许可证另行授权，
**不改变** QueMusic 其余 Apache-2.0 代码的授权方式。

---

## 1. AMLL — Apple Music Like Lyrics (Mesh Gradient 背景渲染器)

| 项目 | 值 |
|------|-----|
| 组件名称 | AMLL Mesh Gradient 背景渲染器（Bicubic Hermite Patch Mesh） |
| 原始项目 | [amll-dev/applemusic-like-lyrics](https://github.com/amll-dev/applemusic-like-lyrics)（原 SteveXMH/applemusic-like-lyrics） |
| 作者 | SteveXMH / AMLL Contributors |
| 开源协议 | **GNU Affero General Public License v3.0**（AGPL-3.0） |
| 许可证全文 | `LICENSES/AGPL-3.0-only.txt`，官方文本：https://www.gnu.org/licenses/agpl-3.0.txt |
| 使用范围 | 仅限 QueMusic 的 Mesh Gradient 动态背景组件 |

### 移植来源文件

QueMusic 中以下文件衍生自 AMLL 的对应源文件，它们位于**独立组件目录
`meshgradient/`**（独立库 target `quemusic_meshgradient`，AGPL-3.0 单独授权）：

| QueMusic 文件 | AMLL 原始文件 |
|---------------|---------------|
| `meshgradient/MeshGradientItem.cpp` / `.h` | `src/renderer/mesh/MeshGradientRenderer.ts`、`bhpmesh.ts`、`cp-presets.ts`、`cp-generate.ts` |
| `meshgradient/shaders/meshgradient.vert` | `src/renderer/mesh/mesh.vert.glsl` |
| `meshgradient/shaders/meshgradient.frag` | `src/renderer/mesh/mesh.frag.glsl` |

### 独立组件结构

`meshgradient/` 目录是**独立编译单元**（`add_subdirectory` 引入的独立
STATIC 库），与 QueMusic 主体（Apache-2.0）保持"聚合（mere aggregation）"
关系：

```
meshgradient/
├── CMakeLists.txt       # 独立库 target：quemusic_meshgradient
├── LICENSE              # AGPL-3.0 全文
├── README.md            # 组件说明 / 来源 / 修改记录
├── MeshGradientItem.h
├── MeshGradientItem.cpp
└── shaders/
    ├── meshgradient.vert
    └── meshgradient.frag
```

该组件由 QueMusic 主体链接使用，但许可证独立：**组件本身 AGPL-3.0，
QueMusic 其余代码 Apache-2.0**，二者互不影响。

### 主要修改内容

1. **语言/框架移植**：将 TypeScript + WebGL 实现移植为 C++17 + Qt Quick Scene Graph 自定义材质（`QSGMaterial` + `QSGGeometryNode`），供 Qt 6 RHI 渲染（D3D11 / OpenGL / Metal / Vulkan）。
2. **着色器改写**：GLSL 语法升级为 440（Vulkan 风格），uniform 改为 `std140` 布局块，经 `qsb` 编译为 `.qsb` 资源。
3. **封面处理**：网络/本地封面加载、32×32 缩放、对比度/饱和度/亮度调整与模糊全部以 Qt (`QImage` / `QtConcurrent`) 重写。
4. **动画驱动**：以 `QTimer`（16ms）驱动动画循环替代 Web 的 `requestAnimationFrame`。
5. **适配修改**：
   - 纹理采用 `Linear` 过滤 + `ClampToEdge` 环绕，修复放大马赛克与镜像渐变方向错乱；
   - 补充 `commitTextureOperations()` 确保纹理上传 GPU；
   - 移除对音频频谱（volume/spectrumData）的硬依赖，音量仅作可选平滑参数；
   - 控制点切线强度调为 0.55，避免 Hermite 过冲导致 patch 翻转。

### 合规说明

依据 AGPL-3.0 第 4/5 节要求：
- 以上衍生文件的 SPDX 头已标注为 `AGPL-3.0-only` 并保留原版权声明；
- 修改内容已在各文件头部显著说明；
- 本组件源码随 QueMusic 一同分发，用户可获取完整对应源代码。

---

## 其他组件

（如后续引入其他第三方组件，请在此处补充声明。）

- **QWindowKit**：窗口标题栏 / 系统集成组件，遵循其自身许可证（见其项目文档）。
- **图标字体**（`feather.ttf`、`poppins.ttf` 等）：仅作资源使用，版权归原作者所有。
