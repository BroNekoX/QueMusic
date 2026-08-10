// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// QRCodeView — 用 Canvas 渲染二维码文本
// qrText 变化时自动重绘；二维码由 qrcode.js（轻量实现）生成
import QtQuick
import "qrc:/QueMusic/api/qrcode.js" as QRCode

Canvas {
    id: root

    property string qrText: ""
    property color darkColor: "#111111"
    property color lightColor: "#ffffff"

    onQrTextChanged: {
        requestPaint();
    }
    Component.onCompleted: requestPaint();

    onPaint: {
        var ctx = getContext("2d");
        ctx.clearRect(0, 0, width, height);
        if (!root.qrText) {
            ctx.fillStyle = root.lightColor;
            ctx.fillRect(0, 0, width, height);
            return;
        }
        var modules = null;
        try {
            modules = QRCode.qrcode(root.qrText, "M");
        } catch (e) {
            console.warn("QRCodeView: 二维码生成失败", e);
            ctx.fillStyle = root.lightColor;
            ctx.fillRect(0, 0, width, height);
            return;
        }
        var n = modules.length;
        var cell = Math.max(1, Math.floor(Math.min(width, height) / n));
        var size = cell * n;
        var ox = Math.floor((width - size) / 2);
        var oy = Math.floor((height - size) / 2);

        ctx.fillStyle = root.lightColor;
        ctx.fillRect(0, 0, width, height);
        ctx.fillStyle = root.darkColor;
        for (var r = 0; r < n; r++) {
            for (var c = 0; c < n; c++) {
                if (modules[r][c]) {
                    ctx.fillRect(ox + c * cell, oy + r * cell, cell, cell);
                }
            }
        }
    }
}
