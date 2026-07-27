#include <QApplication>
#include "mainwindow.h"
#include "dbManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(globals::appName);
    app.setApplicationDisplayName(globals::appName);

    dbManager::initialise();

    initialisePalette();

    MainWindow w;
\
    const auto args{ QCoreApplication::arguments() };

    if (args.size() > 1)
        w.changeScaleSpace(args[1]);

    w.show();

    return app.exec();
}
