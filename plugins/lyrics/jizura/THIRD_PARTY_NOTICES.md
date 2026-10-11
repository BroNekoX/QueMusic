# 第三方声明 / Third-party notices

本目录（`plugins/lyrics/jizura/`）是 JIZURA 引擎的 Qt/QML 移植，**包含 JIZURA 的原始源码**
（由 `tools/build_bundle.py` 按 ASCII 顺序拼接进 `js/jizura.bundle.js`）。
本目录自身的移植代码（`src/runtime/`、`qml/`、`jizura.qml`、`tools/`、`tests/`）同样以 MIT 授权。

---

## JIZURA

- 项目：https://github.com/hakoniwa/JIZURA
- 版本：0.10.1
- 许可：MIT License

```
MIT License

Copyright (c) 2026 hakoniwa

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

上述版权与许可声明同时写在生成物 `js/jizura.bundle.js` 的文件头里，随二进制一起分发。

### 未包含的部分

原项目还带有以下内容，本移植**没有**包含：

- **mp4-muxer 5.2.2**（MIT，Copyright (c) 2023 Vanilagy）：只被导出模块 `src/11_export.js` 使用，
  该文件不在打包清单内（本项目不做视频导出）。
- **字体**：原 Web 版运行时从 Google Fonts 加载 Noto Sans JP 等 12 款字体（各自为 SIL OFL 1.1）。
  本移植改用宿主系统的字体（QML `Qt.fontFamilies()`），不内嵌、不分发任何字体文件。
- **音频分析**（`src/10_audio.js`）与**导出/界面**（`src/11_export.js`、`src/12_ui.js`）：
  不在打包清单内，节拍与能量由宿主播放器提供。
