#include "DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QDateTime>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

bool DatabaseManager::init()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");

    // Zapisz baze danych w AppData/Roaming/DailyStockObserver/
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    m_db.setDatabaseName(dataPath + "/daily_stock_observer.db");

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

    // Tabela aktywów portfela
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS portfolio_assets (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            ticker          TEXT    NOT NULL UNIQUE,
            name            TEXT,
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
    return true;
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
    q.prepare("INSERT INTO portfolio_assets (ticker, name, quantity, avg_buy_price, currency, category) "
              "VALUES (:ticker, :name, :qty, :price, :currency, :category)");
    q.bindValue(":ticker",   asset.ticker.toUpper());
    q.bindValue(":name",     asset.name);
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
              "currency = :currency, category = :category "
              "WHERE id = :id");
    q.bindValue(":qty",      asset.quantity);
    q.bindValue(":price",    asset.avgBuyPrice);
    q.bindValue(":name",     asset.name);
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
    if (!q.exec("SELECT id, ticker, name, quantity, avg_buy_price, currency, category, added_at "
                "FROM portfolio_assets ORDER BY ticker ASC")) {
        qDebug() << "Błąd pobierania aktywów:" << q.lastError().text();
        return assets;
    }

    while (q.next()) {
        PortfolioAsset a;
        a.id           = q.value(0).toInt();
        a.ticker       = q.value(1).toString();
        a.name         = q.value(2).toString();
        a.quantity     = q.value(3).toDouble();
        a.avgBuyPrice  = q.value(4).toDouble();
        a.currency     = q.value(5).toString();
        a.category     = q.value(6).toString();
        a.addedAt      = q.value(7).toString();
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
