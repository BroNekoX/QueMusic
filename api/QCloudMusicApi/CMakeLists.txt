cmake_minimum_required(VERSION 3.16)
project(QCloudMusicApi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Gui)

# crypto++：只用仓库里的源码，直接编静态库（不再经过 cryptopp-cmake）
set(CRYPTOPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cryptopp)
file(GLOB CRYPTOPP_SOURCES CONFIGURE_DEPENDS ${CRYPTOPP_DIR}/*.cpp)
list(FILTER CRYPTOPP_SOURCES EXCLUDE REGEX
     "/(bench[0-9]|validat[0-9]+|regtest[0-9]+|test|datatest|dlltest|fipsalgt|fipstest|pch|simple)\\.cpp$")
if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64|arm64|ARM64)")
    list(FILTER CRYPTOPP_SOURCES EXCLUDE REGEX "/neon_simd\\.cpp$")
endif()
if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(ppc|powerpc)")
    list(FILTER CRYPTOPP_SOURCES EXCLUDE REGEX "/power[0-9]+_ppc\\.cpp$")
endif()

add_library(cryptopp STATIC ${CRYPTOPP_SOURCES})
add_library(cryptopp::cryptopp ALIAS cryptopp)
target_include_directories(cryptopp PUBLIC
    ${CRYPTOPP_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set_target_properties(cryptopp PROPERTIES POSITION_INDEPENDENT_CODE ON)
# 纯 C++ 实现：不依赖每文件 SIMD 开关，各平台同一套配置
target_compile_definitions(cryptopp PUBLIC CRYPTOPP_DISABLE_ASM=1)

file(GLOB QCLOUD_HEADERS CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/util/*.h)
file(GLOB QCLOUD_SOURCES CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/util/*.cpp)

add_library(QCloudMusicApi STATIC ${QCLOUD_HEADERS} ${QCLOUD_SOURCES})
target_include_directories(QCloudMusicApi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(QCloudMusicApi PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Gui
    cryptopp::cryptopp
)
target_compile_definitions(QCloudMusicApi PRIVATE
    QCLOUDMUSICAPI_LIBRARY
    PROJECT_NAME="QCloudMusicApi"
)
