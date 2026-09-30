cmake_minimum_required(VERSION 3.16)

project(QueMusic VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_PATH_PREFIX ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 构建开关（LTO / QML 静态检查 / qmltc；qmltc 为实验性，默认关闭，见 cmake/qmltc.cmake）
option(QUEMUSIC_ENABLE_LTO "Release 构建启用 LTO/IPO" ON)
option(QUEMUSIC_QML_LINT "构建时对 QML 执行 qmllint" ON)
option(QUEMUSIC_ENABLE_QMLTC "启用 qmltc 类型编译（实验性）" OFF)

# 统一输出目录：可执行文件与动态库放 bin/，静态库放 lib/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Network Multimedia Concurrent Sql ShaderTools Widgets)

if(COMMAND qt_policy)
    qt_policy(SET QTP0001 OLD)
endif()

# 第三方依赖（QWindowKit、TagLib、zlib）与部署辅助函数
add_subdirectory(cmake)

set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
qt_add_executable(${PROJECT_NAME}
    main.cpp
    $<$<PLATFORM_ID:Windows>:icon.rc>
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/webdav
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/taglib/taglib
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/taglib/taglib/toolkit
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/taglib/taglib/mpeg
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/taglib/taglib/mpeg/id3v2
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/taglib/taglib/mpeg/id3v2/frames
)

# 预编译 Qt 主头文件，降低全量编译时间
target_precompile_headers(${PROJECT_NAME} PRIVATE
    <QtCore/QtCore>
    <QtGui/QtGui>
    <QtQml/QtQml>
    <QtQuick/QtQuick>
    <QtNetwork/QtNetwork>
    <QtMultimedia/QtMultimedia>
)

# 函数/数据分节 + 链接期垃圾回收（剥离未引用代码）
if(NOT MSVC AND NOT APPLE AND NOT CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$")
    target_compile_options(${PROJECT_NAME} PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--gc-sections)
endif()

# Linux 也要把 QWindowKit 动态库放到 exe 旁边
if(NOT WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:QWindowKit::Core>
            $<TARGET_FILE:QWindowKit::Quick>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying QWindowKit runtime libraries next to the executable...")
endif()

# LTO：合并本工程各翻译单元（Qt 动态库不参与）
if(QUEMUSIC_ENABLE_LTO AND NOT CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT QUEMUSIC_IPO_SUPPORTED OUTPUT QUEMUSIC_IPO_ERROR LANGUAGES CXX)
    if(QUEMUSIC_IPO_SUPPORTED)
        set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
        message(STATUS "QueMusic: LTO 已启用")
    else()
        message(WARNING "QueMusic: 当前工具链不支持 LTO，已跳过（${QUEMUSIC_IPO_ERROR}）")
    endif()
endif()

# 按平台设置应用类型：Windows 用 GUI 子系统，macOS 生成 .app bundle
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "QueMusic"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.bronekox.quemusic"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
    # CI 中由 workflow 生成 icon.icns；本地不存在也不影响配置
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app/icon.icns")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE_ICON_FILE "icon.icns"
            RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/app/icon.icns"
        )
    endif()
endif()

set_source_files_properties(
    components/Style.qml
    components/Options.qml
    components/Playback.qml
    components/PluginSlots.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# QML 按目录递归收集：新增文件不必再改本文件
file(GLOB_RECURSE QML_COMPONENTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "components/*.qml")
file(GLOB_RECURSE LAYOUT_QML RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "layout/*.qml")
file(GLOB_RECURSE PAGE_QML RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "pages/*.qml")
# 歌词界面预设目录（可整体替换的歌词界面）
file(GLOB_RECURSE LYRICS_UI_QML RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "lyricsui/*.qml")

# 模块内全部 QML/JS；qmltc 跳过清单须在 qt6_add_qml_module() 解析参数前备好，故先统一为变量
set(QUE_MUSIC_QML_FILES
    main.qml
    SettingsView.qml
    api/qrcode.js
    ${LAYOUT_QML}
    ${LYRICS_UI_QML}
    ${PAGE_QML}
    ${QML_COMPONENTS}
    )

# qmltc 白名单 / 跳过清单（QUEMUSIC_ENABLE_QMLTC=ON 且布局满足条件时才真正启用）
set(QUE_MUSIC_QMLTC_ACTIVE FALSE)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/qmltc.cmake)

set(QUE_MUSIC_QML_MODULE_OPTIONS)
if(NOT QUEMUSIC_QML_LINT)
    list(APPEND QUE_MUSIC_QML_MODULE_OPTIONS NO_LINT)
endif()
if(QUE_MUSIC_QMLTC_ACTIVE)
    list(APPEND QUE_MUSIC_QML_MODULE_OPTIONS ENABLE_TYPE_COMPILER)
endif()

qt6_add_qml_module(${PROJECT_NAME} # 默认导入项目的
    URI QueMusic
    VERSION 1.0
    ${QUE_MUSIC_QML_MODULE_OPTIONS}
    QML_FILES
        ${QUE_MUSIC_QML_FILES}

    RESOURCES
        resources/window-bar/close.svg resources/window-bar/closed.svg
        resources/window-bar/airplay.svg resources/window-bar/airplayd.svg
        resources/window-bar/maximize.svg resources/window-bar/maximized.svg
        resources/window-bar/minimize.svg resources/window-bar/minimized.svg
        resources/window-bar/restore.svg resources/window-bar/restored.svg
        resources/window-bar/return.svg resources/window-bar/returnd.svg
        resources/window-bar/setting.svg resources/window-bar/settingd.svg
        resources/icon.ico
        resources/loader.gif
        resources/pic/back1.jpg
        resources/pic/back2.jpg
        resources/pic/back3.jpg
        resources/pic/cloudRainbow.png
        resources/app/header.png
        resources/app/rainbowMusicIcon.png
        resources/fonts/feather.ttf
        resources/fonts/poppins.ttf
        resources/app/musicpic.png
        resources/app/cd.png
        resources/app/cdpoint.png
        resources/app/icons/bronekox.jpg
        resources/app/icons/qticon.png
        resources/app/icons/qwk.png
    SOURCES
        shaders/MeshGradientItem.cpp
        shaders/MeshGradientItem.h
        shaders/LyricsStage.cpp
        shaders/LyricsStage.h
        cpp/AppModels.h
        cpp/PlayerDatabase.cpp
        cpp/PlayerDatabase.h
        cpp/DbService.cpp
        cpp/DbService.h
        cpp/CoverHelper.cpp
        cpp/CoverHelper.h
        cpp/ColorExtractor.cpp
        cpp/ColorExtractor.h
        cpp/GetWave.cpp
        cpp/GetWave.h
        cpp/FolderModel.cpp
        cpp/FolderModel.h
        cpp/DownloadManager.cpp
        cpp/DownloadManager.h
        cpp/DownloadedMusicModel.cpp
        cpp/DownloadedMusicModel.h
        cpp/Favorites.cpp
        cpp/Favorites.h
        cpp/AccountManager.cpp
        cpp/AccountManager.h
        cpp/LocalLyricsReader.cpp
        cpp/LocalLyricsReader.h
        cpp/CustomFlow.cpp
        cpp/CustomFlow.h
        cpp/QueueModel.cpp
        cpp/QueueModel.h
        cpp/LocalMusicScanner.cpp
        cpp/LocalMusicScanner.h
        cpp/webdav/WebDavCache.cpp
        cpp/webdav/WebDavCache.h
        cpp/webdav/WebDavClient.cpp
        cpp/webdav/WebDavClient.h
        cpp/webdav/WebDavModel.cpp
        cpp/webdav/WebDavModel.h
        cpp/webdav/WebDavStore.cpp
        cpp/webdav/WebDavStore.h
        cpp/plugins/PluginStore.cpp
        cpp/plugins/PluginStore.h
        cpp/plugins/LyricsPluginStore.cpp
        cpp/plugins/LyricsPluginStore.h
        cpp/plugins/FunctionPluginStore.cpp
        cpp/plugins/FunctionPluginStore.h
        cpp/SearchResultModel.cpp
        cpp/SearchResultModel.h
        cpp/WindowsSmtcManager.cpp
        cpp/WindowsSmtcManager.h
        cpp/LogManager.cpp
        cpp/LogManager.h
        cpp/SystemTrayManager.cpp
        cpp/SystemTrayManager.h
        cpp/audio/AudioEngine.cpp
        cpp/audio/AudioEngine.h
        cpp/audio/AudioDsp.cpp
        cpp/audio/AudioDsp.h
        cpp/audio/AudioRing.h
        cpp/audio/AudioSpectrumSink.h
        cpp/audio/FfmpegDecoder.cpp
        cpp/audio/FfmpegDecoder.h
        cpp/audio/PitchShifter.cpp
        cpp/audio/PitchShifter.h
        cpp/audio/TimeStretch.cpp
        cpp/audio/TimeStretch.h
        api/MusicApiService.cpp
        api/MusicApiService.h
        api/OnlineListModel.cpp
        api/OnlineListModel.h
        api/NeteaseCloudApi.cpp
        api/NeteaseCloudApi.h
        api/KugouApi.cpp
        api/KugouApi.h
        api/BilibiliApi.cpp
        api/BilibiliApi.h
        api/ApiCommon.h
)

qt6_add_shaders(${PROJECT_NAME} "effectshaders"
    PREFIX
        "/shaders"
    BATCHABLE
    FILES
        shaders/lyricfade.frag
        shaders/cardblur.frag
        shaders/cardblur_hq.frag
        shaders/bg_cubes.frag
        shaders/bg_grid.frag
        shaders/bg_beams.frag
        shaders/bg_stars.frag
        shaders/bg_stage.frag
        shaders/bg_void.frag
        shaders/lyricplane.frag
)

qt6_add_shaders(${PROJECT_NAME} "bgshaders"
    PREFIX
        "/"
    BATCHABLE
    FILES
        shaders/background.vert
        shaders/fluid.frag
        shaders/classic.frag
        shaders/stage.vert
        shaders/stage.frag
)

# QCloudMusicApi：网易云音乐在线接口（crypto++ 随其源码放在 api/QCloudMusicApi/3rdparty）
set(QCLOUDMUSICAPI_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(QCLOUDMUSICAPI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory(api/QCloudMusicApi)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/api/QCloudMusicApi/QCloudMusicApi)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Network
        Qt6::Multimedia
        Qt6::Concurrent
        Qt6::Widgets
        QCloudMusicApi
        quemusic::taglib
        quemusic::zlib
        quemusic::ffmpeg
        cryptopp::cryptopp
    PUBLIC
        QWindowKit::Quick
)
# Windows：SMTC 互操作（runtimeobject）与应用身份注册（COM / 注册表）
if(WIN32)
    # crypt32：WebDAV 密码用 DPAPI 加密存盘
    target_link_libraries(${PROJECT_NAME} PRIVATE runtimeobject ole32 shell32 advapi32 crypt32)

    # QWindowKit 的 DLL 输出在 _deps 下，需拷到 exe 同目录，否则运行时找不到 QWKCore/QWKQuick
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:QWindowKit::Core>
                $<TARGET_FILE:QWindowKit::Quick>
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying QWindowKit runtime libraries next to the executable...")

    # FFmpeg 运行库须项目自带（解码层直接链接 avcodec/avformat，Qt 部署不再提供）；
    # 缺目录直接报错，不静默跳过，否则应用能启动但无法播放
    if(NOT EXISTS "${QUEMUSIC_FFMPEG_ROOT}/bin")
        message(FATAL_ERROR
            "缺少 ${QUEMUSIC_FFMPEG_ROOT}/bin（FFmpeg 运行库）。"
            "解码层直接链接 avcodec/avformat，Qt 部署不再提供它们，须由项目自带。")
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${QUEMUSIC_FFMPEG_ROOT}/bin"
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying FFmpeg runtime libraries next to the executable...")
endif()

# macOS 由 macdeployqt 部署；Debug 不部署 Qt 运行时
if(NOT APPLE AND NOT CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$")
    qt_deploy_runtime(${PROJECT_NAME})

    # 删掉部署产物里无用的 Qt Multimedia FFmpeg 后端（全项目无 MediaPlayer，省约 1.7MB）；
    # 解码层要用的 avcodec/avformat/avutil/swresample 保留
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -f
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/multimedia/ffmpegmediaplugin.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/swscale-8.dll"
        COMMENT "Removing unused Qt Multimedia FFmpeg backend (saves ~1.7MB)...")
endif()



