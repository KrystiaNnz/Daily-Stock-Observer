#pragma once

#include <QString>
#include <QList>

struct DataProfile {
    QString id;
    QString name;
    QString description;
    bool isTest = false;
};

struct AppLanguage {
    QString code;
    QString name;
    QString description;
};

class ProfileManager
{
public:
    static QList<DataProfile> profiles();
    static DataProfile profile(const QString& id);
    static QString defaultProfileId();
    static QString normalizeProfileId(const QString& id);

    static void initialize();
    static QString activeProfileId();
    static DataProfile activeProfile();
    static void setActiveProfile(const QString& id);
    static void setStartupProfile(const QString& id);

    static bool askAtStartup();
    static void setAskAtStartup(bool enabled);
    static bool shouldAskAtStartup();

    static QList<AppLanguage> languages();
    static AppLanguage language(const QString& code);
    static QString defaultLanguageCode();
    static QString normalizeLanguageCode(const QString& code);
    static QString languageCode();
    static void setLanguageCode(const QString& code);

    static QString appDataRoot();
    static QString profileDataPath(const QString& id = QString());
    static QString databasePath(const QString& id = QString());
    static QString profileSettingsAppName(const QString& id = QString());
};
