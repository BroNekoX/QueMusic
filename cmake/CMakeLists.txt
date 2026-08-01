# 负责include库，索引所有头文件和库，提供完整的环境。主cmake负责link即可
# 各.cmake只负责构建和提供头文件和库。

cmake_minimum_required(VERSION 3.16)
project(cmakehelper VERSION 1.0 LANGUAGES NONE)

include(external/qwindowkit.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/qtruntime.cmake)
