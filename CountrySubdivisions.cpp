#include "CountrySubdivisions.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>

static QMap<QString, QVector<SubdivisionInfo>>& subdivisionCache()
{
    static QMap<QString, QVector<SubdivisionInfo>> cache;
    return cache;
}

static bool& subdivisionsLoaded()
{
    static bool value = false;
    return value;
}

static QByteArray readSubdivisionsJson()
{
    const QStringList candidates = {
        QStringLiteral(":/python/subdivisions_data.json"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/python/subdivisions_data.json"),
        QDir::currentPath() + QStringLiteral("/python/subdivisions_data.json")
    };

    for (const QString& path : candidates) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly))
            return f.readAll();
    }

    return {};
}

static QVector<QVector<QPointF>> parseRings(const QJsonArray& ringsValue)
{
    QVector<QVector<QPointF>> rings;
    rings.reserve(ringsValue.size());

    for (const QJsonValue& ringValue : ringsValue) {
        const QJsonArray ringArray = ringValue.toArray();
        if (ringArray.size() < 2) continue;

        QVector<QPointF> ring;
        ring.reserve(ringArray.size());
        for (const QJsonValue& pointValue : ringArray) {
            const QJsonArray point = pointValue.toArray();
            if (point.size() < 2) continue;
            ring.append(QPointF(point.at(0).toDouble(), point.at(1).toDouble()));
        }

        if (ring.size() >= 2)
            rings.append(ring);
    }

    return rings;
}

void CountrySubdivisions::ensureLoaded()
{
    if (subdivisionsLoaded()) return;
    subdivisionsLoaded() = true;

    const QByteArray raw = readSubdivisionsJson();
    if (raw.isEmpty()) return;

    const QJsonObject root = QJsonDocument::fromJson(raw).object();
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
        const QString iso = it.key().trimmed().toUpper();
        const QJsonArray rows = it.value().toArray();
        if (iso.isEmpty() || rows.isEmpty()) continue;

        QVector<SubdivisionInfo> subdivisions;
        subdivisions.reserve(rows.size());
        for (const QJsonValue& rowValue : rows) {
            const QJsonObject obj = rowValue.toObject();
            SubdivisionInfo item;
            item.code = obj["code"].toString();
            item.name = obj["name"].toString();
            item.localName = obj["localName"].toString();
            item.type = obj["type"].toString();
            item.country = obj["country"].toString();
            item.labelLat = obj["lat"].toDouble();
            item.labelLon = obj["lon"].toDouble();
            item.rings = parseRings(obj["rings"].toArray());

            if (!item.name.isEmpty() && !item.rings.isEmpty())
                subdivisions.append(item);
        }

        if (!subdivisions.isEmpty())
            subdivisionCache().insert(iso, subdivisions);
    }
}

QVector<SubdivisionInfo> CountrySubdivisions::forIso(const QString& isoCode)
{
    ensureLoaded();
    return subdivisionCache().value(isoCode.trimmed().toUpper());
}

int CountrySubdivisions::countForIso(const QString& isoCode)
{
    ensureLoaded();
    return subdivisionCache().value(isoCode.trimmed().toUpper()).size();
}
