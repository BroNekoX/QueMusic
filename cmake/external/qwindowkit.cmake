# qwindowkit.cmake
# 1) 优先系统包
find_package(QWindowKit QUIET)
if(QWindowKit_FOUND)
    message(STATUS "[MyApp] Using system QWindowKit")
    return()
endif()

# 2) fallback：ThirdParty 子模块
message(STATUS "[MyApp] Using ThirdParty/qwindowkit")

set(QWINDOWKIT_BUILD_QUICK    ON  CACHE BOOL "" FORCE)
set(QWINDOWKIT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QWINDOWKIT_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)

# 修复路径：使用相对于项目根目录的路径，而不是相对于当前cmake目录的路径
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../ThirdParty/qwindowkit
                 ${CMAKE_BINARY_DIR}/_deps/qwindowkit-build # 子级构建目录
                 EXCLUDE_FROM_ALL) #  排除库中的可执行程序构建
        