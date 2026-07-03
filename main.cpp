#include "mainwindow.h"
#include "ProfileManager.h"
#include "ProfileSelectionDialog.h"
#include <QApplication>
#include <QLocale>

namespace {
QLocale localeForLanguage(const QString& code)
{
    const QString language = ProfileManager::normalizeLanguageCode(code);
    if (language == "en")
        return QLocale(QLocale::English, QLocale::UnitedStates);
    return QLocale(QLocale::Polish, QLocale::Poland);
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DailyStockObserver");
    app.setOrganizationName("Kryst");

    // Polskie nazwy dni i miesięcy w kalendarzu
    ProfileManager::initialize();
    QLocale::setDefault(localeForLanguage(ProfileManager::languageCode()));
    if (ProfileManager::shouldAskAtStartup()) {
        ProfileSelectionDialog profileDialog;
        if (profileDialog.exec() != QDialog::Accepted)
            return 0;

        ProfileManager::setActiveProfile(profileDialog.selectedProfileId());
        ProfileManager::setLanguageCode(profileDialog.selectedLanguageCode());
        ProfileManager::setAskAtStartup(profileDialog.askAtStartup());
        QLocale::setDefault(localeForLanguage(ProfileManager::languageCode()));
    }

    MainWindow w;
    w.show();

    return app.exec();
}
