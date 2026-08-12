# QCloudMusicApi: API 参考

QCloudMusicApi API 由以下组件组成：

- [`module.h`](#module.h): 封装了`NeteaseCloudMusicApi`类，实现了所有的API方法
- [`apihelper.h`](#apihelper.h): 封装了`ApiHelper`类，将所有API接口的调用都封装在了一个方法中，支持设置全局的Cookie与网络代理
- [`api_c.h`](#api_c.h): 提供C语言接口，实现对`ApiHelper`类的跨语言调用的支持
- [`plugins.h`](#plugins-api): `Plugins`插件类，供`NeteaseCloudMusicApi`类调用
- [`util/request.h`](#request-api): `Request`网络接口类，通过事件循环实现同步调用网络接口
- [`util/crypto.h`](#crypto-api): 通过调用openssl实现API底层的加解密功能，支持rsa、aes的加解密等
- [`util/index.h`](#index-api): `Index`类，封装了一些通用的方法
- [`util/logger.h`](#logger-api): 管理QCloudMusicApi的日志输出

`NeteaseCloudMusicApi`类和`ApiHelper`类中的所有方法均通过`Q_INVOKABLE`宏注册到Qt元对象系统中，支持反射调用

`module.h`、`apihelper.h`和`api_c.h`为外部调用，其他类均为内部调用

## module.h

`module.h` 封装了`NeteaseCloudMusicApi`类，参考Node.js项目的`module`目录，实现了所有API方法。

继承了`QObject`类并声明了`Q_OBJECT`宏，用于实现Qt的反射调用

```c++
class QCLOUDMUSICAPI_EXPORT NeteaseCloudMusicApi : public QObject {
    Q_OBJECT
public:
    explicit NeteaseCloudMusicApi(QObject* parent = nullptr);

    // 声明所有方法
}
```

类中所有方法均以`Q_INVOKABLE QVariantMap 方法名(QVariantMap)`的格式定义；`Q_INVOKABLE`用于将方法注册到Qt元对象系统中，函数的入参和返回值均为`QVariantMap`类型

```c++
    // 歌手全部歌曲
    Q_INVOKABLE QVariantMap artist_songs(QVariantMap);
```

调用方式：

```c++
    NeteaseCloudMusicApi api;
    qDebug() << api.artist_songs({
        { "id", "1408586353" },
        { "offset", 0 },
        { "limit", 100 }
    });
```

## apihelper.h

`apihelper.h` 封装了`ApiHelper`类，通过反射机制将所有API接口的调用都封装在了一个方法中，支持设置全局的Cookie与网络代理，实现对Cookie的管理；所有方法均使用`Q_INVOKABLE`声明

```c++
    /**
     * @brief 调用API的成员函数
     * @param member 成员函数名
     * @param arg 调用参数
     * @return QVariantMap 返回调用结果
     */
    Q_INVOKABLE QVariantMap invoke(QString member, QVariantMap arg);
```

使用Qt的元对象系统对`NeteaseCloudMusicApi`类的API接口反射调用，若找不到对应的方法，则返回一个空的`QVariantMap`

调用`invoke`方法时，若参数中传有`Cookie`，则将其保存下来，下一次调用无需传入`Cookie`

当调用登录等接口返回值中有`Cookie`，也将其保存下来，存放于内存中

```c++
    QVariantMap ret;
    QMetaObject::invokeMethod(this, member.toUtf8(),
        Qt::DirectConnection,
        Q_RETURN_ARG(QVariantMap, ret),
        Q_ARG(QVariantMap, arg));
    return ret;
```

调用方式：

```c++
    ApiHelper helper;
    qDebug() << helper.invoke("artist_songs",
        {
            { "id", "1408586353" },
            { "offset", 0 },
            { "limit", 100 }
        });
```

## api_c.h

`api_c.h` 提供C语言接口，实现对`ApiHelper`类跨语言调用的支持，所有方法用`extern "C"`包裹，声明为C语言函数,传入参数均为C语言类型

```c++
    /**
     * @brief 通过反射调用API的成员函数
     * @param memberName 调用的函数的名称
     * @param value 函数参数的JSON格式字符
     * @return 调用结果的JSON格式字符串
     */
    QCLOUDMUSICAPI_EXPORT const char* invoke(char* memberName, char* value);
```

调用前创建一个`QCoreApplication`单例，提供事件循环支持，否则无法同步接收网络请求

```c++
    auto currentPath = QDir::currentPath().toStdString();
    // 创建一个QCoreApplication单例，用于支持事件循环QEventLoop
    if (!QCoreApplication::instance()) {
        int argc = 1;
        char* argv[1]{ (char*)currentPath.c_str() };
        app = new QCoreApplication(argc, argv);
    }
```

调用方式参考[跨语言调用](./BINDINGS.md)