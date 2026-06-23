#include "mainwindow.h"
#include <QApplication>
#include <QLocale>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DailyStockObserver");
    app.setOrganizationName("Kryst");

    // Polskie nazwy dni i miesięcy w kalendarzu
    QLocale::setDefault(QLocale(QLocale::Polish, QLocale::Poland));

    MainWindow w;
    w.show();

    return app.exec();
}
