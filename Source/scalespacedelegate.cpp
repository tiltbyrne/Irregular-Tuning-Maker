#include <QPainter>
#include <QApplication>

#include "utilities.h"
#include "scalespacedelegate.h"

ScaleSpaceDelegate::ScaleSpaceDelegate(ScaleSpace* initialScaleSpace,
                                       QObject *parent)
    : QStyledItemDelegate(parent)
    , scaleSpace(initialScaleSpace)
{}

void ScaleSpaceDelegate::paint(QPainter *painter,
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const auto hovering{opt.state & QStyle::State_MouseOver};

    const auto altBlock{ itemShouldBeAltColour(index.row(), index.column()) };

    const auto& palette{ QApplication::palette() };

    QColor colour{ palette.color(QPalette::Base) };

    if ((altBlock && !hovering) || (hovering && !altBlock))
        colour = palette.color(QPalette::AlternateBase);

    painter->fillRect(opt.rect, colour);

    if (opt.state & QStyle::State_HasFocus && opt.state & QStyle::State_Selected)
    {
        const auto penWidth{ 2 };

        QPen pen(palette.color(QPalette::Text), 2);
        pen.setJoinStyle(Qt::MiterJoin);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        painter->drawRect(opt.rect.adjusted(1, 1, -1, -1));
    }

    opt.backgroundBrush = Qt::NoBrush;
    opt.state &= ~QStyle::State_MouseOver;
    opt.state &= ~QStyle::State_Selected;
    opt.state &= ~QStyle::State_HasFocus;
    opt.features &= ~QStyleOptionViewItem::HasDecoration;

    const auto *style{ opt.widget ? opt.widget->style() : QApplication::style() };

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
}

bool ScaleSpaceDelegate::itemShouldBeAltColour(const int& noteFrom, const int& noteTo) const
{
    const auto scaleSize{ scaleSpace->storedSize() };
    return posMod(noteFrom / scaleSize, 2) != posMod(noteTo / scaleSize, 2);
}

