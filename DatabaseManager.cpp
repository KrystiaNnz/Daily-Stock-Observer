#include "DatabaseManager.h"
#include "ProfileManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QDateTime>
#include <QSet>
#include <QtMath>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

bool DatabaseManager::init()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");

    const QString databasePath = ProfileManager::databasePath();
    if (ProfileManager::activeProfileId() == "private" && !QFileInfo::exists(databasePath)) {
        const QString legacyPath = ProfileManager::appDataRoot() + "/daily_stock_observer.db";
        if (QFileInfo::exists(legacyPath))
            QFile::copy(legacyPath, databasePath);
    }

    m_db.setDatabaseName(databasePath);

    if (!m_db.open()) {
        qDebug() << "Błąd otwarcia bazy:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery q;

    // Włącz foreign keys
    q.exec("PRAGMA foreign_keys = ON");

    // Tabela wydarzeń
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS events (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            title       TEXT    NOT NULL,
            date        TEXT    NOT NULL,
            time        TEXT,
            description TEXT,
            created_at  TEXT    DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        qDebug() << "Błąd tabeli events:" << q.lastError().text();
        return false;
    }

    // Tabela celów
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS goals (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            title       TEXT    NOT NULL,
            description TEXT,
            due_date    TEXT,
            created_at  TEXT    DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        qDebug() << "Błąd tabeli goals:" << q.lastError().text();
        return false;
    }

    // Tabela kroków (ON DELETE CASCADE usuwa kroki razem z celem)
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS steps (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            goal_id     INTEGER NOT NULL,
            label       TEXT    NOT NULL,
            position    INTEGER DEFAULT 0,
            done        INTEGER DEFAULT 0,
            done_at     TEXT,
            FOREIGN KEY (goal_id) REFERENCES goals(id) ON DELETE CASCADE
        )
    )")) {
        qDebug() << "Błąd tabeli steps:" << q.lastError().text();
        return false;
    }

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS xp_profile (
            id          INTEGER PRIMARY KEY CHECK (id = 1),
            total_xp    INTEGER NOT NULL DEFAULT 0,
            updated_at  TEXT    DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        qDebug() << "Błąd tabeli xp_profile:" << q.lastError().text();
        return false;
    }

    if (!q.exec("INSERT OR IGNORE INTO xp_profile (id, total_xp) VALUES (1, 0)")) {
        qDebug() << "Błąd inicjalizacji xp_profile:" << q.lastError().text();
        return false;
    }

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS xp_events (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            step_id     INTEGER NOT NULL UNIQUE,
            xp          INTEGER NOT NULL,
            reason      TEXT,
            awarded_at  TEXT DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (step_id) REFERENCES steps(id) ON DELETE CASCADE
        )
    )")) {
        qDebug() << "Błąd tabeli xp_events:" << q.lastError().text();
        return false;
    }

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS xp_activity_events (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            source      TEXT    NOT NULL,
            xp          INTEGER NOT NULL,
            reference   TEXT,
            awarded_at  TEXT    DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        qDebug() << "Błąd tabeli xp_activity_events:" << q.lastError().text();
        return false;
    }

    // Tabela aktywów portfela
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS portfolio_assets (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            ticker          TEXT    NOT NULL UNIQUE,
            name            TEXT,
            asset_type      TEXT    DEFAULT 'Akcja',
            quantity        REAL    NOT NULL,
            avg_buy_price   REAL    NOT NULL,
            currency        TEXT    DEFAULT 'PLN',
            category        TEXT    DEFAULT '',
            added_at        TEXT    DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        qDebug() << "Błąd tabeli portfolio_assets:" << q.lastError().text();
        return false;
    }
    // Migracja: dodaj kolumnę category jeśli DB pochodzi ze starszej wersji
    q.exec("ALTER TABLE portfolio_assets ADD COLUMN category TEXT DEFAULT ''");
    q.exec("ALTER TABLE portfolio_assets ADD COLUMN asset_type TEXT DEFAULT 'Akcja'");

    // Tabela transakcji portfela
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS portfolio_transactions (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            ticker      TEXT    NOT NULL,
            type        TEXT    NOT NULL,
            quantity    REAL    NOT NULL,
            price       REAL    NOT NULL,
            date        TEXT    NOT NULL,
            note        TEXT
        )
    )")) {
        qDebug() << "Błąd tabeli portfolio_transactions:" << q.lastError().text();
        return false;
    }

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS market_price_bars (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            ticker          TEXT    NOT NULL,
            interval        TEXT    NOT NULL DEFAULT '1d',
            timestamp       TEXT    NOT NULL,
            trading_date    TEXT    NOT NULL,
            open            REAL,
            high            REAL,
            low             REAL,
            close           REAL,
            adj_close       REAL,
            volume          REAL,
            currency        TEXT,
            source          TEXT    NOT NULL DEFAULT 'yfinance',
            created_at      TEXT    DEFAULT CURRENT_TIMESTAMP,
            updated_at      TEXT    DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(ticker, interval, timestamp, source)
        )
    )")) {
        qDebug() << "Blad tabeli market_price_bars:" << q.lastError().text();
        return false;
    }

    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS market_price_cache_meta (
            id                      INTEGER PRIMARY KEY AUTOINCREMENT,
            ticker                  TEXT    NOT NULL,
            interval                TEXT    NOT NULL DEFAULT '1d',
            source                  TEXT    NOT NULL DEFAULT 'yfinance',
            currency                TEXT,
            first_timestamp         TEXT,
            last_timestamp          TEXT,
            last_successful_update  TEXT,
            status                  TEXT    DEFAULT 'ok',
            error_message           TEXT,
            updated_at              TEXT    DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(ticker, interval, source)
        )
    )")) {
        qDebug() << "Blad tabeli market_price_cache_meta:" << q.lastError().text();
        return false;
    }

    q.exec("CREATE INDEX IF NOT EXISTS idx_market_price_bars_lookup "
           "ON market_price_bars(ticker, interval, source, timestamp)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_market_price_bars_date "
           "ON market_price_bars(trading_date)");

    return true;
}

bool DatabaseManager::addEvent(const Event& event)
{
    QSqlQuery q;
    q.prepare("INSERT INTO events (title, date, time, description) "
              "VALUES (:title, :date, :time, :desc)");
    q.bindValue(":title", event.title);
    q.bindValue(":date",  event.date);
    q.bindValue(":time",  event.time);
    q.bindValue(":desc",  event.description);

    if (!q.exec()) {
        qDebug() << "Błąd dodawania:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<Event> DatabaseManager::getEventsForDate(const QString& date)
{
    QList<Event> events;

    QSqlQuery q;
    q.prepare("SELECT id, title, date, time, description "
              "FROM events WHERE date = :date "
              "ORDER BY time ASC");
    q.bindValue(":date", date);

    if (!q.exec()) {
        qDebug() << "Błąd pobierania:" << q.lastError().text();
        return events;
    }

    while (q.next()) {
        Event e;
        e.id          = q.value(0).toInt();
        e.title       = q.value(1).toString();
        e.date        = q.value(2).toString();
        e.time        = q.value(3).toString();
        e.description = q.value(4).toString();
        events.append(e);
    }

    return events;
}

bool DatabaseManager::deleteEvent(int id)
{
    QSqlQuery q;
    q.prepare("DELETE FROM events WHERE id = :id");
    q.bindValue(":id", id);

    if (!q.exec()) {
        qDebug() << "Błąd usuwania:" << q.lastError().text();
        return false;
    }
    return true;
}

// ══════════════════════════════════════════════════
// Goals
// ══════════════════════════════════════════════════

bool DatabaseManager::addGoal(Goal& goal)
{
    m_db.transaction();

    QSqlQuery q;
    q.prepare("INSERT INTO goals (title, description, due_date) "
              "VALUES (:title, :desc, :due)");
    q.bindValue(":title", goal.title);
    q.bindValue(":desc",  goal.description);
    q.bindValue(":due",   goal.dueDate);

    if (!q.exec()) {
        qDebug() << "Błąd dodawania celu:" << q.lastError().text();
        m_db.rollback();
        return false;
    }
    goal.id = q.lastInsertId().toInt();

    for (int i = 0; i < goal.steps.size(); ++i) {
        Step& s = goal.steps[i];
        s.goalId   = goal.id;
        s.position = i;
        if (!addStep(s)) {
            m_db.rollback();
            return false;
        }
    }

    return m_db.commit();
}

bool DatabaseManager::deleteGoal(int id)
{
    // ON DELETE CASCADE usuwa powiązane kroki automatycznie
    QSqlQuery q;
    q.prepare("DELETE FROM goals WHERE id = :id");
    q.bindValue(":id", id);

    if (!q.exec()) {
        qDebug() << "Błąd usuwania celu:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<Step> DatabaseManager::getStepsForGoal(int goalId)
{
    QList<Step> steps;

    QSqlQuery q;
    q.prepare("SELECT id, goal_id, label, position, done, done_at "
              "FROM steps WHERE goal_id = :gid ORDER BY position ASC");
    q.bindValue(":gid", goalId);

    if (!q.exec()) {
        qDebug() << "Błąd pobierania kroków:" << q.lastError().text();
        return steps;
    }

    while (q.next()) {
        Step s;
        s.id       = q.value(0).toInt();
        s.goalId   = q.value(1).toInt();
        s.label    = q.value(2).toString();
        s.position = q.value(3).toInt();
        s.done     = q.value(4).toBool();
        s.doneAt   = q.value(5).toString();
        steps.append(s);
    }

    return steps;
}

static QList<Goal> fetchGoalsFromQuery(QSqlQuery& q)
{
    QList<Goal> goals;
    while (q.next()) {
        Goal g;
        g.id          = q.value(0).toInt();
        g.title       = q.value(1).toString();
        g.description = q.value(2).toString();
        g.dueDate     = q.value(3).toString();
        g.createdAt   = q.value(4).toString();
        goals.append(g);
    }
    return goals;
}

QList<Goal> DatabaseManager::getGoalsForDate(const QString& date)
{
    QSqlQuery q;
    q.prepare("SELECT id, title, description, due_date, created_at "
              "FROM goals WHERE due_date = :date ORDER BY created_at ASC");
    q.bindValue(":date", date);

    if (!q.exec()) {
        qDebug() << "Błąd pobierania celów:" << q.lastError().text();
        return {};
    }

    QList<Goal> goals = fetchGoalsFromQuery(q);
    for (Goal& g : goals)
        g.steps = getStepsForGoal(g.id);
    return goals;
}

QList<Goal> DatabaseManager::getGoalsForDateRange(const QString& fromDate,
                                                   const QString& toDate,
                                                   const QString& excludeDate)
{
    QSqlQuery q;
    if (excludeDate.isEmpty()) {
        q.prepare("SELECT id, title, description, due_date, created_at "
                  "FROM goals WHERE due_date >= :from AND due_date <= :to "
                  "ORDER BY due_date ASC, created_at ASC");
        q.bindValue(":from", fromDate);
        q.bindValue(":to",   toDate);
    } else {
        q.prepare("SELECT id, title, description, due_date, created_at "
                  "FROM goals WHERE due_date >= :from AND due_date <= :to "
                  "AND due_date != :excl "
                  "ORDER BY due_date ASC, created_at ASC");
        q.bindValue(":from", fromDate);
        q.bindValue(":to",   toDate);
        q.bindValue(":excl", excludeDate);
    }

    if (!q.exec()) {
        qDebug() << "Błąd pobierania celów tygodnia:" << q.lastError().text();
        return {};
    }

    QList<Goal> goals = fetchGoalsFromQuery(q);
    for (Goal& g : goals)
        g.steps = getStepsForGoal(g.id);
    return goals;
}

QList<Goal> DatabaseManager::getAllGoals(const QString& filter)
{
    QSqlQuery q;
    if (filter.isEmpty()) {
        q.prepare("SELECT id, title, description, due_date, created_at FROM goals "
                  "ORDER BY CASE WHEN due_date IS NULL OR due_date = '' THEN 1 ELSE 0 END,"
                  " due_date ASC, created_at ASC");
    } else {
        q.prepare("SELECT id, title, description, due_date, created_at FROM goals "
                  "WHERE title LIKE :f OR description LIKE :f "
                  "ORDER BY CASE WHEN due_date IS NULL OR due_date = '' THEN 1 ELSE 0 END,"
                  " due_date ASC, created_at ASC");
        q.bindValue(":f", "%" + filter + "%");
    }

    if (!q.exec()) {
        qDebug() << "Błąd getAllGoals:" << q.lastError().text();
        return {};
    }

    QList<Goal> goals = fetchGoalsFromQuery(q);
    for (Goal& g : goals)
        g.steps = getStepsForGoal(g.id);
    return goals;
}

// ══════════════════════════════════════════════════
// Steps
// ══════════════════════════════════════════════════

bool DatabaseManager::addStep(Step& step)
{
    QSqlQuery q;
    q.prepare("INSERT INTO steps (goal_id, label, position, done) "
              "VALUES (:gid, :label, :pos, 0)");
    q.bindValue(":gid",   step.goalId);
    q.bindValue(":label", step.label);
    q.bindValue(":pos",   step.position);

    if (!q.exec()) {
        qDebug() << "Błąd dodawania kroku:" << q.lastError().text();
        return false;
    }
    step.id = q.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::toggleStep(int stepId, bool done)
{
    bool wasDone = false;
    QSqlQuery before;
    before.prepare("SELECT done FROM steps WHERE id = :id");
    before.bindValue(":id", stepId);
    if (before.exec() && before.next())
        wasDone = before.value(0).toBool();

    QSqlQuery q;
    if (done) {
        q.prepare("UPDATE steps SET done = 1, done_at = :dt WHERE id = :id");
        q.bindValue(":dt", QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss"));
    } else {
        q.prepare("UPDATE steps SET done = 0, done_at = NULL WHERE id = :id");
    }
    q.bindValue(":id", stepId);

    if (!q.exec()) {
        qDebug() << "Błąd toggle kroku:" << q.lastError().text();
        return false;
    }
    if (done && !wasDone)
        awardExperienceForStep(stepId);
    return true;
}

int DatabaseManager::xpForLevel(int level) const
{
    if (level <= 1)
        return 100;
    return qRound(100.0 * qPow(1.35, level - 1));
}

bool DatabaseManager::awardExperienceForStep(int stepId)
{
    constexpr int xpPerStep = 25;

    QSqlQuery event;
    event.prepare("INSERT OR IGNORE INTO xp_events (step_id, xp, reason, awarded_at) "
                  "VALUES (:step_id, :xp, :reason, :awarded_at)");
    event.bindValue(":step_id", stepId);
    event.bindValue(":xp", xpPerStep);
    event.bindValue(":reason", "step_completed");
    event.bindValue(":awarded_at", QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss"));

    if (!event.exec()) {
        qDebug() << "Błąd dodawania xp_event:" << event.lastError().text();
        return false;
    }
    if (event.numRowsAffected() == 0)
        return true;

    QSqlQuery profile;
    profile.prepare("UPDATE xp_profile SET total_xp = total_xp + :xp, "
                    "updated_at = :updated WHERE id = 1");
    profile.bindValue(":xp", xpPerStep);
    profile.bindValue(":updated", QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss"));
    if (!profile.exec()) {
        qDebug() << "Błąd aktualizacji XP:" << profile.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::awardExperience(const QString& source, int xp, const QString& reference)
{
    if (xp <= 0)
        return false;

    const QString now = QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss");

    QSqlQuery event;
    event.prepare("INSERT INTO xp_activity_events (source, xp, reference, awarded_at) "
                  "VALUES (:source, :xp, :reference, :awarded_at)");
    event.bindValue(":source", source);
    event.bindValue(":xp", xp);
    event.bindValue(":reference", reference);
    event.bindValue(":awarded_at", now);
    if (!event.exec()) {
        qDebug() << "Błąd dodawania xp_activity_event:" << event.lastError().text();
        return false;
    }

    QSqlQuery profile;
    profile.prepare("UPDATE xp_profile SET total_xp = total_xp + :xp, "
                    "updated_at = :updated WHERE id = 1");
    profile.bindValue(":xp", xp);
    profile.bindValue(":updated", now);
    if (!profile.exec()) {
        qDebug() << "Błąd aktualizacji XP:" << profile.lastError().text();
        return false;
    }
    return true;
}

int DatabaseManager::calculateStreakDays()
{
    QSet<QString> dates;
    QSqlQuery q;
    if (!q.exec("SELECT DISTINCT substr(done_at, 1, 10) FROM steps "
                "WHERE done = 1 AND done_at IS NOT NULL AND done_at != ''")) {
        qDebug() << "Błąd streak query:" << q.lastError().text();
        return 0;
    }

    while (q.next())
        dates.insert(q.value(0).toString());

    int streak = 0;
    QDate cursor = QDate::currentDate();
    while (dates.contains(cursor.toString("yyyy-MM-dd"))) {
        ++streak;
        cursor = cursor.addDays(-1);
    }
    return streak;
}

ExperienceProfile DatabaseManager::getExperienceProfile()
{
    ExperienceProfile profile;

    QSqlQuery q;
    if (q.exec("SELECT total_xp FROM xp_profile WHERE id = 1") && q.next())
        profile.totalXp = q.value(0).toInt();

    int remaining = profile.totalXp;
    int level = 1;
    int threshold = xpForLevel(level);
    while (remaining >= threshold) {
        remaining -= threshold;
        ++level;
        threshold = xpForLevel(level);
    }

    profile.level = level;
    profile.xpIntoLevel = remaining;
    profile.xpForNextLevel = threshold;
    profile.streakDays = calculateStreakDays();

    QSqlQuery today;
    today.prepare("SELECT COUNT(*) FROM steps "
                  "WHERE done = 1 AND substr(done_at, 1, 10) = :today");
    today.bindValue(":today", QDate::currentDate().toString("yyyy-MM-dd"));
    if (today.exec() && today.next())
        profile.completedStepsToday = today.value(0).toInt();

    return profile;
}

bool DatabaseManager::deleteStep(int stepId)
{
    QSqlQuery q;
    q.prepare("DELETE FROM steps WHERE id = :id");
    q.bindValue(":id", stepId);
    if (!q.exec()) {
        qDebug() << "Błąd usuwania kroku:" << q.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::updateGoal(const Goal& goal)
{
    QSqlQuery q;
    q.prepare("UPDATE goals SET title = :title, description = :desc, due_date = :due "
              "WHERE id = :id");
    q.bindValue(":title", goal.title);
    q.bindValue(":desc",  goal.description);
    q.bindValue(":due",   goal.dueDate);
    q.bindValue(":id",    goal.id);

    if (!q.exec()) {
        qDebug() << "Błąd updateGoal:" << q.lastError().text();
        return false;
    }
    return true;
}

// ══════════════════════════════════════════════════
// Portfolio
// ══════════════════════════════════════════════════

bool DatabaseManager::addAsset(PortfolioAsset& asset)
{
    QSqlQuery q;
    q.prepare("INSERT INTO portfolio_assets (ticker, name, asset_type, quantity, avg_buy_price, currency, category) "
              "VALUES (:ticker, :name, :asset_type, :qty, :price, :currency, :category)");
    q.bindValue(":ticker",   asset.ticker.toUpper());
    q.bindValue(":name",     asset.name);
    q.bindValue(":asset_type", asset.assetType.isEmpty() ? "Akcja" : asset.assetType);
    q.bindValue(":qty",      asset.quantity);
    q.bindValue(":price",    asset.avgBuyPrice);
    q.bindValue(":currency", asset.currency.isEmpty() ? "PLN" : asset.currency);
    q.bindValue(":category", asset.category);

    if (!q.exec()) {
        qDebug() << "Błąd dodawania aktywa:" << q.lastError().text();
        return false;
    }
    asset.id = q.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::updateAsset(const PortfolioAsset& asset)
{
    QSqlQuery q;
    q.prepare("UPDATE portfolio_assets "
              "SET quantity = :qty, avg_buy_price = :price, name = :name, "
              "asset_type = :asset_type, currency = :currency, category = :category "
              "WHERE id = :id");
    q.bindValue(":qty",      asset.quantity);
    q.bindValue(":price",    asset.avgBuyPrice);
    q.bindValue(":name",     asset.name);
    q.bindValue(":asset_type", asset.assetType.isEmpty() ? "Akcja" : asset.assetType);
    q.bindValue(":currency", asset.currency);
    q.bindValue(":category", asset.category);
    q.bindValue(":id",       asset.id);

    if (!q.exec()) {
        qDebug() << "Błąd aktualizacji aktywa:" << q.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::deleteAsset(int id)
{
    QSqlQuery q;
    q.prepare("DELETE FROM portfolio_assets WHERE id = :id");
    q.bindValue(":id", id);

    if (!q.exec()) {
        qDebug() << "Błąd usuwania aktywa:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<PortfolioAsset> DatabaseManager::getAllAssets()
{
    QList<PortfolioAsset> assets;

    QSqlQuery q;
    if (!q.exec("SELECT id, ticker, name, asset_type, quantity, avg_buy_price, currency, category, added_at "
                "FROM portfolio_assets ORDER BY ticker ASC")) {
        qDebug() << "Błąd pobierania aktywów:" << q.lastError().text();
        return assets;
    }

    while (q.next()) {
        PortfolioAsset a;
        a.id           = q.value(0).toInt();
        a.ticker       = q.value(1).toString();
        a.name         = q.value(2).toString();
        a.assetType    = q.value(3).toString();
        if (a.assetType.isEmpty())
            a.assetType = "Akcja";
        a.quantity     = q.value(4).toDouble();
        a.avgBuyPrice  = q.value(5).toDouble();
        a.currency     = q.value(6).toString();
        a.category     = q.value(7).toString();
        a.addedAt      = q.value(8).toString();
        assets.append(a);
    }

    return assets;
}

bool DatabaseManager::addTransaction(PortfolioTransaction& tx)
{
    QSqlQuery q;
    q.prepare("INSERT INTO portfolio_transactions (ticker, type, quantity, price, date, note) "
              "VALUES (:ticker, :type, :qty, :price, :date, :note)");
    q.bindValue(":ticker", tx.ticker.toUpper());
    q.bindValue(":type",   tx.type);
    q.bindValue(":qty",    tx.quantity);
    q.bindValue(":price",  tx.price);
    q.bindValue(":date",   tx.date);
    q.bindValue(":note",   tx.note);

    if (!q.exec()) {
        qDebug() << "Błąd dodawania transakcji:" << q.lastError().text();
        return false;
    }
    tx.id = q.lastInsertId().toInt();
    return true;
}

QList<PortfolioTransaction> DatabaseManager::getTransactionsForTicker(const QString& ticker)
{
    QList<PortfolioTransaction> txList;

    QSqlQuery q;
    q.prepare("SELECT id, ticker, type, quantity, price, date, note "
              "FROM portfolio_transactions WHERE ticker = :ticker "
              "ORDER BY date DESC");
    q.bindValue(":ticker", ticker.toUpper());

    if (!q.exec()) {
        qDebug() << "Błąd pobierania transakcji:" << q.lastError().text();
        return txList;
    }

    while (q.next()) {
        PortfolioTransaction tx;
        tx.id       = q.value(0).toInt();
        tx.ticker   = q.value(1).toString();
        tx.type     = q.value(2).toString();
        tx.quantity = q.value(3).toDouble();
        tx.price    = q.value(4).toDouble();
        tx.date     = q.value(5).toString();
        tx.note     = q.value(6).toString();
        txList.append(tx);
    }

    return txList;
}

bool DatabaseManager::upsertMarketPriceBars(const QList<MarketPriceBar>& bars)
{
    if (bars.isEmpty())
        return true;

    m_db.transaction();
    QSqlQuery q;
    q.prepare(R"(
        INSERT INTO market_price_bars (
            ticker, interval, timestamp, trading_date, open, high, low, close,
            adj_close, volume, currency, source, updated_at
        ) VALUES (
            :ticker, :interval, :timestamp, :trading_date, :open, :high, :low, :close,
            :adj_close, :volume, :currency, :source, :updated_at
        )
        ON CONFLICT(ticker, interval, timestamp, source) DO UPDATE SET
            trading_date = excluded.trading_date,
            open = excluded.open,
            high = excluded.high,
            low = excluded.low,
            close = excluded.close,
            adj_close = excluded.adj_close,
            volume = excluded.volume,
            currency = excluded.currency,
            updated_at = excluded.updated_at
    )");

    const QString now = QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss");
    for (const MarketPriceBar& bar : bars) {
        q.bindValue(":ticker", bar.ticker.toUpper());
        q.bindValue(":interval", bar.interval.isEmpty() ? "1d" : bar.interval);
        q.bindValue(":timestamp", bar.timestamp);
        q.bindValue(":trading_date", bar.tradingDate);
        q.bindValue(":open", bar.open);
        q.bindValue(":high", bar.high);
        q.bindValue(":low", bar.low);
        q.bindValue(":close", bar.close);
        q.bindValue(":adj_close", bar.adjClose);
        q.bindValue(":volume", bar.volume);
        q.bindValue(":currency", bar.currency);
        q.bindValue(":source", bar.source.isEmpty() ? "yfinance" : bar.source);
        q.bindValue(":updated_at", now);
        if (!q.exec()) {
            qDebug() << "Blad upsert market_price_bars:" << q.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    return m_db.commit();
}

QList<MarketPriceBar> DatabaseManager::getMarketPriceBars(const QString& ticker,
                                                          const QString& interval,
                                                          const QString& startTimestamp,
                                                          const QString& endTimestamp,
                                                          const QString& source)
{
    QList<MarketPriceBar> bars;

    QSqlQuery q;
    q.prepare("SELECT id, ticker, interval, timestamp, trading_date, open, high, low, close, "
              "adj_close, volume, currency, source, updated_at "
              "FROM market_price_bars "
              "WHERE ticker = :ticker AND interval = :interval AND source = :source "
              "AND timestamp >= :start AND timestamp <= :end "
              "ORDER BY timestamp ASC");
    q.bindValue(":ticker", ticker.toUpper());
    q.bindValue(":interval", interval.isEmpty() ? "1d" : interval);
    q.bindValue(":source", source.isEmpty() ? "yfinance" : source);
    q.bindValue(":start", startTimestamp);
    q.bindValue(":end", endTimestamp);

    if (!q.exec()) {
        qDebug() << "Blad getMarketPriceBars:" << q.lastError().text();
        return bars;
    }

    while (q.next()) {
        MarketPriceBar bar;
        bar.id = q.value(0).toInt();
        bar.ticker = q.value(1).toString();
        bar.interval = q.value(2).toString();
        bar.timestamp = q.value(3).toString();
        bar.tradingDate = q.value(4).toString();
        bar.open = q.value(5).toDouble();
        bar.high = q.value(6).toDouble();
        bar.low = q.value(7).toDouble();
        bar.close = q.value(8).toDouble();
        bar.adjClose = q.value(9).toDouble();
        bar.volume = q.value(10).toDouble();
        bar.currency = q.value(11).toString();
        bar.source = q.value(12).toString();
        bar.updatedAt = q.value(13).toString();
        bars.append(bar);
    }

    return bars;
}

MarketPriceCacheMeta DatabaseManager::getMarketPriceCacheMeta(const QString& ticker,
                                                              const QString& interval,
                                                              const QString& source)
{
    MarketPriceCacheMeta meta;
    meta.ticker = ticker.toUpper();
    meta.interval = interval.isEmpty() ? "1d" : interval;
    meta.source = source.isEmpty() ? "yfinance" : source;

    QSqlQuery q;
    q.prepare("SELECT ticker, interval, source, currency, first_timestamp, last_timestamp, "
              "last_successful_update, status, error_message "
              "FROM market_price_cache_meta "
              "WHERE ticker = :ticker AND interval = :interval AND source = :source");
    q.bindValue(":ticker", meta.ticker);
    q.bindValue(":interval", meta.interval);
    q.bindValue(":source", meta.source);

    if (!q.exec()) {
        qDebug() << "Blad getMarketPriceCacheMeta:" << q.lastError().text();
        return meta;
    }
    if (!q.next())
        return meta;

    meta.exists = true;
    meta.ticker = q.value(0).toString();
    meta.interval = q.value(1).toString();
    meta.source = q.value(2).toString();
    meta.currency = q.value(3).toString();
    meta.firstTimestamp = q.value(4).toString();
    meta.lastTimestamp = q.value(5).toString();
    meta.lastSuccessfulUpdate = q.value(6).toString();
    meta.status = q.value(7).toString();
    meta.errorMessage = q.value(8).toString();
    return meta;
}

bool DatabaseManager::refreshMarketPriceCacheMeta(const QString& ticker,
                                                  const QString& interval,
                                                  const QString& source,
                                                  const QString& currency,
                                                  const QString& status,
                                                  const QString& errorMessage)
{
    const QString cleanTicker = ticker.toUpper();
    const QString cleanInterval = interval.isEmpty() ? "1d" : interval;
    const QString cleanSource = source.isEmpty() ? "yfinance" : source;

    QString firstTimestamp;
    QString lastTimestamp;
    QSqlQuery range;
    range.prepare("SELECT MIN(timestamp), MAX(timestamp) FROM market_price_bars "
                  "WHERE ticker = :ticker AND interval = :interval AND source = :source");
    range.bindValue(":ticker", cleanTicker);
    range.bindValue(":interval", cleanInterval);
    range.bindValue(":source", cleanSource);
    if (range.exec() && range.next()) {
        firstTimestamp = range.value(0).toString();
        lastTimestamp = range.value(1).toString();
    }

    QSqlQuery q;
    q.prepare(R"(
        INSERT INTO market_price_cache_meta (
            ticker, interval, source, currency, first_timestamp, last_timestamp,
            last_successful_update, status, error_message, updated_at
        ) VALUES (
            :ticker, :interval, :source, :currency, :first_timestamp, :last_timestamp,
            :last_successful_update, :status, :error_message, :updated_at
        )
        ON CONFLICT(ticker, interval, source) DO UPDATE SET
            currency = excluded.currency,
            first_timestamp = excluded.first_timestamp,
            last_timestamp = excluded.last_timestamp,
            last_successful_update = excluded.last_successful_update,
            status = excluded.status,
            error_message = excluded.error_message,
            updated_at = excluded.updated_at
    )");
    const QString now = QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss");
    q.bindValue(":ticker", cleanTicker);
    q.bindValue(":interval", cleanInterval);
    q.bindValue(":source", cleanSource);
    q.bindValue(":currency", currency);
    q.bindValue(":first_timestamp", firstTimestamp);
    q.bindValue(":last_timestamp", lastTimestamp);
    q.bindValue(":last_successful_update", status == "ok" ? now : QString());
    q.bindValue(":status", status);
    q.bindValue(":error_message", errorMessage);
    q.bindValue(":updated_at", now);

    if (!q.exec()) {
        qDebug() << "Blad refreshMarketPriceCacheMeta:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<Goal> DatabaseManager::getLongTermGoals(const QString& afterDate)
{
    QSqlQuery q;
    q.prepare("SELECT id, title, description, due_date, created_at FROM goals "
              "WHERE due_date > :after OR due_date IS NULL OR due_date = '' "
              "ORDER BY CASE WHEN due_date IS NULL OR due_date = '' THEN 1 ELSE 0 END,"
              " due_date ASC, created_at ASC");
    q.bindValue(":after", afterDate);

    if (!q.exec()) {
        qDebug() << "Błąd getLongTermGoals:" << q.lastError().text();
        return {};
    }

    QList<Goal> goals = fetchGoalsFromQuery(q);
    for (Goal& g : goals)
        g.steps = getStepsForGoal(g.id);
    return goals;
}

// ══════════════════════════════════════════════════
// Seed
// ══════════════════════════════════════════════════

void DatabaseManager::seedGoals()
{
    QSqlQuery check;
    check.exec("SELECT COUNT(*) FROM goals");
    if (check.next() && check.value(0).toInt() > 0)
        return; // goals already exist — skip seed

    Goal g;
    g.title       = "Rozwój Zawodowy 2026";
    g.description = "Plan kariery i nauki: Process Improvement, makler KNF, Power BI, SQL, Python, CFA";
    g.dueDate     = "2026-12-31";

    const QStringList labels = {
        "VBA / Excel Automation (Q2 2026)",
        "Power Automate (Q2 2026)",
        "Power BI — certyfikat PL-300 (Q3 2026)",
        "Rejestracja na egzamin maklerski KNF (Q3 2026)",
        "SQL for Finance (Q4 2026)",
        "Alteryx (Q4 2026)",
        "Makler — Faza 1: Prawo rynku kapitałowego",
        "Makler — Faza 2: Instrumenty + matematyka finansowa",
        "Makler — Faza 3: Analiza finansowa + rachunkowość",
        "Python for Finance (2027)",
        "Egzamin maklerski KNF — zdanie (2027)",
        "CFA Level I (opcjonalnie, 2027+)"
    };

    for (const QString& label : labels) {
        Step s;
        s.label = label;
        g.steps.append(s);
    }

    addGoal(g);
}
