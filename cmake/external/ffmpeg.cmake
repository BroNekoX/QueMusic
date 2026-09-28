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
set(_qm_ffmpeg_pc_modules libavcodec libavformat libavutil libswresample)         # ← 新增

# 随仓库附带的那套是 Windows 共享库与导入库，仅在 Windows 上使用；
# 其他平台走系统开发包（pkg-config），否则会拿 .dll.a 去链接而失败
if(WIN32 AND EXISTS "${QUEMUSIC_FFMPEG_ROOT}/include/libavcodec/avcodec.h")
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
    "  其他路径: 配置时加 -DQUEMUSIC_FFMPEG_ROOT=<ffmpeg prefix>")
