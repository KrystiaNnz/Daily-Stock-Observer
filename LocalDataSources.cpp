#include "LocalDataSources.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <algorithm>

static QMap<QString, LocalDataSourceInfo>& localSourceCache()
{
    static QMap<QString, LocalDataSourceInfo> cache;
    return cache;
}

static bool& localSourcesLoaded()
{
    static bool value = false;
    return value;
}

static QByteArray readLocalSourcesJson()
{
    const QStringList candidates = {
        QStringLiteral(":/python/local_data_sources.json"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/python/local_data_sources.json"),
        QDir::currentPath() + QStringLiteral("/python/local_data_sources.json")
    };

    for (const QString& path : candidates) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly))
            return f.readAll();
    }

    return {};
}

static QString stringValue(const QJsonObject& obj, const char* key)
{
    return obj[QString::fromLatin1(key)].toString().trimmed();
}

void LocalDataSources::ensureLoaded()
{
    if (localSourcesLoaded()) return;
    localSourcesLoaded() = true;

    const QByteArray raw = readLocalSourcesJson();
    if (raw.isEmpty()) return;

    const QJsonArray rows = QJsonDocument::fromJson(raw).array();
    for (const QJsonValue& value : rows) {
        const QJsonObject obj = value.toObject();

        LocalDataSourceInfo item;
        item.iso = stringValue(obj, "iso").toUpper();
        item.country = stringValue(obj, "country");
        item.sourceName = stringValue(obj, "source_name");
        item.websiteUrl = stringValue(obj, "website_url");
        item.apiStatus = stringValue(obj, "api_status");
        item.apiType = stringValue(obj, "api_type");
        item.apiUrl = stringValue(obj, "api_url");
        item.fallbackName = stringValue(obj, "fallback_name");
        item.fallbackUrl = stringValue(obj, "fallback_url");
        item.notes = stringValue(obj, "notes");
        item.lastChecked = stringValue(obj, "last_checked");

        if (!item.iso.isEmpty())
            localSourceCache().insert(item.iso, item);
    }
}

QVector<LocalDataSourceInfo> LocalDataSources::allSources()
{
    ensureLoaded();

    QVector<LocalDataSourceInfo> rows;
    rows.reserve(localSourceCache().size());
    for (auto it = localSourceCache().constBegin(); it != localSourceCache().constEnd(); ++it)
        rows.append(it.value());

    std::sort(rows.begin(), rows.end(), [](const LocalDataSourceInfo& a, const LocalDataSourceInfo& b) {
        return QString::localeAwareCompare(a.country, b.country) < 0;
    });

    return rows;
}

LocalDataSourceInfo LocalDataSources::sourceForIso(const QString& isoCode)
{
    ensureLoaded();
    return localSourceCache().value(isoCode.trimmed().toUpper());
}

bool LocalDataSources::hasSourceForIso(const QString& isoCode)
{
    ensureLoaded();
    return localSourceCache().contains(isoCode.trimmed().toUpper());
}
