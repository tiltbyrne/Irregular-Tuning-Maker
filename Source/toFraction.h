#ifndef TOFRACTION_H
#define TOFRACTION_H

#include <utility>
#include <cmath>
#include "settings.h"

struct RationalApprox
{
    long double value;
    long double error;
};

static RationalApprox approximate(long double x)
{
    // clamp extreme values early
    if (!std::isfinite(x) || x <= 0)
        return {1.0L, 0.0L};

    // normalize to log space (this is key)
    const auto logx{ std::log2(x) };

    // map to nearest rational grid (bounded resolution)
    //const auto snapped{ std::round(logx * 24.0L) / 24.0L };

    const auto approx{ std::exp2(logx) };

    const auto error{ std::abs(x - approx) };

    return {approx, error};
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
