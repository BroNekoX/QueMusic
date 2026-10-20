// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// Portions based on QWindowKit example code:
// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// Copyright (C) 2021-2023 wangwenx190 (Yuhang Zhao)
#include "WindowsAppIdentity.h"

#if defined(Q_OS_WIN)

#include <QFile>
#include <QString>

#include <windows.h>
#include <winreg.h>
#include <shobjidl.h>
#include <propsys.h>
#include <propkey.h>

#include <string>

namespace {
constexpr const wchar_t *kAppId = L"BroNekoX.QueMusic";
constexpr const wchar_t *kAppIdKey = L"Software\\Classes\\AppUserModelId\\BroNekoX.QueMusic";

// 取不到 APPDATA 时返回空串
QString startMenuShortcutPath()
{
    const QString appData = qEnvironmentVariable("APPDATA");
    if (appData.isEmpty())
        return {};
    return appData + QStringLiteral("\\Microsoft\\Windows\\Start Menu\\Programs\\QueMusic.lnk");
}

// 快捷方式、注册表项都在且 IconUri 指向当前 exe，才算注册过（exe 移动过要重写）
bool alreadyRegistered(const QString &lnkPath, const QString &exePath)
{
    if (lnkPath.isEmpty() || !QFile::exists(lnkPath))
        return false;

    wchar_t displayName[64] = {};
    DWORD nameBytes = sizeof(displayName);
    if (RegGetValueW(HKEY_CURRENT_USER, kAppIdKey, L"DisplayName", RRF_RT_REG_SZ,
                     nullptr, displayName, &nameBytes) != ERROR_SUCCESS
        || displayName[0] == L'\0') {
        return false;
    }

    wchar_t iconUri[MAX_PATH] = {};
    DWORD iconBytes = sizeof(iconUri);
    if (RegGetValueW(HKEY_CURRENT_USER, kAppIdKey, L"IconUri", RRF_RT_REG_SZ,
                     nullptr, iconUri, &iconBytes) != ERROR_SUCCESS) {
        return false;
    }
    return QString::fromWCharArray(iconUri) == exePath;
}

void writeRegistryIdentity(const QString &exePath)
{
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kAppIdKey, 0, nullptr, 0, KEY_SET_VALUE,
                        nullptr, &key, nullptr) != ERROR_SUCCESS)
        return;

    auto setString = [key](const wchar_t *name, const std::wstring &text) {
        RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE *>(text.c_str()),
                       DWORD((text.size() + 1) * sizeof(wchar_t)));
    };
    setString(L"DisplayName", L"QueMusic");
    if (!exePath.isEmpty())
        setString(L"IconUri", exePath.toStdWString());   // exe 自带 .ico 资源
    RegCloseKey(key);
}

// 写 AppUserModelID 到开始菜单快捷方式（SMTC 取名用）
void writeStartMenuShortcut(const QString &lnkPath, const QString &exePath)
{
    // GUI 线程的 STA 是 Qt 初始化的（此时返回 S_FALSE），释放会让拖放/托盘失效；只有 S_OK 才配对
    const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool weInitialized = (hr == S_OK);
    if (!weInitialized && hr != S_FALSE && hr != RPC_E_CHANGED_MODE)
        return;

    IShellLinkW *shellLink = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IShellLinkW, reinterpret_cast<void **>(&shellLink)))
        && shellLink) {
        const std::wstring exe = exePath.toStdWString();
        shellLink->SetPath(exe.c_str());
        shellLink->SetDescription(L"QueMusic");
        shellLink->SetIconLocation(exe.c_str(), 0);

        IPropertyStore *propStore = nullptr;
        if (SUCCEEDED(shellLink->QueryInterface(IID_IPropertyStore,
                                                reinterpret_cast<void **>(&propStore)))
            && propStore) {
            PROPVARIANT pv;
            ZeroMemory(&pv, sizeof(pv));
            pv.vt = VT_LPWSTR;
            pv.pwszVal = const_cast<wchar_t *>(kAppId);
            propStore->SetValue(PKEY_AppUserModel_ID, pv);
            propStore->Commit();
            propStore->Release();
        }

        IPersistFile *persistFile = nullptr;
        if (SUCCEEDED(shellLink->QueryInterface(IID_IPersistFile,
                                                reinterpret_cast<void **>(&persistFile)))
            && persistFile) {
            persistFile->Save(reinterpret_cast<const wchar_t *>(lnkPath.utf16()), TRUE);
            persistFile->Release();
        }
        shellLink->Release();
    }

    if (weInitialized)
        CoUninitialize();
}
} // namespace

namespace WindowsAppIdentity {

void applyProcessIdentity()
{
    // 动态取址以兼容极老系统
    static const auto setAppUserModelId = []() -> HRESULT(WINAPI *)(PCWSTR) {
        const HMODULE shell32 = LoadLibraryW(L"shell32.dll");
        if (!shell32)
            return nullptr;
        return reinterpret_cast<HRESULT(WINAPI *)(PCWSTR)>(
            reinterpret_cast<void *>(GetProcAddress(shell32, "SetCurrentProcessExplicitAppUserModelID")));
    }();
    if (setAppUserModelId)
        setAppUserModelId(kAppId);
}

void ensureShellIntegration()
{
    static bool done = false;
    if (done)
        return;
    done = true;

    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    const QString exe = QString::fromWCharArray(exePath);

    const QString lnkPath = startMenuShortcutPath();
    if (alreadyRegistered(lnkPath, exe))
        return;

    writeRegistryIdentity(exe);
    if (!lnkPath.isEmpty())
        writeStartMenuShortcut(lnkPath, exe);
}

} // namespace WindowsAppIdentity

#else   // !Q_OS_WIN

namespace WindowsAppIdentity {
void applyProcessIdentity() { }
void ensureShellIntegration() { }
} // namespace WindowsAppIdentity

#endif  // Q_OS_WIN
