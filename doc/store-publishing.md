# QueMusic 应用商店上架指南

> 目标渠道：**Flathub**、**AUR**、**Microsoft Store**（附带 winget / AppImage / Snap）。
> 本文基于仓库现状（0.6.0 预览期、Qt 6.10.3、Apache-2.0）写成，命令可直接照抄。
> 标注“未验证”的地方需要在 Linux / Windows 实机上跑一遍——本文只在 Windows 上审阅了仓库。

---

## 0. 先看结论

| 渠道 | 现在能不能上 | 主要卡点 | 预计耗时 |
|---|---|---|---|
| **AUR** | 可以，几乎没有审核 | 无（法律风险由打包者承担） | 1 天 |
| **Flathub** | 技术上可以，**政策上高风险** | 在线音源 + 下载功能；App ID 归属；Wayland 下的无边框窗口 | 1–3 周（含 review 往返） |
| **Microsoft Store** | 技术可行，**政策风险最高** | 同上；Partner Center、年龄分级、隐私政策；未打包安装器还要买代码签名证书 | 2–4 周 |
| **winget** | 强烈建议先做 | 需要一个稳定的安装包 URL | 3–7 天 |
| **AppImage / Snap** | 已有基础 | Snap 的 Qt6 版本受 kde-neon 扩展限制 | 2–3 天 |

**建议顺序**：GitHub Release（已有）→ winget → AUR → Flathub → Microsoft Store。
前两步不牵扯商店政策，还能先把截图、安装包、版本号理顺。

---

## 1. 现状审计：距离“可上架”还差什么

| 项 | 现状 | 影响 | 处理 |
|---|---|---|---|
| CMake install() | 原来一条都没有，打包脚本手工 cp | Flatpak/AUR 都要求 cmake --install 能落盘 | 已补（§2.1） |
| 版本号 | CMake 写 0.1，README/UI 写 0.6.0 | 商店版本、macOS bundle、包管理器全部错位 | 已改为 0.6.0 |
| App ID | com.bronekox.quemusic 无对应域名 | Flathub 审核会核对 App ID 域名归属 | Linux 侧改为 io.github.BroNekoX.QueMusic |
| .desktop | 旧文件 Exec/Icon 与 App ID 不匹配 | 启动器图标丢失、Wayland 下窗口归类错误 | 新增规范文件（§2.3） |
| AppStream metainfo | 不存在 | Flathub 硬性要求，商店展示也用它 | 新增（§2.3） |
| 图标 | resources/icon.png 只有 128×128 | AppStream 建议 ≥256，Flathub 会挑 | 安装 512×512 的 rainbowMusicIcon.png |
| Wayland app_id | 未调用 setDesktopFileName | 任务栏图标/分组错误 | 已在 main.cpp 补 |
| 截图比例 | 1156×759（≈3:2） | AppStream 只接受 16:9 / 4:3 | 需裁成 1200×675 或 1024×768 |
| 在线音源 + 下载 | 网易云/酷狗/B站 + DownloadManager | **Flathub / MS Store 的政策红线** | 见 §6，建议做 distro 构建 |
| README 免责声明 | “仅供学习与研究，禁止商业用途” | 与“上架商店”叙事冲突，审核会读 | 商店版 README/描述单独写 |
| 子模块 | flatpak-builder 的 git source 默认递归拉取 | 无 | 已在 manifest 中利用 |
| Qt 许可 | LGPLv3 动态链接，Apache-2.0 应用可分发 | 无 | 保留 NOTICE |
| 自动更新 | 没有自更新器 | MSIX 正好禁止自更新 | 无 |

---

## 2. 已落地的通用改造

### 2.1 CMake 安装规则

CMakeLists.txt 末尾新增：

```cmake
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(UNIX AND NOT APPLE)
    set(QUE_MUSIC_APP_ID "io.github.BroNekoX.QueMusic")
    install(FILES packaging/linux/${QUE_MUSIC_APP_ID}.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/linux/${QUE_MUSIC_APP_ID}.metainfo.xml
            DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
    install(FILES resources/app/rainbowMusicIcon.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps
            RENAME ${QUE_MUSIC_APP_ID}.png)
endif()
```

验证：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
DESTDIR="$PWD/pkg" cmake --install build
find pkg -type f
```

预期至少看到 usr/bin/QueMusic、usr/share/applications/io.github.BroNekoX.QueMusic.desktop、
usr/share/metainfo/io.github.BroNekoX.QueMusic.metainfo.xml、usr/share/icons/hicolor/512x512/apps/io.github.BroNekoX.QueMusic.png。

### 2.2 App ID 为什么必须换

Flathub 的审核清单第一条就是“核对 App ID 里的域名是否由提交者控制”。com.bronekox 对应
bronekox.com，如果你没有这个域名，审核会卡住。io.github.* 可以用 GitHub 账号直接验证
（仓库 owner 登录 Flathub 即可），是成本最低的方案。

如果坚持保留 com.bronekox.quemusic，需要在 bronekox.com 放
https://bronekox.com/.well-known/org.flathub.VerifiedApps.txt，或加 TXT 记录
_flathub.bronekox.com。两者都没有就换 ID。

换 ID 只影响 Linux 桌面集成（desktop 文件名、Wayland app_id、Flatpak ID），
不会改变 QSettings 的 BroNekoX/QueMusic.ini 路径，老用户数据不受影响。
**但一旦上架 Flathub 就不要再换**，所以现在就定下来。

### 2.3 元数据文件

- packaging/linux/io.github.BroNekoX.QueMusic.desktop
- packaging/linux/io.github.BroNekoX.QueMusic.metainfo.xml

自检（Linux 上）：

```bash
appstreamcli validate --no-net packaging/linux/io.github.BroNekoX.QueMusic.metainfo.xml
desktop-file-validate packaging/linux/io.github.BroNekoX.QueMusic.desktop
```

常见告警：
- 截图不是 16:9 / 4:3 —— 用 doc/example/home.jpg 裁成 1200×675。
- <release> 日期必须是过去时间。
- <developer id> 必须和 App ID 前缀一致。

---

## 3. Flathub

### 3.1 先决条件

- 运行时用 org.kde.Platform//6.10（Qt 6.10，已发布；不要用 6.8，Qt 版本不够）。
- 应用必须能在无网络、/app 只读的沙箱里启动。
- 构建机 **无网络**：所有源码必须声明在 manifest 里。本项目的三个子模块由
  flatpak-builder 的 git source 自动递归检出（manpage 中 disable-submodules 默认关闭），
  所以只需要一条 source。
- FFmpeg 走 SDK 里的开发包（pkg-config libavcodec），不要依赖仓库里被 gitignore 的 ffmpeg/。

### 3.2 本地构建

```bash
flatpak install -y flathub org.flatpak.Builder org.kde.Platform//6.10 org.kde.Sdk//6.10

flatpak run --command=flatpak-builder-lint org.flatpak.Builder \
  manifest packaging/flatpak/io.github.BroNekoX.QueMusic.yml

flatpak run --command=flathub-build org.flatpak.Builder --install \
  packaging/flatpak/io.github.BroNekoX.QueMusic.yml

flatpak run io.github.BroNekoX.QueMusic
```

manifest 里已带 -DQUEMUSIC_ENABLE_LTO=OFF（缩短 CI 时间）和 -DQUEMUSIC_QML_LINT=OFF
（新 Qt 的 qmllint 规则变化会误伤）。如果 SDK 里找不到 FFmpeg 头文件，构建会停在
pkg_check_modules，先确认：

```bash
flatpak run --command=sh org.kde.Sdk//6.10 -c 'pkg-config --modversion libavcodec'
```

### 3.3 提交

1. Fork flathub/flathub，切到 new-pr 分支（**不要**对 master 开 PR）。
2. 新建目录 io.github.BroNekoX.QueMusic/，放入：
   - io.github.BroNekoX.QueMusic.yml
   - io.github.BroNekoX.QueMusic.metainfo.xml（或确保安装到 /app/share/metainfo）
3. PR 标题：Add io.github.BroNekoX.QueMusic，填写模板里的每个问题。
4. 审核会要求跑 bot, build；通过后由 reviewer 合并并建仓库。

### 3.4 常见拒绝原因（按概率排序）

1. **版权/服务条款**：应用提供第三方平台音源抓取与下载。审核会问“这是否经过平台授权”。
   这是本项目最大的不确定性，见 §6。
2. App ID 域名无法验证。
3. 构建联网 / 依赖未声明（例如直接引用仓库里的 ffmpeg/、build/）。
4. 权限过大：--filesystem=home、--filesystem=host。
5. metainfo 缺截图/版本/开发者，或 appstreamcli 报错。
6. 在沙箱里无法播放（没有 pulseaudio、或 QWindowKit 在 Wayland 下崩溃）。
7. 应用名/图标涉及商标。

### 3.5 更新

Flathub 仓库建好后，每次发版改 manifest 的 commit/tag 并提 PR；
也可以接 flathub/flathub-external-data-checker 自动跟 GitHub Release。

---

## 4. AUR

### 4.1 现在就能提交

packaging/aur/PKGBUILD 是 quemusic-git（跟踪 main，VCS 包），
.SRCINFO 已生成。先本地验证：

```bash
cd packaging/aur
makepkg -si          # 构建并安装
namcap PKGBUILD      # 静态检查
namcap *.pkg.tar.zst
```

### 4.2 认证与推送

AUR 只接受 SSH，且必须先有账号：

```bash
# 1) 生成密钥（已有可跳过）
ssh-keygen -t ed25519 -C "aur@quemusic"
cat ~/.ssh/id_ed25519.pub

# 2) 打开 https://aur.archlinux.org/account
#    把上面这行公钥粘到 "SSH Public Key" 并保存

# 3) 验证（返回 "Welcome to AUR" 之类即成功）
ssh aur@aur.archlinux.org help
```

首次发布时仓库还不存在，**不能先 clone**，要本地初始化再推：

```bash
cd dist/aur/quemusic-git          # 已准备好的本地仓库
git remote add origin ssh://aur@aur.archlinux.org/quemusic-git.git
git push -u origin master
```

后续更新才用 clone：

```bash
git clone ssh://aur@aur.archlinux.org/quemusic-git.git
```

### 4.3 注意点

- **不要**在 package() 里手工拷 build/bin/QueMusic，现在统一走 cmake --install，
  否则会漏掉 desktop/metainfo/icon。
- 子模块在 prepare() 里拉取，意味着打包时需要网络；这是 -git 包的常见做法，
  但如果你要做稳定包 quemusic，应改成 release tarball + 三个子模块 tarball + 真实 sha256。
- Arch 的 ffmpeg 可能已经升到 8.x，而项目按 7.1.3 的 API 写的；如果编译报
  avcodec 相关错误，临时把 ffmpeg 换成 AUR 的 ffmpeg7，或给上游提兼容补丁。
- qt6-5compat 必须装：QML 里用了 Qt5Compat.GraphicalEffects。
- 上架后建议再发一个 quemusic（稳定版）和一个 quemusic-bin（AppImage），
  三个包共用同一份上游源码。

---

## 5. Microsoft Store

微软现在有两条路，选一条即可。

### 5.1 路线 A：MSIX（推荐）

优点：**商店免费签名**，自动更新由系统负责，卸载干净。
缺点：需要处理包身份带来的路径/快捷方式差异。

1. 注册 Partner Center（个人约 $19，公司约 $99，以官网为准），**先预留应用名**。
2. 记下 Identity Name（例如 BroNekoX.QueMusic）和 Publisher（形如 CN=xxxx）。
3. 在 Windows 上构建 Release，然后：

```powershell
.\packaging\windows\build-msix.ps1 `
  -QtBin "C:\Qt\6.10.3\llvm-mingw_64\bin" `
  -IdentityName "BroNekoX.QueMusic" `
  -Publisher "CN=你的Publisher" `
  -Version 0.6.0.0
```

脚本会：拷 build/.../bin → 跑 windeployqt → 生成 5 张 MSIX 图标 →
填 AppxManifest.xml → makeappx pack → （可选）signtool 签名 → 打 .msixupload。

4. Partner Center 上传 .msixupload，填写：
   - 年龄分级（IARC 问卷；纯播放器一般 3+）
   - 隐私政策 URL（有网络请求就必须有）
   - 至少 1 张 1366×768 或 1920×1080 截图
   - 分类 Music，支持语言 zh-CN / en-US
5. 用 **Windows App Certification Kit** 本地先跑一遍，能省一轮返工。

**MSIX 下的已知坑**：
- main.cpp 里的 registerSmtcAppIdentity() 会往开始菜单写 .lnk。打包应用不允许
  自建开始菜单快捷方式，轻则失败、重则启动报错。建议加：

```cpp
// 包身份下跳过：快捷方式由清单声明
UINT32 len = 0;
if (GetCurrentPackageFullName(&len, nullptr) == APPMODEL_ERROR_NO_PACKAGE) { /* 旧逻辑 */ }
```

- 安装目录只读：项目目前只写 %APPDATA% / %LOCALAPPDATA%，没问题。
- 不要内置更新器（本项目没有，保持）。

### 5.2 路线 B：未打包 EXE（更省事，但要买证书）

微软允许直接把 .exe/.msi 安装包挂到 Store 列表（2021 年起）。
本项目已有 Inno Setup 脚本 packaging/windows/QueMusic.iss：

```powershell
iscc /DBuildDir="C:\path\to\deployed\bin" packaging\windows\QueMusic.iss
```

要求：安装包必须**离线可用**、URL 内容不可变、只安装本产品。
这条路的代价是：**必须自己买 CA 代码签名证书**（约 $200–400/年），否则 SmartScreen
和 Store 认证都会拦。MSIX 则由商店代签，长期更划算。

### 5.3 更现实的第一步：winget

```powershell
winget install wingetcreate
wingetcreate new https://github.com/BroNekoX/QueMusic/releases/download/v0.6.0/QueMusic-0.6.0-win64-setup.exe
```

按提示填 Publisher / License / 静默参数（Inno 默认 /VERYSILENT /SUPPRESSMSGBOXES /NORESTART），
然后它会自动向 microsoft/winget-pkgs 提 PR。通常 1–3 天合并，比 Store 快一个数量级。

### 5.4 商店政策风险

Microsoft Store 对“帮助用户获取受版权保护内容”的应用有明确限制。审核可能先过、
后续因投诉下架；这类下架会污染开发者账号。**不要用“仅供学习”当免责声明**——
审核会把它当作“非生产用途”的证据。

---

## 6. 关键决策：是否做“商店版”

这是能不能上 Flathub / MS Store 的分水岭。

### 6.1 风险点

- api/ 接的是网易云、酷狗、B站的非官方接口；
- DownloadManager 可以把在线音频落盘；
- 插件系统还能继续扩展来源；
- README 明确写“仅供学习与研究”。

Flathub 的审核是人来判断“是否促进未经授权的分发”，MS Store 则可能直接按知识产权
投诉处理。两条渠道都不适合原样提交。

### 6.2 建议方案：QUEMUSIC_DISTRO_BUILD

加一个 CMake 开关，商店版默认：

```cmake
option(QUEMUSIC_DISTRO_BUILD "Build without third-party online sources" OFF)
if(QUEMUSIC_DISTRO_BUILD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE QUEMUSIC_DISTRO_BUILD)
endif()
```

在 C++/QML 里最少改三处：

1. MusicApiService 初始化时只注册 WebDav 与本地源，Netease/Kugou/Bilibili 返回空；
2. 隐藏首页的“在线推荐/搜索”入口与下载按钮（pages/HomePage.qml、DownloadManager 相关）；
3. 关于页显示“商店版仅支持本地/WebDAV 曲库”，并链接完整版 GitHub Release。

这样：
- **Flathub/MS Store** 拿到的是合规的本地播放器；
- **AUR/GitHub/官网** 保留完整功能；
- 代码只有一套，CI 多一个构建变体。

### 6.3 如果坚持原样提交

可以试，但要预期：
- Flathub 可能直接拒，或在合并后被 recall；
- MS Store 即使过审，也存在后续下架风险；
- 不要在同一账号下反复提交，容易被标记。

---

## 7. CI 流水线建议

| Workflow | 触发 | 作用 |
|---|---|---|
| flatpak.yml | PR / tag | flatpak-builder-lint + 试构建，防止 manifest 漂移 |
| aur.yml | tag | makepkg 校验 + 自动 push .SRCINFO |
| msix.yml | tag | 构建 + build-msix.ps1，产物挂 Release |
| winget.yml | release | wingetcreate update |

三端版本号统一从 git tag 注入：

```bash
cmake -B build -DCMAKE_PROJECT_VERSION="${GITHUB_REF_NAME#v}"
```

---

## 8. 发布检查清单

**发版前**
- [ ] git tag v0.6.0 并推送（Flathub 的 tag/commit 要对得上）
- [ ] CMakeLists.txt 的 VERSION、Options.qml 的版本号、metainfo <release> 三处一致
- [ ] cmake --install 在干净目录能产出全部文件
- [ ] appstreamcli validate / desktop-file-validate 无 error
- [ ] 截图换成 16:9
- [ ] Linux 上 X11 + Wayland 各启动一次，播放本地文件
- [ ] Flatpak 沙箱内启动、播放、托盘、文件对话框正常
- [ ] 商店版确认不含在线源与下载入口

**Flathub**
- [ ] manifest 的 commit 指向 tag
- [ ] flatpak-builder-lint 通过
- [ ] 权限最小化，无 home/host
- [ ] PR 描述里说明应用用途、许可证、以及在线功能的法律依据（或说明商店版已移除）

**AUR**
- [ ] .SRCINFO 与 PKGBUILD 同步
- [ ] namcap 无 error
- [ ] depends 含 qt6-5compat
- [ ] package() 用 cmake --install

**Microsoft Store**
- [ ] Partner Center 名称已预留
- [ ] Identity Name / Publisher 与证书一致
- [ ] WACK 通过
- [ ] 年龄分级 + 隐私政策
- [ ] MSIX 里没有自更新器、没有写安装目录

---

## 9. 时间表（保守估计）

| 周 | 事项 |
|---|---|
| 第 1 周 | 修版本/图标/截图；跑通 cmake --install；发 v0.6.0 tag；提交 winget |
| 第 2 周 | AUR 首次提交；Flathub 本地构建 + lint |
| 第 3–4 周 | Flathub PR 往返；同时做 QUEMUSIC_DISTRO_BUILD |
| 第 5–6 周 | MSIX 构建 + WACK；Partner Center 提交 |

---

## 附：本次新增/修改的文件

```
CMakeLists.txt                                   # 版本号 + install 规则
main.cpp                                         # setDesktopFileName
packaging/linux/io.github.BroNekoX.QueMusic.desktop
packaging/linux/io.github.BroNekoX.QueMusic.metainfo.xml
packaging/flatpak/io.github.BroNekoX.QueMusic.yml
packaging/aur/PKGBUILD
packaging/aur/.SRCINFO
packaging/windows/AppxManifest.xml
packaging/windows/build-msix.ps1
packaging/windows/QueMusic.iss
doc/store-publishing.md
```
