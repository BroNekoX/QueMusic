# 第三方组件声明 / Third-Party Notices

本文件记录 QueMusic 项目中使用的第三方开源组件及其许可信息。
The following third-party components are used in QueMusic.

QueMusic 主体代码以 **Apache License 2.0** 授权（见根目录 `LICENSE`）。

---

## 其他组件

（如后续引入其他第三方组件，请在此处补充声明。）

- **FFmpeg 7.1.3**（libavcodec 61.19.101 / libavformat 61 / libavutil 59 / libswresample 5）：音频后端的
  解封装、解码与重采样。**动态链接、未修改上游源码**，不构成派生作品，因此 QueMusic 主体代码无需据此开源。
  - 许可证：**LGPL version 2.1 or later**。该结论取自库自身的 `avutil_license()` 输出，非人工推断；
    构建参数（`avutil_configuration()`）为
    `--disable-programs --disable-doc --disable-debug --enable-network --disable-lzma --enable-pic
    --disable-vulkan --disable-v4l2-m2m --disable-decoder=truemotion1 --enable-shared --disable-static`，
    **不含 `--enable-gpl` / `--enable-nonfree`**，即不含任何 GPL / 不可再分发组件
  - 共享库来源：**Qt 6.10.3 自带分发**（`<Qt>/bin/avcodec-61.dll` 等四项），与项目早先拉取的
    `ffmpeg-n8.1-*-lgpl-shared` 同属 LGPL 构建；改用后授权义务不变，二进制体积由约 111MB 降至约 19MB
  - 对应源码：<https://ffmpeg.org/releases/ffmpeg-7.1.3.tar.xz>（未修改的原始源码）
  - 重新分发要求：随包附上 LGPL-2.1 许可证文本与上述构建配置说明；不得限制用户替换这些共享库
  - `include/`（编译期头文件）、`lib/`（导入库）与 `LICENSE.txt` **随仓库提供**，构建无需联网下载；
    运行时共享库**不重复入库**，构建时直接从 `<Qt>/bin` 拷贝同版本 dll；
    `cmake/fetch_ffmpeg.ps1` 仅用于维护（重新生成/替换这批文件）
  - **Linux 构建不使用上述 Windows 文件**：通过 pkg-config 链接**发行版自带的 FFmpeg**
    （Debian/Ubuntu：`libavcodec-dev libavformat-dev libavutil-dev libswresample-dev`；Arch：`ffmpeg`；
    见 `packaging/build-linux.sh` 与 `packaging/PKGBUILD`）。所调用的 API 均为 FFmpeg ≥ 5.1 即存在的
    接口（`av_channel_layout_*`、`swr_alloc_set_opts2`、`avcodec_send_packet/receive_frame` 等），
    故各发行版版本均可编译
  - ⚠️ **注意发行版的构建类型**：Debian/Ubuntu、Arch 等的 FFmpeg 多以 `--enable-gpl` 构建，此时 Linux 版
    链接的即含 GPL 组件。按 FSF 立场，该组合作品应按 GPL 分发；本项目主体为 Apache-2.0（与 GPLv3 兼容）
    且源码公开，已满足 GPL 的源代码提供义务。**若需完全规避**，应改为链接 LGPL 构建的 FFmpeg
    （自建 `--disable-gpl`，或随包附带 LGPL 版）。发行前可用 `avutil_license()` 确认目标环境的实际授权
- **Qt 6.10.3**（Core / Gui / Qml / Quick / Network / Multimedia / Concurrent / Sql / ShaderTools / Widgets）：
  界面框架、音频输出设备（`QAudioSink`）、网络请求与本地数据库。发行包内随附 Qt 运行时 DLL。
  - 授权：**LGPL v3**（亦提供 GPL 与商业授权）。本项目以**动态链接**方式按 LGPLv3 使用，
    故 QueMusic 主体代码无需据此开源
  - 义务：随包附上 LGPLv3 许可证文本；不得限制用户替换这些库（本项目为 Apache-2.0，无此类限制）；
    不得修改 Qt 源码后仍按 LGPLv3 分发（本项目未修改）
  - 许可证文本见 Qt 安装目录的 `LICENSES/`；源码：<https://download.qt.io/official_releases/qt/6.10/>
  - 注：Qt 的 FFmpeg 后端属于 Qt 的附加组件，其授权为 FFmpeg 自身的 LGPL-2.1+（见上一条），与 Qt 自身许可相互独立
- **QWindowKit**：窗口标题栏 / 系统集成组件，遵循其自身许可证（见其项目文档）。
- **TagLib 2.3.1**：本地音频元数据与 ID3v2 歌词读取，采用 LGPL-2.1-or-later / MPL-1.1 双许可证；源码与许可证位于 `ThirdParty/taglib/`。
- **图标字体**（`feather.ttf`、`poppins.ttf` 等）：仅作资源使用，版权归原作者所有。
