#ifndef DBMANAGER_H
#define DBMANAGER_H

#include "dbcurrnet.h"

struct dbManager
{
    static void initialise();

    static QStringList databases();

    static bool createDatabase(const QString& name);

    static bool createDatabase(const QUrl& url);

    static std::unique_ptr<dbCurrnet> openDatabase(const QString& name,
                                                   const QString& directory = dbUtils::databaseDirectory);

    static std::unique_ptr<dbCurrnet> openDatabase(const QUrl& url);
};

#endif // DBMANAGER_H
