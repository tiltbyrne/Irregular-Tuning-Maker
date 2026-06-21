#ifndef INTERVAL_H
#define INTERVAL_H

/*
  Musically, an interval between two notes is the factor you need to multiply one note by to a arrive
  at the other. Mathematically, this number is the interval's size, while it's weight represents how
  "important" it is (howver that is interpreted).
*/
class Interval
{
public:
    /*
      Constructs an interval of size and weight = 1;
    */
    Interval();

    /*
      Constructs an interval of size s and weight w. These values are clamped to limits if they exceed
      numeric limits.
    */
    Interval(const long double& s, const long double& w = 1);

    /*
      Returns the size of the interval.
    */
    inline long double getSize() const
    {
        return size;
    }

    /*
      Returns the weight of the interval.
    */
    inline long double getWeight() const
    {
        return weight;
    }

    /*
      Sets the size of the interval and clamps it's value to numeric limits if necassary.
    */
    void setSize(const long double& newSize);

    /*
      Sets the size of the interval, clamps it's value to numeric limits, and manages weight = 0 if necassary.
    */
    void setWeight(const long double& newWeight);

    /*
      Sets both the size and weight of the interval.
    */
    void setInterval(const long double& newSize, const long double& newWeight);

private:
    /*
      The distance between two notes.
    */
    long double size;
    /*
      The "importance" of the interval.
    */
    long double weight;

    /*
      Weights <= 0, which are impossible accordin to the model of tuning used as they can lead to division by
      0, are set to the numeric limits lowest value for long double.
    */
    void manageZeroWeight();
};

#endif // INTERVAL_H
