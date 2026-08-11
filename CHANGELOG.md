# Changelog

本项目所有重要的变更都会记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### ✨ 新增
- 设置页（主题、界面、功能、播放、快捷键、插件、关于、Debug 八大模块）
- 桌面歌词 / 桌面部件
- 下载管理器（队列、进度、重试、已完成列表）
- 本地音乐库（我的文件夹 / 本地文件夹、导入、重命名、删除）

### 🎨 界面
- 全局单例样式系统（Style.qml），支持浅色/深色/跟随系统
- 自适应封面主色提取（ColorExtractor）
- 歌词逐字滚动 + 逐行弹簧动画 + 背景动态流体着色器

## [0.1.0] - 2026-08-01

### ✨ 新增
- Qt 6.9 / QML / RHI 跨平台框架
- 集成网易云、酷狗音乐 API（搜索、歌单、排行榜、新歌、歌词）
- QWindowKit 无边框窗口（毛玻璃 / 云母效果）
- 多平台支持：Windows / macOS / Linux

### 🧩 依赖
- Qt 6.9.3（Core, Gui, Qml, Quick, Network, Multimedia, Concurrent, Sql, ShaderTools）
- QWindowKit（Apache-2.0）
- pako.js（MIT）


### [0.2.5] - 2026-08-10

！重大更新！！！！

### 新增
- 全面将在线api转到c++提升性能，稳定性，为之后接入QCloudMusicApi以及QQ音乐做好准备
- 全面重做歌词界面，背景效果由AMLL Core移植，歌词组件抛弃ListView转用自定义排列，效果大更新
- 增加更多功能，在歌词界面，本地文件夹，设置调节功能，界面功能都有大量更新

### 其他
- 修复大量Bug，大量之前一直存在的一些小问题，部分优化性能
- 规范部分代码，规范协议