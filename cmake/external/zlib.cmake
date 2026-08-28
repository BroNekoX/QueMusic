# zlib —— api/KugouApi.cpp 解码 KRC 歌词需要 uncompress()
#
# CMake 自带的 FindZLIB 只搜索系统库目录，不会搜索编译器自带的 sysroot，
# 而 LLVM-MinGW 之类的工具链并不附带 zlib，所以这里做分级查找：
#   1) 系统 zlib，或用 -DZLIB_ROOT 指定
#   2) 同一 Qt 安装目录下其他 MinGW 工具链附带的 libz.a（纯 C 静态库，可跨工具链链接）
#
# 对外导出 target：quemusic::zlib
if(TARGET quemusic::zlib)
    return()
endif()

find_package(ZLIB QUIET)

if(ZLIB_FOUND)
    add_library(quemusic::zlib INTERFACE IMPORTED GLOBAL)
    set_target_properties(quemusic::zlib PROPERTIES
        INTERFACE_LINK_LIBRARIES ZLIB::ZLIB
    )
    return()
endif()

find_library(QUEMUSIC_ZLIB_LIBRARY
    NAMES z zlib zlib1
    HINTS "${ZLIB_ROOT}" "$ENV{ZLIB_ROOT}" "${CMAKE_SYSROOT}"
    PATH_SUFFIXES lib usr/lib
)

# 注意用 WIN32 而不是 MINGW 判断：LLVM-MinGW 使用 Clang，CMake 不会置位 MINGW
if(NOT QUEMUSIC_ZLIB_LIBRARY AND WIN32)
    get_filename_component(_qt_tools_dir "${Qt6_DIR}/../../../../../Tools" ABSOLUTE)
    file(GLOB _mingw_libz "${_qt_tools_dir}/*/x86_64-w64-mingw32/lib/libz.a")
    if(_mingw_libz)
        list(GET _mingw_libz 0 QUEMUSIC_ZLIB_LIBRARY)
    endif()
    unset(_mingw_libz)
    unset(_qt_tools_dir)
endif()

if(NOT QUEMUSIC_ZLIB_LIBRARY)
    message(FATAL_ERROR
        "zlib not found, but KugouApi requires it to decode KRC lyrics. "
        "Install zlib, or configure with -DZLIB_ROOT=<zlib prefix>.")
endif()

add_library(quemusic::zlib STATIC IMPORTED GLOBAL)
set_target_properties(quemusic::zlib PROPERTIES
    IMPORTED_LOCATION "${QUEMUSIC_ZLIB_LIBRARY}"
)
message(STATUS "[QueMusic] Using zlib: ${QUEMUSIC_ZLIB_LIBRARY}")
