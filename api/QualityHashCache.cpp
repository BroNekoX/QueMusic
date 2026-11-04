// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "QualityHashCache.h"

#include "ApiCommon.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
constexpr int kMaxEntries = 5000;
constexpr int kMaxReverseEntries = 20000;
constexpr int kSaveDelayMs = 1500;
}

QualityHashCache::QualityHashCache(QObject *parent)
    : QObject(parent)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(kSaveDelayMs);
    connect(&m_saveTimer, &QTimer::timeout, this, &QualityHashCache::save);
    load();
}

QualityHashCache::~QualityHashCache()
{
    if (m_dirty)
        save();
}

QString QualityHashCache::cachePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                        + QStringLiteral("/BroNekoX");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/QueMusicQuality.json");
}

void QualityHashCache::load()
{
    QFile file(cachePath());
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const auto loadSection = [this](const QJsonObject &section, bool hq) {
        for (auto it = section.constBegin(); it != section.constEnd(); ++it) {
            const QString base = it.key();
            const QString alt = it.value().toString();
            if (base.isEmpty() || alt.isEmpty() || alt == base)
                continue;
            Alternates &alternates = m_alternates[base];
            (hq ? alternates.hq : alternates.sq) = alt;
            m_baseOf.insert(alt, base);
        }
    };
    loadSection(root.value(QStringLiteral("hq")).toObject(), true);
    loadSection(root.value(QStringLiteral("sq")).toObject(), false);
}

void QualityHashCache::remember(const QVariantList &items)
{
    if (items.isEmpty() || m_alternates.size() > kMaxEntries)
        return;

    bool changed = false;
    for (const QVariant &value : items) {
        const QVariantMap item = value.toMap();
        const QString base = item.value(ApiCommon::kHash).toString();
        if (base.isEmpty())
            continue;

        const QString hq = item.value(ApiCommon::kHashHq).toString();
        const QString sq = item.value(ApiCommon::kHashSq).toString();
        Alternates alternates = m_alternates.value(base);
        if (!hq.isEmpty() && hq != base && alternates.hq != hq) {
            alternates.hq = hq;
            m_baseOf.insert(hq, base);
            changed = true;
        }
        if (!sq.isEmpty() && sq != base && alternates.sq != sq) {
            alternates.sq = sq;
            m_baseOf.insert(sq, base);
            changed = true;
        }
        if (!alternates.hq.isEmpty() || !alternates.sq.isEmpty())
            m_alternates.insert(base, alternates);
    }

    if (changed) {
        pruneIfNeeded();
        scheduleSave();
    }
}

QString QualityHashCache::resolve(const QString &hash, int quality) const
{
    const QString base = m_baseOf.value(hash, hash);
    const Alternates alternates = m_alternates.value(base);
    if (quality >= 2 && !alternates.sq.isEmpty())
        return alternates.sq;
    if (quality == 1 && !alternates.hq.isEmpty())
        return alternates.hq;
    return base;
}

QString QualityHashCache::baseOf(const QString &hash) const
{
    return m_baseOf.value(hash, hash);
}

void QualityHashCache::pruneIfNeeded()
{
    if (m_baseOf.size() <= kMaxReverseEntries || m_alternates.isEmpty())
        return;

    m_baseOf.clear();
    m_baseOf.reserve(m_alternates.size() * 2);
    for (auto it = m_alternates.constBegin(); it != m_alternates.constEnd(); ++it) {
        if (!it.value().hq.isEmpty())
            m_baseOf.insert(it.value().hq, it.key());
        if (!it.value().sq.isEmpty())
            m_baseOf.insert(it.value().sq, it.key());
    }
}

void QualityHashCache::scheduleSave()
{
    m_dirty = true;
    m_saveTimer.start();
}

void QualityHashCache::save()
{
    QJsonObject hq;
    QJsonObject sq;
    for (auto it = m_alternates.constBegin(); it != m_alternates.constEnd(); ++it) {
        if (!it.value().hq.isEmpty())
            hq.insert(it.key(), it.value().hq);
        if (!it.value().sq.isEmpty())
            sq.insert(it.key(), it.value().sq);
    }

    QJsonObject root;
    root.insert(QStringLiteral("hq"), hq);
    root.insert(QStringLiteral("sq"), sq);

    QSaveFile file(cachePath());
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    if (file.commit())
        m_dirty = false;
}
