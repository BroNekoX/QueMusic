# SPDX-License-Identifier: MIT
#
# LyricPvCore —— 歌词 PV 的 C++ 计算内核。
#
# 单独做成一个 QML 模块 (URI: LyricPvCore) 而不是塞进 QueMusic 模块, 是为了让它
# 可复用: 任何"录制显示列表 -> 用 Shapes/Text 回放"的歌词 PV 效果, 只要
# `import LyricPvCore` 就能拿到路径->SVG、颜色、矩阵这些每帧都要跑的通用运算,
# 不必自己再实现一遍。
#
# 静态库 + 自有 QML 模块: 宿主 (QueMusic) 链接 LyricPvCore 与 LyricPvCoreplugin,
# 类型注册和 qmldir 都在链接时进入可执行文件, 不产生额外的运行期文件依赖。

qt_add_library(LyricPvCore STATIC)

qt_add_qml_module(LyricPvCore
    URI LyricPvCore
    VERSION 1.0
    SOURCES
        pvcore.h
        pvcore.cpp
        pvcolor.h
        pvcolor.cpp
        pvgeometry.h
        pvgeometry.cpp
)

target_include_directories(LyricPvCore
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 纯计算, 只需要 Core/Gui/Qml: 不碰 Quick, 免得把渲染依赖带进内核
target_link_libraries(LyricPvCore
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
)