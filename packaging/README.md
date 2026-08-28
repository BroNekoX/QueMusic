# QueMusic 打包指南

本目录包含 QueMusic 在各平台的打包脚本与配置。

## 一、各平台打包方式一览

| 平台 | 产物 | 打包位置 | 是否需要本机 |
|------|------|----------|--------------|
| **Windows** | `QueMusic-setup.exe`（Inno Setup） | 本机 | ✅ 需要 Windows |
| **Linux** | `QueMusic-x86_64.AppImage` | 本机（任何发行版） | ✅ 需要 Linux |
| **macOS** | `QueMusic-0.1.0-macOS-{arm64,x86_64}.dmg` | GitHub Actions 云端 | ❌ 不需要 Mac！ |

## 二、macOS 打包（无需 Mac，用 GitHub Actions）

本项目内置了 GitHub Actions 工作流：`.github/workflows/build-macos.yml`。

### 工作流会自动完成
1. 在 GitHub 的 macOS 虚拟机（Apple Silicon + Intel 各一台）上安装 Qt 6.10.3
2. 拉取子模块并构建 Release 版
3. 用 `macdeployqt` 把 Qt 库打进 `.app`
4. ad-hoc 签名（无证书也能本机运行）
5. 打包成 `.dmg` 并上传为 artifact

### 触发方式

#### 方式 A：手动触发（不发布也能出包）
1. 打开仓库 GitHub 页面 → **Actions** 标签
2. 左侧点 **Build macOS** → 右侧 **Run workflow** → 绿色按钮
3. 等待几分钟，完成后在 workflow 运行页底部 **Artifacts** 下载 `.dmg`

#### 方式 B：打 tag 自动触发（配合 Release）
```bash
git tag v0.1.0
git push origin v0.1.0
```
推送 tag 后，工作流自动跑，并把 `.dmg` 直接挂到对应 Release 上。

### 下载后的 .dmg 使用说明
macOS 用户首次打开若提示"无法验证开发者"：
- 右键点击 App → **打开**（选择仍要打开）
- 或终端执行：`xattr -dr com.apple.quarantine /Applications/QueMusic.app`

> 注：由于没有 Apple Developer 证书，App 未签名/未公证，属正常现象。
> 未来若想消除提示，需要注册 Apple Developer（$99/年）并配置签名与公证。

### 想只出 Apple Silicon 版？
编辑 `.github/workflows/build-macos.yml`，把 matrix 里的 `macos-13` 删掉即可。

## 三、Linux AppImage 打包

在 Linux 上执行：

```bash
bash packaging/build-linux.sh
```

脚本会自动完成：
1. 检查系统依赖（cmake / ninja / wget / python3-pip）
2. 用 **aqtinstall 安装 Qt 6.10.3**（与 Windows 版完全一致，避免系统 Qt 版本不一致带来的兼容风险）
3. 下载 linuxdeploy + qt 插件
4. 构建 Release 版
5. 打包出自包含的 **`QueMusic-x86_64.AppImage`**

### 试运行

```bash
chmod +x QueMusic-x86_64.AppImage
./QueMusic-x86_64.AppImage
```

> 若 AppImage 无法运行（缺 FUSE）：
> ```bash
> sudo pacman -S fuse2        # Arch
> sudo apt install libfuse2   # Debian/Ubuntu
> ```

## 四、Arch 原生包（Nyarch / Arch）

### 方式 A：makepkg 本地打包

```bash
cd packaging
makepkg -si        # 构建并安装
```

### 方式 B：直接构建（使用当前源码）

```bash
cp PKGBUILD .      # 放到项目根目录
makepkg -si
```

> ⚠️ **注意**：Arch 包使用**系统 Qt 6.11** 编译，QWindowKit 会针对系统 Qt 重新构建。
> 如果遇到窗口/兼容问题，请优先使用 AppImage 版本。

## 五、发布前必测项（Wayland / macOS）

1. **窗口拖拽**：无边框窗口在 KWin Wayland / macOS 下是否能正常拖动
2. **最小化/最大化/关闭**：QWindowKit 系统按钮是否正常
3. **毛玻璃效果**：Wayland / macOS 下 DWM 模糊可能受限，可退化为普通背景
4. **多媒体播放**：本地文件 + 在线播放是否正常
5. **桌面部件**：QDesktopSpot 桌面部件在不同平台下的行为

> 💡 macOS 上建议在 **Intel + Apple Silicon** 两种机型各测一遍。

## 六、目录结构

```
.github/workflows/
├── build-macos.yml     # macOS 云端打包（GitHub Actions）
packaging/
├── build-linux.sh      # Linux AppImage 一键打包脚本
├── QueMusic.desktop    # Linux 桌面入口文件
├── PKGBUILD            # Arch 原生包脚本
└── README.md           # 本说明
```

