#ifndef CUSTOMSPINBOX_H
#define CUSTOMSPINBOX_H

#include <QObject>
#include <QSpinBox>

class CustomSpinBox : public QSpinBox
{
public:
    explicit CustomSpinBox(QWidget *parent = nullptr);
protected:
    void focusInEvent(QFocusEvent *event) override;
    void stepBy(int steps) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // CUSTOMSPINBOX_H
