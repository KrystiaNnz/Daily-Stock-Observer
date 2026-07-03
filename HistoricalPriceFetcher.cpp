#include "HistoricalPriceFetcher.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

HistoricalPriceFetcher::HistoricalPriceFetcher(QObject* parent)
    : QObject(parent)
{}

QString HistoricalPriceFetcher::scriptPath() const
{
    return QCoreApplication::applicationDirPath() + "/python/historical_prices.py";
}

void HistoricalPriceFetcher::fetch(const QString& ticker,
                                   const QString& startDate,
                                   const QString& endDate,
                                   const QString& interval)
{
    if (ticker.trimmed().isEmpty() || m_process)
        return;

    m_ticker = ticker.trimmed().toUpper();
    m_interval = interval.isEmpty() ? "1d" : interval;

    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished,
            this, &HistoricalPriceFetcher::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &HistoricalPriceFetcher::onProcessError);

    m_process->start("cmd.exe", {"/C", "python", scriptPath(), "fetch",
                                 m_ticker, startDate, endDate, m_interval});
}

void HistoricalPriceFetcher::onProcessFinished(int /*exitCode*/, QProcess::ExitStatus /*status*/)
{
    const QByteArray out = m_process->readAllStandardOutput();
    const QByteArray err = m_process->readAllStandardError();
    m_process->deleteLater();
    m_process = nullptr;

    QJsonDocument doc = QJsonDocument::fromJson(out);
    if (!doc.isObject()) {
        const QString raw = QString::fromUtf8(out).left(200);
        emit fetchError(raw.isEmpty()
            ? "historical_prices.py nie zwrocilo danych"
            : "Nieoczekiwana odpowiedz: " + raw);
        return;
    }

    QJsonObject obj = doc.object();
    const QString error = obj.value("error").toString();
    if (!error.isEmpty()) {
        emit fetchError(error);
        return;
    }

    const QString ticker = obj.value("ticker").toString(m_ticker).toUpper();
    const QString interval = obj.value("interval").toString(m_interval);
    const QString source = obj.value("source").toString("yfinance");
    const QString currency = obj.value("currency").toString();

    QList<MarketPriceBar> bars;
    const QJsonArray rows = obj.value("rows").toArray();
    for (const QJsonValue& value : rows) {
        const QJsonObject row = value.toObject();
        MarketPriceBar bar;
        bar.ticker = ticker;
        bar.interval = interval;
        bar.source = source;
        bar.currency = currency;
        bar.timestamp = row.value("timestamp").toString();
        bar.tradingDate = row.value("trading_date").toString();
        bar.open = row.value("open").toDouble();
        bar.high = row.value("high").toDouble();
        bar.low = row.value("low").toDouble();
        bar.close = row.value("close").toDouble();
        bar.adjClose = row.value("adj_close").toDouble();
        bar.volume = row.value("volume").toDouble();
        if (!bar.timestamp.isEmpty())
            bars.append(bar);
    }

    if (!err.isEmpty()) {
        // yfinance bywa rozmowne na stderr; same ostrzezenia nie blokuja danych.
    }
    emit fetchFinished(ticker, interval, source, currency, bars);
}

void HistoricalPriceFetcher::onProcessError()
{
    m_process->deleteLater();
    m_process = nullptr;
    emit fetchError("Nie mozna uruchomic historical_prices.py - sprawdz Python i yfinance.");
}
