#include "weights.h"
#include "settings.h"
#include "toFraction.h"
//whatever librarys are needed for populating functionsMap should be able to be kept seperate here, then
//I can include WeightFunctions.h in other places and not have to worry about large binaries, right?

IntervalsPattern makeIntervalsPattern(const ScaleSpace& space, const SizeToWeightFunction& function,
                                      const int& intervalsPatternSize)
{
    if (intervalsPatternSize == 0)
        return {};

    IntervalsPattern intervalsPattern;
    intervalsPattern.reserve(intervalsPatternSize);

    for (auto fromNote{ 0 }; fromNote != intervalsPatternSize; ++fromNote)
    {
        intervalsPattern.push_back({});
        intervalsPattern[fromNote].reserve((intervalsPatternSize - fromNote));

        for (auto toNote{ 0 }; toNote != intervalsPatternSize - fromNote; ++toNote)
        {
            const auto intervalSize{ space.getIntervalSize(toNote, fromNote) };

            intervalsPattern[fromNote].push_back({intervalSize, function(intervalSize)});
        }
    }

    return intervalsPattern;
}

std::array<std::pair<QString, SizeToWeightFunction>,
           settings::weightFunctionsSize> populateWeightFunctionsMap()
{
    return {
        {
        {
            settings::customWeightFuncName[0],
            [](const long double& size)
            {
              auto returnValue{ 1 / sumFractionParts(size) };
              return std::clamp(returnValue, 0.L, 1.L);
            }
        },
        {
            settings::customWeightFuncName[1],
            [](const long double& size)
            {
                return 1;
            }
        }}
    };
}
