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
    explicit CustomHeaderView(Qt::Orientation orientation,
                              QWidget *parent = nullptr);

public slots:
    void enableAction(const QString& actionName, const bool& shouldBeEnabled = true);

signals:
    void leftClicked(int note);
    void addNote(int noteToAdd, bool cameFromAddBefore = false);
    void deleteNote(int noteToDelete);
    void selectNote(int note);
    void fillSelection();
    void clearSelection();
    void fixNote(int note);
    void fillFixed();
    void clearFixed();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QMenu* contextMenu;
    QAction* addAfter;
    QAction* addBefore;
    QAction* delNote;
    QAction* select;
    QAction* selectAll;
    QAction* deselectAll;
    QAction* fix;
    QAction* fixAll;
    QAction* unfixAll;
};

#endif // CUSTOMHEADERVIEW_H
