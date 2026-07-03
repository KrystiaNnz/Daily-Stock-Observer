#include "mainwindow.h"
#include "ProfileManager.h"
#include "ProfileSelectionDialog.h"
#include <QApplication>
#include <QLocale>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DailyStockObserver");
    app.setOrganizationName("Kryst");

    // Polskie nazwy dni i miesięcy w kalendarzu
    QLocale::setDefault(QLocale(QLocale::Polish, QLocale::Poland));

    ProfileManager::initialize();
    if (ProfileManager::shouldAskAtStartup()) {
        ProfileSelectionDialog profileDialog;
        if (profileDialog.exec() != QDialog::Accepted)
            return 0;

        ProfileManager::setActiveProfile(profileDialog.selectedProfileId());
        ProfileManager::setAskAtStartup(profileDialog.askAtStartup());
    }

    MainWindow w;
    w.show();

    return app.exec();
}
