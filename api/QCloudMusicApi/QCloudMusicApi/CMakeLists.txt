cmake_minimum_required(VERSION 3.14)

project(QCloudMusicApi LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Gui)

FILE(GLOB HEADERS ./*.h ./util/*.h)
FILE(GLOB SOURCES ./*.cpp ./util/*.cpp)
if(QCLOUDMUSICAPI_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED
        ${HEADERS}
        ${SOURCES}
    )
else()
    add_library(${PROJECT_NAME} STATIC
        ${HEADERS}
        ${SOURCES}
    )
endif()

if (ANDROID)
    include(FetchContent)
    FetchContent_Declare(
        android_openssl
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
        )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)

    add_android_openssl_libraries(${PROJECT_NAME})
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Gui
    cryptopp::cryptopp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    QCLOUDMUSICAPI_LIBRARY
    PROJECT_NAME="${PROJECT_NAME}"
)
if(QCLOUDMUSICAPI_BUILD_SHARED)
    target_compile_definitions(${PROJECT_NAME} PUBLIC QCLOUDMUSICAPI_SHARED)
endif()
