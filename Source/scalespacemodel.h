#ifndef SCALESPACEMODEL_H
#define SCALESPACEMODEL_H
#include <QAbstractTableModel>
#include <QObject>

#include "scaleSpace.h"
#include "settings.h"
#include "interval.h"

using WeightFunction = std::function<long double(long double)>;

class ScaleSpaceModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ScaleSpaceModel(QObject *parent = nullptr);

    explicit ScaleSpaceModel(ScaleSpace* initialScaleSpace,
                             QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void setRange(int newRange);

    int getRange() const;

    void setScaleSpace(ScaleSpace* newScaleSpace);

    void setDisplayMode(DisplayMode newMode);

    void setIntervalMode(IntervalMode newMode);

    void setPrecision(const int& newPrecision);

    long double sizeValue(const int& noteFrom, const int& noteTo) const;
    //QString sizeText(const int& noteFrom, const int& noteTo) const;
    QString sizeText(long double intervalSize) const;

    long double weightValue(const int& noteFrom, const int& noteTo) const;

    //QString weightText(const int& noteFrom, const int& noteTo) const;
    QString weightText(long double intervalWeight) const;

    long double currentValue(const int& noteFrom, const int& noteTo) const;

    Interval interval(const int& noteFrom, const int& noteTo) const;

    void setAttenuation(int newAttenuation);

    int getPrecision() const;

    void setWeightMode(WeightMode newWeightMode);

    void setWeightFunction(const WeightFunction &newWeightFunction);

    IntervalMode getIntervalMode() const;

    DisplayMode getDisplayMode() const;

    void recomputeCache();

signals:
    void weightModeArbitrary();

private:
    int range{ 0 };

    int precision;

    int attenuation;

    ScaleSpace* scaleSpace{ nullptr };

    DisplayMode displayMode{ DisplayMode::ratio };

    IntervalMode intervalMode{ IntervalMode::size };

    WeightMode weightMode{  WeightMode::Deterministic};

    WeightFunction weightFunction;

    /*
    struct IntervalKey
    {
        int noteFrom;
        int noteTo;

        bool operator==(const IntervalKey& other) const
        {
            return noteFrom == other.noteFrom &&
                   noteTo   == other.noteTo;
        }

        bool operator<(const IntervalKey& other) const
        {
            return std::tie(noteFrom, noteTo) <
                   std::tie(other.noteFrom, other.noteTo);
        }
    };

    struct IntervalKeyHash
    {
        std::size_t operator()(const IntervalKey& key) const
        {
            std::size_t h1 = std::hash<int>{}(key.noteFrom);
            std::size_t h2 = std::hash<int>{}(key.noteTo);

            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };
*/
    struct CellCache
    {
        CellCache()
        {}
        CellCache(const Interval& i, const QString& s, const QString& w)
            : interval(i)
            , displaySize(s)
            , displayWeight(w)
        {}

        Interval interval;
        QString displaySize;
        QString displayWeight;
    };

    using TableCache = std::vector<CellCache>;
    mutable TableCache cache;

    //mutable std::unordered_map<IntervalKey, QString, IntervalKeyHash> sizeDisplayCache;

    //mutable std::unordered_map<IntervalKey, QString, IntervalKeyHash> weightDisplayCache;

    QString makeValue(int noteFrom, int noteTo) const;

    long double calculateIntervalWeight(const long double& intervalSize) const;

    long double makeDataValue(const QModelIndex &index, const QVariant &value, const int& role, bool& isValid) const;

    long double forceEditValueRatio(const long double& size) const;

    void setSizeData(const QModelIndex &index, const long double& data);

    void setWeightData(const QModelIndex &index, const long double& data);

    void updateCache(const int& noteFrom, const int& noteTo) const;

    void updateCacheWeight(const int& noteFrom, const int& noteTo) const;

    void updateCacheWeight(const long double& newWeight, const int& noteFrom, const int& noteTo) const;

    void precomputeCache(const int& oldRange = 0) const;

    void refreshSizeDisplayCache() const;

    void refreshWeightCache() const;

    void updateDisplayCaches() const;

    CellCache& cellCache(const int& noteFrom, const int& noteTo) const;

    void resizeCache(const int& oldRange) const;

    void resizeCacheColumns(const int& newRange) const;

    CellCache* cacheRow(const int& noteFrom) const;

    bool itemShouldBeAltColour(const int& noteFrom, const int& noteTo) const;
};

#endif // SCALESPACEMODEL_H
