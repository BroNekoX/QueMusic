# 贡献指南（CONTRIBUTING）

感谢愿意一起把 QueMusic 做得更好。这份文档只讲**怎么让改动被顺利合进来**，读完能省下几轮来回。

## 1. 提 Issue 之前

- **先搜一遍**已有 issue（含已关闭的），避免重复。
- 用模板提：Bug 用 [`bug.yml`](.github/ISSUE_TEMPLATE/bug.yml)，崩溃用 [`crash.yml`](.github/ISSUE_TEMPLATE/crash.yml)。
  模板里的「版本 / 系统 / 复现步骤 / 日志」请尽量填全 —— 缺日志的 bug 基本只能靠猜。
- 日志位置：设置 → Debug → 打开日志目录（或安装目录下的 `logs/`）。
- **在线接口相关的问题**（网易云登录风控、某个平台取不到数据）请注明平台与账号状态：
  上游收紧时我们只能降级，不能保证一定能拿到数据。

## 2. 开发环境

| 项目 | 要求 |
| --- | --- |
| Qt | **6.10.3**（Core / Gui / Quick / Qml / Network / Multimedia / Sql / Concurrent / ShaderTools） |
| 编译器 | MSVC 2022 / GCC 13+ / LLVM-MinGW 17+ |
| CMake | ≥ 3.24（内置预设），推荐 Ninja |
| 额外依赖 | 子模块 **QWindowKit**、**TagLib**；FFmpeg 运行库（`cmake/fetch_ffmpeg.ps1` 可自动获取） |

```bash
# 克隆（务必带子模块，否则窗口框架与标签库都缺）
git clone --recursive https://github.com/BroNekoX/QueMusic.git
cd QueMusic

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<你的 Qt 路径>
cmake --build build -j
```

## 3. 提交前的自检（务必跑）

CI 会跑**完全一样**的检查，本地先跑一遍能省一次红：

```bash
# 1) QML 静态检查（属性名写错、类型不存在都能查出来）
<Qt>/bin/qmllint -I build $(find . -name '*.qml' -not -path './build/*' -not -path './ThirdParty/*')

# 2) 启动冒烟：离屏跑一次应用，出现 QML / 绑定 / 着色器错误即失败
tests/smoke-stderr.sh build/bin/QueMusic 12
```

> 改了 `.frag` / `.vert` 着色器必须重新编译成 `.qsb`（Qt 6 不做运行时编译），
> 否则界面上效果会**静默消失**。插件侧写法见 [QuePlugins 规范](https://github.com/bronekox/queplugins)。

## 4. 代码约定

### QML

- 组件放 `components/`（可复用的基础件）、页面放 `pages/`、布局壳放 `layout/`、歌词界面放 `lyricsui/`；
  跨模块共享的单例放 `components/` 并用 `pragma Singleton`。
- 业务逻辑不要堆进 `main.qml`；新页面/新单例各自成文件。
- 注释只写**为什么**（约束、踩过的坑），不写「这行在给 x 赋值」。
- 面向 QML 导出的单例命名与 `QML_NAMED_ELEMENT` 保持一致，避免两套名字。

### C++

- 每个文件带 SPDX 头：`// SPDX-License-Identifier: Apache-2.0`。
- 暴露给 QML 的类型用 `QML_ELEMENT` + 单例工厂 `create()` 的既有范式（见 `cpp/plugins/LyricsPluginStore.h`）。
- 构造函数**不要**给 QObject 参数默认值（会让 QML 单例走默认构造分支，实例化逻辑不执行）。
- 耗时操作不要放在 GUI 线程；网络请求统一走 `api/ApiHttp.h`（带超时）。

### 提交信息

写清「改了什么 + 为什么」，例如 `修复: WebDAV 目录列表重复请求（加 30s 缓存）`。
避免 `优化代码`、`等其他优化` 这类无法检索的信息。

## 5. Pull Request

1. 从 `develop`（若尚未启用则从 `main`）开分支，一个 PR 只做一件事。
2. PR 描述里回答三件事：
   - **动机**：解决什么问题 / 哪个 issue；
   - **做法**：关键改动点；
   - **验证**：怎么证明它没问题（跑了什么、看到什么结果；有截图/日志更好）。
3. **CI 必须全绿**（构建 + qmllint + 启动冒烟）才进入 review。
4. 改动涉及界面的，请附**改动前后截图**。
5. 涉及第三方代码/资源的，请说明**来源与许可证**，并同步更新 `NOTICE`。

## 6. 写插件（不改主仓库也能参与）

- 歌词界面插件：一个文件夹 = 一个插件（`info.json` + 入口 QML + 预览图 + 可选着色器）。
- 规范与示例：[QuePlugins 仓库](https://github.com/bronekox/queplugins)。
- 插件仓库的 PR 走同样的标准：能跑、有截图、有说明。

## 7. 行为准则

技术讨论可以很直接，但对人保持尊重：就事论事、不揣测动机、不刷屏。
维护者会优先处理**能复现、有日志、范围清晰**的问题。
