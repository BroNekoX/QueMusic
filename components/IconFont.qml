// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

// 图标字体单例。以前各文件靠 main.qml 的根上下文 id（iconFont）跨文件取名字，
// 这种动态查找 AOT 编译不了（也无法静态校验）；换成单例后 font.family: IconFont.name 可编译。
pragma Singleton
import QtQuick

QtObject {
    readonly property FontLoader loader: FontLoader {
        source: "qrc:/QueMusic/resources/fonts/feather.ttf"
    }

    readonly property string name: loader.name
}
