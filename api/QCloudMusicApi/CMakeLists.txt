cmake_minimum_required(VERSION 3.14)

project(QCloudMusicApiProject VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

option(QCLOUDMUSICAPI_BUILD_TEST "Build QCloudMusicApi tests" ON)
option(QCLOUDMUSICAPI_BUILD_SHARED "Build QCloudMusicApi as a shared library" ON)

# 设置第三方仓库
include_directories(3rdparty)
set(CRYPTOPP_BUILD_TESTING OFF)
set(CRYPTOPP_USE_MASTER_BRANCH ON)
add_subdirectory(3rdparty/cryptopp-cmake/)

add_subdirectory(QCloudMusicApi)
include_directories(QCloudMusicApi)
if(QCLOUDMUSICAPI_BUILD_TEST)
    set(WITH_TOOLS OFF CACHE BOOL "Build utility tools" FORCE)
    add_subdirectory(3rdparty/libqrencode)
    include_directories(3rdparty/libqrencode)

    add_subdirectory(Test)
endif()
