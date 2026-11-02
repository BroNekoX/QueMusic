// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#pragma once

#include "QueueModel.h"

#include <QMutex>
#include <QRandomGenerator>
#include <QVector>

class PlaybackCursor
{
public:
    enum Cycle { List = 0, One = 1, Shuffle = 2, Stop = 3 };

    void setTracks(const QVector<QueueTrack> &tracks)
    {
        QMutexLocker lock(&m_mutex);
        m_tracks = tracks;
        if (m_index >= m_tracks.size())
            m_index = int(m_tracks.size()) - 1;
        m_bag.clear();
    }

    void setIndex(int index)
    {
        QMutexLocker lock(&m_mutex);
        m_index = index >= 0 && index < m_tracks.size() ? index : -1;
        m_bag.removeAll(m_index);
    }

    void setCycle(int cycle)
    {
        QMutexLocker lock(&m_mutex);
        m_cycle = cycle;
        m_bag.clear();
    }

    void setAvoid(int avoid)
    {
        QMutexLocker lock(&m_mutex);
        m_avoid = qMax(0, avoid);
        trimRecent();
        m_bag.clear();
    }

    int size() const
    {
        QMutexLocker lock(&m_mutex);
        return int(m_tracks.size());
    }

    int index() const
    {
        QMutexLocker lock(&m_mutex);
        return m_index;
    }

    QueueTrack track(int index) const
    {
        QMutexLocker lock(&m_mutex);
        return valid(index) ? m_tracks.at(index) : QueueTrack();
    }

    int peekAuto()
    {
        QMutexLocker lock(&m_mutex);
        if (m_tracks.isEmpty() || m_cycle == Stop)
            return -1;
        if (m_cycle == One)
            return m_index >= 0 ? m_index : 0;
        if (m_cycle == Shuffle)
            return pickShuffle(false);
        return nextList();
    }

    int takeAuto(bool note = true)
    {
        QMutexLocker lock(&m_mutex);
        if (m_tracks.isEmpty() || m_cycle == Stop)
            return -1;
        int index = -1;
        if (m_cycle == One)
            index = m_index >= 0 ? m_index : 0;
        else if (m_cycle == Shuffle)
            index = pickShuffle(true);
        else
            index = nextList();
        if (index < 0)
            return -1;
        m_index = index;
        if (note)
            noteLocked(index);
        return index;
    }

    int takeManual(bool random)
    {
        QMutexLocker lock(&m_mutex);
        if (m_tracks.isEmpty())
            return -1;
        m_index = random && m_tracks.size() > 1 ? pickShuffle(true) : nextList();
        return m_index;
    }

    int takePrevious()
    {
        QMutexLocker lock(&m_mutex);
        if (m_tracks.isEmpty())
            return -1;
        m_index = m_index > 0 ? m_index - 1 : int(m_tracks.size()) - 1;
        return m_index;
    }

    void note(int index)
    {
        QMutexLocker lock(&m_mutex);
        noteLocked(index);
    }

private:
    bool valid(int index) const { return index >= 0 && index < m_tracks.size(); }

    int nextList() const
    {
        if (m_tracks.isEmpty())
            return -1;
        return m_index < 0 ? 0 : (m_index + 1) % int(m_tracks.size());
    }

    int pickShuffle(bool consume)
    {
        if (m_tracks.isEmpty())
            return -1;
        if (m_bag.isEmpty()) {
            for (int i = 0; i < m_tracks.size(); ++i) {
                if (i != m_index && !m_recent.contains(i))
                    m_bag.append(i);
            }
            if (m_bag.isEmpty()) {
                for (int i = 0; i < m_tracks.size(); ++i)
                    m_bag.append(i);
            }
            for (int i = int(m_bag.size()) - 1; i > 0; --i) {
                const int j = QRandomGenerator::global()->bounded(i + 1);
                m_bag.swapItemsAt(i, j);
            }
        }
        if (m_bag.isEmpty())
            return -1;
        const int index = consume ? m_bag.takeLast() : m_bag.last();
        return index;
    }

    void noteLocked(int index)
    {
        if (!valid(index))
            return;
        if (!m_recent.isEmpty() && m_recent.last() == index)
            return;
        m_recent.append(index);
        trimRecent();
    }

    void trimRecent()
    {
        while (m_recent.size() > m_avoid)
            m_recent.removeFirst();
    }

    mutable QMutex m_mutex;
    QVector<QueueTrack> m_tracks;
    QVector<int> m_bag;
    QVector<int> m_recent;
    int m_index = -1;
    int m_cycle = List;
    int m_avoid = 2;
};