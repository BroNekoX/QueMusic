// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 单个功能插件的加载器：按 info.source 取入口组件，注入 PluginApi 后实例化。
// 插件根对象本身不参与显示（界面一律经 api.mount / api.loader 挂到扩展点）；
// 宿主停用或删除插件时，连插件挂出去的对象一起回收。
import QtQuick
import QueMusic 1.0

Item {
    id: loader

    width: 0
    height: 0
    visible: false

    property var info: null        // FunctionPlugins.plugins 中的一项
    property bool active: false    // 由宿主按启用状态绑定

    property QtObject api: null    // 插件接口实例（与插件根拿到的 api 是同一个）
    property var instance: null    // 插件根对象

    property bool _loading: false  // 入口组件正在异步编译

    Component.onCompleted: if (active) load()
    onActiveChanged: active ? load() : unload()
    Component.onDestruction: unload()

    Component {
        id: apiComponent
        PluginApi {}
    }

    // 插件是插件目录里的外部文件，只能按 url 动态取组件；异步编译，插件再多也不占首帧
    function load(): void {
        if (_loading || instance || !info || !info.source)
            return;
        _loading = true;
        const component = Qt.createComponent(info.source, Component.Asynchronous, loader);
        if (!component) {
            fail("入口文件读不到");
            return;
        }
        if (component.status === Component.Loading)
            component.statusChanged.connect(() => loader.finish(component));
        else
            finish(component);
    }

    function finish(component: var): void {
        if (!active || instance)
            return;   // 编译期间被停用/卸载了，别再把插件建起来
        if (component.status !== Component.Ready) {
            fail(component.errorString());
            return;
        }
        const iface = apiComponent.createObject(loader, {
            pluginId: info.id,
            pluginName: info.name,
            pluginDir: info.path,
            // 入口 source 形如 file:///…/plugin.qml，砍掉文件名就是插件目录的 URL
            pluginUrl: info.source.substring(0, info.source.lastIndexOf("/") + 1),
            window: loader.Window.window
        });
        if (!iface) {
            fail("无法创建插件接口");
            return;
        }
        api = iface;
        // 初始属性在 Component.onCompleted 之前生效，插件可以在 onCompleted 里直接 mount
        instance = component.createObject(loader, { api: iface });
        if (!instance) {
            unload();
            fail("插件根对象无法实例化（根需是 QtObject/Item 且声明 property QtObject api）");
            return;
        }
        if (instance.api !== iface) {   // 根对象漏写 `property QtObject api`：早点报错比静默不干活好
            unload();
            fail("插件根对象没有声明 property QtObject api");
            return;
        }
        // 可选钩子：需要等宿主界面完全就绪再挂载的插件写一个 activate()，宿主在注入后调用一次
        if (typeof instance.activate === "function")
            instance.activate();
    }

    function unload(): void {
        _loading = false;
        if (api) {
            api.destroyAll();
            api.destroy();
            api = null;
        }
        if (instance) {
            instance.destroy();
            instance = null;
        }
    }

    // 坏插件自动停用：否则每次启动都要弹一次错误
    function fail(text: string): void {
        unload();
        Style.warned("功能插件「" + (info && info.name ? info.name : "?") + "」加载失败：" + text, 0);
        if (info && info.id)
            FunctionPlugins.setEnabled(info.id, false);
    }
}
