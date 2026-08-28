# 第三方依赖统一在此导入，主 CMakeLists.txt 只负责定义 target 并链接。
# 每个 external/*.cmake 只负责构建依赖并导出 target，不改动子模块源码。

cmake_minimum_required(VERSION 3.16)
project(cmakehelper VERSION 1.0 LANGUAGES NONE)

include(external/zlib.cmake)
include(external/taglib.cmake)
include(external/qwindowkit.cmake)
include(qtruntime.cmake)
