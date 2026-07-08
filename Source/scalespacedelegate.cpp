#include <QPainter>
#include <QApplication>
#include <QAbstractItemView>

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
    if (!index.isValid())
        return;

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const auto hovering{opt.state & QStyle::State_MouseOver};

    const auto altBlock{ itemShouldBeAltColour(index.row(), index.column()) };

    const auto& palette{ QApplication::palette() };

    auto fillColour{ palette.color(QPalette::Midlight) };

    if ((altBlock && !hovering) || (hovering && !altBlock))
        fillColour = palette.color(QPalette::Mid);

    painter->fillRect(opt.rect, fillColour);

    if ((opt.state & QStyle::State_HasFocus && opt.state & QStyle::State_Selected) ||
        (lastSelectedIndex.has_value() && index == lastSelectedIndex.value()))
    {
        QPen pen(palette.color(QPalette::Highlight), 2);
        pen.setJoinStyle(Qt::MiterJoin);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        painter->drawRect(opt.rect.adjusted(1, 1, -1, -1));

        if (auto *view{ qobject_cast<QAbstractItemView*>(parent()) })
        {
            const QRect rect{ view->visualRect(index) };
            QMetaObject::invokeMethod( view->viewport(),
                                      [vp = view->viewport(), rect]
                                      {
                                          vp->update(rect);
                                      },
                                      Qt::QueuedConnection);
        }

        setLastSelectedIndex(index);
    }

    //painter->setPen(textColour);
    //painter->drawText(opt.rect, Qt::AlignLeft | Qt::AlignVCenter, opt.text);

    opt.backgroundBrush = Qt::NoBrush;
    opt.state &= ~QStyle::State_MouseOver;
    opt.state &= ~QStyle::State_Selected;
    opt.state &= ~QStyle::State_HasFocus;
    opt.features &= ~QStyleOptionViewItem::HasDecoration;

    const auto *style{ opt.widget ? opt.widget->style() : QApplication::style() };

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
}

std::optional<QModelIndex> ScaleSpaceDelegate::getLastSelectedIndex() const
{
    return lastSelectedIndex;
}

void ScaleSpaceDelegate::setLastSelectedIndex(std::optional<QModelIndex> newLastSelectedIndex) const
{
    lastSelectedIndex = newLastSelectedIndex;
}

void ScaleSpaceDelegate::setLastSelectedIndex(QModelIndex newLastSelectedIndex) const
{
    setLastSelectedIndex(std::make_optional(newLastSelectedIndex));
}

bool ScaleSpaceDelegate::itemShouldBeAltColour(const int& noteFrom, const int& noteTo) const
{
    const auto scaleSize{ scaleSpace->storedSize() };
    return posMod(noteFrom / scaleSize, 2) != posMod(noteTo / scaleSize, 2);
}

