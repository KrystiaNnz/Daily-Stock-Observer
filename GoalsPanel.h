#pragma once

#include <QWidget>
#include <QFrame>
#include <QDate>
#include "DatabaseManager.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
class QProgressBar;
class QScrollArea;
class QLineEdit;

// ──────────────────────────────────────────────────────
// GoalItemWidget — karta pojedynczego celu (zwijana)
// ──────────────────────────────────────────────────────
class GoalItemWidget : public QFrame
{
    Q_OBJECT

public:
    explicit GoalItemWidget(const Goal& goal, QWidget* parent = nullptr);

signals:
    void deleted(int goalId);
    void editRequested(int goalId);

private slots:
    void toggleExpand();
    void onDelete();

private:
    void setupUi();
    void rebuildSteps();
    void updateProgress();
    void addStepInline(const QString& label);

    Goal          m_goal;
    bool          m_expanded  = false;

    QPushButton*  m_expandBtn     = nullptr;
    QLabel*       m_titleLabel    = nullptr;
    QLabel*       m_progressLabel = nullptr;
    QProgressBar* m_progressBar   = nullptr;
    QWidget*      m_stepsBox      = nullptr;
    QVBoxLayout*  m_stepsLayout   = nullptr;
    QLineEdit*    m_newStepEdit   = nullptr;
};

// ──────────────────────────────────────────────────────
// GoalsPanel — prawy panel z celami dnia i tygodnia
// ──────────────────────────────────────────────────────
class GoalsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GoalsPanel(QWidget* parent = nullptr);
    void setDate(const QDate& date);

public slots:
    void refreshGoals();

private slots:
    void onAddGoal();
    void onAddLongTermGoal();
    void onEditGoal(int goalId);

private:
    void setupUi();
    void clearContainer(QVBoxLayout* layout);
    void populateDayGoals();
    void populateWeekGoals();
    void populateLongTermGoals();

    QDate        m_date;
    int          m_weekGoalCount     = 0;
    bool         m_weekExpanded      = false;
    int          m_longTermGoalCount = 0;
    bool         m_longTermExpanded  = false;

    QLabel*      m_dateLabel        = nullptr;
    QWidget*     m_dayContent       = nullptr;
    QVBoxLayout* m_dayLayout        = nullptr;
    QWidget*     m_weekContent      = nullptr;
    QVBoxLayout* m_weekLayout       = nullptr;
    QPushButton* m_weekBtn          = nullptr;
    QWidget*     m_weekSep          = nullptr;
    QWidget*     m_longTermContent  = nullptr;
    QVBoxLayout* m_longTermLayout   = nullptr;
    QPushButton* m_longTermBtn      = nullptr;
    QWidget*     m_longTermSep      = nullptr;
    QPushButton* m_addBtn           = nullptr;
    QPushButton* m_addLongTermBtn   = nullptr;
};
