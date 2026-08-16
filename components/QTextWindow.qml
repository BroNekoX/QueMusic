import QtQuick
import QueMusic 1.0

Window {
    id: root
    visible: textWatch.active
    width: 800
    height: 600
    onClosing: {
        textWatch.active = false;
    }

    QScrollView {
        anchors.fill: parent
    // 使用 Text 或 TextEdit 来显示 Markdown
    contentChildren: TextEdit {
        id: markdownViewer
        width: root.width
        textFormat: TextEdit.MarkdownText // 关键：设置为 Markdown 格式[reference:4]

        // 从资源文件中读取 Markdown 内容
        /*Component.onCompleted: {
            var file = new XMLHttpRequest();
            file.open("GET", "qrc:/QueMusic/README.md", true); // 同步加载资源文件
            file.onreadystatechange = function() {
                if (file.readyState === XMLHttpRequest.DONE) {
                    if (file.status === 200) {
                        text = file.responseText;
                    } else {
                        console.error("Error:", file.statusText);
                    }
                }
            };
            file.send();
        }*/

        // 设置为只读，更像一个阅读器
        readOnly: true
        wrapMode: TextEdit.Wrap

        text: textWatch.info ? '# 🎵 QueMusic Project


***基于 C++/Qt/QML 构建的强大高性能跨平台音乐播放器

> **可能是桌面跨平台上UI最美丽丝滑，性能最强的开源音乐播放器？**
> **基于 Qt 6.9 / QML 与 GPU 加速 RHI 渲染的开源跨平台音乐播放器，支持接入网易云、酷狗等平台的公开音乐服务接口。**
> 动效美丽，性能出众，开发者坚持 **永久免费 & 开源**。
>
> 🚧 项目正处于 **开发/预览阶段**，部分功能尚未完善, 仍存在部分问题，有一些功能无法使用，会持续更新，欢迎 Star & Fork 一起参与！
> 快速下载本应用及历史版本：[123网盘快速下载](https://1816090463.share.123pan.cn/123pan/0HQ5Vv-jfjld)

---

> [!WARNING]
>
> 1.本项目仅供用户学习与研究使用，禁止将本项目用于任何非法用途。
>
> 2.本项目开发者不接受任何形式的赞助，打赏，捐赠行为，禁止任何用户向本开发者赞助，打赏，捐赠。
>
> 3.本项目的使用者出现的任何侵权、盗用、版权问题等违规情况，与本项目无关。
>
> 4.本项目并不提供公共云端曲库与媒体分发服务，在线音频获取的能力均使用第三方平台个人账号授权获取，付费内容，会员内容，受限制的内容请遵循第三方平台版权。
>
> 5.如果音乐平台发现本项目包含侵权或有问题的行为，可联系开发者进行更改或移除。
>
> 6.本项目使用了一些第三方模块，如果你认为本项目违反了部分协议，可联系开发者进行更改或移除。

---

## ✨ 核心特性

| 维度 | 亮点 |
|------|------|
| **🎶 多平台音乐** | 支持网易云、酷狗等平台公开接口接入（仅访问公开内容，详见[免责声明](#免责声明)） |
| **⚡ 性能出众** | C++ 核心模块 + QML RHI 场景渲染，内存占用低，核显 / 老旧 CPU 依然流畅 |
| **🎨 精美 UI** | 高级毛玻璃圆角卡片、可自定义主题色 & 界面样式，自研 Theme / 配色系统 |
| **🔄 流畅动画** | 自定义贝塞尔曲线动画，歌词界面丝滑，QML Animation 全局稳定 60fps |
| **📦 功能丰富** | 歌词滚动 / 桌面歌词、10 段均衡器、歌单管理、搜索推荐、收藏同步 |
| **🛡️ 可靠性高** | 自制 JS API 管理层，统一错误处理，速度快且持续优化 |
| **💻 跨平台** | 全面支持 **Windows / macOS / Linux** 三大桌面端 |

---

## 🛠️ 技术栈

| 类别 | 技术 |
|------|------|
| **框架** | Qt 6.9.3 Community |
| **构建** | CMake ≥ 3.24 / Ninja |
| **语言** | C++17 / JavaScript / QML |
| **音频** | Qt Multimedia (FFmpeg7.1.1 后端) |
| **渲染** | QtRHI — 基于平台原生GPU渲染器 |
| **数据库** | Qt SQL / SQLite |
| **工具链** | MSVC 2022 / GCC 13+ / MinGW 13+ |

---

## 📥 安装与运行

### 前置条件

- Qt **6.9+**（含 Qt Multimedia, Qt SQL, Qt ShaderTools等基础Qt库）
- CMake ≥ **3.24**
- 编译器：MSVC 2022 / GCC 13+ / MinGW 13+

### 克隆（含子模块）

```bash
git clone --recurse-submodules https://github.com/BroNekoX/QueMusic.git
cd QueMusic
```

> ⚠️ **重要**：本项目使用 QWindowKit 作为 git 子模块，务必加上 `--recurse-submodules`。
> 如果已经 clone 但忘记拉子模块，运行：
>
> ```bash
> git submodule update --init --recursive
> ```

### Windows 构建

```bash
# 方式一：命令行
cmake -B build -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.x/mingw_64
cmake --build build --parallel
./build/bin/QueMusic

# 方式二：Qt Creator
# 本项目就是QueMusic的源代码，直接用 Qt Creator 打开项目根目录的 CMakeLists.txt，配置后运行即可
```

> 💡 **提示**：推荐使用 **Qt Creator** 打开本项目，配置、编译、调试一步到位。

### Linux 构建

```bash
# 方式一：AppImage 一键打包（推荐，自包含 Qt 6.9.3）
bash packaging/build-linux.sh
# 产物：QueMusic-x86_64.AppImage

# 方式二：直接构建（使用系统 Qt 或已安装的 Qt 6.9+）
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=~/Qt/6.9.3/gcc_64
cmake --build build -j"$(nproc)"
./build/bin/QueMusic

#方式三：使用KDevelop或Qt Creator
#使用KDevelop或Qt Creator都可以进行快速构建和测试

# Arch Linux 用户也可以用 PKGBUILD 打包：
# cd packaging && makepkg -si
```

### MacOS 构建

本项目内置 GitHub Actions 工作流 `.github/workflows/build-macos.yml`，在 GitHub 的 macOS 虚拟机（Apple Silicon + Intel）上自动构建 `.dmg`：

```bash
# 方式一：手动触发
# 仓库页面 → Actions → Build macOS → Run workflow → 下载 Artifacts 里的 .dmg

# 方式二：打 tag 自动构建并挂到 Release
git tag v0.1.0
git push origin v0.1.0
```

> 📦 三种平台的可执行安装包都会随 [Release](https://github.com/BroNekoX/QueMusic/releases) 发布。

---

## 📁 项目结构

```
QueMusic/
├── CMakeLists.txt              # 顶层构建配置
├── cmake/                      # CMake 模块
│   ├── external/qwindowkit.cmake # QWindowKit 子模块集成
│   └── qtruntime.cmake
├── main.cpp                    # C++ 程序入口
├── main.qml                    # QML 主入口
├── cpp/                        # C++ 后端模块
│   ├── CoverHelper.cpp/h       # 封面图片处理
│   ├── ColorExtractor.cpp/h    # 颜色提取（自适应主题色）
│   ├── GetWave.cpp/h           # 音频波形数据
│   ├── FolderModel.cpp/h       # 本地文件夹模型
│   ├── DownloadManager.cpp/h   # 下载管理器
│   └── Favorites.cpp/h         # 收藏管理
├── meshgradient/               # 🧩 独立 Mesh Gradient 背景组件（AGPL-3.0）
│   ├── CMakeLists.txt          # 独立库 target：quemusic_meshgradient
│   ├── LICENSE                 # GNU AGPL v3.0 全文
│   ├── README.md               # 组件说明 / 来源 / 修改记录
│   ├── MeshGradientItem.cpp/h  # 网格渐变渲染（衍生自 AMLL）
│   └── shaders/                # meshgradient.vert/.frag（衍生自 AMLL）
├── api/                        # JavaScript API 层
│   ├── QCloudMusicApi/         # 存放QCloudMusicApi第三方项目
│   ├── MusicApiService.cpp/h   # 在线音乐 API总部
│   ├── KugouApi.cpp/h          # 酷狗音乐 API
│   ├── NeteaseApi.cpp/h        # 网易云音乐 API
│   └── OnlinelistModel.cpp/h   # 在线api的列表模型自定义组件
├── components/                 # QML 组件库（自研 UI 库）
│   ├── Q***.qml                # 各自控件，QueMusic由它们组成
│   ├── MusicApi.qml            # 在线音乐整合单例
│   ├── Style.qml               # 全局单例样式
│   ├── Options.qml             # 设置文件
│   └── ...
├── layout/                     # 页面布局
│   ├── LeftSideBar.qml         # 左侧导航栏
│   ├── MainContent.qml         # 主内容区
│   ├── PlayerControl.qml       # 播放控制栏
│   └── PlayerMaxCenter.qml     # 全屏 / 最大化歌词中心
├── pages/                      # 页面
│   ├── HomePage.qml            # 首页 / 推荐
│   ├── SearchPage.qml          # 搜索
│   ├── PlaylistPage.qml        # 歌单详情
│   ├── FavouritePage.qml       # 收藏
│   ├── FilePage.qml            # 本地文件
│   └── DownloadPage.qml        # 下载管理
├── resources/                  # 资源文件
│   ├── app/                    # 应用图标、图片
│   ├── fonts/                  # 字体（Poppins, Feather Icons）
│   ├── window-bar/             # 窗口按钮图标
│   ├── pic/                    # 背景图片
│   └── app/shaders/            # GLSL 着色器
├── ThirdParty/
│   └── qwindowkit/             # Git Submodule — 无边框窗口框架
├── .gitignore
├── .gitattributes
├── .gitmodules
├── LICENSE                     # Apache License 2.0
└── README.md
```

---

## 🎮 未来计划

- **首要-功能完善**：补全设置、编辑、歌单管理等功能，增强稳定性
- **品牌统一性**: 在名称以及宣传上计划使用一个新的名称或定义，统一形象
- **推动发展**: 后面计划推出QueMusic网站，建立QQ群，与社区共建生态
- **优化性能**：持续优化内存 & GPU 占用，解决性能瓶颈
- **加入沉浸播放**：参考 Folia / MineRadio 概念，引入 3D 可视化与高度自定义歌词
- **UI 强化**：继续打磨自研 QML 组件库，统一设计语言
- **国际化**：可选计划，由于使用国内音乐平台，不一定更新
- **更多**：自定义插件系统,自定义主题UI插件系统

即使不断更新，QueMusic开发者始终保持开源，免费，没有付费内容，保持完全的免费，但是对于歌曲版权方面，请自费购买平台VIP或付费歌曲，登录平台账号进行收听（即将更新）。

---

## 🤝 贡献指南

欢迎任何形式的贡献！

| 方式 | 说明 |
|------|------|
| 🐛 **报告 Bug** | 提交 [Issue](https://github.com/bronekox/quemusic/issues)，附上复现步骤和环境 |
| 💡 **提出新功能** | 在 [Discussion](https://github.com/bronekox/quemusic/discussions) 中发起讨论 |
| ⭐ **Star** | 点亮 GitHub Star，支持持续开发 |
| 🧪 **测试** | 构建并试用，反馈兼容性问题 |
| 🔧 **Pull Request** | 修复 Bug、优化代码、完善功能 —— **欢迎任何人** |
| 📖 **利用** | 基于Apache-2.0 协议，开发者欢迎任何项目使用本项目的代码 |

### 开发流程

1. Fork 本仓库
2. 创建功能分支：`git checkout -b feat/your-feature`
3. 提交修改：`git commit -m "feat: add xxx"`
4. 推送：`git push origin feat/your-feature`
5. 发起 Pull Request

> 代码风格请参考现有文件，遵循 **C++17 / Qt6 / QML best practices**。

---

## 📄 许可证

本项目主体遵循 **Apache License 2.0** —— 欢迎自由使用、修改、分发，甚至商用（需保留版权声明与许可证副本）。

```
Apache License
Version 2.0, January 2004
Copyright (c) 2025-2026 QueMusic Contributors
```

> 💡 **Apache-2.0 要点**：允许商用、修改、分发；需在衍生作品中保留原始版权声明与 NOTICE；对专利授权有明确条款，为用户提供额外保护。

### 🧩 第三方组件：Mesh Gradient 背景（AGPL-3.0）

本项目中的 **`meshgradient/` 独立组件**（动态流体渐变背景）衍生自
[AMLL Core(Apple Music Like Lyrics)](https://github.com/amll-dev/applemusic-like-lyrics)，
以 **GNU Affero General Public License v3.0** 单独授权：

- 该组件作为**独立库**（`quemusic_meshgradient`）编译，与主体保持"聚合"关系；
- 组件许可证不影响 QueMusic 其余 Apache-2.0 代码；
- 来源文件、修改内容详见 [`meshgradient/README.md`](meshgradient/README.md)；
- 全部第三方组件声明见 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。

---

## 📢 免责声明

### 1. 音乐版权

本项目中的所有音乐内容（歌曲、歌词、封面等）版权均归其原始权利人所有。本项目**不提供、不存储、不缓存**任何音乐文件，所有播放内容均来自用户自行选择的第三方公开网络服务。

### 2. 在线服务接口

- 本项目仅调用各音乐平台对外**公开**的接口，**不包含任何破解、绕过付费、解锁 VIP、盗取音源等行为**；
- 不提供任何付费内容的非法获取途径，也无法播放需要单独授权的加密内容；
- 各平台接口可能随时调整或失效，本项目不对接口的可用性与稳定性作任何保证。

### 3. 商标与品牌

本项目中出现的所有商标、产品名称、服务名称均为其各自所有者的财产，仅用于描述兼容性，不代表任何官方授权、认可或关联。

### 4. 使用者责任

使用者应遵守所在地法律法规以及各第三方平台的服务条款。因使用本项目而产生的任何直接或间接后果，由使用者自行承担，项目开发者不承担任何责任。

### 5. 无担保

本项目按 **"现状"（AS-IS）** 提供，不附带任何明示或暗示的担保。详细免责条款请参阅 [LICENSE](LICENSE) 文件。

---

## 🙏 致谢

- [Qt Project](https://www.qt.io/) — 提供强大的跨平台框架
- [QWindowKit](https://github.com/stdware/qwindowkit) — 无边框窗口解决方案
- [AMLL Core(Apple Music Like Lyrics)](https://github.com/amll-dev/applemusic-like-lyrics) — 背景着色器的算法（AGPL-3.0，见 `meshgradient/` 组件）
- [qiuliw/Qt6_QWindowKit_QML_demo](https://github.com/qiuliw/Qt6_QWindowKit_QML_demo) — 项目框架参考
- [QCloudMusicApi](https://github.com/s12mmm3/QCloudMusicApi) — 使用了本项目api服务，以实现在线音乐网易云音乐平台部分
- [Cryptopp](https://github.com/weidai11/cryptopp) — 用于QCloudMusicApi解析
- [libqrencode](https://github.com/weidai11/cryptopp) — 用于QCloudMusicApi
- 以下虽然可能没有使用到他们的代码，但是我仍然致谢他们所带来的精神。
- [EvolveUI](https://evolveui.top/) — 部分组件设计参考
- [ShaderToy](https://www.shadertoy.com/) — 着色器灵感来源
- 所有贡献者与测试者

---

## 联系开发者

- QQ：241422517
- 邮箱：uihugd@outlook.com
- Bilibili: 695207057

---

<p align="center">
  <sub>Built with ❤️ by the QueMusic Project</sub><br/>
  <sub>最后更新：2026-8-14</sub>
</p>
            ' : '                QueMusic 使用 Apache License 2.0授权开源
                                 Apache License
                           Version 2.0, January 2004
                        http://www.apache.org/licenses/

   TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

   1. Definitions.

      "License" shall mean the terms and conditions for use, reproduction,
      and distribution as defined by Sections 1 through 9 of this document.

      "Licensor" shall mean the copyright owner or entity authorized by
      the copyright owner that is granting the License.

      "Legal Entity" shall mean the union of the acting entity and all
      other entities that control, are controlled by, or are under common
      control with that entity. For the purposes of this definition,
      "control" means (i) the power, direct or indirect, to cause the
      direction or management of such entity, whether by contract or
      otherwise, or (ii) ownership of fifty percent (50%) or more of the
      outstanding shares, or (iii) beneficial ownership of such entity.

      "You" (or "Your") shall mean an individual or Legal Entity
      exercising permissions granted by this License.

      "Source" form shall mean the preferred form for making modifications,
      including but not limited to software source code, documentation
      source, and configuration files.

      "Object" form shall mean any form resulting from mechanical
      transformation or translation of a Source form, including but
      not limited to compiled object code, generated documentation,
      and conversions to other media types.

      "Work" shall mean the work of authorship, whether in Source or
      Object form, made available under the License, as indicated by a
      copyright notice that is included in or attached to the work
      (an example is provided in the Appendix below).

      "Derivative Works" shall mean any work, whether in Source or Object
      form, that is based on (or derived from) the Work and for which the
      editorial revisions, annotations, elaborations, or other modifications
      represent, as a whole, an original work of authorship. For the purposes
      of this License, Derivative Works shall not include works that remain
      separable from, or merely link (or bind by name) to the interfaces of,
      the Work and Derivative Works thereof.

      "Contribution" shall mean any work of authorship, including
      the original version of the Work and any modifications or additions
      to that Work or Derivative Works thereof, that is intentionally
      submitted to Licensor for inclusion in the Work by the copyright owner
      or by an individual or Legal Entity authorized to submit on behalf of
      the copyright owner. For the purposes of this definition, "submitted"
      means any form of electronic, verbal, or written communication sent
      to the Licensor or its representatives, including but not limited to
      communication on electronic mailing lists, source code control systems,
      and issue tracking systems that are managed by, or on behalf of, the
      Licensor for the purpose of discussing and improving the Work, but
      excluding communication that is conspicuously marked or otherwise
      designated in writing by the copyright owner as "Not a Contribution."

      "Contributor" shall mean Licensor and any individual or Legal Entity
      on behalf of whom a Contribution has been received by Licensor and
      subsequently incorporated within the Work.

   2. Grant of Copyright License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      copyright license to reproduce, prepare Derivative Works of,
      publicly display, publicly perform, sublicense, and distribute the
      Work and such Derivative Works in Source or Object form.

   3. Grant of Patent License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      (except as stated in this section) patent license to make, have made,
      use, offer to sell, sell, import, and otherwise transfer the Work,
      where such license applies only to those patent claims licensable
      by such Contributor that are necessarily infringed by their
      Contribution(s) alone or by combination of their Contribution(s)
      with the Work to which such Contribution(s) was submitted. If You
      institute patent litigation against any entity (including a
      cross-claim or counterclaim in a lawsuit) alleging that the Work
      or a Contribution incorporated within the Work constitutes direct
      or contributory patent infringement, then any patent licenses
      granted to You under this License for that Work shall terminate
      as of the date such litigation is filed.

   4. Redistribution. You may reproduce and distribute copies of the
      Work or Derivative Works thereof in any medium, with or without
      modifications, and in Source or Object form, provided that You
      meet the following conditions:

      (a) You must give any other recipients of the Work or
          Derivative Works a copy of this License; and

      (b) You must cause any modified files to carry prominent notices
          stating that You changed the files; and

      (c) You must retain, in the Source form of any Derivative Works
          that You distribute, all copyright, patent, trademark, and
          attribution notices from the Source form of the Work,
          excluding those notices that do not pertain to any part of
          the Derivative Works; and

      (d) If the Work includes a "NOTICE" text file as part of its
          distribution, then any Derivative Works that You distribute must
          include a readable copy of the attribution notices contained
          within such NOTICE file, excluding those notices that do not
          pertain to any part of the Derivative Works, in at least one
          of the following places: within a NOTICE text file distributed
          as part of the Derivative Works; within the Source form or
          documentation, if provided along with the Derivative Works; or,
          within a display generated by the Derivative Works, if and
          wherever such third-party notices normally appear. The contents
          of the NOTICE file are for informational purposes only and
          do not modify the License. You may add Your own attribution
          notices within Derivative Works that You distribute, alongside
          or as an addendum to the NOTICE text from the Work, provided
          that such additional attribution notices cannot be construed
          as modifying the License.

      You may add Your own copyright statement to Your modifications and
      may provide additional or different license terms and conditions
      for use, reproduction, or distribution of Your modifications, or
      for any such Derivative Works as a whole, provided Your use,
      reproduction, and distribution of the Work otherwise complies with
      the conditions stated in this License.

   5. Submission of Contributions. Unless You explicitly state otherwise,
      any Contribution intentionally submitted for inclusion in the Work
      by You to the Licensor shall be under the terms and conditions of
      this License, without any additional terms or conditions.
      Notwithstanding the above, nothing herein shall supersede or modify
      the terms of any separate license agreement you may have executed
      with Licensor regarding such Contributions.

   6. Trademarks. This License does not grant permission to use the trade
      names, trademarks, service marks, or product names of the Licensor,
      except as required for reasonable and customary use in describing the
      origin of the Work and reproducing the content of the NOTICE file.

   7. Disclaimer of Warranty. Unless required by applicable law or
      agreed to in writing, Licensor provides the Work (and each
      Contributor provides its Contributions) on an "AS IS" BASIS,
      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
      implied, including, without limitation, any warranties or conditions
      of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
      PARTICULAR PURPOSE. You are solely responsible for determining the
      appropriateness of using or redistributing the Work and assume any
      risks associated with Your exercise of permissions under this License.

   8. Limitation of Liability. In no event and under no legal theory,
      whether in tort (including negligence), contract, or otherwise,
      unless required by applicable law (such as deliberate and grossly
      negligent acts) or agreed to in writing, shall any Contributor be
      liable to You for damages, including any direct, indirect, special,
      incidental, or consequential damages of any character arising as a
      result of this License or out of the use or inability to use the
      Work (including but not limited to damages for loss of goodwill,
      work stoppage, computer failure or malfunction, or any and all
      other commercial damages or losses), even if such Contributor
      has been advised of the possibility of such damages.

   9. Accepting Warranty or Additional Liability. While redistributing
      the Work or Derivative Works thereof, You may choose to offer,
      and charge a fee for, acceptance of support, warranty, indemnity,
      or other liability obligations and/or rights consistent with this
      License. However, in accepting such obligations, You may act only
      on Your own behalf and on Your sole responsibility, not on behalf
      of any other Contributor, and only if You agree to indemnify,
      defend, and hold each Contributor harmless for any liability
      incurred by, or claims asserted against, such Contributor by reason
      of your accepting any such warranty or additional liability.

   END OF TERMS AND CONDITIONS

   APPENDIX: How to apply the Apache License to your work.

      To apply the Apache License to your work, attach the following
      boilerplate notice, with the fields enclosed by brackets "[]"
      replaced with your own identifying information. (Do not include
      the brackets!)  The text should be enclosed in the appropriate
      comment syntax for the file format. We also recommend that a
      file or class name and description of purpose be included on the
      same "printed page" as the copyright notice for easier
      identification within third-party archives.

   Copyright 2024-2026 QueMusic Contributors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.'

    }
    }
}