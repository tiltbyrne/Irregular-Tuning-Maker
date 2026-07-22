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

static IntervalSizePattern edoFromSizeArray(const std::vector<long double>& sizeArray)
{
    IntervalSizePattern returnVector;
    returnVector.reserve(sizeArray.size());

    for (auto note{ 0 }; note != sizeArray.size(); ++note)
    {
        returnVector.push_back({});
        returnVector.back().reserve(sizeArray.size() - note);

        for (auto itr{ sizeArray.begin() }; itr != sizeArray.end() - note; ++itr)
            returnVector.back().push_back(*itr);
    }

    return returnVector;
}

static std::vector<std::pair<QString, IntervalSizePattern>> initialPatterns()
{
    return {
        {
            settings::scaleSpaceNames[0],
            edoFromSizeArray({8.L/7.L, 4.L/3.L, 3.L/2.L, 7.L/4.L, 2.L})},
        {
            settings::scaleSpaceNames[1],
            edoFromSizeArray({10.L/9.L, 11.L/9.L, 4.L/3.L, 3.L/2.L, 18.L/11.L,
                              9.L/5.L, 2.L})
        },
        {
            settings::scaleSpaceNames[2],
            edoFromSizeArray({16.L/15.L, 9.L/8.L, 6.L/5.L, 5.L/4.L, 4.L/3.L,
                              45.L/32.L, 3.L/2.L, 8.L/5.L, 5.L/3.L, 9.L/5.L,
                              15.L/8.L, 2.L})
        },
        {
            settings::scaleSpaceNames[3],
            edoFromSizeArray({std::pow(2.L, 1.L/15.L), 11.L/10.L, 8.L/7.L, 6.L/5.L, 5.L/4.L,
                              4.L/3.L, 11.L/8.L, 13.L/9.L, 3.L/2.L, 8.L/5.L,
                              5.L/3.L, 7.L/4.L, 11.L/6.L, std::pow(2.L, 14.L/15.L), 2.L})
        },
        {
            settings::scaleSpaceNames[4],
            edoFromSizeArray({25.L/24.L, 12.L/11.L, 9.L/8.L, 13.L/11.L, 11.L/9.L,
                              9.L/7.L, 4.L/3.L, 11.L/8.L, 13.L/9.L, 3.L/2.L,
                              11.L/7.L, 13.L/8.L, 12.L/7.L, 7.L/4.L, 11.L/6.L,
                              27.L/14.L, 2.L})
        },
        {
            settings::scaleSpaceNames[5],
            edoFromSizeArray({28.L/27.L, 15.L/14.L, 9.L/8.L, 7.L/6.L, 6.L/5.L,
                              5.L/4.L, 9.L/7.L, 4.L/3.L, 7.L/5.L, 10.L/7.L,
                              3.L/2.L, 14.L/9.L, 8.L/5.L, 5.L/3.L, 7.L/4.L,
                              9.L/5.L, 15.L/8.L, 27.L/14.L, 2.L})
        },
        {
            settings::scaleSpaceNames[6],
            edoFromSizeArray({std::pow(2.L, 1.L/22.L), 17.L/16.L, 11.L/10.L, 9.L/8.L, 7.L/6.L,
                              6.L/5.L, 5.L/4.L, 9.L/7.L, 4.L/3.L, 11.L/8.L,
                              7.L/5.L, 16.L/11.L, 3.L/2.L, 14.L/9.L, 8.L/5.L,
                              5.L/3.L, 12.L/7.L, 7.L/4.L, 11.L/6.L, 15.L/8.L,
                              std::pow(2.L, 21.L/22.L), 2.L})
        },
    };
}

inline QString makeUrlString(const QString& name, const QString& directory)
{
    return { directory + "/" + name + "." + dbUtils::filetypeName };
}

};

#endif // DBUTILS_H
