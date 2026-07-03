#pragma once

#include <QObject>
#include <QProcess>
#include <QList>
#include "DatabaseManager.h"

class HistoricalPriceFetcher : public QObject
{
    Q_OBJECT

public:
    explicit HistoricalPriceFetcher(QObject* parent = nullptr);

    bool isFetching() const { return m_process != nullptr; }
    void fetch(const QString& ticker,
               const QString& startDate,
               const QString& endDate,
               const QString& interval = "1d");

signals:
    void fetchFinished(const QString& ticker,
                       const QString& interval,
                       const QString& source,
                       const QString& currency,
                       const QList<MarketPriceBar>& bars);
    void fetchError(const QString& message);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError();

private:
    QString scriptPath() const;

    QProcess* m_process = nullptr;
    QString m_ticker;
    QString m_interval;
};
