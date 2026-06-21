#ifndef DBUTILS_H
#define DBUTILS_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include "scaleSpace.h"
#include "settings.h"

namespace dbUtils
{
const auto databaseDirectory{ globals::configPath + "/Pitch Spaces" };

const QString filetypeName{ "itm" };

static std::vector<std::pair<QString, IntervalSizePattern>> initialPatterns()
{
    return {
        {settings::scaleSpaceNames[0],
         {
             {8.L/7.L, 4.L/3.L, 3.L/2.L, 7.L/4.L, 2.L/1.L},
             {8.L/7.L, 4.L/3.L, 3.L/2.L, 7.L/4.L},
             {8.L/7.L, 4.L/3.L, 3.L/2.L},
             {8.L/7.L, 4.L/3.L},
             {8.L/7.L}
         }},
        {settings::scaleSpaceNames[1],
         {
             {10.L/9.L, 11.L/9.L, 4.L/3.L, 3.L/2.L, 18.L/11.L, 9.L/5.L, 2.L/1.L},
             {10.L/9.L, 11.L/9.L, 4.L/3.L, 3.L/2.L, 18.L/11.L, 9.L/5.L},
             {10.L/9.L, 11.L/9.L, 4.L/3.L, 3.L/2.L, 18.L/11.L},
             {10.L/9.L, 11.L/9.L, 4.L/3.L, 3.L/2.L},
             {10.L/9.L, 11.L/9.L, 4.L/3.L},
             {10.L/9.L, 11.L/9.L},
             {10.L/9.L}
         }},
        {settings::scaleSpaceNames[2],
         {
             {16.L/15.L, 9.L/8.L, 6.L/5.L, 5.L/4.L, 4.L/3.L, 45.L/32.L, 3.L/2.L, 8.L/5.L, 5.L/3.L, 9.L/5.L, 15.L/8.L, 2.L/1.L},
             {16.L/15.L, 9.L/8.L, 6.L/5.L, 5.L/4.L, 4.L/3.L, 45.L/32.L, 3.L/2.L, 8.L/5.L, 5.L/3.L, 9.L/5.L, 15.L/8.L},
             {16.L/15.L, 9.L/8.L, 6.L/5.L, 5.L/4.L, 4.L/3.L, 45.L/32.L, 3.L/2.L, 8.L/5.L, 5.L/3.L, 9.L/5.L},
             {16.L/15.L, 9.L/8.L, 6.L/5.L, 5.L/4.L, 4.L/3.L, 45.L/32.L, 3.L/2.L, 8.L/5.L, 5.L/3.L},
             {16.L/15.L, 9.L/8.L, 6.L/5.L, 5.L/4.L, 4.L/3.L, 45.L/32.L, 3.L/2.L, 8.L/5.L},
             {16.L/15.L, 9.L/8.L, 6.L/5.L, 5.L/4.L, 4.L/3.L, 45.L/32.L, 3.L/2.L},
             {16.L/15.L, 9.L/8.L, 6.L/5.L, 5.L/4.L, 4.L/3.L, 45.L/32.L},
             {16.L/15.L, 9.L/8.L, 6.L/5.L, 5.L/4.L, 4.L/3.L},
             {16.L/15.L, 9.L/8.L, 6.L/5.L, 5.L/4.L},
             {16.L/15.L, 9.L/8.L, 6.L/5.L},
             {16.L/15.L, 9.L/8.L},
             {16.L/15.L}
         }},
        {settings::scaleSpaceNames[3],
         {
             {9.L/8.L, 5.L/4.L, 4.L/3.L, 3.L/2.L, 5.L/3.L, 15.L/8.L, 2.L/1.L},
             {9.L/8.L, 6.L/5.L, 4.L/3.L, 3.L/2.L, 5.L/3.L, 9.L/5.L},
             {16.L/15.L, 6.L/5.L, 4.L/3.L, 3.L/2.L, 8.L/5.L},
             {9.L/8.L, 5.L/4.L, 45.L/32.L, 3.L/2.L},
             {9.L/8.L, 5.L/4.L, 4.L/3.L},
             {9.L/8.L, 6.L/5.L},
             {16.L/15.L}
         }},
        //{settings::scaleSpaceNames[4],
         //{
         //    {1200.L/14.L, 11.L/9.L, 4.L/3.L, 3.L/2.L, 18.L/11.L, 9.L/5.L, 2.L/1.L},
         //}},
        };
}

inline QString makeUrlString(const QString& name, const QString& directory)
{
    return { directory + "/" + name + "." + dbUtils::filetypeName };
}

};

#endif // DBUTILS_H
