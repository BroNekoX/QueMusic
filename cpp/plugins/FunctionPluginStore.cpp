// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#include "plugins/FunctionPluginStore.h"

#include <QSettings>

namespace {
// 启用开关按插件 id 单独存；插件卸载后键保留，重装即恢复用户原来的选择
QString enabledKey(const QString &id)
{
    return QStringLiteral("Plugins/function/") + id;
}
} // namespace

FunctionPluginStore::FunctionPluginStore(QObject *parent)
    : PluginStore(QStringLiteral("功能插件"), QStringLiteral("function"), parent)
{
    load();
    loadEnabled();
}

void FunctionPluginStore::loadEnabled()
{
    QStringList next;
    next.reserve(m_plugins.size());
    for (const QVariant &item : std::as_const(m_plugins)) {
        const QString id = item.toMap().value(QStringLiteral("id")).toString();
        if (!id.isEmpty() && QSettings().value(enabledKey(id), true).toBool())
            next << id;
    }
    if (next == m_enabled)
        return;
    m_enabled = next;
    emit enabledChanged();
}

void FunctionPluginStore::setEnabled(const QString &id, bool on)
{
    if (id.isEmpty() || on == m_enabled.contains(id) || !find(id, nullptr))
        return;
    if (on)
        m_enabled << id;
    else
        m_enabled.removeAll(id);
    QSettings().setValue(enabledKey(id), on);
    emit enabledChanged();
}

void FunctionPluginStore::rescan()
{
    PluginStore::rescan();
    loadEnabled();   // 新装的插件自动进来（默认启用），已卸载的自动移出
}
