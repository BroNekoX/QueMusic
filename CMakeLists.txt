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

qt_add_executable(${PROJECT_NAME}
    main.cpp
    icon.rc
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE FALSE
)

set_source_files_properties(
    components/Style.qml
    components/Options.qml
    components/MusicApi.qml
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
        api/musicWorker.mjs
        api/kugouapi.mjs
        api/necloudapi.mjs
        api/pako.mjs
        ${QML_COMPONENTS}

    RESOURCES
        resources/window-bar/close.svg resources/window-bar/closed.svg
        resources/window-bar/airplay.svg resources/window-bar/airplayd.svg
        resources/window-bar/maximize.svg resources/window-bar/maximized.svg
        resources/window-bar/minimize.svg resources/window-bar/minimized.svg
        resources/window-bar/restore.svg resources/window-bar/restored.svg
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
        resources/window-bar/return.svg resources/window-bar/returnd.svg
        resources/window-bar/setting.svg resources/window-bar/settingd.svg
        resources/app/musicpic.png
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
)

qt6_add_shaders(${PROJECT_NAME} "effectshaders"
    PREFIX
        "/shaders"
    FILES
        resources/app/shaders/mesh.frag
        resources/app/shaders/mesh.vert
)


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

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
    PUBLIC QWindowKit::Quick
)

qt_deploy_runtime(${PROJECT_NAME})