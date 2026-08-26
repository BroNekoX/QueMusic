cmake_minimum_required(VERSION 3.16)

project(QueMusic VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置所有配置的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)  # 可执行文件和Windows DLL
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)  # Linux/macOS 动态库
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)  # 静态库

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

add_subdirectory(cmake)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Network Multimedia Concurrent Sql ShaderTools)
#add_subdirectory(api)
#include_directories(api)

# 独立 Mesh Gradient 组件（AGPL-3.0，衍生自 AMLL）：以独立库 target 聚合
add_subdirectory(meshgradient)

qt_add_executable(${PROJECT_NAME}
    main.cpp
    $<$<PLATFORM_ID:Windows>:icon.rc>
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api
)

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
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

file(GLOB_RECURSE QML_COMPONENTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "components/*.qml")


qt6_add_qml_module(${PROJECT_NAME} # 默认导入项目的
    URI QueMusic
    VERSION 1.0
    QML_FILES
        main.qml
        SettingsView.qml
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
        resources/app/icons/amll.svg
        resources/app/icons/qticon.png
        resources/app/icons/qwk.png
    SOURCES
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
        cpp/Favorites.cpp
        cpp/Favorites.h
        cpp/AccountManager.cpp
        cpp/AccountManager.h
        cpp/CustomFlow.cpp
        cpp/CustomFlow.h
        cpp/WindowsSmtcManager.cpp
        cpp/WindowsSmtcManager.h
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
        resources/app/shaders/lyricfade.frag
        meshgradient/shaders/meshgradient.frag
        meshgradient/shaders/meshgradient.vert
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# QCloudMusicApi：第三方网易云音乐在线接口
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
        QCloudMusicApi
        cryptopp::cryptopp
        quemusic_meshgradientplugin
        z
    PUBLIC QWindowKit::Quick
)
# Windows SMTC 互操作使用 RoGetActivationFactory / RoActivateInstance，需要 runtimeobject 导入库
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE runtimeobject)
endif()

# macOS 上由 macdeployqt 统一部署，避免与 qt_deploy_runtime 冲突
if(NOT APPLE)
    qt_deploy_runtime(${PROJECT_NAME})
endif()