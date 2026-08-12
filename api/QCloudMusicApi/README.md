# QCloudMusicApi - Qt版 网易云音乐 API

[![GitHub Actions CI Status](https://github.com/s12mmm3/QCloudMusicApi/actions/workflows/windows.yml/badge.svg)](https://github.com/s12mmm3/QCloudMusicApi/actions/workflows/windows.yml)
[![GitHub Actions CI Status](https://github.com/s12mmm3/QCloudMusicApi/actions/workflows/macos.yml/badge.svg)](https://github.com/s12mmm3/QCloudMusicApi/actions/workflows/macos.yml)
[![GitHub Actions CI Status](https://github.com/s12mmm3/QCloudMusicApi/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/s12mmm3/QCloudMusicApi/actions/workflows/ubuntu.yml)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fs12mmm3%2FQCloudMusicApi.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Fs12mmm3%2FQCloudMusicApi?ref=badge_shield)

![C++ version](https://img.shields.io/badge/C++-11-00599C?logo=++)
[![Qt version](https://img.shields.io/badge/Qt-5.12+-41CD52?logo=qt)](https://www.qt.io)
[![Qt version](https://img.shields.io/badge/Qt-6.0+-41CD52?logo=qt)](https://www.qt.io)
![GitHub license](https://img.shields.io/github/license/s12mmm3/QCloudMusicApi)

## 简介

本项目翻译自Node.js项目[Binaryify/NeteaseCloudMusicApi](https://github.com/Binaryify/NeteaseCloudMusicApi)

更新尽量与原项目同步，实现了大部分接口；支持[跨语言调用](./doc/BINDINGS.md)，支持跨平台编译，支持多种编译器编译

推荐直接将本仓库嵌入到其他项目中编译使用

使用方式参考[原项目文档](https://binaryify.github.io/NeteaseCloudMusicApi)，参数与返回结果与原项目完全一致

### 样例项目

[QListenTogether](https://github.com/s12mmm3/QListenTogether) - 网易云 一起听客户端

[QCloudMusicServer](https://github.com/s12mmm3/QCloudMusicServer) - 网易云API HTTP服务器

[QRadioPlayer](https://github.com/s12mmm3/QRadioPlayer) - 网络广播电台客户端

### 目录

- [需求和依赖](#需求和依赖)
- [使用说明](#使用说明)
- [编译方式](#编译方式)
- [License](#License)

---

## 需求和依赖

- [Qt 5.12+](https://www.qt.io/download-qt-installer)

## 使用说明

可参考文档[API 参考](./doc/API.md)

#### 使用ApiHelper类

该类实现了API的cookie管理；支持Qt反射调用

通过对应方法字符串调用
```C++
#include "apihelper.h"

ApiHelper helper;
qDebug() << helper.invoke("lyric_new", { { "id", "1408586353" } });
```
传入Url调用
```C++
ApiHelper helper;
qDebug() << helper.invokeUrl("/song/url/v1?id=2058263032, 2057797340&level=exhigh");
qDebug() << helper.invokeUrl("http://localhost:3000/activate/init/profile");
```

#### 使用NeteaseCloudMusicApi类

若无需保存cookie，可直接使用该类，直接调用类函数；
支持Qt反射调用，所有方法均注册至Qt元对象系统中

```C++
#include "module.h"

NeteaseCloudMusicApi api;
qDebug() << api.lyric_new({ { "id", "1408586353" } });
```

### 跨语言调用

动态库提供C接口，开放给Python、Java和C#等支持与C语言交互的语言调用
```C++
/**
 * @brief 反射调用API的成员函数。
 * @param memberName 调用成员函数的名称
 * @param value 参数的JSON格式字符串
 * @return 调用结果的JSON格式字符串
 */
const char* invoke(char* memberName, char* value);
```

调用例子参考[跨语言调用](./doc/BINDINGS.md)

### 在CMake项目中嵌入本项目

在CMakeLists.txt文件中加上
```CMake
add_subdirectory(QCloudMusicApi)
include_directories(QCloudMusicApi)
add_executable(${PROJECT_NAME}
    main.cpp
)
target_link_libraries(${PROJECT_NAME} QCloudMusicApi)
```
引用头文件
```C++
#include "QCloudMusicApi/apihelper.h"
```

## 编译方式

```Shell
git clone --recursive https://github.com/s12mmm3/QCloudMusicApi.git
cd QCloudMusicApi
cmake -B build
cmake --build build -j
```

## License

[The MIT License (MIT)](LICENSE)


[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fs12mmm3%2FQCloudMusicApi.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Fs12mmm3%2FQCloudMusicApi?ref=badge_large)