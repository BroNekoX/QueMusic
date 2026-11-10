# QWindowKit —— 跨平台无边框窗口框架，提供 QWindowKit::Quick

find_package(QWindowKit QUIET)

message(STATUS "[QueMusic] Using bundled QWindowKit from ThirdParty/qwindowkit")

set(QWINDOWKIT_BUILD_QUICK    ON  CACHE BOOL "" FORCE)
set(QWINDOWKIT_BUILD_WIDGETS  OFF CACHE BOOL "" FORCE)
set(QWINDOWKIT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QWINDOWKIT_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(QWINDOWKIT_BUILD_STATIC   ON  CACHE BOOL "" FORCE)
set(QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS OFF CACHE BOOL "" FORCE)

# Linux使用系统窗口
if(UNIX AND NOT APPLE)
    set(QWINDOWKIT_BUILD_STATIC ON CACHE BOOL "" FORCE)
endif()

get_filename_component(_qwindowkit_root
    "${CMAKE_CURRENT_LIST_DIR}/../../ThirdParty/qwindowkit" ABSOLUTE)
add_subdirectory("${_qwindowkit_root}"
                 "${CMAKE_BINARY_DIR}/_deps/qwindowkit-build"
                 EXCLUDE_FROM_ALL)
unset(_qwindowkit_root)
