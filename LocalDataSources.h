#pragma once

#include <QString>
#include <QVector>

struct LocalDataSourceInfo
{
    QString iso;
    QString country;
    QString sourceName;
    QString websiteUrl;
    QString apiStatus;
    QString apiType;
    QString apiUrl;
    QString fallbackName;
    QString fallbackUrl;
    QString notes;
    QString lastChecked;
};

class LocalDataSources
{
public:
    static QVector<LocalDataSourceInfo> allSources();
    static LocalDataSourceInfo sourceForIso(const QString& isoCode);
    static bool hasSourceForIso(const QString& isoCode);

private:
    static void ensureLoaded();
};
