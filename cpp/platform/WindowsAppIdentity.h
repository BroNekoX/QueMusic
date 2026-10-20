// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// Windows 应用身份（AppUserModelID）注册，分两步以便不拖慢启动：
//   applyProcessIdentity()  仅设进程 AppUserModelID，必须早于任何 UI；
//   ensureShellIntegration() 写注册表 + 开始菜单快捷方式，已注册时直接返回，可延后调用。
// 非 Windows 平台两者都是空实现。
#ifndef QUEMUSIC_WINDOWSAPPIDENTITY_H
#define QUEMUSIC_WINDOWSAPPIDENTITY_H

namespace WindowsAppIdentity {

// 必须早于任何窗口创建。
void applyProcessIdentity();
// 幂等：已完成时只做存在性检查。
void ensureShellIntegration();

} // namespace WindowsAppIdentity

#endif // QUEMUSIC_WINDOWSAPPIDENTITY_H
