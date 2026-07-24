#ifndef SCALESPACE_H
#define SCALESPACE_H

#include <vector>
#include <QtCore>

using IntervalSizePattern = std::vector<std::vector<long double>>;

/*
  A ScaleSpace stores all data from which a scale can be constructed, but does not contain the logic to produce a tuning of itself.
  This will be used to store the data of scales in a scale space, which is a collection of scales which are related to eachother by
  their intervals.
*/
class ScaleSpace : public QObject
{
    Q_OBJECT
public:
    /*
        Constructs a named scale with a default intervals pattern.
    */
    explicit ScaleSpace(const QString& n, const IntervalSizePattern& r, QObject *parent = nullptr);

    //size as in the size of the array
    inline int storedSize() const
    {
        return sizePattern.size();
    }

    //size as in the number of notes
    inline int size() const
    {
        return storedSize() + 1;
    }

    void setScaleSpace(const QString& newName, const IntervalSizePattern& newSizePattern);

    long double getIntervalSize(int noteTo, int noteFrom) const;

    void setIntervalSize(const int& noteTo, const int& noteFrom, const long double& newSize);

    IntervalSizePattern getSizePattern() const;

    void setSizePattern(const IntervalSizePattern& newRelationPattern);

    int getBaseNote(const int& note) const;

    long double repetitionIntervalSize(const int& exponent = 1) const;

    QString getName() const;

    void setName(const QString &newName);

    IntervalSizePattern makeSubSizePattern(std::vector<int> notes) const;

    void addNote(int note, std::vector<long double> sizes = {}, bool addedNoteIsrepetition = false);

    void removeNote(int note);

public slots:

signals:
    void spaceTooSmall(bool);
    void sizeChanged(int newSize);

private:
    QString name;
    IntervalSizePattern sizePattern;

};

#endif // SCALESPACE_H
