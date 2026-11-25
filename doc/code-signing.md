# 代码签名与首次运行警告：Windows / macOS / Linux

> 目标：让用户第一次双击时不再看到“未知发布者 / 已保护你的电脑 / 已损坏 / 不受信任”。
> 结论先说：**Windows 和 macOS 能靠签名解决，Linux 不能靠“签 exe”解决，只能靠分发渠道。**

---

## 0. 三种提示，三套机制

| 平台 | 用户看到的提示 | 真正原因 | 能否彻底消除 |
|---|---|---|---|
| Windows | “Windows 已保护你的电脑”（SmartScreen） | 文件哈希 / 证书没有信誉记录 | 只有 **Microsoft Store** 安装包能做到“永不提示”；直接下载只能等信誉累积 |
| Windows | “未知发布者”（UAC） | 安装包/程序没有 Authenticode 签名 | 有效代码签名证书即可 |
| macOS | “Apple 无法检查其是否包含恶意软件” | 未公证（notarization） | Developer ID 签名 + 公证 + stapling |
| macOS | “已损坏，无法打开” | 签名本身无效（嵌套库漏签、签完又改文件） | 修好签名即可，和公证是两件事 |
| Linux | “不受信任的应用程序启动器” | 文件带可执行位但桌面环境不认识来源 | 没有系统级发布者签名；用 Flathub / Snap / 发行版仓库 |

**关键认知**：Windows 上“有签名”不等于“没有 SmartScreen”。微软官方文档明确写着——OV/EV 证书在新文件上仍可能弹警告，直到该文件哈希或发布者证书积累足够信誉；EV **早已不再**默认绕过 SmartScreen。唯一“第一次下载就不弹”的渠道是 Microsoft Store。

参考：[SmartScreen reputation for Windows app developers](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation)

---

## 1. Windows

### 1.1 四条可选路线

| 方案 | 成本 | 首次运行提示 | 适合谁 |
|---|---|---|---|
| **Microsoft Store（MSIX）** | 个人约 $19 一次性 | **完全没有**（Store 用微软证书重签） | 愿意走商店审核 |
| **SignPath Foundation** | 开源项目免费 | 仍可能提示，但显示“已验证的发布者” | 开源项目首选 |
| **Azure Artifact Signing**（原 Trusted Signing） | $9.99/月起 | 仍可能提示，发布者名可见；信誉逐步累积 | 有 Azure 账号、要 CI 自动化 |
| **传统 OV 证书** | 约 $200–400/年 + HSM/云签名 | 仍可能提示，发布者名可见 | 商业软件、企业采购 |
| EV 证书 | 约 $300–600/年 + 硬件令牌 | **不再比 OV 更好**（微软已明确） | 只为满足企业采购要求 |

要点：

- **自签名证书没有用**：行为和未签名完全一样，仍会提示。
- 证书私钥自 2023 年起必须存放在 FIPS 140-2 硬件中，不能再导出一个裸 PFX 随便用；CI 上要么用云签名服务，要么用支持云的 HSM。
- **不要频繁换证书**：发布者信誉绑定证书；换证会让信誉归零。续期时尽量保持同一 Subject/密钥。

### 1.2 SignPath（开源项目最划算）

<https://signpath.org/> 为开源项目免费提供代码签名证书，私钥存在他们的 HSM 里，签名由 GitHub Actions 上传未签名产物、下载已签名产物完成。它验证的是“这个二进制确实由这个仓库构建”，对开源项目很合适。

流程概要：

1. 在 SignPath 申请项目，关联 GitHub 仓库；
2. 在仓库里加他们的 GitHub Action（`signpath/github-action-submit-signing-request`）；
3. 先构建出未签名的 exe/安装包，action 上传后返回签名后的文件。

它不会让 SmartScreen 立刻消失，但会去掉“未知发布者”，并让发布者名称显示出来。

### 1.3 传统证书 / Azure 签名：具体命令

仓库里已经提供脚本：[packaging/windows/sign.ps1](packaging/windows/sign.ps1)

```powershell
# 签整个 build/bin（主程序 + Qt DLL + 插件），再签安装包
.\packaging\windows\sign.ps1 -Path .\build\bin -PfxPath .\cert.pfx -PfxPassword '***'
.\packaging\windows\sign.ps1 -Path .\dist\QueMusic-0.6.0-win64-setup.exe -Thumbprint <证书SHA1>
```

等价的原始命令：

```powershell
signtool sign /fd SHA256 /td SHA256 /tr http://timestamp.digicert.com ^
  /f cert.pfx /p *** QueMusic.exe

signtool verify /pa /v QueMusic.exe
```

注意：

- **必须带时间戳**（`/tr` + `/td SHA256`），否则证书过期后签名失效。
- **主程序和安装包都要签**。只签安装包，用户从 ZIP 里直接运行 exe 时仍然是“未知发布者”。
- 递归签 `build/bin` 下所有 exe/dll；漏签某个 DLL 不影响启动，但会让“发布者”信息不完整。
- Inno Setup 想在卸载器上也带签名，需要配置 `SignTool` + `SignedUninstaller=yes`（脚本里已留注释）。

### 1.4 CI 里怎么配

`.github/workflows/build-windows.yml` 已改成：

- 构建后如果存在 `WINDOWS_PFX_BASE64`，自动调用 `sign.ps1` 签 `build/bin`；
- 用 Inno Setup 打安装包；
- 再签安装包；
- 上传 exe + 安装包。

仓库 Secrets 需要：

| Secret | 内容 |
|---|---|
| `WINDOWS_PFX_BASE64` | `.pfx` 的 base64（单行或多行均可） |
| `WINDOWS_PFX_PASSWORD` | 导出 `.pfx` 时设置的密码 |

生成 base64：

```powershell
[Convert]::ToBase64String([IO.File]::ReadAllBytes('cert.pfx')) | Set-Clipboard
```

> 如果用的是 SignPath 或 Azure Artifact Signing，就不要配这两个 Secret，改为接它们的 action / `signtool /dlib`。

### 1.5 关于 SmartScreen 的现实预期

- 新证书 + 新文件：前几周大概率仍提示；
- 用户点“更多信息 → 仍要运行”会帮助该文件哈希积累信誉；
- 同一证书签后续版本，证书信誉可以复用一部分；
- 官方没有“提交白名单”通道，只能靠下载量自然积累；
- **winget 不能绕过 SmartScreen**，它只是安装入口；
- 想“第一次就零提示”，只有 Microsoft Store。

---

## 2. macOS

### 2.1 需要什么

- Apple Developer Program：**$99/年**（个人/公司均可）；
- 证书类型：**Developer ID Application**（不是 Mac App Distribution）；
- 公证：`notarytool` 上传，Apple 扫描后发票据；
- 最后 `stapler` 把票据钉到 `.app` / `.dmg` 上，离线也能验证。

没有这三步，用户看到的就是：

- 未签名：`“QueMusic” 已损坏，无法打开` 或 `无法验证开发者`；
- 只签名未公证：`Apple 无法检查其是否包含恶意软件`。

### 2.2 仓库里已经写好了

[.github/workflows/build-macos.yml](.github/workflows/build-macos.yml) 已经包含：

1. `macdeployqt` 部署 Qt；
2. 从内到外逐层 `codesign`（framework → dylib → 可执行文件 → .app），不用 `--deep`；
3. `codesign --verify --deep --strict` 门禁；
4. 签名 DMG；
5. `notarytool submit --wait`；
6. `stapler staple` + `spctl` 验证。

这次补上了关键的一步：**把 Secret 里的 `.p12` 导入临时 keychain**。
之前流程假设证书已经在 runner 上，但 GitHub 的 macOS runner 是干净的，没这一步 `codesign` 会直接找不到身份。

### 2.3 一次性配置

1. 加入 Apple Developer Program；
2. 打开 <https://developer.apple.com/account/resources/certificates>，创建 **Developer ID Application** 证书；
3. 下载 `.cer`，双击导入“钥匙串访问”，在“我的证书”里找到它；
4. 右键导出为 `.p12`，设置密码；
5. 生成 App 专用密码：<https://appleid.apple.com> → 登录与安全 → App 专用密码；
6. 记下 Team ID（开发者账号页面右上角）。

仓库 Secrets：

| Secret | 示例 / 说明 |
|---|---|
| `APPLE_SIGN_IDENTITY` | `Developer ID Application: Your Name (ABCDE12345)` |
| `APPLE_CERT_P12` | `.p12` 的 base64 |
| `APPLE_CERT_PASSWORD` | 导出 `.p12` 时的密码 |
| `APPLE_ID` | Apple ID 邮箱 |
| `APPLE_APP_PASSWORD` | App 专用密码（不是登录密码） |
| `APPLE_TEAM_ID` | `ABCDE12345` |

生成 base64（macOS/Linux）：

```bash
base64 -i cert.p12 | pbcopy
```

### 2.4 本地验证

```bash
codesign --verify --deep --strict --verbose=2 QueMusic.app
codesign -dv --verbose=4 QueMusic.app
spctl -a -vvv -t exec QueMusic.app

xcrun stapler validate QueMusic.dmg
spctl -a -t open --context context:primary-signature -v QueMusic.dmg
```

预期：

- `codesign` 无输出、退出码 0；
- `spctl` 显示 `accepted`、`source=Notarized Developer ID`；
- `stapler validate` 显示 `The validate action worked!`。

### 2.5 常见“已损坏”

- 签完名又 `lipo` 裁剪架构 → 签名失效（本仓库已把瘦身放在签名之前）；
- `.app/Contents/MacOS` 里有运行期写入的 `Account.ini` 之类非可执行文件 → codesign 拒绝签主程序（workflow 已清理）；
- 嵌套 dylib 漏签 → 主程序签名报 `In subcomponent`；
- 用 `--deep` 图省事 → 经常留下无法验证的签名。

如果暂时没有证书，只能走 ad-hoc 签名 + 用户手动放行：

```bash
xattr -dr com.apple.quarantine /Applications/QueMusic.app
```

这不是“解决”，只是让用户绕过；正式分发还是得上 Developer ID。

---

## 3. Linux

Linux 没有 Windows Authenticode / macOS Gatekeeper 那种“发布者签名”体系。用户看到的警告通常来自：

1. **桌面环境不信任这个可执行文件**（GNOME “不受信任的应用程序启动器”）——这是文件来源标记问题，不是密码学签名；
2. 浏览器下载 `.AppImage` 后的“可能有害”提示；
3. AUR/第三方仓库的“未知打包者”——AUR 本来就是用户自行构建。

### 3.1 真正的解法：走带签名的分发渠道

| 渠道 | 信任来源 | 用户是否看到警告 |
|---|---|---|
| **Flathub** | OSTree 仓库由 Flathub 签名，客户端校验 | 否 |
| **Snap Store** | Canonical 签名 | 否 |
| **发行版仓库**（Debian/Fedora/Arch extra） | 包管理器与仓库签名 | 否 |
| **AppImage 直链** | 无自动校验，靠用户手动核对 | 可能有 |
| **AUR** | 用户本地构建，靠投票/评论 | 无签名概念 |

所以 Linux 侧的“签名问题”答案是：**把 Flathub 做出来**，而不是研究怎么签 AppImage。

### 3.2 AppImage 仍然可以做的事

- GPG 签名 + 发布公钥：

```bash
gpg --detach-sign --armor QueMusic-x86_64.AppImage
sha256sum QueMusic-x86_64.AppImage > QueMusic-x86_64.AppImage.sha256
```

- GitHub Artifact Attestations（免费，推荐）：

```yaml
permissions:
  id-token: write
  contents: read
  attestations: write

steps:
  - uses: actions/attest-build-provenance@v2
    with:
      subject-path: dist/*
```

用户验证：

```bash
gh attestation verify QueMusic-x86_64.AppImage --repo BroNekoX/QueMusic
```

这能证明“这个文件由该仓库的 CI 构建”，但**不会**让 GNOME 的“不受信任”提示消失——那个提示只看可执行位和来源标记。

---

## 4. 成本与推荐

| 平台 | 推荐方案 | 年成本 | 效果 |
|---|---|---|---|
| Windows | Microsoft Store（MSIX） | $19 一次性 | 零警告（最优） |
| Windows | SignPath Foundation | 免费 | 去掉“未知发布者”，SmartScreen 仍需信誉 |
| Windows | Azure Artifact Signing | $120/年 | 同上，CI 集成好 |
| macOS | Developer ID + 公证 | $99/年 | 零警告 |
| Linux | Flathub | 免费 | 零警告 + 沙箱 |

**给 QueMusic 的顺序：**

1. macOS：证书是唯一硬门槛，付 $99 后现有 workflow 直接生效；
2. Windows：先用 SignPath 免费证书去掉“未知发布者”，同时把 Store 的 MSIX 流程走通（Store 才是零警告）；
3. Linux：Flathub 上架后，AppImage 只作为备用下载；
4. 所有平台：Release 附带 `SHA256SUMS` + GitHub Attestation。

---

## 5. 检查清单

**Windows**
- [ ] `signtool verify /pa /v` 对 exe 和安装包都通过
- [ ] 签名带 RFC3161 时间戳
- [ ] 同一张证书连续签多个版本（不要每版换证）
- [ ] 安装包内所有 exe/dll 都已签名
- [ ] 若走 Store：MSIX 的 Publisher 与 Partner Center 一致

**macOS**
- [ ] `codesign --verify --deep --strict` 通过
- [ ] `spctl` 显示 `Notarized Developer ID`
- [ ] `stapler validate` 通过
- [ ] DMG 也签名并公证
- [ ] 签名后再做任何二进制修改（lipo/strip）都会失效

**Linux**
- [ ] Flathub 包能从 `flatpak install` 直接装、无警告
- [ ] AppImage 提供 `.sha256` 与 GPG 签名
- [ ] Release 附带 GitHub Attestation
- [ ] 文档说明“Linux 没有 exe 签名，请优先用 Flatpak”
