#ifndef SCALESPACEDELEGATE_H
#define SCALESPACEDELEGATE_H

#include <QStyledItemDelegate>
#include "scaleSpace.h"

class ScaleSpaceDelegate : public QStyledItemDelegate
{
public:
    explicit ScaleSpaceDelegate(ScaleSpace* initialScaleSpace,
                                QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    std::optional<QModelIndex> getLastSelectedIndex() const;
    void setLastSelectedIndex(std::optional<QModelIndex> newLastSelectedIndex) const;
    void setLastSelectedIndex(QModelIndex newLastSelectedIndex) const;

private:
    ScaleSpace* scaleSpace{ nullptr };

    mutable std::optional<QModelIndex> lastSelectedIndex{ std::nullopt };

    bool itemShouldBeAltColour(const int& noteFrom, const int& noteTo) const;
};

#endif // SCALESPACEDELEGATE_H
