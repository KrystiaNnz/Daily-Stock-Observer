#pragma once

#include <QString>
#include <QList>
#include <QSqlDatabase>

// Struktura reprezentująca jedno wydarzenie
struct Event {
    int     id = 0;
    QString title;
    QString date;        // format: YYYY-MM-DD
    QString time;        // format: HH:MM (może być puste)
    QString description;
};

// Krok w ramach celu
struct Step {
    int     id       = 0;
    int     goalId   = 0;
    QString label;
    int     position = 0;
    bool    done     = false;
    QString doneAt;      // ISO datetime lub puste
};

// Cel do wykonania
struct Goal {
    int         id = 0;
    QString     title;
    QString     description;
    QString     dueDate;     // format: YYYY-MM-DD
    QString     createdAt;
    QList<Step> steps;
};

// Aktywo w portfelu inwestycyjnym
struct PortfolioAsset {
    int     id           = 0;
    QString ticker;               // np. "CDR.WA", "AAPL"
    QString name;
    double  quantity     = 0.0;
    double  avgBuyPrice  = 0.0;   // średnia cena zakupu
    QString currency;             // domyślnie "PLN"
    QString category;             // sektor / kategoria spółki
    QString addedAt;
};

// Pojedyncza transakcja kupna / sprzedaży
struct PortfolioTransaction {
    int     id       = 0;
    QString ticker;
    QString type;                 // "buy" | "sell"
    double  quantity = 0.0;
    double  price    = 0.0;
    QString date;                 // YYYY-MM-DD
    QString note;
};

// Giełda papierów wartościowych
struct StockExchange {
    int     id               = 0;
    QString country;
    QString region;
    QString name;
    QString website;
    QString type;
    QString city;
    double  lat              = 0.0;
    double  lon              = 0.0;
    QString address;
    int     foundedYear      = 0;
    int     listedCompanies  = 0;
};

// Kraj z bazy danych
struct Country {
    int     id              = 0;
    QString iso2;
    QString namePl;
    QString capital;
    QString continent;
    QString currency;
    QString languages;
    double  areaKm2         = 0.0;
    double  populationMln   = 0.0;
    double  gdpBlnUsd       = 0.0;
    double  gdpPerCapitaPpp = 0.0;
    double  hdi             = 0.0;
    QString demographicTrend;
};

// DatabaseManager - obsługuje bazę danych SQLite
// Wzorzec Singleton: jedna instancja przez całe życie aplikacji
class DatabaseManager
{
public:
    static DatabaseManager& instance();

    // Otwiera (lub tworzy) plik bazy danych i tworzy tabele
    bool init();

    // ── Events ──────────────────────────────────────────
    bool         addEvent(const Event& event);
    QList<Event> getEventsForDate(const QString& date);
    bool         deleteEvent(int id);

    // ── Goals ────────────────────────────────────────────
    // addGoal ustawia goal.id oraz step.id po sukcesie
    bool         addGoal(Goal& goal);
    bool         deleteGoal(int id);
    QList<Goal>  getGoalsForDate(const QString& date);
    // Zwraca cele w zakresie dat (bez excludeDate), ze krokami
    QList<Goal>  getGoalsForDateRange(const QString& fromDate,
                                      const QString& toDate,
                                      const QString& excludeDate = QString());
    // Zwraca wszystkie cele (opcjonalnie filtrowane po tytule/opisie)
    QList<Goal>  getAllGoals(const QString& filter = QString());
    // Zwraca cele długoterminowe: due_date > afterDate LUB brak due_date
    QList<Goal>  getLongTermGoals(const QString& afterDate);

    // ── Steps ────────────────────────────────────────────
    bool         addStep(Step& step);          // ustawia step.id
    bool         toggleStep(int stepId, bool done);
    bool         deleteStep(int stepId);
    QList<Step>  getStepsForGoal(int goalId);

    // ── Goal update ──────────────────────────────────────
    // Aktualizuje tytuł, opis i dueDate (nie dotyka kroków)
    bool         updateGoal(const Goal& goal);

    // ── Seed ─────────────────────────────────────────────
    // Wstawia domyślne cele tylko przy pierwszym uruchomieniu (pusta tabela goals)
    void seedGoals();

    // ── Stock Exchanges ──────────────────────────────────
    // Wstawia 207 giełd z exchanges_data.json tylko przy pierwszym uruchomieniu
    void                 seedExchanges();
    QList<StockExchange> getAllExchanges();

    // ── Countries ────────────────────────────────────────
    // Wstawia ~195 krajów z countries_data.json tylko przy pierwszym uruchomieniu
    void    seedCountries();
    Country getCountry(const QString& iso2);

    // ── Portfolio ────────────────────────────────────────
    // addAsset ustawia asset.id po sukcesie
    bool                      addAsset(PortfolioAsset& asset);
    bool                      updateAsset(const PortfolioAsset& asset);
    bool                      deleteAsset(int id);
    QList<PortfolioAsset>     getAllAssets();

    // addTransaction ustawia tx.id po sukcesie
    bool                           addTransaction(PortfolioTransaction& tx);
    QList<PortfolioTransaction>    getTransactionsForTicker(const QString& ticker);

private:
    DatabaseManager() = default;
    QSqlDatabase m_db;
};
