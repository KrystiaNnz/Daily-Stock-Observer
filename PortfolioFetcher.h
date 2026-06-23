#pragma once

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QDateTime>
#include <QProcess>

// Kurs pojedynczego instrumentu zwrócony przez portfolio.py
struct PortfolioQuote {
    QString ticker;
    QString name;
    double  price          = 0.0;  // cena live w oryginalnej walucie giełdy
    double  priceConverted = 0.0;  // cena live w walucie zakupu (po przewalutowaniu)
    double  changePct      = 0.0;
    double  exchangeRate   = 1.0;  // kurs live_currency → buy_currency
    QString currency;              // waluta zakupu (buy currency)
    QString liveCurrency;          // waluta live z giełdy
    QString timestamp;
    bool    valid          = false;
    QString error;
};

// Asynchroniczne pobieranie kursów przez yfinance (subprocess Python)
class PortfolioFetcher : public QObject
{
    Q_OBJECT

public:
    explicit PortfolioFetcher(QObject* parent = nullptr);

    // Uruchamia portfolio.py; ignoruje wywołanie jeśli fetch już trwa
    void fetchPrices(const QStringList& tickers);

    QMap<QString, PortfolioQuote> cachedQuotes() const { return m_cache; }

    // true gdy ostatnie pobranie odbyło się mniej niż CACHE_TTL_MIN minut temu
    bool isCacheValid() const;

    bool isFetching() const { return m_process != nullptr; }

signals:
    void fetchFinished(const QMap<QString, PortfolioQuote>& quotes);
    void fetchError(const QString& message);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError();

private:
    QString                       scriptPath() const;

    QProcess*                     m_process  = nullptr;
    QMap<QString, PortfolioQuote> m_cache;
    QDateTime                     m_cacheTime;

    static constexpr int CACHE_TTL_MIN = 5;
};
