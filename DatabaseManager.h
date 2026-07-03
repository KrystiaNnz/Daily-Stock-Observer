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
    QString assetType = "Akcja";
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

struct MarketPriceBar {
    int     id = 0;
    QString ticker;
    QString interval = "1d";
    QString timestamp;
    QString tradingDate;
    double  open = 0.0;
    double  high = 0.0;
    double  low = 0.0;
    double  close = 0.0;
    double  adjClose = 0.0;
    double  volume = 0.0;
    QString currency;
    QString source = "yfinance";
    QString updatedAt;
};

struct MarketPriceCacheMeta {
    bool    exists = false;
    QString ticker;
    QString interval = "1d";
    QString source = "yfinance";
    QString currency;
    QString firstTimestamp;
    QString lastTimestamp;
    QString lastSuccessfulUpdate;
    QString status = "ok";
    QString errorMessage;
};

// Profil doświadczenia użytkownika
struct ExperienceProfile {
    int totalXp = 0;
    int level = 1;
    int xpIntoLevel = 0;
    int xpForNextLevel = 100;
    int streakDays = 0;
    int completedStepsToday = 0;
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

    // ── Experience / XP ───────────────────────────────────
    ExperienceProfile getExperienceProfile();
    int               xpForLevel(int level) const;
    bool              awardExperience(const QString& source, int xp, const QString& reference = QString());

    // ── Goal update ──────────────────────────────────────
    // Aktualizuje tytuł, opis i dueDate (nie dotyka kroków)
    bool         updateGoal(const Goal& goal);

    // ── Seed ─────────────────────────────────────────────
    // Wstawia domyślne cele tylko przy pierwszym uruchomieniu (pusta tabela goals)
    void seedGoals();

    // ── Portfolio ────────────────────────────────────────
    // addAsset ustawia asset.id po sukcesie
    bool                      addAsset(PortfolioAsset& asset);
    bool                      updateAsset(const PortfolioAsset& asset);
    bool                      deleteAsset(int id);
    QList<PortfolioAsset>     getAllAssets();

    // addTransaction ustawia tx.id po sukcesie
    bool                           addTransaction(PortfolioTransaction& tx);
    QList<PortfolioTransaction>    getTransactionsForTicker(const QString& ticker);

    // Historyczne ceny/cache rynku
    bool                       upsertMarketPriceBars(const QList<MarketPriceBar>& bars);
    QList<MarketPriceBar>      getMarketPriceBars(const QString& ticker,
                                                  const QString& interval,
                                                  const QString& startTimestamp,
                                                  const QString& endTimestamp,
                                                  const QString& source = "yfinance");
    MarketPriceCacheMeta       getMarketPriceCacheMeta(const QString& ticker,
                                                       const QString& interval,
                                                       const QString& source = "yfinance");
    bool                       refreshMarketPriceCacheMeta(const QString& ticker,
                                                           const QString& interval,
                                                           const QString& source,
                                                           const QString& currency,
                                                           const QString& status = "ok",
                                                           const QString& errorMessage = QString());

private:
    DatabaseManager() = default;
    bool awardExperienceForStep(int stepId);
    int  calculateStreakDays();
    QSqlDatabase m_db;
};
