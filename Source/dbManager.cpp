#include "dbManager.h"
#include "dbUtils.h"

void dbManager::initialise()
{
    QDir().mkpath(dbUtils::databaseDirectory);

    const auto initialPatterns{ dbUtils::initialPatterns() };

    for (const auto& [name, pattern] : initialPatterns)
        if (createDatabase(name))
            openDatabase(name)->savePattern(pattern);
}

QStringList dbManager::databases()
{
    const QDir dir(dbUtils::databaseDirectory);

    auto databases { dir.entryList({"*." + dbUtils::filetypeName},
                     QDir::Files) };

    for (auto& database : databases)
        database.chop(dbUtils::filetypeName.size() + 1);

    return databases;
}

bool dbManager::createDatabase(const QString& name)
{
    auto db{ openDatabase(name) };

    return db != nullptr;
}

bool dbManager::createDatabase(const QUrl& url)
{
    auto db{ openDatabase(url) };

    return db != nullptr;
}

std::unique_ptr<dbCurrnet> dbManager::openDatabase(const QString& name, const QString& directory)
{
    const auto path{ dbUtils::makeUrlString(name, directory) };

    auto db{ std::make_unique<dbCurrnet>(path) };

    if (!db->open())
        return nullptr;

    return db;
}

std::unique_ptr<dbCurrnet> dbManager::openDatabase(const QUrl& url)
{
    auto db{ std::make_unique<dbCurrnet>(url.toLocalFile()) };

    if (!db->open())
        return nullptr;

    return db;
}
