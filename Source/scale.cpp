#include "scale.h"
#include "utilities.h"
#include <algorithm>
#include <QDebug>

Scale::Scale(QObject *parent)
{
}

Scale::Scale(const IntervalsPattern& i, QObject *parent)
    : intervalsPattern(patternHasTriangularDimensions(i) ? i : IntervalsPattern{})
{
}

inline size_t Scale::size() const
{
    return intervalsPattern.size() + 1;
}

inline size_t Scale::storedSize() const
{
    return intervalsPattern.size();
}

void Scale::setIntervalsPattern(const IntervalsPattern& newIntervalsPattern)
{
    if (patternHasTriangularDimensions(newIntervalsPattern))
    {
        intervalsPattern = newIntervalsPattern;

        normaliseWeights();
    }
}

long double Scale::getWeightCutoff() const
{
    return weightCutoff;
}

bool Scale::unweighted() const
{
    return isUnweighted;
}

void Scale::setWeightCutoff(long double newWeightCutoff)
{    
    weightCutoff = std::clamp(newWeightCutoff, 0.L, 1.L);

    //emit cutoffUpdated(newWeightCutoff);
}

void Scale::setUnweighted(bool shouldBeUnweighted)
{
    isUnweighted = shouldBeUnweighted;
}

std::vector<std::vector<long double>> Scale::getWeights() const
{
    std::vector<std::vector<long double>> weights;
    weights.reserve(storedSize());

    for (auto noteFrom{0}; noteFrom != storedSize(); ++noteFrom)
    {
        weights.push_back({});
        weights[noteFrom].reserve(storedSize() - noteFrom);
        for (auto noteTo{0}; noteTo != storedSize()- noteFrom; ++noteTo)
            weights[noteFrom].push_back(intervalsPattern[noteFrom][noteTo].getWeight());
    }

    return weights;
}

void Scale::setWeights(const std::vector<std::vector<long double>>& newWeights)
{
    if (patternHasTriangularDimensions(newWeights) && newWeights.size() == storedSize())
        for (auto noteFrom{0}; noteFrom != storedSize(); ++noteFrom)
            for (auto noteTo{0}; noteTo != storedSize()- noteFrom; ++noteTo)
                intervalsPattern[noteFrom][noteTo].setWeight(newWeights[noteFrom][noteTo]);

    normaliseWeights();
}

std::vector<std::vector<long double>> Scale::getSizes() const
{
    std::vector<std::vector<long double>> sizes;
    sizes.reserve(storedSize());

    for (auto noteFrom{0}; noteFrom != storedSize(); ++noteFrom)
    {
        sizes.push_back({});
        sizes[noteFrom].reserve(storedSize() - noteFrom);
        for (auto noteTo{0}; noteTo != storedSize()- noteFrom; ++noteTo)
            sizes[noteFrom].push_back(intervalsPattern[noteFrom][noteTo].getSize());
    }

    return sizes;
}

void Scale::setSizes(const std::vector<std::vector<long double>>& newSizes)
{
    if (patternHasTriangularDimensions(newSizes) && newSizes.size() == storedSize())
        for (auto noteFrom{0}; noteFrom != storedSize(); ++noteFrom)
            for (auto noteTo{0}; noteTo != storedSize()- noteFrom; ++noteTo)
                intervalsPattern[noteFrom][noteTo].setSize(newSizes[noteFrom][noteTo]);
}

Interval Scale::getInterval(const int& noteTo, const int& noteFrom) const
{
    if (noteFrom > noteTo)
        return { 1L / intervalsPattern[noteTo][noteFrom - noteTo - 1].getSize(),
                intervalsPattern[noteTo][noteFrom - noteTo - 1].getWeight() };

    if (noteTo == noteFrom)
        return { 1, 1 };

    return intervalsPattern[noteFrom][noteTo - noteFrom - 1];
}

std::vector<long double> Scale::tuneScale(std::atomic_bool& cancelationRequest,
                                          std::function<void(int)> progressCallback,
                                          const int& rootNote) const
{
    if (cancelationRequest)
    {
        progressCallback(0);
        return {};
    }

    const auto tuning{isUnweighted ? makeUnWeightedTuning()
                                   : makeWeightedTuning(cancelationRequest, progressCallback)};

    progressCallback(!cancelationRequest * 100);

    return tuning;
}

long double Scale::prodWeights(const int& noteFrom, const std::vector<int>& notesTo) const
{
    if (notesTo.empty())
        return 0;

    long double prod{ 1 };

    for (const auto& noteTo : notesTo)
        prod *= getInterval(noteFrom, noteTo).getWeight();

    return clampLongDoubleToLimits(prod);
}

long double Scale::sumProdWeights(const std::vector<int>& notesFrom) const
{
    long double sum{ 0 };

    for (const auto& noteFrom : notesFrom)
        sum += prodWeights(noteFrom, notesFrom);

    return sum;
}

long double Scale::tuneNote(int& note, std::atomic_bool& cancelationRequest) const
{
    if (cancelationRequest || note == 0)
        return 1;

    std::vector<int> nextNotes;
    nextNotes.reserve(size());

    for (auto i{0}; i != size(); ++i)
        nextNotes.push_back(i);

    const auto reciporicalSumOfProdWeights{ 1 / sumProdWeights(nextNotes) };

    return traversePath(note,
                        nextNotes,
                        1,
                        reciporicalSumOfProdWeights,
                        cancelationRequest);
}

long double Scale::traversePath(const int& currentNoteIndex, const std::vector<int>& possibleNextNotes,
                                const long double& rollingWeight, const long double& reciporicalSumOfProdWeights,
                                std::atomic_bool& cancelationRequest) const
{
    if (cancelationRequest)
        return 1;

    const auto currentNote{ possibleNextNotes[currentNoteIndex] };

    auto nextPossibleNextNotes{ possibleNextNotes };
    nextPossibleNextNotes.erase(nextPossibleNextNotes.begin() + currentNoteIndex);

    const auto nextReciporicalSumProdWeights{ 1 / sumProdWeights(nextPossibleNextNotes) };

    long double returnValue{ 1 };

    for (auto nextNoteIndex{ 0 }; nextNoteIndex != possibleNextNotes.size(); ++nextNoteIndex)
    {
        const auto nextNote{ possibleNextNotes[nextNoteIndex] };
        const auto exponent{ clampLongDoubleToLimits(prodWeights(nextNote, possibleNextNotes) * reciporicalSumOfProdWeights) };

        const auto intervalSize{ (nextNote == 0 || nextNote == currentNote || exponent * rollingWeight <= weightCutoff)
                ? getInterval(currentNote, 0).getSize()
                : getInterval(currentNote, nextNote).getSize() * traversePath(nextNoteIndex - (currentNoteIndex < nextNoteIndex ? 1 : 0),
                                                                              nextPossibleNextNotes,
                                                                              clampLongDoubleToLimits(exponent * rollingWeight),
                                                                              nextReciporicalSumProdWeights,
                                                                              cancelationRequest)
        };

        returnValue *= std::pow(intervalSize, exponent);
    }

    return returnValue;
}

std::vector<long double> Scale::makeWeightedTuning(std::atomic_bool& cancelationRequest,
                                                   std::function<void(int)> progressCallback) const
{
    if (cancelationRequest)
    {
        progressCallback(0);
        return {};
    }

    std::vector<long double> tuning;
    tuning.reserve(size());

    for (auto note{ 0 }; note != size(); ++note)
    {
        tuning.push_back(tuneNote(note, cancelationRequest));

        progressCallback( static_cast<float>(note) * 100.f / static_cast<float>(size() - 1) );
    }

    progressCallback(!cancelationRequest * 100);

    return tuning;
}

std::vector<long double> Scale::makeUnWeightedTuning() const
{
    std::vector<long double> tuning;
    tuning.reserve(size());

    for (auto noteTo{ 0 }; noteTo != size(); ++noteTo)
    {
        tuning.push_back(1);

        if (noteTo == 0)
            continue;

        for (auto noteFrom{ 0 }; noteFrom != size(); ++noteFrom)
            tuning[noteTo] *= clampLongDoubleToLimits(
                (noteFrom == 0 || noteFrom == noteTo) ? getInterval(noteTo, 0).getSize()
                                                      : getInterval(noteTo, noteFrom).getSize() *
                                                        getInterval(noteFrom, 0).getSize());

        tuning[noteTo] = std::pow(tuning[noteTo], 1.0 / size());
    }

    return tuning;
}

void Scale::normaliseWeights()
{
    setWeights(normalisedMatrix(getWeights()));
}

void Scale::applyWeightFunction(SizeToWeightFunction function)
{
    if (storedSize() == 0)
        return;

    for (auto noteFrom{ 0 }; noteFrom != storedSize(); ++noteFrom)
        for (auto noteTo{ 0 }; noteTo != storedSize() - noteFrom; ++noteTo)
        {
            auto& interval{ intervalsPattern[noteFrom][noteTo] };

            interval.setWeight(function(interval.getSize()));
        }
}

void Scale::expandWeights(const long double& expansionFactor)
{
    for (auto& intervals : intervalsPattern)
        for (auto& interval : intervals)
            interval.setWeight(std::pow(interval.getWeight(), 1 - expansionFactor));
}
