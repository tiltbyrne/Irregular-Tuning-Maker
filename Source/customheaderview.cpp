#include "customheaderview.h"

#include <QMouseEvent>

CustomHeaderView::CustomHeaderView(Qt::Orientation orientation,
                                   QWidget *parent)
    : QHeaderView(orientation, parent)
    , contextMenu(new QMenu(this))
    , addAfter(contextMenu->addAction("Insert after"))
    , addBefore(contextMenu->addAction("Insert before"))
    , delNote(contextMenu->addAction("Delete"))
    , selectAll(contextMenu->addAction("Select all"))
    , deselectAll(contextMenu->addAction("Deselect all"))
    , fixAll(contextMenu->addAction("Fix all"))
    , unfixAll(contextMenu->addAction("Unfix all"))
{
    addAfter->setObjectName("addAfter");
    addBefore->setObjectName("addBefore");
    delNote->setObjectName("delNote");
    selectAll->setObjectName("selectAll");
    deselectAll->setObjectName("deselectAll");
    fixAll->setObjectName("fixAll");
    unfixAll->setObjectName("unfixAll");
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
    else if(chosen == selectAll)
    {
        emit fillSelection();
    }
    else if(chosen == deselectAll)
    {
        emit clearSelection();
    }
    else if(chosen == fixAll)
    {
        emit fillFixed();
    }
    else if(chosen == unfixAll)
    {
        emit clearFixed();
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
    else if(actionName == selectAll->objectName())
    {
        selectAll->setEnabled(shouldBeEnabled);
    }
    else if(actionName == deselectAll->objectName())
    {
        deselectAll->setEnabled(shouldBeEnabled);
    }
    else if(actionName == fixAll->objectName())
    {
        fixAll->setEnabled(shouldBeEnabled);
    }
    else if(actionName == unfixAll->objectName())
    {
        unfixAll->setEnabled(shouldBeEnabled);
    }
}
