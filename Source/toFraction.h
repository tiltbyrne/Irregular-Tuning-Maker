#ifndef TOFRACTION_H
#define TOFRACTION_H

#include <utility>
#include <cmath>
#include "settings.h"

static std::pair<long long, long long> approximateFraction(long double value,
                                                           long double epsilon = settings::epsilon,
                                                           long long maxDenominator = 1000000,
                                                           int maxIterations = 64)
{
    if (!std::isfinite(value))
        return {0, 1};

    if (value == 0.0L)
        return {0, 1};

    const bool negative = value < 0;
    value = std::abs(value);

    long long h0 = 0, h1 = 1;
    long long k0 = 1, k1 = 0;

    long double x = value;

    for (int i = 0; i < maxIterations; ++i)
    {
        const auto a = static_cast<long long>(std::floor(x));

        // Overflow guard
        if (a != 0 &&
            (h1 > (std::numeric_limits<long long>::max() - h0) / a ||
             k1 > (std::numeric_limits<long long>::max() - k0) / a))
            break;

        const long long h = a * h1 + h0;
        const long long k = a * k1 + k0;

        if (k > maxDenominator)
            break;

        const long double approximation =
            static_cast<long double>(h) / k;

        if (std::abs(approximation - value) <= epsilon)
            return {negative ? -h : h, k};

        const long double frac = x - a;

        if (frac <= std::numeric_limits<long double>::epsilon())
            return {negative ? -h : h, k};

        h0 = h1;
        h1 = h;
        k0 = k1;
        k1 = k;

        x = 1.0L / frac;
    }

    return {negative ? -h1 : h1, k1};
}

/*
static std::pair<long long, long long> toFraction(long double value,
                                                  long double epsilon = std::pow(10, -settings::precisionMax))
{
    long long h1{ 1 }, h2{ 0 };
    long long k1{ 0 }, k2{ 1 };

    auto b{ value };

    while (true)
    {
        const auto a{ static_cast<long long>(std::floor(b)) };

        long long h = a * h1 + h2;
        long long k = a * k1 + k2;

        const auto approx{ static_cast<long double>(h) / k };

        const auto diff{ (value > approx) ? value - approx : approx - value };
        if (diff <= epsilon)
            return {h, k};

        h2 = h1;
        h1 = h;
        k2 = k1;
        k1 = k;

        long double frac = b - a;

        if (frac == 0.0L)
            break;

        b = 1.0L / frac;
    }

    return {h1, k1};
}

static long double sumFractionParts(const long double& value)
{
    const auto [numerator, denominator] = toFraction(value);

    return numerator * denominator;
}
*/

#endif // TOFRACTION_H
