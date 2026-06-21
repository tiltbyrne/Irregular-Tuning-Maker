#include "dbcurrnet.h"
#include "utilities.h"

dbCurrnet::dbCurrnet(
    const QString& path)
    : path(path)
    , connectionName(QUuid::createUuid().toString())
{
    //connectionName = QUuid::createUuid().toString();
}

dbCurrnet::~dbCurrnet()
{
    database.close();
    database = QSqlDatabase();

    QSqlDatabase::removeDatabase(connectionName);
}

bool dbCurrnet::open()
{
    database = QSqlDatabase::addDatabase("QSQLITE", connectionName);

    database.setDatabaseName(path);

    if (!database.open())
        return false;

    return createTables();
}

bool dbCurrnet::createTables()
{
    QSqlQuery query(database);

    return query.exec(R"(
        CREATE TABLE IF NOT EXISTS matrix (
            row_id INTEGER,
            col_id INTEGER,
            value TEXT,
            PRIMARY KEY(row_id, col_id)
        )
    )");
}

bool dbCurrnet::savePattern(const IntervalSizePattern& pattern)
{
    QSqlQuery query(database);

    query.exec("DELETE FROM matrix");

    query.prepare(R"(
        INSERT INTO matrix
        (row_id, col_id, value)
        VALUES (?, ?, ?)
    )");

    for (auto row{ 0 }; row != pattern.size(); ++row)
        for (auto column{ 0 }; column != pattern[row].size(); ++column)
        {
            query.addBindValue(row);
            query.addBindValue(column);

            query.addBindValue(ldtqs(pattern[row][column]));

            query.exec();
        }

    return true;
}

IntervalSizePattern dbCurrnet::loadPattern()
{
    IntervalSizePattern pattern;

    QSqlQuery query(R"(
        SELECT row_id, col_id, value
        FROM matrix
        ORDER BY row_id, col_id
    )", database);

    while (query.next())
    {
        const auto row{ query.value(0).toInt() };

        const auto col{ query.value(1).toInt() };

        const auto value{ std::stold(query.value(2).toString().toStdString())};

        if (pattern.size() <= row)
            pattern.resize(row + 1);

        if (pattern[row].size() <= col)
            pattern[row].resize(col + 1);

        pattern[row][col] = value;
    }

    return pattern;
}
