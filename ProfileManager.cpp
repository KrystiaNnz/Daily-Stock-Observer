#include "ProfileManager.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

namespace {
QString g_activeProfileId;

QSettings launcherSettings()
{
    return QSettings("Kryst", "DailyStockObserverLauncher");
}
}

QList<DataProfile> ProfileManager::profiles()
{
    return {
        {"private", "Prywatny", "Twoje wlasciwe dane, cele, portfel i notatki.", false},
        {"test", "Testowy", "Bezpieczne miejsce do testowania funkcji i danych probnych.", true},
    };
}

DataProfile ProfileManager::profile(const QString& id)
{
    const QString normalized = normalizeProfileId(id);
    for (const DataProfile& item : profiles()) {
        if (item.id == normalized)
            return item;
    }
    return profiles().first();
}

QString ProfileManager::defaultProfileId()
{
    return "private";
}

QString ProfileManager::normalizeProfileId(const QString& id)
{
    const QString clean = id.trimmed().toLower();
    for (const DataProfile& item : profiles()) {
        if (item.id == clean)
            return item.id;
    }
    return defaultProfileId();
}

QList<AppLanguage> ProfileManager::languages()
{
    return {
        {"pl", "Polski", "Interfejs aplikacji po polsku."},
        {"en", "English", "English application interface."},
    };
}

AppLanguage ProfileManager::language(const QString& code)
{
    const QString normalized = normalizeLanguageCode(code);
    for (const AppLanguage& item : languages()) {
        if (item.code == normalized)
            return item;
    }
    return languages().first();
}

QString ProfileManager::defaultLanguageCode()
{
    return "pl";
}

QString ProfileManager::normalizeLanguageCode(const QString& code)
{
    const QString clean = code.trimmed().toLower();
    for (const AppLanguage& item : languages()) {
        if (item.code == clean)
            return item.code;
    }
    return defaultLanguageCode();
}

QString ProfileManager::languageCode()
{
    QSettings settings = launcherSettings();
    return normalizeLanguageCode(settings.value("ui/language", defaultLanguageCode()).toString());
}

void ProfileManager::setLanguageCode(const QString& code)
{
    QSettings settings = launcherSettings();
    settings.setValue("ui/language", normalizeLanguageCode(code));
}

void ProfileManager::initialize()
{
    QSettings settings = launcherSettings();
    g_activeProfileId = normalizeProfileId(settings.value("profile/lastProfile", defaultProfileId()).toString());
}

QString ProfileManager::activeProfileId()
{
    if (g_activeProfileId.isEmpty())
        initialize();
    return g_activeProfileId;
}

DataProfile ProfileManager::activeProfile()
{
    return profile(activeProfileId());
}

void ProfileManager::setActiveProfile(const QString& id)
{
    g_activeProfileId = normalizeProfileId(id);
    QSettings settings = launcherSettings();
    settings.setValue("profile/lastProfile", g_activeProfileId);
}

void ProfileManager::setStartupProfile(const QString& id)
{
    QSettings settings = launcherSettings();
    settings.setValue("profile/lastProfile", normalizeProfileId(id));
}

bool ProfileManager::askAtStartup()
{
    QSettings settings = launcherSettings();
    return settings.value("profile/askAtStartup", false).toBool();
}

void ProfileManager::setAskAtStartup(bool enabled)
{
    QSettings settings = launcherSettings();
    settings.setValue("profile/askAtStartup", enabled);
}

bool ProfileManager::shouldAskAtStartup()
{
    QSettings settings = launcherSettings();
    return askAtStartup() || !settings.contains("profile/lastProfile");
}

QString ProfileManager::appDataRoot()
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (root.isEmpty())
        root = QDir::homePath() + "/DailyStockObserver";
    QDir().mkpath(root);
    return root;
}

QString ProfileManager::profileDataPath(const QString& id)
{
    const QString profileId = id.isEmpty() ? activeProfileId() : normalizeProfileId(id);
    const QString path = appDataRoot() + "/profiles/" + profileId;
    QDir().mkpath(path);
    return path;
}

QString ProfileManager::databasePath(const QString& id)
{
    return profileDataPath(id) + "/daily_stock_observer.db";
}

QString ProfileManager::profileSettingsAppName(const QString& id)
{
    const QString profileId = id.isEmpty() ? activeProfileId() : normalizeProfileId(id);
    return "DailyStockObserver_" + profileId;
}
