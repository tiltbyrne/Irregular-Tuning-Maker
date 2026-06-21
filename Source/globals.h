#ifndef GLOBALS_H
#define GLOBALS_H

#include <QtCore>
#include <QString>

namespace globals
{
const QString appName{"Irregular Tuning Maker"};

const auto configPath{ QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/" +
                       appName };

constexpr int longDoubleLimit{ std::numeric_limits<long double>::max_digits10 };
};

#endif // GLOBALS_H
