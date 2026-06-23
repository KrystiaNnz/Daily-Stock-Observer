#pragma once

#include <QPointF>
#include <QString>
#include <QVector>

struct SubdivisionInfo
{
    QString code;
    QString name;
    QString localName;
    QString type;
    QString country;
    double labelLat = 0.0;
    double labelLon = 0.0;
    QVector<QVector<QPointF>> rings;
};

class CountrySubdivisions
{
public:
    static QVector<SubdivisionInfo> forIso(const QString& isoCode);
    static int countForIso(const QString& isoCode);

private:
    static void ensureLoaded();
};
