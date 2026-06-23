#include "CountryBorders.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>

static QMap<QString, QVector<QVector<QPointF>>>& borderCache()
{
    static QMap<QString, QVector<QVector<QPointF>>> cache;
    return cache;
}

static bool& loaded()
{
    static bool value = false;
    return value;
}

static QByteArray readBordersJson()
{
    const QStringList candidates = {
        QStringLiteral(":/python/borders_data.json"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/python/borders_data.json"),
        QDir::currentPath() + QStringLiteral("/python/borders_data.json")
    };

    for (const QString& path : candidates) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly))
            return f.readAll();
    }

    return {};
}

void CountryBorders::ensureLoaded()
{
    if (loaded()) return;
    loaded() = true;

    const QByteArray raw = readBordersJson();
    if (raw.isEmpty()) return;

    const QJsonArray rows = QJsonDocument::fromJson(raw).array();
    for (const QJsonValue& value : rows) {
        const QJsonObject obj = value.toObject();
        const QString iso = obj["iso"].toString().toUpper();
        const QJsonArray coords = obj["coords"].toArray();
        if (iso.isEmpty() || coords.size() < 3) continue;

        QVector<QPointF> polygon;
        polygon.reserve(coords.size());
        for (const QJsonValue& coordValue : coords) {
            const QJsonArray coord = coordValue.toArray();
            if (coord.size() < 2) continue;
            const double lon = coord.at(0).toDouble();
            const double lat = coord.at(1).toDouble();
            polygon.append(QPointF(lon, lat));
        }

        if (polygon.size() >= 3)
            borderCache()[iso].append(polygon);
    }
}

QVector<QVector<QPointF>> CountryBorders::polygonsForIso(const QString& isoCode)
{
    ensureLoaded();
    return borderCache().value(isoCode.trimmed().toUpper());
}
