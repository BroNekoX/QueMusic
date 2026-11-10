// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#include "plugins/FunctionPluginStore.h"

#include "AppSettings.h"

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
        if (!id.isEmpty() && AppSettings::value(QStringLiteral("Plugins/function"), id, true).toBool())
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
    AppSettings::setValue(QStringLiteral("Plugins/function"), id, on);
    AppSettings::sync();
    emit enabledChanged();
}

void FunctionPluginStore::rescan()
{
    PluginStore::rescan();
    loadEnabled();   // 新装的插件自动进来（默认启用），已卸载的自动移出
}
