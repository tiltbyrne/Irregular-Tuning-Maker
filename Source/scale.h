#ifndef SCALE_H
#define SCALE_H

//#include <numeric>
//#include <limits>
#include <vector>
#include <QtCore>
#include "interval.h"

using IntervalsPattern = std::vector<std::vector<Interval>>;
using SizeToWeightFunction = std::function<long double(const long double&)>;

/*
  A Scale represents a collection of notes as the ideal pattern intervals between those notes.
  It also contains the logic necessary to produce a tuning of itself, output by tuneScale().
  It is beyond the scope the documentation of this class to discuss exactly how this tuning
  is calculated, but it involves treating scales as complete weighted graphs.
*/
class Scale : public QObject
{
    Q_OBJECT
public://--------------------------------------------------------------------------------
    explicit Scale(QObject *parent = nullptr);

    /*
       Constructs a named scale with intervals pattern i. If i has non-triangular dimensions
       (defined by patternHasTriangularDimensions() function in Utilities.h) then the pattern is
       default.
    */
    explicit Scale(const IntervalsPattern& i, QObject *parent = nullptr);

    /*
      Returns the number of notes in the scale.
    */
    inline size_t size() const;

    /*
      Returns the number size of the intervals pattern.
    */
    inline size_t storedSize() const;

    /*
      Returns the weight at which scale traversal will be cut off.
    */
    long double getWeightCutoff() const;

    /*
      Returns the size of all intervals.
    */
    std::vector<std::vector<long double>> getSizes() const;

    /*
      Returns the weight of all intervals.
    */
    std::vector<std::vector<long double>> getWeights() const;

    bool unweighted() const;

public slots://--------------------------------------------------------------------------------

    void expandWeights(const long double& expansionFactor);

    void applyWeightFunction(SizeToWeightFunction function);

    /*
      Sets intervalsPattern to newIntervalsPattern if it has triangular dimensions, (defined by
      patternHasTriangularDimensions() function in Utilities.h).
    */
    void setIntervalsPattern(const IntervalsPattern& newIntervalsPattern);

    /*
      Sets the weight at which scale traversal will be cut off. Expects a normalised value
      between 0 and 1.
    */
    void setWeightCutoff(long double newWeightCutoff);

    /*
      Sets the weight of all intervals and normalises them.
    */
    void setWeights(const std::vector<std::vector<long double>>& newWeights);

    /*
      Sets the size of all intervals.
    */
    void setSizes(const std::vector<std::vector<long double>>& newSizes);

    void setUnweighted(bool shouldBeUnweighted = true);

    /*
      Produces a tuning of the scale. The tuning of the note at index = rootNote will always equal 1f
      Depending on the size of the scale and the weights of it's intervals weightCutoff can have a large
      influence on the time it takes for this function to return. WeightRangeIsSmallerThanPercentage
      also has a large influence on this time, as if it returns true then a much faster tuning algorithm is used.
    */
    std::vector<long double> tuneScale(std::atomic_bool& cancelationRequest,
                                       std::function<void(int)> progressCallback,
                                       const int& rootNote = 0) const;

signals://--------------------------------------------------------------------------------
    /*
      I could emit this if I need to pass on the message.
    */
    void cutoffUpdated(long double normalizedValue);

private://--------------------------------------------------------------------------------
    /*
      The ideal intervals between all notes in the scale. The interval between notes A and B is equal to
      the reciporical of the interval between B and A. The weight of both intervals is the same. So,
      we only need to store the interval between A and B for any value of A or B.
    */
    IntervalsPattern intervalsPattern;

    /*
      The total weight of an interval tuning a note at which scale traversal will be halted and that interval
      will be replaced by an interval from note 0 to the note at the index of that interval.
    */
    long double weightCutoff{ 0 };

    bool isUnweighted{ false };

    /*
      Accesses or calculates the value of the interval from noteFrom to noteTo depending on whether or not
      it is contained in intervalsPattern.
    */
    Interval getInterval(const int& noteTo, const int& noteFrom) const;

    /*
      Returns the product of weights of all notes in notesFrom to noteTo.
    */
    long double prodWeights(const int& noteFrom, const std::vector<int>& notesTo) const;

    /*
      Returns the sum of the product of all notes in notesFrom to eachother.
    */
    long double sumProdWeights(const std::vector<int>& notesFrom) const;

    /*
      Calculates the tuning of a single note for a scale, assuming a single rootNote.
    */
    long double tuneNote(int& note, std::atomic_bool& cancelationRequest) const;

    /*
      Iteratively traverses across the scale as if it were a graph. Iteration is halted by either finding a
      path which originates at 0, or arriving at a path whose rollingWeight < weightCutoff. Being that
      this function is called many times, it could be a sensible place to begin optimisation.
    */
    long double traversePath(const int& currentNoteIndex, const std::vector<int>& possibleNextNotes,
                             const long double& rollingWeight, const long double& reciporicalSumOfProdWeights,
                             std::atomic_bool& cancelationRequest) const;

    /*
      Manages calls to makeTuning() for all notes and tracks progress of this calculation.
    */
    std::vector<long double> makeWeightedTuning(std::atomic_bool& cancelationRequest,
                                                std::function<void(int)> progressCallback) const;

    /*
      Makes a tuning of the scale where all intervals have equal weight which can be significantly faster to
      calculate than a weighted tuning.
    */
    std::vector<long double> makeUnWeightedTuning() const;

    /*
      Normalises the weights of all intervals in intervalsPattern to a range of (0, 1].
    */
    void normaliseWeights();
};

#endif // SCALE_H
