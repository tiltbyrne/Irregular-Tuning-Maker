#include "interval.h"
#include "utilities.h"

Interval::Interval()
    : size{ 1 }
    , weight{ 1 }
{
}

Interval::Interval(const long double& s, const long double& w)
    : size{ clampLongDoubleToLimits(s) }
    , weight{ clampLongDoubleToLimits(w) }
{
    manageZeroWeight();
}

void Interval::setSize(const long double& newSize)
{
    size = clampLongDoubleToLimits(newSize);
}

void Interval::setWeight(const long double& newWeight)
{
    weight = clampLongDoubleToLimits(newWeight);
    manageZeroWeight();
}

void Interval::setInterval(const long double& newSize, const long double& newWeight)
{
    setSize(newSize);
    setWeight(newWeight);
}

void Interval::manageZeroWeight()
{
    if (weight <= 0)
        weight = std::numeric_limits<long double>::lowest();
}
