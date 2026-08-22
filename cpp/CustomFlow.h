#ifndef CUSTOMFLOW_H
#define CUSTOMFLOW_H

#include <QQuickItem>
#include <QtQml/qqml.h>

class CustomFlow : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(double spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged)

public:
    enum Alignment {
        AlignLeft,
        AlignRight,
        AlignCenter
    };
    Q_ENUM(Alignment)

    explicit CustomFlow(QQuickItem *parent = nullptr);
    ~CustomFlow();

    double spacing() const { return m_spacing; }
    void setSpacing(double spacing);

    Alignment alignment() const { return m_alignment; }
    void setAlignment(Alignment align);

signals:
    void spacingChanged();
    void alignmentChanged();

protected:
    void componentComplete() override;
    void updatePolish() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;

private:
    void doLayout();
    void markDirty();

private:
    double m_spacing = 0.0;
    Alignment m_alignment = AlignLeft;
    bool m_complete = false;
    bool m_dirty = true;
};

#endif // CUSTOMFLOW_H