# qwindowkit.cmake
# 1) 优先系统包
find_package(QWindowKit QUIET)
if(QWindowKit_FOUND)
    message(STATUS "[MyApp] Using system QWindowKit")
    return()
endif()

# 2) fallback：ThirdParty 子模块
message(STATUS "[MyApp] Using ThirdParty/qwindowkit")

# 重要：qwindowkit 内部 qmsetup 通过 execute_process 启动独立 cmake 子进程
# 查找 Qt，它不会自动继承主项目的 CMAKE_PREFIX_PATH。
# 这里显式把 Qt 路径通过环境变量 QTDIR / QT_DIR 传给 qmsetup 子进程，
# 并设置 CMake 变量，确保 find_package(QT) 能找到 Qt6Config.cmake。
if(QT_DIR AND NOT DEFINED ENV{QTDIR})
    set(ENV{QTDIR} "${QT_DIR}")
endif()
if(QT_DIR AND NOT DEFINED ENV{QT_DIR})
    set(ENV{QT_DIR} "${QT_DIR}")
endif()
if(CMAKE_PREFIX_PATH AND NOT DEFINED QMSETUP_QT_PATH)
    set(QMSETUP_QT_PATH "${CMAKE_PREFIX_PATH}" CACHE PATH "Path to Qt for qmsetup" FORCE)
endif()

set(QWINDOWKIT_BUILD_QUICK    ON  CACHE BOOL "" FORCE)
set(QWINDOWKIT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QWINDOWKIT_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)

# 修复路径：使用相对于项目根目录的路径，而不是相对于当前cmake目录的路径
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../ThirdParty/qwindowkit
                 ${CMAKE_BINARY_DIR}/_deps/qwindowkit-build # 子级构建目录
                 EXCLUDE_FROM_ALL) #  排除库中的可执行程序构建
        