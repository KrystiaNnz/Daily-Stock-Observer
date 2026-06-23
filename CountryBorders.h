#pragma once

#include <QPointF>
#include <QString>
#include <QVector>

class CountryBorders
{
public:
    static QVector<QVector<QPointF>> polygonsForIso(const QString& isoCode);

private:
    static void ensureLoaded();
};
