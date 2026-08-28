# QWindowKit —— 跨平台无边框窗口框架，提供 QWindowKit::Quick
#
# 优先使用系统已安装的 QWindowKit，否则回退到仓库内的子模块。
if(TARGET QWindowKit::Quick)
    return()
endif()

find_package(QWindowKit QUIET)

if(QWindowKit_FOUND)
    message(STATUS "[QueMusic] Using system QWindowKit")
    return()
endif()

message(STATUS "[QueMusic] Using bundled QWindowKit from ThirdParty/qwindowkit")

set(QWINDOWKIT_BUILD_QUICK    ON  CACHE BOOL "" FORCE)
set(QWINDOWKIT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QWINDOWKIT_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS OFF CACHE BOOL "" FORCE)

# QWindowKit 以动态库提供，运行时需与可执行文件放在一起
#（见主 CMakeLists.txt 中的 POST_BUILD 复制步骤）
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)

get_filename_component(_qwindowkit_root
    "${CMAKE_CURRENT_LIST_DIR}/../../ThirdParty/qwindowkit" ABSOLUTE)
add_subdirectory("${_qwindowkit_root}"
                 "${CMAKE_BINARY_DIR}/_deps/qwindowkit-build"
                 EXCLUDE_FROM_ALL)
unset(_qwindowkit_root)
