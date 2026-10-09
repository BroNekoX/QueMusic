// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 主窗口生命周期：托盘隐藏/恢复、关闭前保存会话与窗口几何、退出流程。
import QtQuick
import QueMusic 1.0

QtObject {
    id: root

    property Window targetWindow
    property SystemTrayManager tray

    property bool forceQuit: false
    property int restoreVisibility: 0
    property bool trayTipShown: false

    signal sessionClosing()

    function closeToTray(): bool {
        return Config.closeToManage && !forceQuit && tray && tray.available
    }

    function toClosing(): void {
        if (closeToTray()) {
            hideToTray()
            return
        }
        const q = Playback.queue
        const p = Playback.player
        const idx = q ? q.playListIndex : -1
        if (q && q.count > 0 && idx >= 0) {
            const item = q.get(idx)
            Options.lastSongs.name = Playback.musicTitle
            Options.lastSongs.artist = Playback.musicArtist
            Options.lastSongs.cover = (p && p.urlStr) || "qrc:/QueMusic/resources/app/musicpic.png"
            Options.lastSongs.hash = item.path
            Options.lastSongs.source = item.source
        }
        if (Config.rememberWindow && targetWindow
                && targetWindow.visibility !== Window.Maximized
                && targetWindow.visibility !== Window.FullScreen) {
            Config.winX = targetWindow.x
            Config.winY = targetWindow.y
            Config.winW = targetWindow.width
            Config.winH = targetWindow.height
        }
        Playback.saveQueue()
        Playback.flush()
        sessionClosing()
        if (tray)
            tray.quitApplication()
    }

    function hideToTray(): void {
        if (!targetWindow || !targetWindow.visible)
            return
        restoreVisibility = targetWindow.visibility
        targetWindow.hide()
        if (!trayTipShown) {
            trayTipShown = true
            if (tray)
                tray.showMessage("QueMusic", "已最小化到系统托盘，点击托盘图标可恢复",
                                 SystemTrayManager.Information)
        }
    }

    function restoreWindow(): void {
        if (!targetWindow)
            return
        if (!targetWindow.visible) {
            if (restoreVisibility === Window.Maximized)
                targetWindow.showMaximized()
            else if (restoreVisibility === Window.FullScreen)
                targetWindow.showFullScreen()
            else
                targetWindow.showNormal()
        } else if (targetWindow.visibility === Window.Minimized) {
            targetWindow.showNormal()
        }
        targetWindow.raise()
        targetWindow.requestActivate()
    }

    function quitApp(): void {
        forceQuit = true
        toClosing()
    }
}
