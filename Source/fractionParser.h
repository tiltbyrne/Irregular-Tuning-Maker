#ifndef FRACTIONPARSER_H
#define FRACTIONPARSER_H

#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QRegularExpression>

inline bool inputCanBeFraction(QString input)
{
    input = input.trimmed();
    QRegularExpression fractionRegEx(R"(^\s*(-?\d+)(?:\s+(\d+)/(\d+)|/(\d+))?\s*$)");
    QRegularExpressionMatch match = fractionRegEx.match(input);

    return match.hasMatch();
}

// Function to parse a fraction string like "3/4", "-2/5", "5 1/2" into long double
inline long double parseFraction(const QString& input)
{
    QString trimmed = input.trimmed();

    // Match patterns: "a/b" or "a b/c"
    QRegularExpression re(R"(^\s*(-?\d+)(?:\s+(\d+)/(\d+)|/(\d+))?\s*$)");
    QRegularExpressionMatch match = re.match(trimmed);

    if (!match.hasMatch())
    {
        throw std::invalid_argument("Invalid fraction format");
    }

    int whole = match.captured(1).toInt();
    int num = 0;
    int den = 1;

    if (!match.captured(2).isEmpty() && !match.captured(3).isEmpty()) // Mixed number: whole num/den
    {
        num = match.captured(2).toInt();
        den = match.captured(3).toInt();
        if (den == 0) throw std::runtime_error("Denominator cannot be zero");
        // Convert to improper fraction
        if (whole < 0)
            num = whole * den - num;
        else
            num = whole * den + num;
    }
    else if (!match.captured(4).isEmpty()) // Simple fraction: whole/den
    {
        num = whole;
        den = match.captured(4).toInt();
        if (den == 0) throw std::runtime_error("Denominator cannot be zero");
    }
    else // Just an integer
    {
        num = whole;
        den = 1;
    }

    return static_cast<long double>(num) / static_cast<long double>(den != 0 ? den : 1);
}

#endif // FRACTIONPARSER_H
