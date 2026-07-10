#include <QColor>
#include <QApplication>

#include "scalespacemodel.h"
#include "utilities.h"
#include "settings.h"
#include "fractionParser.h"

ScaleSpaceModel::ScaleSpaceModel(QObject *parent)
    : QAbstractTableModel(parent)
    , attenuation(settings::attenuationMidpoint)
    , precision(settings::precision)
    , weightFunction([](long double size){return 1.L;})
{
}

ScaleSpaceModel::ScaleSpaceModel(ScaleSpace *initialScaleSpace, QObject *parent)
    : QAbstractTableModel(parent)
    , scaleSpace(initialScaleSpace)
    , range(initialScaleSpace->size())
    , attenuation(settings::attenuationMidpoint)
    , oldAttenuation(settings::attenuationMidpoint)
    , precision(settings::precision)
    , weightFunction([](long double size){return 1.L;})
{
    updateCache();
}

int ScaleSpaceModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return range;
}

int ScaleSpaceModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return range;
}

QVariant ScaleSpaceModel::data(const QModelIndex &index, int role) const
{
    if ((role != Qt::DisplayRole /*&& role != Qt::BackgroundRole*/) || !index.isValid())
        return {};

    return makeValue(index.row(), index.column());
}

bool ScaleSpaceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.row() == index.column())
        return false;

    bool isValid{ false };
    const auto data{ makeDataValue(index, value, role, isValid) };

    if (!isValid)
        return false;

    if (intervalMode == IntervalMode::size)
    {
        setSizeData(index, data);
    }
    else if (intervalMode == IntervalMode::weight)
    {
        setWeightData(index, data);
    }

    return true;
}

Qt::ItemFlags ScaleSpaceModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

void ScaleSpaceModel::setRange(int newRange)
{
    if (range == newRange)
        return;

    beginResetModel();

    const auto oldRange{ range };

    range = newRange;

    updateCache(oldRange);

    endResetModel();
}

int ScaleSpaceModel::getRange() const
{
    return range;
}

void ScaleSpaceModel::setScaleSpace(ScaleSpace* newScaleSpace)
{
    beginResetModel();

    scaleSpace = newScaleSpace;

    updateCache();

    endResetModel();
}

void ScaleSpaceModel::setDisplayMode(DisplayMode newMode)
{
    if (displayMode == newMode)
        return;

    beginResetModel();

    displayMode = newMode;

    refreshSizeDisplayCache();

    endResetModel();
}

void ScaleSpaceModel::setIntervalMode(IntervalMode newMode)
{
    if (intervalMode == newMode)
        return;

    beginResetModel();

    intervalMode = newMode;

    endResetModel();
}

void ScaleSpaceModel::setPrecision(const int &newPrecision)
{
    if (precision == newPrecision)
        return;

    beginResetModel();

    precision = newPrecision;

    updateDisplayCaches();

    endResetModel();
}

long double ScaleSpaceModel::sizeValue(const int &noteFrom, const int &noteTo) const
{
    long double returnValue;

    if (noteFrom >= range || noteTo >= range)
        returnValue = calculateIntervalWeight(scaleSpace->getIntervalSize(noteTo, noteFrom));

    returnValue = cellCache(noteFrom, noteTo).interval.getSize();

    switch (displayMode)
    {
    case DisplayMode::ratio:
        return returnValue;
        break;
    case DisplayMode::cents:
        return centsFromRatio(returnValue);
        break;
    }

    return 1;
}

QString ScaleSpaceModel::makeValue(int noteFrom, int noteTo) const
{
    if (noteFrom >= range || noteTo >= range)
        return calculateValue(noteFrom, noteTo);

    switch (intervalMode)
    {
    case IntervalMode::size:
        return cellCache(noteFrom, noteTo).displaySize;
        break;

    case IntervalMode::weight:
        return cellCache(noteFrom, noteTo).displayWeight;
        break;
    }

    return QString("Invalid Interval Mode");
}

long double ScaleSpaceModel::calculateIntervalWeight(const long double &intervalSize) const
{
    if (weightMode == WeightMode::Deterministic)
    {
        const auto exponent{ std::pow(settings::attenuationScaling, 1 - (attenuation / settings::attenuationMidpoint)) };

        const auto base{ weightFunction(intervalSize) };

        return std::pow(base, exponent);
    }

    return 1;
}

long double ScaleSpaceModel::makeDataValue(const QModelIndex &index, const QVariant &value, const int& role, bool& isValid) const
{
    const auto canBeString{ value.canConvert<QString>() };

    if (!index.isValid() || !canBeString || role != Qt::EditRole)
    {
        isValid = false;
        return 1;
    }

    const auto text{ value.toString().trimmed() };

    const auto row{ index.row() },
        column{ index.column() };

    bool canBeDouble{ false };
    text.toDouble(&canBeDouble);
    const auto canBeFraction{ inputCanBeFraction(text) };

    if (!(canBeDouble || canBeFraction) || text.trimmed().isEmpty() || row == column)
    {
        isValid = false;
        return 1;
    }

    isValid = true;

    auto returnValue{ canBeFraction ? parseFraction(text) : qstld(text) };

    if (intervalMode == IntervalMode::size)
        returnValue = forceEditValueRatio(returnValue);

    return returnValue;
}

long double ScaleSpaceModel::forceEditValueRatio(const long double &size) const
{
    switch (displayMode)
    {
    case DisplayMode::ratio:
        return size;
        break;

    case DisplayMode::cents:
        return ratioFromCents(size);
        break;

    default:
        break;
    }

    return 1;
}

void ScaleSpaceModel::setSizeData(const QModelIndex &index, const long double &data)
{
    const auto row{ index.row() },
               column{ index.column() },
               scaleSpaceSize{ scaleSpace->storedSize() };

    if (std::abs(column - row) % scaleSpaceSize == 0) //interval is an 'octave'
    {
        scaleSpace->setIntervalSize(scaleSpaceSize,
                                    0,
                                    std::pow(data,
                                             static_cast<long double>(scaleSpaceSize /
                                             static_cast<long double>(column - row))));

        updateCache();

        emit dataChanged(this->index(0, 0), this->index(range - 1, range - 1), {Qt::DisplayRole, Qt::EditRole});
    }
    else //interval is a normal interval
    {
        auto sizeChange{ data / scaleSpace->getIntervalSize(column, row) };

        auto noteFrom{ scaleSpace->getBaseNote(row) },
             noteTo{ scaleSpace->getBaseNote(column) };

        if (noteFrom > noteTo)
        {
            std::swap(noteFrom, noteTo);
            sizeChange = 1.L / sizeChange;
        }

        scaleSpace->setIntervalSize(noteTo, noteFrom, scaleSpace->getIntervalSize(noteTo, noteFrom) * sizeChange);

        const auto origionalNoteTo{ noteTo };
        const auto difference{ noteTo - noteFrom };

        QModelIndexList indecies;

        while (noteFrom < range)
        {
            noteTo = origionalNoteTo;

            while (noteTo < range)
            {
                updateCache(noteFrom, noteTo);
                updateCache(noteTo, noteFrom);

                indecies.push_back(this->index(noteFrom, noteTo));
                indecies.push_back(this->index(noteTo, noteFrom));

                if (noteTo - noteFrom != difference && noteFrom + difference < range && noteTo - difference < range)
                {
                    updateCache(noteFrom + difference, noteTo - difference);

                    indecies.push_back(this->index(noteFrom + difference, noteTo - difference));
                }

                noteTo += scaleSpaceSize;
            }

            noteFrom += scaleSpaceSize;
        }

        for (const auto editedIndex : indecies)
            emit dataChanged(editedIndex, editedIndex, {Qt::DisplayRole, Qt::EditRole});
    }
}

void ScaleSpaceModel::setWeightData(const QModelIndex &index, const long double &data)
{
    if (weightMode == WeightMode::Deterministic)
        setWeightMode(WeightMode::Arbitrary);

    auto value{ data };
    if (value < 0.L)
        value = 0.L;

    const auto row{ index.row() },
               column{ index.column() };

    if (value < 1.L)
    {
        updateCacheWeight(value, row, column);
        updateCacheWeight(value, column, row);

        const auto fromToIdx{ this->index(row, column) },
                   toFromIdx{ this->index(column, row) };

        emit dataChanged(fromToIdx, fromToIdx, {Qt::DisplayRole, Qt::EditRole});
        emit dataChanged(toFromIdx, toFromIdx, {Qt::DisplayRole, Qt::EditRole});
    }
    else
    {
        updateCacheWeight(1.L, row, column);
        updateCacheWeight(1.L, column, row);

        for (auto noteFrom{ 0 }; noteFrom != range; ++noteFrom)
            for (auto noteTo{ noteFrom + 1 }; noteTo != range; ++noteTo)
            {
                if ((noteFrom == row && noteTo == column) ||
                    (noteTo == row && noteFrom == column))
                    continue;

                updateCacheWeight(weightValue(noteFrom, noteTo) / value, noteFrom, noteTo);
                updateCacheWeight(weightValue(noteTo, noteFrom) / value, noteTo, noteFrom);

                qDebug() << ldtqs(weightValue(noteFrom, noteTo));
                qDebug() << ldtqs(weightValue(noteTo, noteFrom));
            }

        emit dataChanged(this->index(0, 0), this->index(range - 1, range - 1), {Qt::DisplayRole, Qt::EditRole});
    }
}

void ScaleSpaceModel::updateCache(const int& noteFrom, const int& noteTo) const
{
    if (noteFrom >= range || noteTo >= range)
        return;

    const auto intervalSize{ scaleSpace->getIntervalSize(noteTo, noteFrom) };

    const auto intervalWeight{ calculateIntervalWeight(intervalSize) };

    cellCache(noteFrom, noteTo) = CellCache(Interval{intervalSize, intervalWeight},
                                            sizeText(intervalSize),
                                            weightText(intervalWeight));
}

void ScaleSpaceModel::updateCacheWeight(const int& noteFrom, const int& noteTo) const
{
    if (weightMode == WeightMode::Arbitrary)
    {
        const auto exponent{ std::pow(settings::attenuationScaling, 1 - (attenuation / settings::attenuationMidpoint)) };

        const auto base{ std::pow(cellCache(noteFrom, noteTo).interval.getWeight(),
                                  1.L / std::pow(settings::attenuationScaling, 1 - (oldAttenuation / settings::attenuationMidpoint))) };

        updateCacheWeight(std::pow(base, exponent), noteFrom, noteTo);
    }
    else
    {
        updateCacheWeight(calculateIntervalWeight(scaleSpace->getIntervalSize(noteTo, noteFrom)), noteFrom, noteTo);
    }
}

void ScaleSpaceModel::updateCacheWeight(long double newWeight, const int& noteFrom, const int& noteTo) const
{
    if (noteFrom >= range || noteTo >= range)
        return;

    cellCache(noteFrom, noteTo).interval.setWeight(newWeight);
    cellCache(noteTo, noteFrom).displayWeight = weightText(newWeight);
}

void ScaleSpaceModel::updateCache(const int& oldRange) const
{
    resizeCache(oldRange);

    const auto rangeDecreased{ range < oldRange };

    for (auto noteFrom{ 0 }; noteFrom != range; ++noteFrom)
    {
        if (rangeDecreased)
            continue;

        for (auto noteTo{ noteFrom < oldRange ? oldRange : 0 }; noteTo != range; ++noteTo)
            updateCache(noteFrom, noteTo);
    }
}

void ScaleSpaceModel::refreshSizeDisplayCache() const
{
    for (auto noteFrom{ 0 }; noteFrom != range; ++noteFrom)
        for (auto noteTo{ 0 }; noteTo != range; ++noteTo)
            cellCache(noteFrom, noteTo).displaySize = sizeText(cellCache(noteFrom, noteTo).interval.getSize());
}

void ScaleSpaceModel::refreshWeightCache() const
{
    for (auto noteFrom{ 0 }; noteFrom != range; ++noteFrom)
        for (auto noteTo{ 0 }; noteTo != range; ++noteTo)
            updateCacheWeight(noteFrom, noteTo);
}

void ScaleSpaceModel::updateDisplayCaches() const
{
    for (auto noteFrom{ 0 }; noteFrom != range; ++noteFrom)
        for (auto noteTo{ 0 }; noteTo != range; ++noteTo)
        {
            cellCache(noteFrom, noteTo).displaySize = sizeText(cellCache(noteFrom, noteTo).interval.getSize());
            cellCache(noteFrom, noteTo).displayWeight = weightText(cellCache(noteFrom, noteTo).interval.getWeight());
        }
}

void ScaleSpaceModel::resizeCache(const int& oldRange) const
{
    std::vector<CellCache> newCache(range * range);

    const int copyRange = std::min(oldRange, range);

    for (int row = 0; row < copyRange; ++row)
        std::copy_n(cache.begin() + row * oldRange,
                    copyRange,
                    newCache.begin() + row * range);

    cache.swap(newCache);
}

ScaleSpaceModel::CellCache& ScaleSpaceModel::cellCache(const int &noteFrom, const int &noteTo) const
{
    return cache.at(noteFrom * range + noteTo);
}

QString ScaleSpaceModel::calculateValue(const int& noteFrom, const int& noteTo) const
{
    const auto intervalSize{ scaleSpace->getIntervalSize(noteTo, noteFrom) };

    const auto modeIsCents{ displayMode == DisplayMode::cents };

    if (intervalMode == IntervalMode::size)
    {
        return ldtqs(modeIsCents ? centsFromRatio(intervalSize) : intervalSize, precision, modeIsCents);
    }
    else
    {
        return ldtqs(calculateIntervalWeight(intervalSize), precision, true);
    }
}

QString ScaleSpaceModel::sizeText(long double intervalSize) const
{
    const auto modeIsCents{ displayMode == DisplayMode::cents };

    if (modeIsCents)
        intervalSize = centsFromRatio(intervalSize);

    return ldtqs(intervalSize, precision, modeIsCents);
}

long double ScaleSpaceModel::weightValue(const int &noteFrom, const int &noteTo) const
{
    if (noteFrom >= range || noteTo >= range)
        return calculateIntervalWeight(scaleSpace->getIntervalSize(noteTo, noteFrom));

    return cellCache(noteFrom, noteTo).interval.getWeight();
}

long double ScaleSpaceModel::currentValue(const int &noteFrom, const int &noteTo) const
{
    switch (intervalMode)
    {
    case IntervalMode::size:
        return sizeValue(noteFrom, noteTo);
        break;

    case IntervalMode::weight:
        return weightValue(noteFrom, noteTo);
        break;
    }
}

QString ScaleSpaceModel::weightText(long double intervalWeight) const
{
    return ldtqs(intervalWeight, precision, true);
}

Interval ScaleSpaceModel::interval(const int &noteFrom, const int &noteTo) const
{
    if (noteFrom >= range || noteTo >= range)
    {
        const auto intervalSize{ scaleSpace->getIntervalSize(noteTo, noteFrom) };

        return Interval{intervalSize, calculateIntervalWeight(intervalSize)};
    }

    return cellCache(noteFrom, noteTo).interval;
}

void ScaleSpaceModel::setAttenuation(int newAttenuation)
{
    if (attenuation == newAttenuation)
        return;

    beginResetModel();

    oldAttenuation = attenuation;

    attenuation = newAttenuation;

    refreshWeightCache();

    endResetModel();
}

int ScaleSpaceModel::getPrecision() const
{
    return precision;
}

void ScaleSpaceModel::setWeightMode(WeightMode newWeightMode)
{
    if (weightMode == newWeightMode)
        return;

    weightMode = newWeightMode;

    if (newWeightMode == WeightMode::Arbitrary)
        emit weightModeArbitrary();
}

void ScaleSpaceModel::setWeightFunction(const WeightFunction &newWeightFunction)
{
    beginResetModel();

    weightFunction = newWeightFunction;

    setWeightMode(WeightMode::Deterministic);

    refreshWeightCache();

    endResetModel();
}

IntervalMode ScaleSpaceModel::getIntervalMode() const
{
    return intervalMode;
}

DisplayMode ScaleSpaceModel::getDisplayMode() const
{
    return displayMode;
}

void ScaleSpaceModel::reset()
{
    beginResetModel();

    updateCache();

    endResetModel();
}

WeightMode ScaleSpaceModel::getWeightMode() const
{
    return weightMode;
}

QString ScaleSpaceModel::defaultText() const
{
    switch (intervalMode)
    {
    case IntervalMode::size:
        return displayMode == DisplayMode::ratio ? "1" : "0";
        break;
    case IntervalMode::weight:
        return "1";
        break;
    }
}
