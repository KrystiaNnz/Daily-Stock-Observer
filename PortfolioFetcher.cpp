#include "PortfolioFetcher.h"

#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCoreApplication>
#include <QDebug>

PortfolioFetcher::PortfolioFetcher(QObject* parent)
    : QObject(parent)
{}

QString PortfolioFetcher::scriptPath() const
{
    return QCoreApplication::applicationDirPath() + "/python/portfolio.py";
}

bool PortfolioFetcher::isCacheValid() const
{
    if (!m_cacheTime.isValid()) return false;
    return m_cacheTime.secsTo(QDateTime::currentDateTime()) < CACHE_TTL_MIN * 60;
}

void PortfolioFetcher::fetchPrices(const QStringList& tickers)
{
    if (tickers.isEmpty() || m_process) return;

    m_process = new QProcess(this);

    connect(m_process, &QProcess::finished,
            this, &PortfolioFetcher::onProcessFinished);

    connect(m_process, &QProcess::errorOccurred,
            this, &PortfolioFetcher::onProcessError);

    // Aliasy AppX (Microsoft Store Python) wymagają cmd.exe do rozwiązania
    m_process->start("cmd.exe", {"/C", "python", scriptPath(), tickers.join(",")});
}

void PortfolioFetcher::onProcessFinished(int exitCode, QProcess::ExitStatus /*status*/)
{
    QByteArray out = m_process->readAllStandardOutput();
    QByteArray err = m_process->readAllStandardError();

    m_process->deleteLater();
    m_process = nullptr;

    if (!err.isEmpty())
        qDebug() << "portfolio.py stderr:" << err;

    QJsonDocument doc = QJsonDocument::fromJson(out);

    // Skrypt zwrócił {"error": "..."} zamiast tablicy
    if (doc.isObject()) {
        QString msg = doc.object().value("error").toString();
        emit fetchError(msg.isEmpty() ? "Nieznany błąd portfolio.py" : msg);
        return;
    }

    if (!doc.isArray()) {
        QString raw = QString::fromUtf8(out).left(200);
        emit fetchError(raw.isEmpty()
            ? "portfolio.py nie zwróciło żadnych danych (pusta odpowiedź)"
            : "Nieoczekiwana odpowiedź: " + raw);
        return;
    }

    QMap<QString, PortfolioQuote> quotes;
    for (const QJsonValue& val : doc.array()) {
        QJsonObject obj = val.toObject();
        PortfolioQuote q;
        q.ticker          = obj["ticker"].toString();
        q.name            = obj["name"].toString();
        q.price           = obj["price"].toDouble();
        q.priceConverted  = obj["price_converted"].toDouble();
        q.changePct       = obj["change_pct"].toDouble();
        q.exchangeRate    = obj["exchange_rate"].toDouble(1.0);
        q.currency        = obj["currency"].toString();
        q.liveCurrency    = obj["live_currency"].toString();
        q.timestamp       = obj["timestamp"].toString();
        q.valid           = obj["valid"].toBool();
        q.error           = obj["error"].toString();
        quotes[q.ticker] = q;
    }

    m_cache     = quotes;
    m_cacheTime = QDateTime::currentDateTime();

    emit fetchFinished(m_cache);
}

void PortfolioFetcher::onProcessError()
{
    m_process->deleteLater();
    m_process = nullptr;
    emit fetchError("Nie można uruchomić portfolio.py — sprawdź, czy Python jest zainstalowany i dostępny w PATH.");
}
