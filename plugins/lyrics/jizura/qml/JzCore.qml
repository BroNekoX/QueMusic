// SPDX-License-Identifier: MIT
//
// JIZURA × QueMusic — C++ 计算内核 (LyricPvCore) 的适配层。
//
// 为什么单独一个文件、并且由 jizura.qml 用 Loader 加载, 而不是在入口文件里直接
// `import LyricPvCore`: jizura 目录同时被当作"可外置安装的插件"来用 (拷到别的
// 播放器也能跑)。在没有链接该模块的宿主里, 直接 import 会让整个歌词主题加载
// 失败; 放在这里则只有这一个 Item 失败, PV 会自动退回纯 JS 实现。
import QtQuick
import LyricPvCore

Item {
    id: adapter

    // LyricPvCore 注册成了 QML 单例, 类型名本身就是那个实例。运行时据此把
    // 路径->SVG、颜色、矩阵这些重活转到 C++ (见 src/runtime/40_bridge.js)。
    readonly property var core: LyricPvCore
}