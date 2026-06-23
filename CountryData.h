#pragma once

#include <QString>

struct CountryInfo {
    QString isoCode;
    QString name;
    QString capital;
    QString population;
    QString gdp;
    QString currency;
    QString languages;
    QString continent;
    QString area;
    QString centralBankName;
    QString centralBankCity;
    QString centralBankUrl;
    double  centralBankLat = 0.0;
    double  centralBankLon = 0.0;
    bool    hasCentralBankLocation = false;
};

class CountryData
{
public:
    static CountryInfo byIso(const QString& isoCode, const QString& fallbackName = QString());
};
