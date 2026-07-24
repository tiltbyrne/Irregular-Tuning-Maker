#include "customheaderview.h"

#include <QMouseEvent>

CustomHeaderView::CustomHeaderView(Qt::Orientation orientation,
                                   QWidget *parent)
    : QHeaderView(orientation, parent)
    , contextMenu(new QMenu(this))
    , addAfter(contextMenu->addAction("Insert After"))
    , addBefore(contextMenu->addAction("Insert Before"))
    , delNote(contextMenu->addAction("Delete"))
    , fill(contextMenu->addAction("Fill Selection"))
    , clear(contextMenu->addAction("Clear Selection"))
{
    addAfter->setObjectName("addAfter");
    addBefore->setObjectName("addBefore");
    delNote->setObjectName("delNote");
    fill->setObjectName("fill");
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

    //addBefore->setEnabled(note % (scaleSpaceSize - 1) != 0);

    if (note < 0)
        return;

    const QAction* chosen = contextMenu->exec(event->globalPos());

    if (chosen == addAfter)
    {
        addNote(note + 1);
    }
    else if (chosen == addBefore)
    {
        emit addNote(note, true);
    }
    else if(chosen == delNote)
    {
        emit deleteNote(note);
    }
    else if(chosen == fill)
    {
        emit fillSelection();
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
    else if(actionName == fill->objectName())
    {
        fill->setEnabled(shouldBeEnabled);
    }
    else if(actionName == clear->objectName())
    {
        clear->setEnabled(shouldBeEnabled);
    }
}
