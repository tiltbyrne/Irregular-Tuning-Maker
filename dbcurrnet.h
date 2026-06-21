#ifndef DBCURRNET_H
#define DBCURRNET_H

#include "dbUtils.h"

class dbCurrnet
{
public:
    explicit dbCurrnet(const QString& path);

    ~dbCurrnet();

    bool savePattern(const IntervalSizePattern& pattern);

    IntervalSizePattern loadPattern();

    bool open();

private:
    bool createTables();

    QString path;

    QString connectionName;

    QSqlDatabase database;
};

#endif // DBCURRNET_H
