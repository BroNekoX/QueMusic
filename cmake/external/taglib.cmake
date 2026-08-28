# TagLib —— 随仓库提供的本地音乐元数据解析库
#
# vendored 是为了让各平台的元数据解析行为一致，不依赖发行版自带的包。
#
# 对外导出 target：quemusic::taglib（静态库 tag + 头文件搜索路径）
if(TARGET quemusic::taglib)
    return()
endif()

get_filename_component(_taglib_root "${CMAKE_CURRENT_LIST_DIR}/../../ThirdParty/taglib" ABSOLUTE)

set(BUILD_BINDINGS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# QWindowKit 会改写全局的 BUILD_SHARED_LIBS，这里固定 TagLib 为静态构建，
# 部署时就不必再额外带一个 libtag 动态库。
set(_quemusic_build_shared_libs ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
add_subdirectory("${_taglib_root}" "${CMAKE_BINARY_DIR}/_deps/taglib-build" EXCLUDE_FROM_ALL)
set(BUILD_SHARED_LIBS ${_quemusic_build_shared_libs})
unset(_quemusic_build_shared_libs)

add_library(quemusic_taglib INTERFACE)
target_include_directories(quemusic_taglib INTERFACE
    "${_taglib_root}/taglib"
    "${_taglib_root}/taglib/toolkit"
    "${_taglib_root}/taglib/mpeg"
    "${_taglib_root}/taglib/mpeg/id3v2"
    "${_taglib_root}/taglib/mpeg/id3v2/frames"
)
target_link_libraries(quemusic_taglib INTERFACE tag)

add_library(quemusic::taglib ALIAS quemusic_taglib)
unset(_taglib_root)
