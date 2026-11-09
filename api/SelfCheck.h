// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef APISELFCHECK_H
#define APISELFCHECK_H

#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class MusicSourceRegistry;
class QualityHashCache;

struct SelfCheckContext
{
    QVariantList sources;
    QVariantList sourceIds;
    QStringList sourceNames;
    QVariantList plugins;
    const MusicSourceRegistry *registry = nullptr;
    QualityHashCache *qualityCache = nullptr;
    int currentSource = -1;
    int pendingRequests = 0;
};

namespace ApiSelfCheck {
QVariantMap run(const SelfCheckContext &context);
}

#endif // APISELFCHECK_H
