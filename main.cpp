// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// Portions based on QWindowKit example code:
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QStandardPaths>
#include <QtQuick/QQuickWindow>
#include <QSettings>
#include <QFileInfo>
#include "cpp/FolderModel.h"
#include "cpp/Favorites.h"
#include "cpp/AccountManager.h"
#include "api/MusicApiService.h"
#include "cpp/LogManager.h"
#include "meshgradient/MeshGradientItem.h"
#include <QWKQuick/qwkquickglobal.h>

#include <QtQml/QQmlExtensionPlugin>
Q_IMPORT_QML_PLUGIN(MeshGradientItemPlugin)

extern void qml_register_types_QueMusic();
extern void qml_register_types_MeshGradientItem();

#if defined(Q_OS_WIN)
// Windows SMTC 弹窗里显示的应用名来自进程的 AppUserModelID（AUMID）。
// 仅写注册表 DisplayName 只对“通知”有效；媒体弹窗（SMTC）取名的真正来源是
// 开始菜单里带 AppUserModelID 属性的快捷方式，因此这里同时做三件事：
//   1) 设置显式 AUMID；2) 注册表 DisplayName/IconUri（供通知等部件）；
//   3) 创建带 AUMID 的开始菜单快捷方式（SMTC 据此显示应用名/图标）。
#include <windows.h>
#include <winreg.h>
#include <shobjidl.h>
#include <propsys.h>
#include <propkey.h>
#include <string>

static void registerSmtcAppIdentity()
{
    const wchar_t *appId = L"BroNekoX.QueMusic";

    // 1) 设置当前进程的显式 AppUserModelID（需在展示任何 UI 之前调用）
    typedef HRESULT (WINAPI *SetAppUserModelIDFn)(PCWSTR);
    HMODULE shell32 = LoadLibraryW(L"shell32.dll");
    if (shell32) {
        auto fn = reinterpret_cast<SetAppUserModelIDFn>(
            reinterpret_cast<void *>(GetProcAddress(shell32, "SetCurrentProcessExplicitAppUserModelID")));
        if (fn)
            fn(appId);
    }

    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    // 2) 注册表 DisplayName + IconUri（通知等系统部件解析显示名/图标用）
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                        L"Software\\Classes\\AppUserModelId\\BroNekoX.QueMusic",
                        0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS) {
        const wchar_t *displayName = L"QueMusic";
        RegSetValueExW(key, L"DisplayName", 0, REG_SZ,
                       reinterpret_cast<const BYTE *>(displayName),
                       (DWORD)((wcslen(displayName) + 1) * sizeof(wchar_t)));
        if (exePath[0]) {
            // exe 自身带有 .ico 资源，直接指向 exe 即可提取应用图标
            RegSetValueExW(key, L"IconUri", 0, REG_SZ,
                           reinterpret_cast<const BYTE *>(exePath),
                           (DWORD)((wcslen(exePath) + 1) * sizeof(wchar_t)));
        }
        RegCloseKey(key);
    }

    // 3) 创建开始菜单快捷方式并写入 AppUserModelID（SMTC 媒体弹窗取名的关键）
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool comReady = SUCCEEDED(hr); // S_OK / S_FALSE 表示本线程 COM 已就绪
    if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE) {
        wchar_t appData[MAX_PATH] = {};
        if (GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH)) {
            const std::wstring lnkPath = std::wstring(appData)
                + L"\\Microsoft\\Windows\\Start Menu\\Programs\\QueMusic.lnk";

            IShellLinkW *shellLink = nullptr;
            hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_IShellLinkW, reinterpret_cast<void **>(&shellLink));
            if (SUCCEEDED(hr) && shellLink) {
                shellLink->SetPath(exePath);
                shellLink->SetDescription(L"QueMusic");
                shellLink->SetIconLocation(exePath, 0);

                IPropertyStore *propStore = nullptr;
                if (SUCCEEDED(shellLink->QueryInterface(IID_IPropertyStore,
                                                        reinterpret_cast<void **>(&propStore)))
                    && propStore) {
                    PROPVARIANT pv;
                    ZeroMemory(&pv, sizeof(pv));
                    pv.vt = VT_LPWSTR;
                    pv.pwszVal = const_cast<wchar_t *>(appId);
                    propStore->SetValue(PKEY_AppUserModel_ID, pv);
                    propStore->Commit();
                    propStore->Release();
                }

                IPersistFile *persistFile = nullptr;
                if (SUCCEEDED(shellLink->QueryInterface(IID_IPersistFile,
                                                        reinterpret_cast<void **>(&persistFile)))
                    && persistFile) {
                    persistFile->Save(lnkPath.c_str(), TRUE);
                    persistFile->Release();
                }
                shellLink->Release();
            }
        }
        if (comReady)
            CoUninitialize();
    }
}
#endif

int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN)
    registerSmtcAppIdentity();
#endif
    // 从Options.ini读取设置，设置一些高级项喵~
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QSettings opt(configPath + QStringLiteral("/BroNekoX/QueMusic.ini"), QSettings::IniFormat);
    switch (opt.value(QStringLiteral("Options/gpuRenderMode"), 0).toInt()) {
        case 1: qputenv("QSG_RHI_BACKEND", "opengl"); break;
        case 2: qputenv("QSG_RHI_BACKEND", "vulkan"); break;
        case 3: qputenv("QT_QUICK_BACKEND", "software"); break;
    }
    if (opt.value(QStringLiteral("Options/timerAnimator"), 0).toBool())
        qputenv("QSG_NO_VSYNC", "1");
    if (opt.value(QStringLiteral("Options/qmlAnimator"), 0).toBool() == false)
        qputenv("QSG_USE_SIMPLE_ANIMATION_DRIVER", "1");
    qputenv("QSG_INFO", "1");

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QGuiApplication application(argc, argv);

    QQuickWindow::setDefaultAlphaBuffer(true);
    //QQuickWindow::setTextRenderType(QQuickWindow::CurveTextRendering);
    QQmlApplicationEngine engine;

    // 显式注册QML_ELEMENT 类型
    qml_register_types_QueMusic();
    qml_register_types_MeshGradientItem();

    application.setOrganizationName("BroNekoX");
    application.setOrganizationDomain("com.bronekox.quemusic");
    application.setWindowIcon(QIcon("qrc:/QPlayer/resources/icon.ico"));
    application.setApplicationName("QueMusic");

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, configPath);

    // 日志系统：接管 Qt 消息并写入“安装目录/logs”，中文、可分级筛选（默认记录错误及以上）
    LogManager *logManager = new LogManager(&engine);
    engine.rootContext()->setContextProperty("logManager", logManager);

    // 创建模型实例
    FolderModel *myFolderModel = new FolderModel(&engine);
    myFolderModel->setFilterType("my");
    FolderModel *localFolderModel = new FolderModel(&engine);
    localFolderModel->setFilterType("local");
    SongModel *songModel = new SongModel(&engine);
    FavoritesModel *favSongModel = new FavoritesModel(&engine);
    favSongModel->setFilterType("song");
    FavoritesModel *favPlaylistModel = new FavoritesModel(&engine);
    favPlaylistModel->setFilterType("playlist");
    FavoritesModel *favArtistModel = new FavoritesModel(&engine);
    favArtistModel->setFilterType("artist");

    // 暴露给 QML
    engine.rootContext()->setContextProperty("myFolderModel", myFolderModel);
    engine.rootContext()->setContextProperty("localFolderModel", localFolderModel);
    engine.rootContext()->setContextProperty("songModel", songModel);
    engine.rootContext()->setContextProperty("favoritesSong", favSongModel);
    engine.rootContext()->setContextProperty("favoritesList", favPlaylistModel);
    engine.rootContext()->setContextProperty("favoritesArtist", favArtistModel);
    AccountManager *accountManager = new AccountManager(&engine);
    engine.rootContext()->setContextProperty("accountManager", accountManager);
    // 在线音乐 API 单例
    MusicApiService::setSharedAccountManager(accountManager);

    // 关键修改：使用标准配置目录，而不是应用程序目录
    engine.rootContext()->setContextProperty("configDir", configPath);
    engine.rootContext()->setContextProperty("$curveRenderingAvailable", true);

    QWK::registerTypes(&engine);
    engine.load(QUrl(QStringLiteral("qrc:/QueMusic/main.qml")));
    return application.exec();
}
