#include <QSpinBox>
#include <QLineEdit>
#include <QTimer>
#include <QEvent>

#include "customspinbox.h"

CustomSpinBox::CustomSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    lineEdit()->setReadOnly(true);
    lineEdit()->setFocusPolicy(Qt::NoFocus);
    lineEdit()->installEventFilter(this);
}

void CustomSpinBox::focusInEvent(QFocusEvent *event)
{
    QSpinBox::focusInEvent(event);

    QTimer::singleShot(0, this, [this] {
        lineEdit()->deselect();
    });
}

void CustomSpinBox::stepBy(int steps)
{
    QSpinBox::stepBy(steps);

    QTimer::singleShot(0, this, [this] {
        lineEdit()->deselect();
    });
}

bool CustomSpinBox::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == lineEdit())
    {
        switch (event->type())
        {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::FocusIn:
            return true;    // Eat the event.
        default:
            break;
        }
    }

    return QSpinBox::eventFilter(obj, event);
}
