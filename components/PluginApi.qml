// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

// 功能插件接口：宿主为每个插件创建一个，作为初始属性注入插件根的 api。
import QtQuick
import QtCore
import QueMusic 1.0

QtObject {
    id: api

    property string pluginId: ""
    property string pluginName: ""
    property string pluginDir: ""      // 插件目录（本地路径，显示用）
    property url pluginUrl: ""         // 插件目录的 URL，加载插件自带资源用
    property var window: null
    readonly property int apiVersion: 1

    readonly property Settings settings: Settings {
        category: "Plugins/settings/" + api.pluginId
    }

    // 插件挂出来的对象，停用时统一回收
    property var owned: []

    readonly property Component loaderTemplate: Component {
        Loader { asynchronous: true }
    }

    function slot(name: string): var {
        return PluginSlots.item(name);
    }

    // source 传 Component（同步创建）或 url（异步 Loader）；props 是初始属性
    function mount(slotName: string, source: var, props: var): var {
        const target = PluginSlots.item(slotName);
        if (!target) {
            api.warn("扩展点不存在：" + slotName);
            return null;
        }
        if (!source) {
            api.warn("mount() 缺少 source");
            return null;
        }
        const byUrl = typeof source === "string";
        const obj = byUrl ? api.loaderTemplate.createObject(target, {})
                          : source.createObject(target, props || {});
        if (!obj) {
            api.warn("挂载到 " + slotName + " 失败");
            return null;
        }
        // url 挂载建出来的是 Loader：属性要交给 setSource 作为 item 的初始属性，
        // 直接设到 Loader 上会报「Loader does not have a property called …」而丢失
        if (byUrl)
            obj.setSource(source, props || {});
        owned.push(obj);
        return obj;
    }

    function unmount(obj: var): void {
        const i = owned.indexOf(obj);
        if (i >= 0)
            owned.splice(i, 1);
        if (obj)
            obj.destroy();
    }

    function destroyAll(): void {
        for (let i = owned.length - 1; i >= 0; --i) {
            if (owned[i])
                owned[i].destroy();
        }
        owned = [];
    }

    Component.onDestruction: destroyAll()

    function toast(text: string, type: int): void { Style.warned(text, type); }
    function warn(text: string): void { console.warn("[插件 " + api.pluginId + "] " + text); }
}
