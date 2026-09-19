cmake_minimum_required(VERSION 3.16)

project(QueMusic VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_PATH_PREFIX ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 优化开关
# LTO：跨翻译单元内联 + 死代码消除（Release / RelWithDebInfo 生效）
option(QUEMUSIC_ENABLE_LTO "Release 构建启用 LTO/IPO" ON)
# QML 静态检查：只影响构建耗时，不影响运行性能
option(QUEMUSIC_QML_LINT "构建时对 QML 执行 qmllint" ON)
# qmltc：把可编译的 QML 提前编译成 C++ 类型（默认关闭，原因见 cmake/qmltc.cmake）
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

qt_add_executable(${PROJECT_NAME}
    main.cpp
    $<$<PLATFORM_ID:Windows>:icon.rc>
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
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

# 构建优化
# 函数/数据分节 + 链接期垃圾回收：剥离未引用代码，减小体积、改善加载与指令缓存
if(NOT MSVC AND NOT APPLE AND NOT CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$")
    target_compile_options(${PROJECT_NAME} PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--gc-sections)
endif()

# LTO：把 cpp/ 与 api/ 各翻译单元合并优化（Qt 自身动态库无法参与，收益集中在本工程代码）
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
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

file(GLOB_RECURSE QML_COMPONENTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "components/*.qml")
file(GLOB_RECURSE CENTER_COMPONENTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "centers/*.qml")

# 本模块包含的全部 QML/JS 文件。qmltc 的跳过清单必须在 qt6_add_qml_module()
# 解析参数之前设置好，所以这里先统一为变量
set(QUE_MUSIC_QML_FILES
    main.qml
    SettingsView.qml
    FullCenterView.qml
    layout/MainContent.qml
    layout/PlayerControl.qml
    layout/PlayerMaxCenter.qml
    layout/LeftSideBar.qml
    pages/HomePage.qml
    pages/PlaylistPage.qml
    pages/FavouritePage.qml
    pages/FilePage.qml
    pages/DownloadPage.qml
    pages/SearchPage.qml
    api/qrcode.js
    ${QML_COMPONENTS}
    ${CENTER_COMPONENTS}
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
        cpp/AppModels.h
        cpp/PlayerDatabase.cpp
        cpp/PlayerDatabase.h
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
        cpp/SearchResultModel.cpp
        cpp/SearchResultModel.h
        cpp/WindowsSmtcManager.cpp
        cpp/WindowsSmtcManager.h
        cpp/LogManager.cpp
        cpp/LogManager.h
        cpp/SystemTrayManager.cpp
        cpp/SystemTrayManager.h
        api/MusicApiService.cpp
        api/MusicApiService.h
        api/OnlineListModel.cpp
        api/OnlineListModel.h
        api/NeteaseCloudApi.cpp
        api/NeteaseCloudApi.h
        api/KugouApi.cpp
        api/KugouApi.h
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
)

qt6_add_shaders(${PROJECT_NAME} "bgshaders"
    PREFIX
        "/"
    BATCHABLE
    FILES
        shaders/background.vert
        shaders/fluid.frag
        shaders/classic.frag
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
        cryptopp::cryptopp
        quemusic::zlib
    PUBLIC
        QWindowKit::Quick
)
# Windows SMTC 互操作使用 RoGetActivationFactory / RoActivateInstance，需要 runtimeobject 导入库；
# 应用身份注册（AUMID 快捷方式/注册表）使用 COM 与注册表 API，需要 ole32/shell32/advapi32。
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE runtimeobject ole32 shell32 advapi32)

    # qwindowkit 通过 qmsetup 把自身 DLL 输出到 <build>/_deps/qwindowkit-build/out/bin，
    # 与可执行文件不在同一目录，运行时会找不到 QWKCore / QWKQuick。
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:QWindowKit::Core>
                $<TARGET_FILE:QWindowKit::Quick>
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying QWindowKit runtime libraries next to the executable...")
endif()

# macOS 上由 macdeployqt 统一部署；Debug 构建不部署 Qt 运行时
if(NOT APPLE AND NOT CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$")
    qt_deploy_runtime(${PROJECT_NAME})
endif()

enable_testing()
find_package(Qt6 REQUIRED COMPONENTS Test)
add_executable(quemusic_local_lyrics_test
    tests/local_lyrics_reader_test.cpp
    cpp/LocalLyricsReader.cpp
)
target_include_directories(quemusic_local_lyrics_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
)
target_link_libraries(quemusic_local_lyrics_test PRIVATE
    Qt6::Core
    Qt6::Test
    quemusic::taglib
)
add_test(NAME quemusic_local_lyrics_test COMMAND quemusic_local_lyrics_test)
