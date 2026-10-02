// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

// 功能插件宿主：放进主窗口末尾（此时扩展点已登记）。按启用状态给每个插件起一个加载器，
// 入口异步编译；停用/删除时把插件挂出来的界面一起回收，加载失败自动停用。
import QtQuick
import QueMusic 1.0

Item {
    id: host

    width: 0
    height: 0
    visible: false

    // 插件实例建好了：宿主据此做一次界面层收尾（标题栏扩展点要标记可命中）
    signal pluginLoaded()

    Component {
        id: apiComponent
        PluginApi {}
    }

    Repeater {
        model: FunctionPlugins.plugins

        delegate: Item {
            id: holder

            width: 0
            height: 0
            visible: false

            property QtObject api: null
            property var instance: null
            property bool loading: false
            property bool wanted: FunctionPlugins.enabledIds.indexOf(modelData.id) >= 0

            Component.onCompleted: if (holder.wanted) holder.load()
            onWantedChanged: holder.wanted ? holder.load() : holder.unload()
            Component.onDestruction: holder.unload()

            function load(): void {
                if (holder.loading || holder.instance || !modelData.source)
                    return;
                holder.loading = true;
                const component = Qt.createComponent(modelData.source, Component.Asynchronous, holder);
                if (!component) {
                    holder.fail("入口文件读不到");
                    return;
                }
                if (component.status === Component.Loading)
                    component.statusChanged.connect(() => holder.finish(component));
                else
                    holder.finish(component);
            }

            function finish(component: var): void {
                if (!holder.wanted || holder.instance)
                    return;   // 编译期间被停用
                if (component.status !== Component.Ready) {
                    holder.fail(component.errorString());
                    return;
                }
                holder.api = apiComponent.createObject(holder, {
                    pluginId: modelData.id,
                    pluginName: modelData.name,
                    pluginDir: modelData.path,
                    pluginUrl: modelData.source.substring(0, modelData.source.lastIndexOf("/") + 1),
                    window: holder.Window.window
                });
                if (!holder.api) {
                    holder.fail("无法创建插件接口");
                    return;
                }
                holder.instance = component.createObject(holder, { api: holder.api });
                if (!holder.instance || holder.instance.api !== holder.api) {
                    holder.unload();
                    holder.fail("插件根需声明 property QtObject api");
                    return;
                }
                if (typeof holder.instance.activate === "function")
                    holder.instance.activate();
                host.pluginLoaded();
            }

            function unload(): void {
                holder.loading = false;
                if (holder.api) {
                    holder.api.destroyAll();
                    holder.api.destroy();
                    holder.api = null;
                }
                if (holder.instance) {
                    holder.instance.destroy();
                    holder.instance = null;
                }
            }

            function fail(text: string): void {
                holder.unload();
                Style.warned("功能插件「" + modelData.name + "」加载失败：" + text, 0);
                FunctionPlugins.setEnabled(modelData.id, false);
            }
        }
    }
}
