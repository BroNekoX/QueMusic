// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef LYRICTIMELINE_H
#define LYRICTIMELINE_H

#include <QList>

#include <algorithm>

namespace LyricTimeline {

inline int indexAt(const QList<qlonglong> &times, qlonglong position)
{
    if (times.isEmpty())
        return 0;
    const auto it = std::upper_bound(times.cbegin(), times.cend(), position);
    return it == times.cbegin() ? 0 : int(it - times.cbegin()) - 1;
}

} // namespace LyricTimeline

#endif // LYRICTIMELINE_H
