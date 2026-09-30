// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 功能插件接口：宿主加载每个插件时创建一个，作为初始属性注入插件根对象的 `api`。
// 插件只用这里的契约（扩展点 / Loader / 设置 / 提示），不必依赖宿主内部结构。
import QtQuick
import QtCore
import QueMusic 1.0

QtObject {
    id: api

    // ── 身份（宿主注入，插件只读）──
    property string pluginId: ""
    property string pluginName: ""
    property string pluginDir: ""      // 插件目录（本地路径，显示/打开目录用）
    property url pluginUrl: ""         // 插件目录的 URL，加载插件自带的 QML/图片用：api.pluginUrl + "X.qml"
    property var window: null          // 主窗口：建 Window / Popup 或给对象找 parent 时用
    readonly property int apiVersion: 1

    // 插件自己的配置（自动持久化，按插件 id 分开存，卸载重装不丢）
    readonly property Settings settings: Settings {
        category: "Plugins/settings/" + api.pluginId
    }

    // 插件挂出来的界面对象与自建 Loader，卸载时统一回收
    property var owned: []

    // ── 扩展点 ──
    // 取扩展点容器；宿主没开这个扩展点就返回 null
    function slot(name: string): var {
        return PluginSlots.item(name);
    }

    // 把 component 挂到扩展点上；props 是初始属性，在 Component.onCompleted 之前生效
    function mount(slotName: string, component: var, props: var): var {
        const target = PluginSlots.item(slotName);
        if (!target) {
            api.warn("扩展点不存在：" + slotName);
            return null;
        }
        const obj = component ? component.createObject(target, props || {}) : null;
        if (!obj) {
            api.warn("挂载到 " + slotName + " 失败");
            return null;
        }
        owned.push(obj);
        return obj;
    }

    // 主窗口下新建一个 Loader 加载界面；source 传 url 或 Component，异步加载不卡首帧
    function loader(source: var, props: var, parent: var): var {
        if (!source) {
            api.warn("loader() 缺少 source");
            return null;
        }
        const host = parent || PluginSlots.item("window.overlay") || api.window;
        const item = api.loaderTemplate.createObject(host, props || {});
        if (!item) {
            api.warn("创建 Loader 失败");
            return null;
        }
        if (typeof source === "string")
            item.source = source;
        else
            item.sourceComponent = source;
        owned.push(item);
        return item;
    }

    // 立刻回收一个挂出来的对象（不必等到插件卸载）
    function unmount(obj: var): void {
        const i = owned.indexOf(obj);
        if (i >= 0)
            owned.splice(i, 1);
        if (obj)
            obj.destroy();
    }

    // 宿主卸载插件时调用：回收插件挂出来的全部界面
    function destroyAll(): void {
        for (let i = owned.length - 1; i >= 0; --i) {
            const obj = owned[i];
            if (obj)
                obj.destroy();
        }
        owned = [];
    }

    // 接口销毁（宿主关掉插件 / 退出）时兜底回收，防止插件挂出去的界面变成孤儿
    Component.onDestruction: destroyAll()

    // ── 便捷 ──
    function toast(text: string, type: int): void { Style.warned(text, type); }
    function warn(text: string): void { console.warn("[插件 " + api.pluginId + "] " + text); }

    // 新建 Loader 的模板（QtObject 没有默认属性，只能写成属性值，不能当子对象）
    readonly property Component loaderTemplate: Component {
        Loader { asynchronous: true }
    }
}
