// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
pragma Singleton
import QtQuick
import QueMusic 1.0

QtObject {
    property int playerMode: 0
    property Loader player: Loader {
        active: false
        visible: active
        sourceComponent: DesktopPlayerWindow {}
    }
    property Loader lyrics: Loader {
        active: false
        visible: active
        sourceComponent: DesktopLyrics {}
    }

    function setMode(mode: int): void {
        playerMode = mode
        player.active = mode === 1
        lyrics.active = mode === 2
    }

    function closeAll(): void {
        setMode(0)
    }
}
