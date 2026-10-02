// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

// 正文字体单例，同 IconFont：避免跨文件引用 main.qml 的根上下文 id。
pragma Singleton
import QtQuick

QtObject {
    readonly property FontLoader loader: FontLoader {
        source: "qrc:/QueMusic/resources/fonts/poppins.ttf"
    }

    readonly property string name: loader.name
}
