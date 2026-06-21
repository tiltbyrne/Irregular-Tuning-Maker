#include "customheaderview.h"

#include <QMouseEvent>

CustomHeaderView::CustomHeaderView(int initialSize,
                                   Qt::Orientation orientation,
                                   QWidget *parent)
    : scaleSpaceSize(initialSize)
    , QHeaderView(orientation, parent)
    , contextMenu(new QMenu(this))
    , addAfter(contextMenu->addAction("Insert After"))
    , addBefore(contextMenu->addAction("Insert Before"))
    , delNote(contextMenu->addAction("Delete"))
    , clear(contextMenu->addAction("Clear Selections"))
{
    addAfter->setObjectName("addAfter");
    addBefore->setObjectName("addBefore");
    delNote->setObjectName("delNote");
    clear->setObjectName("clear");
}

void CustomHeaderView::mousePressEvent(QMouseEvent *event)
{
    const auto note{ logicalIndexAt(event->position().toPoint()) };

    if (event->button() == Qt::LeftButton && note >= 0)
        emit leftClicked(note);

    QHeaderView::mousePressEvent(event);
}

void CustomHeaderView::contextMenuEvent(QContextMenuEvent *event)
{
    const auto note{ logicalIndexAt(event->pos()) };

    addBefore->setEnabled(note % (scaleSpaceSize - 1) != 0);

    if (note < 0)
        return;

    const QAction* chosen = contextMenu->exec(event->globalPos());

    if (chosen == addAfter)
    {
        addNote(note + 1);
    }
    else if (chosen == addBefore)
    {
        emit addNote(note);
    }
    else if(chosen == delNote)
    {
        emit deleteNote(note);
    }
    else if(chosen == clear)
    {
        emit clearSelection();
    }

    event->accept();
}

void CustomHeaderView::enableAction(const QString& actionName, const bool& shouldBeEnabled)
{
    if (actionName == addAfter->objectName())
    {
        addAfter->setEnabled(shouldBeEnabled);
    }
    else if (actionName == addBefore->objectName())
    {
        addBefore->setEnabled(shouldBeEnabled);
    }
    else if(actionName == delNote->objectName())
    {
        delNote->setEnabled(shouldBeEnabled);
    }
    else if(actionName == clear->objectName())
    {
        clear->setEnabled(shouldBeEnabled);
    }
}

void CustomHeaderView::handleScaleSpaceSizeChanged(int newSize)
{
    scaleSpaceSize = newSize;
}
