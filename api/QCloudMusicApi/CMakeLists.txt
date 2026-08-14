cmake_minimum_required(VERSION 3.14)

project(QCloudMusicApiProject VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

option(QCLOUDMUSICAPI_BUILD_TEST "Build QCloudMusicApi tests" OFF)
option(QCLOUDMUSICAPI_BUILD_SHARED "Build QCloudMusicApi as a shared library" ON)

# 设置第三方仓库
include_directories(3rdparty)
set(CRYPTOPP_BUILD_TESTING OFF)
# 使用本地已解压的 crypto++ 源码，避免构建时联网下载
set(CRYPTOPP_USE_MASTER_BRANCH OFF)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cryptopp")
    # FORCE 确保即使旧 cache 里 CRYPTOPP_SOURCES 为空也会被覆盖为本地的值
    set(CRYPTOPP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cryptopp" CACHE PATH "Local crypto++ sources" FORCE)
    message(STATUS "QCloudMusicApi: 使用本地 crypto++ 源码 ${CRYPTOPP_SOURCES}（跳过联网下载）")
else()
    message(WARNING "QCloudMusicApi: 未找到 3rdparty/cryptopp，将联网下载 crypto++")
endif()
add_subdirectory(3rdparty/cryptopp-cmake/)

add_subdirectory(QCloudMusicApi)
include_directories(QCloudMusicApi)
