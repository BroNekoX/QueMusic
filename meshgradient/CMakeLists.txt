# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (c) 2024-2026 QueMusic Contributors
# Copyright (c) 2022-2024 AMLL Contributors
#
# quemusic_meshgradient —— 独立 Mesh Gradient 背景渲染组件
#
# 本组件衍生自 AMLL (Apple Music Like Lyrics)：
#   https://github.com/amll-dev/applemusic-like-lyrics
# 依据 GNU Affero General Public License v3.0 授权（见本目录 LICENSE）。
#
# 作为独立库 target 编译，QueMusic 主体（Apache-2.0）以链接方式使用，
# 二者保持"聚合（mere aggregation）"关系，许可证互不影响。
cmake_minimum_required(VERSION 3.16)
project(quemusic_meshgradient LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Network Concurrent)

qt6_add_library(quemusic_meshgradient STATIC
    MeshGradientItem.cpp
    MeshGradientItem.h
)

target_include_directories(quemusic_meshgradient PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(quemusic_meshgradient PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Network
    Qt6::Concurrent
)
