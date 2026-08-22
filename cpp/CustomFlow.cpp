#include "CustomFlow.h"
#include <QQuickWindow>
#include <QQmlEngine>
#include <QDebug>

CustomFlow::CustomFlow(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, false);
    setAcceptedMouseButtons(Qt::NoButton);
    setFiltersChildMouseEvents(false);
}

CustomFlow::~CustomFlow() {}

void CustomFlow::setSpacing(double spacing)
{
    if (qFuzzyCompare(m_spacing, spacing))
        return;
    m_spacing = spacing;
    emit spacingChanged();
    markDirty();
}

void CustomFlow::setAlignment(Alignment align)
{
    if (m_alignment == align)
        return;
    m_alignment = align;
    emit alignmentChanged();
    markDirty();
}

void CustomFlow::componentComplete()
{
    QQuickItem::componentComplete();
    m_complete = true;
    markDirty();
    polish();
}

void CustomFlow::itemChange(ItemChange change, const ItemChangeData &data)
{
    QQuickItem::itemChange(change, data);
    if (!m_complete)
        return;
    if (change == ItemChildAddedChange || change == ItemChildRemovedChange) {
        markDirty();
        polish();
    }
}

void CustomFlow::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (m_complete && newGeometry.size() != oldGeometry.size()) {
        markDirty();
        polish();
    }
}

void CustomFlow::updatePolish()
{
    if (m_dirty)
        doLayout();
}

void CustomFlow::markDirty()
{
    m_dirty = true;
}

void CustomFlow::doLayout()
{
    if (!m_complete || width() <= 0 || height() <= 0)
        return;

    const double availWidth = width();
    const double spacing = m_spacing;
    const Alignment align = m_alignment;

    // 收集可见子项
    const auto children = childItems();
    QList<QQuickItem*> visibleChildren;
    visibleChildren.reserve(children.size());
    for (auto *item : children) {
        if (item->isVisible())
            visibleChildren.append(item);
    }

    if (visibleChildren.isEmpty())
        return;

    // 预计算尺寸
    struct ItemSize { double w, h; };
    QList<ItemSize> sizes;
    sizes.reserve(visibleChildren.size());
    for (auto *item : visibleChildren) {
        double w = item->width();
        double h = item->height();
        if (w < 1.0) w = 1.0;
        if (h < 1.0) h = 1.0;
        sizes.append({w, h});
    }

    // 分行
    struct Row {
        int startIndex;
        int endIndex; // 不包含
        double totalWidth;
        double maxHeight;
    };
    QList<Row> rows;
    int i = 0;
    const int n = visibleChildren.size();
    while (i < n) {
        double rowWidth = 0;
        double maxH = 0;
        int start = i;
        int end = i;
        while (i < n) {
            double w = sizes[i].w;
            if (rowWidth > 0) w += spacing;
            if (rowWidth + w > availWidth && end > start) {
                break;
            }
            rowWidth += w;
            if (sizes[i].h > maxH) maxH = sizes[i].h;
            end = ++i;
        }
        if (end == start) {
            rowWidth = sizes[start].w;
            maxH = sizes[start].h;
            end = ++i;
        }
        rows.append({start, end, rowWidth, maxH});
    }

    // 放置
    double y = 0;
    for (const Row &row : rows) {
        double totalWidth = row.totalWidth;
        double startX = 0;
        switch (align) {
        case AlignLeft:   startX = 0; break;
        case AlignRight:  startX = availWidth - totalWidth; break;
        case AlignCenter: startX = (availWidth - totalWidth) * 0.5; break;
        }
        double x = startX;
        for (int idx = row.startIndex; idx < row.endIndex; ++idx) {
            QQuickItem *item = visibleChildren[idx];
            item->setX(x);
            item->setY(y);
            x += sizes[idx].w + spacing;
        }
        y += row.maxHeight + spacing;
    }

    setImplicitHeight(y - spacing);
    m_dirty = false;
}