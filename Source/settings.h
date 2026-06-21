#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QSettings>
#include "globals.h"

namespace settings
{
//window

//parameters
constexpr int cutoff{ 50 };

constexpr int weightFunctionsSize{ 2 };

const QString uniformWeightFunctionName{ "Uniform" };

const QStringList weightFunctionNames{ "Inverse Benedetti",
                                       uniformWeightFunctionName };

const QString scaleSpaceName{ "Major Scale" };

const QString customWeightFuncName{ "Arbitrary" };

constexpr int range{ 6 };

constexpr int precision{ 4 };

constexpr int maxTableSize{ 256 };

constexpr int darknessFactor{ 100 };

constexpr int precisionMax{ globals::longDoubleLimit < 15 ? globals::longDoubleLimit
                                                          : 15 };

constexpr long double attenuationScaling{ 5 };

constexpr long double cutoffValueExponentNumerator{ 1000 };

const QString customScaleSpaceName{ "Open Custom..." };

const QStringList scaleSpaceNames{ "5edo",
                                   "7edo",
                                   "12edo",
                                   "Major Scale",
                                   //"14edo",
                                   customScaleSpaceName };

}

#endif // SETTINGS_H
