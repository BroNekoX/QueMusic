# QCloudMusicApi: 跨语言调用C接口

## 简介

API动态库提供[C接口](./QCloudMusicApi/api_c.h)，开放给Python、Java和C#等支持与C语言交互的语言调用

通过C接口调用API库，是因为C语言提供了稳定的ABI、没有C++的名称修饰问题。

无论使用何种语言，都需要先将项目编译成动态库

```C++
#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief 通过反射调用API的成员函数
     * @param memberName 调用的函数的名称
     * @param value 函数参数的JSON格式字符
     * @return 调用结果的JSON格式字符串
     */
    QCLOUDMUSICAPI_EXPORT const char* invoke(char* memberName, char* value);

#ifdef __cplusplus
}
#endif
```

下面列出一些常用语言调用的例子

### 目录

- [Python调用](#Python调用)
- [C#调用](#C#调用)
- [Rust调用](#Rust调用)

## Python调用

[示例](./QCloudMusicApi/example/capi.py)

使用ctypes在运行时调用动态库

```Python
import ctypes
import json

def getSuffix():
    return {
        "Windows": "dll",
        "Linux": "so",
        "Darwin": "dylib",
    }.get(platform.system())

lib = ctypes.CDLL(f"./QCloudMusicApi.{getSuffix()}")

lib.invoke.argtypes = [ ctypes.c_char_p, ctypes.c_char_p ]
lib.invoke.restype = ctypes.c_char_p

def invoke(name, value):
    result = lib.invoke(ctypes.create_string_buffer(name.encode()),
                         ctypes.create_string_buffer(value.encode()))
    return result.decode()

if __name__ == '__main__':
    result = invoke("lyric_new", json.dumps({
        "id": "2058263032"
    }))
    print("result", json.dumps(json.loads(result), indent = 4, ensure_ascii = False))
```

## C#调用

[示例](./QCloudMusicApi/example/Program.cs)

在代码中声明外部函数，从而可以调用存储在DLL中的函数

使用Marshal类，在托管代码和非托管代码之间进行数据转换

```C#
using System;
using System.Runtime.InteropServices;

class Api
{
    public const string dllName = "QCloudMusicApi.dll";
    [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr invoke(string memberName, string value);
}

class ApiHelper
{
    public static string PtrToStringUTF8(IntPtr str)
    {
        // 返回char*，在C#中对应IntPtr，需要转换为C#中的string
        string result = Marshal.PtrToStringUTF8(str);
        return result;
    }

    // 通过反射调用API的成员函数
    public static string invoke(string memberName, string value)
    {
        return PtrToStringUTF8(Api.invoke(memberName, value));
    }
}

class Program
{
    static void Main()
    {
        Console.WriteLine(ApiHelper.invoke("lyric_new", "{\"id\": \"2058263032\"}"));
    }
};

```

## Rust调用

[示例](./QCloudMusicApi/example/main.rs)

使用外部块（extern “C”）定义外部函数接口（FFI），调用C语言编写的动态库中的函数

由于FFI可以导致未定义行为，所以对动态库的调用需要在unsafe代码块中进行

```Rust
use std::os::raw::c_char;
use std::ffi::{CStr, CString};

#[link(name = "QCloudMusicApi", kind = "dylib")]
extern "C" {
    fn invoke(memberName: *const c_char, value: *const c_char) -> *const c_char;
}

pub struct ApiHelper;

impl ApiHelper {
    // 反射调用API的成员函数
    pub fn invoke(&self, member_name: &str, value: &str) -> String {
        let mut _result = String::new();
        unsafe {
            _result = CStr::from_ptr(invoke(CString::new(member_name).expect("").as_ptr(),
            CString::new(value).expect("").as_ptr()))
            .to_str().expect("").to_string()
        }
        _result
    }
}

fn main() {
    let helper = ApiHelper;
    println!("{}", helper.invoke("artist_detail", "{\"id\": \"15396\"}"));

}

```