# FFmpeg —— 音频后端的解码层（libavformat/libavcodec/libavutil/libswresample）
#
# 采用 Qt 6.10.3 自带的 FFmpeg 7.1.3（LGPL-2.1-or-later，动态链接，详见 THIRD_PARTY_NOTICES.md）。
# 目录约定：<repo>/ffmpeg/{include,lib,bin}，三部分均随仓库提供，构建无需联网下载。
# 也可用 -DQUEMUSIC_FFMPEG_ROOT=<prefix> 指定，或安装系统开发包让 pkg-config 找到。
#
# 对外导出 target：quemusic::ffmpeg
if(TARGET quemusic::ffmpeg)
    return()
endif()

set(QUEMUSIC_FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/ffmpeg"
    CACHE PATH "FFmpeg 根目录（包含 include 与 lib）")

set(_qm_ffmpeg_modules avcodec avformat avutil swresample)                        # find_library 用（保持不变）
set(_qm_ffmpeg_pc_modules libavcodec libavformat libavutil libswresample)         # pkg-config 用

set(_qm_ffmpeg_default_root "${CMAKE_SOURCE_DIR}/ffmpeg")
if(QUEMUSIC_FFMPEG_ROOT STREQUAL "${_qm_ffmpeg_default_root}")
    set(_qm_ffmpeg_root_explicit FALSE)
else()
    set(_qm_ffmpeg_root_explicit TRUE)   # 用户显式传了 -DQUEMUSIC_FFMPEG_ROOT=<prefix>
endif()

# 走「指定前缀里的 include/ + lib/」这条路的两种情况：
#   1) Windows：仓库自带的那套 dll + 导入库只在 Windows 上用（其他平台拿 .dll.a 链接必挂）；
#   2) 任何平台显式指定了 QUEMUSIC_FFMPEG_ROOT：尊重用户给的 ffmpeg 前缀。
if((WIN32 OR _qm_ffmpeg_root_explicit) AND EXISTS "${QUEMUSIC_FFMPEG_ROOT}/include/libavcodec/avcodec.h")
    foreach(_mod IN LISTS _qm_ffmpeg_modules)
        find_library(QUEMUSIC_FFMPEG_${_mod}_LIB
            NAMES ${_mod} lib${_mod} ${_mod}.lib lib${_mod}.dll.a ${_mod}.dll.a
            PATHS "${QUEMUSIC_FFMPEG_ROOT}/lib"
            NO_DEFAULT_PATH)
        if(NOT QUEMUSIC_FFMPEG_${_mod}_LIB)
            message(FATAL_ERROR
                "QueMusic: 在 ${QUEMUSIC_FFMPEG_ROOT}/lib 中找不到 ${_mod} 导入库。")
        endif()
        list(APPEND _qm_ffmpeg_libs "${QUEMUSIC_FFMPEG_${_mod}_LIB}")
    endforeach()

    add_library(quemusic::ffmpeg INTERFACE IMPORTED GLOBAL)
    set_target_properties(quemusic::ffmpeg PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${QUEMUSIC_FFMPEG_ROOT}/include"
        INTERFACE_LINK_LIBRARIES "${_qm_ffmpeg_libs}"
    )
    message(STATUS "[QueMusic] Using FFmpeg: ${QUEMUSIC_FFMPEG_ROOT}")
    return()
endif()

# ---- macOS：用官方 Qt 包里自带的那套 FFmpeg ----
# Qt 的 macOS 包会随 QtMultimedia 一起装 libavcodec.61/libavformat.61/libavutil.59/libswresample.5
# 的 .dylib（FFmpeg 7.1.3，universal），但只给动态库、不给公共头文件与 .pc，
# 所以既走不了 pkg-config，也不能像 Windows 那样用 .dll.a 导入库。
# 仓库里 ffmpeg/include 恰好是同一版本（libavcodec 61）的公共头文件，直接与之配对：
#   * 版本严格一致（和 Windows 那份同源），不会有 ABI 漂移；
#   * 这些 dylib 只依赖系统库与彼此（@rpath/@loader_path），随包只有几 MB。
if(APPLE AND EXISTS "${CMAKE_SOURCE_DIR}/ffmpeg/include/libavcodec/avcodec.h")
    get_filename_component(_qm_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)   # <Qt>/6.x.y/macos

    # 只有 Qt 前缀里确实带 libav*.dylib 才走这条路；自制/精简 Qt 可能没有，
    # 那就原样落到下面的 pkg-config（brew 的 ffmpeg）分支。
    if(EXISTS "${_qm_qt_prefix}/lib/libavcodec.dylib")
        set(_qm_ffmpeg_libs "")
        foreach(_mod IN LISTS _qm_ffmpeg_modules)
            find_library(QUEMUSIC_FFMPEG_${_mod}_LIB
                NAMES ${_mod} lib${_mod}
                PATHS "${_qm_qt_prefix}/lib"
                NO_DEFAULT_PATH)
            if(NOT QUEMUSIC_FFMPEG_${_mod}_LIB)
                message(FATAL_ERROR
                    "QueMusic: 在 ${_qm_qt_prefix}/lib 里找不到 ${_mod} 动态库。\n"
                    "  官方 Qt 包自带整套 FFmpeg 动态库，这里只找到一部分，安装包可能不完整。")
            endif()
            list(APPEND _qm_ffmpeg_libs "${QUEMUSIC_FFMPEG_${_mod}_LIB}")
        endforeach()

        add_library(quemusic::ffmpeg INTERFACE IMPORTED GLOBAL)
        set_target_properties(quemusic::ffmpeg PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/ffmpeg/include"
            INTERFACE_LINK_LIBRARIES "${_qm_ffmpeg_libs}"
        )
        message(STATUS "[QueMusic] Using FFmpeg: Qt 自带（${_qm_qt_prefix}/lib）")
        unset(_qm_qt_prefix)
        return()
    endif()

    message(STATUS "[QueMusic] Qt 安装里没有自带 libav*.dylib，改走系统 pkg-config")
    unset(_qm_qt_prefix)
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_FFMPEG QUIET IMPORTED_TARGET ${_qm_ffmpeg_pc_modules})
    if(TARGET PkgConfig::PC_FFMPEG)
        add_library(quemusic::ffmpeg INTERFACE IMPORTED GLOBAL)
        set_target_properties(quemusic::ffmpeg PROPERTIES
            INTERFACE_LINK_LIBRARIES PkgConfig::PC_FFMPEG
        )
        message(STATUS "[QueMusic] Using FFmpeg via pkg-config")
        return()
    endif()
endif()

message(FATAL_ERROR
    "FFmpeg not found. The audio backend decodes with libavcodec/libavformat/libavutil/libswresample.\n"
    "  Windows: ${QUEMUSIC_FFMPEG_ROOT} 应随仓库附带 include/、lib/、bin/，请确认该目录完整；\n"
    "           如需重建，运行 cmake/fetch_ffmpeg.ps1（仅维护用，正常构建无需联网）\n"
    "  Linux:   安装 libavcodec-dev libavformat-dev libavutil-dev libswresample-dev\n"
    "  macOS:   官方 Qt 包自带 libav*.dylib，通常无需额外安装；\n"
    "           若用的是自制 Qt，可 brew install ffmpeg 并让 pkg-config 能找到（.pc 在 $(brew --prefix)/lib/pkgconfig）")
