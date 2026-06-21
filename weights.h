#ifndef WEIGHTS_H
#define WEIGHTS_H

#include <functional>
#include <QString>
#include "interval.h"
#include "ScaleSpace.h"
#include "settings.h"

using IntervalsPattern = std::vector<std::vector<Interval>>;
using SizeToWeightFunction = std::function<long double(const long double&)>;

IntervalsPattern makeIntervalsPattern(const ScaleSpace& space, const SizeToWeightFunction& function,
                                      const int& intervalsPatternSize);

std::array<std::pair<QString, SizeToWeightFunction>,
           settings::weightFunctionsSize> populateWeightFunctionsMap();

#endif // WEIGHTS_H
