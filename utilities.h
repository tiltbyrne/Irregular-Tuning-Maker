#ifndef UTILITIES_H
#define UTILITIES_H

#include <cmath>
#include <vector>
#include <algorithm>
#include <QString>

#include "globals.h"

template <typename T>
static std::vector<T> normalisedVector(const std::vector<T>& v)
{
    const auto max{ *max_element(v.begin(), v.end()) };

    auto normalisedVector{ v };
    for (auto& element : normalisedVector)
        element /= max;

    normalisedVector;
}

template <typename T>
static std::vector<std::vector<T>> normalisedMatrix(const std::vector<std::vector<T>>& m)
{
    auto max{ 0 };
    for (const auto& v : m)
    {
        auto vectorMax{ *max_element(v.begin(), v.end()) };
        if (vectorMax > max)
            max = vectorMax;
    }

    auto normalisedMatrix{ m };
    for (auto& v : normalisedMatrix)
        std::for_each(v.begin(), v.end(), [&max](T& n) { n /= max; });

    normalisedMatrix;
}

static int posMod(int a, int b)
{
    if (b == 0)
        return 0;

    return ((a % b) + std::abs(b)) % std::abs(b);
}

/*
  Returns the greatest common denominator of integers a and b, used by Fraction.
*/
static int gcd(int a, int b)
{
    while (b != 0)
    {
        int temp = b;
        b = a % b;
        a = temp;
    }

    return std::abs(a);
}
/*
  Returns base raised to exponent.
*/
static int integerPower(int base, int exponent)
{
    int result = 1;
    for (;;)
    {
        if (exponent & 1)
            result *= base;
        exponent >>= 1;
        if (!exponent)
            break;
        base *= base;
    }

    return result;
}

/*
  Returns cents values (one 100th of a 12edo semitone) from a frequency ratio.
*/
static inline long double centsFromRatio(const long double& ratio)
{
    return 1200 * std::log2(ratio);
}

static inline long double ratioFromCents(const long double& cents)
{
    return std::pow(2, cents / 1200.);
}

/*
  Returns the frequency (Hz) of a ratio from baseFrequency.
*/
static inline double frequencyFromRatio(const double& ratio, const double& baseFrequency)
{
    return ratio * baseFrequency;
}

/*
  Enforces value does not exceed it's numeric limits and if it is NaN returns 0.
*/
static long double clampLongDoubleToLimits(const long double& value)
{
    constexpr long double min{ std::numeric_limits<long double>::min() },
        max{ std::numeric_limits<long double>::max() };

    if (value < min)
        return min;

    if (value > max)
        return max;

    return value;
}

/*
  Returns true if pattern is triangular. A pattern is triangular if pattern[n].size() == pattern[n + 1].size() + 1
  for all vectors of T in pattern.
*/
template<typename T>
static bool patternHasTriangularDimensions(const std::vector<std::vector<T>>& pattern)
{
    for (auto rowSize{ 0 }; rowSize != pattern.size(); ++rowSize)
        if (pattern[rowSize].size() != pattern.size() - rowSize)
            return false;

    return true;
}

static QString ldtqs(const long double& value, const int& significantFigures = globals::longDoubleLimit)
{
    std::ostringstream oss;

    oss << std::setprecision(significantFigures)
        << std::scientific
        << value;

    return QString::fromStdString(oss.str());
}

static QString ldtqs(const long double& value, const int& significantFigures, const bool& shouldBeScientific)
{
    std::ostringstream oss;

    oss << std::setprecision(significantFigures);

    if (shouldBeScientific)
        oss << std::scientific;

    oss << value;

    return QString::fromStdString(oss.str());
}

static long double qstld(const QString& string)
{
    return std::stold(string.toStdString());
}

static long double factorial(const int& n)
{
    if (n <= 1)
        return 1;

    auto result = 1.L;

    for (auto i{ 1 }; i <= n; ++i)
    {
        result *= i;

        if (result > std::numeric_limits<long double>::max())
            return std::numeric_limits<long double>::infinity();
    }

    return result;
}

#endif // UTILITIES_H
