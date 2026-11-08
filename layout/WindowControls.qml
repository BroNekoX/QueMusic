// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 主窗口生命周期：托盘隐藏/恢复、关闭前保存会话与窗口几何、退出流程。
// 会话保存 / 退出 / 窗口可见性记忆均已下沉 SessionController（C++ 单例），
// 托盘气泡提示下沉 SystemTrayManager.notifyTrayHidden()。
import QtQuick
import QueMusic 1.0

QtObject {
    id: root

    property Window targetWindow

    property bool forceQuit: false

    signal sessionClosing()

    function closeToTray(): bool {
        return Config.closeToManage && !forceQuit && SystemTrayManager.available
    }

    function toClosing(): void {
        if (closeToTray()) {
            hideToTray()
            return
        }
        sessionClosing()
        // 会话保存（LastMedia 五键 / 窗口几何 / 播放队列）+ 退出都由 C++ 完成
        SessionController.quitApp(targetWindow)
    }

    function hideToTray(): void {
        if (!targetWindow || !targetWindow.visible)
            return
        // 隐藏前把可见性记忆交给常驻层（托盘恢复时按此恢复最大化/全屏形态）
        SessionController.markHidden(targetWindow)
        targetWindow.hide()
        // 隐藏后主动释放场景图资源（纹理/几何缓冲）：此路径引擎与窗口都保留，
        // 不释放则托盘常驻期间一直占着显存/纹理；重新显示时 Qt 会自动重建所需资源
        targetWindow.releaseResources()
        // 一次性气泡提示（内部记录已提示标志）
        SystemTrayManager.notifyTrayHidden()
    }

    function quitApp(): void {
        forceQuit = true
        toClosing()
    }

    property int startupDelay: 400

    Component.onCompleted: {
        const file = firstAudioArg(Application.arguments)
        if (file) {
            kick.file = file
            kick.start()
        }
    }

    property Timer kick: Timer {
        id: kick
        property string file: ""
        interval: root.startupDelay
        repeat: false
        onTriggered: root.playStartup(file)
    }

    function firstAudioArg(args) {
        if (!args) return ""
        const exts = ["mp3", "flac", "wav", "m4a", "aac", "ogg", "oga", "opus",
                      "ape", "wma", "aiff", "aif", "mka"]
        for (let i = 1; i < args.length; ++i) {
            const arg = String(args[i]).trim()
            if (!arg || arg.charAt(0) === "-") continue
            const cut = Math.max(arg.lastIndexOf("/"), arg.lastIndexOf("\\"))
            const name = arg.substring(cut + 1).split("?")[0]
            const dot = name.lastIndexOf(".")
            if (dot > 0 && exts.indexOf(name.substring(dot + 1).toLowerCase()) >= 0)
                return arg
        }
        return ""
    }

    function playStartup(file) {
        const cut = Math.max(file.lastIndexOf("/"), file.lastIndexOf("\\"))
        const name = file.substring(cut + 1).split("?")[0]
        let title = name
        try { title = decodeURIComponent(name) } catch (e) {}
        Playback.playLocalSong(file, title)
    }
}
