using System;
using System.Runtime.InteropServices;

class Api
{
    public const string dllName = "QCloudMusicApi.dll";
    [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr invoke(string memberName, string value);
    [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr invokeUrl(string url);
    [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr memberName(int i);
    [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int memberCount();
    [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void set_cookie(string cookie);
    [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr cookie();
    [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void set_proxy(string proxy);
    [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr proxy();
    [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void setFilterRules(string rules);
    [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int loadPlugin(string fileName);
    [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern bool unloadPlugin(int id);
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

    // 通过反射调用API的成员函数
    public static string invokeUrl(string url)
    {
        return PtrToStringUTF8(Api.invokeUrl(url));
    }

    //获取API列表
    public static List<string> memberList()
    {
        List<string> list = [];
        for (int i = 0; i < Api.memberCount(); i++)
        {
            var member = Api.memberName(i);
            list.Add(PtrToStringUTF8(member));
        }
        return list;
    }

    // 设置全局cookie
    public static void set_cookie(string cookie)
    {
        Api.set_cookie(cookie);
    }

    // 获取cookie
    public static string cookie()
    {
        return PtrToStringUTF8(Api.cookie());
    }

    // 设置全局代理
    public static void set_proxy(string proxy)
    {
        Api.set_proxy(proxy);
    }

    // 获取代理
    public static string proxy()
    {
        return PtrToStringUTF8(Api.proxy());
    }

    // 设置log规则
    public static void setFilterRules(string rules)
    {
        Api.setFilterRules(rules);
    }

    // 加载插件
    public static int loadPlugin(string fileName)
    {
        return Api.loadPlugin(fileName);
    }

    // 卸载插件（通过ID）
    public static bool unloadPlugin(int id)
    {
        return Api.unloadPlugin(id);
    }
}

class Program
{
    static void Main()
    {
        ApiHelper.setFilterRules("QCloudMusicApi.debug=false");
        foreach (var member in ApiHelper.memberList()) {
            Console.WriteLine(member);
        }

        Console.WriteLine(ApiHelper.invoke("lyric_new", "{\"id\": \"2058263032\"}"));
        Console.WriteLine(ApiHelper.invokeUrl("/lyric_new?id=2058263032, 2057797340&level=exhigh"));
    }
};
