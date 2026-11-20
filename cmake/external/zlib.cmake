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

# 头文件路径：必须与 libz 配套，且不能把工具链 include 整目录加进搜索路径
# （Windows 上会遮蔽 libc++ 的 stddef.h/stdint.h）；Windows 只取 zlib.h/zconf.h。
set(_qm_zlib_include_dir "")
if(WIN32)
    get_filename_component(_qm_zlib_libdir "${QUEMUSIC_ZLIB_LIBRARY}" DIRECTORY)
    get_filename_component(_qm_zlib_sysroot "${_qm_zlib_libdir}" DIRECTORY)
    set(_qm_zlib_src_include "${_qm_zlib_sysroot}/include")
    if(EXISTS "${_qm_zlib_src_include}/zlib.h" AND EXISTS "${_qm_zlib_src_include}/zconf.h")
        set(_qm_zlib_include_dir "${CMAKE_BINARY_DIR}/zlib-include")
        file(MAKE_DIRECTORY "${_qm_zlib_include_dir}")
        configure_file("${_qm_zlib_src_include}/zlib.h" "${_qm_zlib_include_dir}/zlib.h" COPYONLY)
        configure_file("${_qm_zlib_src_include}/zconf.h" "${_qm_zlib_include_dir}/zconf.h" COPYONLY)
    endif()
else()
    get_filename_component(_qm_zlib_libdir "${QUEMUSIC_ZLIB_LIBRARY}" DIRECTORY)
    get_filename_component(_qm_zlib_prefix "${_qm_zlib_libdir}" DIRECTORY)
    foreach(_qm_dir IN ITEMS
            "${ZLIB_ROOT}/include"
            "$ENV{ZLIB_ROOT}/include"
            "${_qm_zlib_prefix}/include"
            "/usr/include"
            "/usr/local/include")
        if(EXISTS "${_qm_dir}/zlib.h")
            set(_qm_zlib_include_dir "${_qm_dir}")
            break()
        endif()
    endforeach()
endif()

if(NOT _qm_zlib_include_dir)
    message(FATAL_ERROR
        "zlib.h not found. Install zlib development headers (e.g. zlib1g-dev on Debian/Ubuntu) "
        "or configure with -DZLIB_ROOT=<zlib prefix>.")
endif()

set_target_properties(quemusic::zlib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_qm_zlib_include_dir}"
)
message(STATUS "[QueMusic] Using zlib: ${QUEMUSIC_ZLIB_LIBRARY} (${_qm_zlib_include_dir})")
