// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
import QtQuick
import QtCore

QtObject {
    id: ctx

    property string pluginId: ""
    property string pluginName: ""
    property string pluginDir: ""
    property url pluginUrl: ""

    property Window hostWindow: null
    property Item hostTitleBar: null
    property Item hostPlayerBar: null
    property Item hostSidebar: null
    property Item hostOverlay: null

    readonly property Settings settings: Settings {
        category: "Plugins/settings/" + ctx.pluginId
    }

    // 插件挂出来的界面：停用时统一回收
    property var owned: []

    readonly property Component loaderTemplate: Component {
        Loader { asynchronous: true }
    }

    // source 传 Component（同步）或 url（异步 Loader）；props 是初始属性
    function attach(parent: Item, source: var, props: var): var {
        if (!parent || !source)
            return null;
        let item = null;
        if (typeof source === "string") {
            item = ctx.loaderTemplate.createObject(parent, {});
            if (item)
                item.setSource(source, props || {});
        } else {
            item = source.createObject(parent, props || {});
        }
        if (item)
            ctx.owned.push(item);
        return item;
    }

    function unmount(item: var): void {
        const i = ctx.owned.indexOf(item);
        if (i >= 0)
            ctx.owned.splice(i, 1);
        if (item)
            item.destroy();
    }

    Component.onDestruction: {
        for (let i = ctx.owned.length - 1; i >= 0; --i) {
            if (ctx.owned[i])
                ctx.owned[i].destroy();
        }
        ctx.owned = [];
    }

    function toast(text: string, type: int): void {
        Options.warned(text, type);
    }

    function warn(text: string): void {
        console.warn("[插件 " + ctx.pluginId + "] " + text);
    }
}