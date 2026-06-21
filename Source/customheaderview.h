#ifndef CUSTOMHEADERVIEW_H
#define CUSTOMHEADERVIEW_H

#include <QHeaderView>
#include <QObject>
#include <QContextMenuEvent>
#include <QMenu>

class CustomHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    explicit CustomHeaderView(int initialSize,
                              Qt::Orientation orientation,
                              QWidget *parent = nullptr);

public slots:
    void enableAction(const QString& actionName, const bool& shouldBeEnabled = true);
    void handleScaleSpaceSizeChanged(int newSize);

signals:
    void leftClicked(int note);
    void addNote(int noteToAdd);
    void deleteNote(int noteToDelete);
    void clearSelection();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QMenu* contextMenu;
    QAction* addAfter;
    QAction* addBefore;
    QAction* delNote;
    QAction* clear;

    int scaleSpaceSize;
};

#endif // CUSTOMHEADERVIEW_H
