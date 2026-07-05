#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QSettings>
#include <QPalette>
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

const QString customWeightFuncName{ "Arbitrary" };

const std::pair<int, int> attenuationRange{0, 100};

const auto attenuationMidpoint{ (static_cast<long double>(settings::attenuationRange.second) -
                                 static_cast<long double>(settings::attenuationRange.first)) / 2.L };

//constexpr int range{ 6 };

constexpr int precision{ 4 };

constexpr int maxTableSize{ 4096 };

constexpr int precisionMax{ globals::longDoubleLimit < 15 ? globals::longDoubleLimit
                                                          : 15 };

const long double epsilon{ std::pow(10.L, - static_cast<long double>(precisionMax)) };

constexpr long double attenuationScaling{ 10 };

constexpr long double cutoffValueExponentNumerator{ 1000 };

const QString customScaleSpaceName{ "Open Custom..." };

const QStringList scaleSpaceNames{ "5edo",
                                   "7edo",
                                   "12edo",
                                   "Major Scale",
                                   "15edo",
                                   "17edo",
                                   "19edo",
                                   customScaleSpaceName };

const QString scaleSpaceName{ scaleSpaceNames[3] };

const QColor highlight{255, 128, 0};

}

enum class DisplayMode
{
    ratio = 0,
    cents = 1
};

enum class IntervalMode
{
    size = 0,
    weight = 1
};

enum class WeightMode
{
    Deterministic = 0,
    Arbitrary = 1
};

#endif // SETTINGS_H
