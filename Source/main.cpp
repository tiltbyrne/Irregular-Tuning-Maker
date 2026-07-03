#include <QApplication>
#include "mainwindow.h"
//#include "settings.h"
#include "dbManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(globals::appName);
    app.setApplicationDisplayName(globals::appName);
    dbManager::initialise();

    QApplication::setStyle("Fusion");

    QPalette pal = qApp->palette();
    pal.setColor(QPalette::Highlight, QColor(255, 128, 0));
    pal.setColor(QPalette::Accent, QColor(255, 128, 0));
    qApp->setPalette(pal);

    MainWindow w;
\
    w.show();

    return app.exec();
}
