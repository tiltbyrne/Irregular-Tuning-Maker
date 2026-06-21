#include "scaleSpace.h"
#include "utilities.h"
#include <QDebug>

ScaleSpace::ScaleSpace(const QString& n, const IntervalSizePattern& s, QObject *parent)
    : name(n)
    , sizePattern(patternHasTriangularDimensions(s) ? s : IntervalSizePattern{})
{
    emit sizeChanged(size());
}

long double ScaleSpace::getIntervalSize(int noteTo, int noteFrom) const
{
    auto floorDiv = [](const int& x, const int& y)
    {
        auto q{ x / y };
        const auto r{ x % y };

        if (r != 0 && ((r < 0) != (y < 0)))//what is this logic?? thanks chatGPT
            --q;

        return q;
    };

    const auto patternSize{ storedSize() };

    const auto wrapNoteTo{ floorDiv(noteTo, patternSize) };
    const auto wrapNoteFrom{ floorDiv(noteFrom, patternSize) };

    noteTo -= wrapNoteTo * patternSize;
    noteFrom -= wrapNoteFrom * patternSize;

    if (noteTo == noteFrom)
    {
        return std::pow(repetitionIntervalSize(), wrapNoteTo - wrapNoteFrom);
    }
    else if (noteTo > noteFrom)
    {
        return sizePattern[noteFrom][noteTo - noteFrom - 1] *
               std::pow(repetitionIntervalSize(), wrapNoteTo - wrapNoteFrom);
    }
    else
    {
        return (1.L / sizePattern[noteTo][noteFrom - noteTo - 1]) *
               std::pow(repetitionIntervalSize(), wrapNoteTo - wrapNoteFrom);
    }
}

IntervalSizePattern ScaleSpace::getSizePattern() const
{
    return sizePattern;
}

void ScaleSpace::setSizePattern(const IntervalSizePattern& newRelationPattern)
{
    if (patternHasTriangularDimensions(newRelationPattern))
        sizePattern = newRelationPattern;

    emit sizeChanged(size());
}

long double ScaleSpace::repetitionIntervalSize(const int& exponent) const
{
    return sizePattern.empty() ? 0 : std::pow(sizePattern[0].back(), exponent);
}

QString ScaleSpace::getName() const
{
    return name;
}

void ScaleSpace::setName(const QString &newName)
{
    name = newName;
}

int ScaleSpace::getBaseNote(const int& note) const
{
    const auto floorDiv{ [](const int& x, const int& y)
    {
        auto q{ x / y };
        const auto r{ x % y };

        if (r != 0 && ((r < 0) != (y < 0)))//what is this logic?? thanks chatGPT
            --q;

        return q;
    }};

    const auto patternSize{ storedSize() };

    return note - floorDiv(note, patternSize) * storedSize();
}

void ScaleSpace::setIntervalSize(const int& noteTo, const int& noteFrom, const long double& newSize)
{
    if (noteFrom >= storedSize() || noteTo - noteFrom - 1 >= sizePattern[noteFrom].size())
        return;

    sizePattern[noteFrom][noteTo - noteFrom - 1] = newSize;
}

void ScaleSpace::setScaleSpace(const QString& newName, const IntervalSizePattern& newSizePattern)
{
    setName(newName);
    setSizePattern(newSizePattern);
}

IntervalSizePattern ScaleSpace::makeSubSizePattern(std::vector<int> notes) const
{
    const auto notesSize{ notes.size() };

    if (notesSize == 0)
        return sizePattern;

    std::sort(notes.begin(), notes.end());

    IntervalSizePattern subScaleSpace;
    subScaleSpace.reserve(notesSize);

    for (auto noteFromIdx{ 0 }; noteFromIdx != notesSize - 1; ++noteFromIdx)
    {
        subScaleSpace.push_back(std::vector<long double>());
        subScaleSpace.back().reserve(notesSize - noteFromIdx - 1);

        for (auto noteToIdx{ noteFromIdx + 1 }; noteToIdx != notesSize; ++noteToIdx)
            subScaleSpace.back().push_back(
                getIntervalSize(notes[noteToIdx], notes[noteFromIdx]));
    }

    return subScaleSpace;
}

void ScaleSpace::addNote(int note, std::vector<long double> sizes)
{
    const auto preInsertionSize{ size() };

    note %= size() - 1;

    if (note == 0)
        note = size() - 1;

    if (sizes.size() != preInsertionSize)
        sizes.resize(preInsertionSize, 1.L);

    for (auto noteFrom{ 0 }; noteFrom < note; ++noteFrom)
        sizePattern[noteFrom].insert(sizePattern[noteFrom].begin() + note - noteFrom - 1,
                                     1.L / sizes[noteFrom]);

    std::vector<long double> sizesFromInsertedNote;

    for (auto noteTo{ note + 1 }; noteTo <= preInsertionSize; ++noteTo)
        sizesFromInsertedNote.push_back(sizes[noteTo - 1]);

    sizePattern.insert(sizePattern.begin() + note,
                       std::move(sizesFromInsertedNote));

    if (preInsertionSize <= 2 && preInsertionSize != size())
        emit spaceTooSmall(false);

    emit sizeChanged(size());
}

void ScaleSpace::removeNote(int note)
{
    const auto preRemovalSize{ size() };

    if (preRemovalSize == 0)
        return;

    note %= size() - 1;

    if (note == 0)
    {
        std::vector<int> notes(storedSize() - 1);
        std::iota(notes.begin(), notes.end(), 1);
        notes.push_back(size());

        setSizePattern(makeSubSizePattern(notes));
    }
    else
    {
        for (auto noteFrom{ 0 }; noteFrom < note; ++noteFrom)
            sizePattern[noteFrom].erase(
                sizePattern[noteFrom].begin()
                + note - noteFrom - 1);

        sizePattern.erase(sizePattern.begin() + note);
    }

    if (storedSize() == 1)
        emit spaceTooSmall(true);

    emit sizeChanged(size());
}
