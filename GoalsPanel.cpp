#include "GoalsPanel.h"
#include "AddGoalDialog.h"

#include <algorithm>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QScrollArea>
#include <QCheckBox>
#include <QLineEdit>
#include <QMessageBox>

// ══════════════════════════════════════════════════════
// GoalItemWidget
// ══════════════════════════════════════════════════════

GoalItemWidget::GoalItemWidget(const Goal& goal, QWidget* parent)
    : QFrame(parent), m_goal(goal)
{
    setObjectName("goalItem");
    setFrameShape(QFrame::StyledPanel);
    setupUi();
}

void GoalItemWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(4);

    // ── Nagłówek ──────────────────────────────────────
    auto* header = new QHBoxLayout();
    header->setSpacing(6);

    m_expandBtn = new QPushButton("▶", this);   // ▶
    m_expandBtn->setObjectName("goalExpandBtn");
    m_expandBtn->setFixedSize(22, 22);
    m_expandBtn->setCursor(Qt::PointingHandCursor);
    m_expandBtn->setFlat(true);
    header->addWidget(m_expandBtn);

    m_titleLabel = new QLabel(m_goal.title, this);
    m_titleLabel->setObjectName("goalTitle");
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    header->addWidget(m_titleLabel, 1);

    m_progressLabel = new QLabel(this);
    m_progressLabel->setObjectName("goalProgressLabel");
    header->addWidget(m_progressLabel);

    auto* editBtn = new QPushButton("✎", this);  // ✎ pencil
    editBtn->setObjectName("goalEditBtn");
    editBtn->setFixedSize(22, 22);
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setFlat(true);
    editBtn->setToolTip("Edytuj cel");
    connect(editBtn, &QPushButton::clicked, this,
            [this]() { emit editRequested(m_goal.id); });
    header->addWidget(editBtn);

    auto* delBtn = new QPushButton("×", this);  // x
    delBtn->setObjectName("goalDeleteBtn");
    delBtn->setFixedSize(22, 22);
    delBtn->setCursor(Qt::PointingHandCursor);
    delBtn->setFlat(true);
    connect(delBtn, &QPushButton::clicked, this, &GoalItemWidget::onDelete);
    header->addWidget(delBtn);

    root->addLayout(header);

    // ── Pasek postępu ─────────────────────────────────
    m_progressBar = new QProgressBar(this);
    m_progressBar->setObjectName("goalProgressBar");
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(4);
    m_progressBar->setRange(0, 100);
    root->addWidget(m_progressBar);

    // ── Kontener kroków (domyślnie ukryty) ────────────
    m_stepsBox = new QWidget(this);
    m_stepsBox->setVisible(false);

    auto* stepsOuter = new QVBoxLayout(m_stepsBox);
    stepsOuter->setContentsMargins(0, 6, 0, 0);
    stepsOuter->setSpacing(4);

    // Przewijalna lista kroków
    auto* stepsScroll = new QScrollArea(m_stepsBox);
    stepsScroll->setObjectName("stepsScroll");
    stepsScroll->setFrameShape(QFrame::NoFrame);
    stepsScroll->setWidgetResizable(true);
    stepsScroll->setMaximumHeight(180);
    stepsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* stepsContent = new QWidget();
    m_stepsLayout = new QVBoxLayout(stepsContent);
    m_stepsLayout->setContentsMargins(2, 2, 2, 2);
    m_stepsLayout->setSpacing(1);
    stepsScroll->setWidget(stepsContent);
    stepsOuter->addWidget(stepsScroll);

    // Wiersz "nowy krok"
    auto* newRow = new QHBoxLayout();
    newRow->setSpacing(4);

    m_newStepEdit = new QLineEdit(m_stepsBox);
    m_newStepEdit->setObjectName("newStepEdit");
    m_newStepEdit->setPlaceholderText("Nowy krok…");

    auto* addStepBtn = new QPushButton("+", m_stepsBox);
    addStepBtn->setObjectName("addStepBtn");
    addStepBtn->setFixedWidth(28);
    addStepBtn->setCursor(Qt::PointingHandCursor);

    connect(addStepBtn, &QPushButton::clicked, this,
        [this]() { addStepInline(m_newStepEdit->text().trimmed()); });
    connect(m_newStepEdit, &QLineEdit::returnPressed, this,
        [this]() { addStepInline(m_newStepEdit->text().trimmed()); });

    newRow->addWidget(m_newStepEdit, 1);
    newRow->addWidget(addStepBtn);
    stepsOuter->addLayout(newRow);

    root->addWidget(m_stepsBox);

    connect(m_expandBtn, &QPushButton::clicked, this, &GoalItemWidget::toggleExpand);

    updateProgress();
}

void GoalItemWidget::toggleExpand()
{
    m_expanded = !m_expanded;
    m_expandBtn->setText(m_expanded ? "▼" : "▶");  // ▼ / ▶

    if (m_expanded) {
        m_goal.steps = DatabaseManager::instance().getStepsForGoal(m_goal.id);
        rebuildSteps();
    }

    m_stepsBox->setVisible(m_expanded);
}

void GoalItemWidget::rebuildSteps()
{
    QLayoutItem* item;
    while ((item = m_stepsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->setParent(nullptr);
            item->widget()->deleteLater();
        }
        delete item;
    }

    if (m_goal.steps.isEmpty()) {
        auto* lbl = new QLabel("(brak kroków — dodaj poniżej)", this);
        lbl->setStyleSheet("color: #bbb; font-size: 11px; padding: 4px 2px;");
        m_stepsLayout->addWidget(lbl);
    } else {
        for (const Step& s : m_goal.steps) {
            auto* row = new QWidget();
            auto* rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(2);

            auto* cb = new QCheckBox(s.label, row);
            cb->setObjectName("stepCheckbox");
            cb->setChecked(s.done);
            cb->setStyleSheet(s.done
                ? "QCheckBox { color: #bbb; text-decoration: line-through;"
                  " font-size: 12px; padding: 3px 2px; }"
                : "QCheckBox { font-size: 12px; padding: 3px 2px; }");

            connect(cb, &QCheckBox::toggled, this,
                [this, stepId = s.id, cb](bool checked) {
                    DatabaseManager::instance().toggleStep(stepId, checked);
                    for (Step& step : m_goal.steps) {
                        if (step.id == stepId) { step.done = checked; break; }
                    }
                    cb->setStyleSheet(checked
                        ? "QCheckBox { color: #bbb; text-decoration: line-through;"
                          " font-size: 12px; padding: 3px 2px; }"
                        : "QCheckBox { font-size: 12px; padding: 3px 2px; }");
                    updateProgress();
                });

            auto* delStep = new QPushButton("×", row);
            delStep->setObjectName("stepDeleteBtn");
            delStep->setFixedSize(16, 16);
            delStep->setFlat(true);
            delStep->setCursor(Qt::PointingHandCursor);
            delStep->setToolTip("Usuń krok");
            delStep->setStyleSheet("color: #bbb; font-size: 10px;");
            connect(delStep, &QPushButton::clicked, this,
                [this, stepId = s.id]() {
                    DatabaseManager::instance().deleteStep(stepId);
                    m_goal.steps.erase(
                        std::remove_if(m_goal.steps.begin(), m_goal.steps.end(),
                            [stepId](const Step& st){ return st.id == stepId; }),
                        m_goal.steps.end());
                    rebuildSteps();
                    updateProgress();
                });

            rowLayout->addWidget(cb, 1);
            rowLayout->addWidget(delStep);
            m_stepsLayout->addWidget(row);
        }
    }

    m_stepsLayout->addStretch();
}

void GoalItemWidget::updateProgress()
{
    int total = m_goal.steps.size();
    int done  = 0;
    for (const Step& s : m_goal.steps) {
        if (s.done) ++done;
    }
    m_progressLabel->setText(QString("%1/%2").arg(done).arg(total));
    m_progressBar->setValue(total > 0 ? (done * 100 / total) : 0);
}

void GoalItemWidget::addStepInline(const QString& label)
{
    if (label.isEmpty()) return;

    Step s;
    s.goalId   = m_goal.id;
    s.label    = label;
    s.position = m_goal.steps.size();

    if (DatabaseManager::instance().addStep(s)) {
        m_goal.steps.append(s);
        rebuildSteps();
        updateProgress();
        m_newStepEdit->clear();
    }
}

void GoalItemWidget::onDelete()
{
    auto reply = QMessageBox::question(this, "Usuń cel",
        "Czy na pewno chcesz usunąć cel:\n\""
        + m_goal.title + "\"?\nWszystkie kroki zostaną usunięte.",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        DatabaseManager::instance().deleteGoal(m_goal.id);
        emit deleted(m_goal.id);
    }
}

// ══════════════════════════════════════════════════════
// GoalsPanel
// ══════════════════════════════════════════════════════

GoalsPanel::GoalsPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("goalsPanel");
    m_date = QDate::currentDate();
    setupUi();
    refreshGoals();
}

void GoalsPanel::setDate(const QDate& date)
{
    if (m_date == date) return;
    m_date = date;
    refreshGoals();
}

void GoalsPanel::setupUi()
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    auto* scroll = new QScrollArea(this);
    scroll->setObjectName("goalsPanelScroll");
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* content = new QWidget();
    auto* cl = new QVBoxLayout(content);
    cl->setContentsMargins(14, 14, 14, 14);
    cl->setSpacing(8);

    // Nagłówek: "Cele" + przycisk dodaj
    auto* headerRow = new QHBoxLayout();
    auto* panelTitle = new QLabel("Cele", this);
    panelTitle->setObjectName("goalsPanelTitle");
    headerRow->addWidget(panelTitle);
    headerRow->addStretch();
    m_addBtn = new QPushButton("+ Dodaj cel", this);
    m_addBtn->setObjectName("goalsAddBtn");
    m_addBtn->setCursor(Qt::PointingHandCursor);
    connect(m_addBtn, &QPushButton::clicked, this, &GoalsPanel::onAddGoal);
    headerRow->addWidget(m_addBtn);
    cl->addLayout(headerRow);

    // Etykieta daty
    m_dateLabel = new QLabel(this);
    m_dateLabel->setObjectName("goalsDayLabel");
    cl->addWidget(m_dateLabel);

    // Kontener celów dnia
    m_dayContent = new QWidget(this);
    m_dayLayout  = new QVBoxLayout(m_dayContent);
    m_dayLayout->setContentsMargins(0, 0, 0, 0);
    m_dayLayout->setSpacing(6);
    cl->addWidget(m_dayContent);

    // Separator tygodniowy
    m_weekSep = new QWidget(this);
    m_weekSep->setObjectName("goalsSep");
    m_weekSep->setFixedHeight(1);
    cl->addWidget(m_weekSep);

    m_weekBtn = new QPushButton(this);
    m_weekBtn->setObjectName("weekToggle");
    m_weekBtn->setCursor(Qt::PointingHandCursor);
    m_weekBtn->setFlat(true);
    connect(m_weekBtn, &QPushButton::clicked, this, [this]() {
        m_weekExpanded = !m_weekExpanded;
        m_weekContent->setVisible(m_weekExpanded && m_weekGoalCount > 0);
        m_weekBtn->setText(
            QString("%1  Cele na ten tydzień (%2)")
            .arg(m_weekExpanded ? "▼" : "▶")
            .arg(m_weekGoalCount));
    });
    cl->addWidget(m_weekBtn);

    m_weekContent = new QWidget(this);
    m_weekContent->setVisible(false);
    m_weekLayout  = new QVBoxLayout(m_weekContent);
    m_weekLayout->setContentsMargins(0, 4, 0, 0);
    m_weekLayout->setSpacing(6);
    cl->addWidget(m_weekContent);

    // Separator długoterminowy
    m_longTermSep = new QWidget(this);
    m_longTermSep->setObjectName("goalsSep");
    m_longTermSep->setFixedHeight(1);
    cl->addWidget(m_longTermSep);

    // Nagłówek sekcji długoterminowej: toggle + przycisk dodaj
    auto* ltHeader = new QHBoxLayout();
    ltHeader->setSpacing(4);

    m_longTermBtn = new QPushButton(this);
    m_longTermBtn->setObjectName("weekToggle");
    m_longTermBtn->setCursor(Qt::PointingHandCursor);
    m_longTermBtn->setFlat(true);
    m_longTermBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(m_longTermBtn, &QPushButton::clicked, this, [this]() {
        m_longTermExpanded = !m_longTermExpanded;
        m_longTermContent->setVisible(m_longTermExpanded && m_longTermGoalCount > 0);
        m_longTermBtn->setText(
            QString("%1  Projekty długoterminowe (%2)")
            .arg(m_longTermExpanded ? "▼" : "▶")
            .arg(m_longTermGoalCount));
    });
    ltHeader->addWidget(m_longTermBtn, 1);

    m_addLongTermBtn = new QPushButton("+", this);
    m_addLongTermBtn->setObjectName("goalsAddBtn");
    m_addLongTermBtn->setFixedSize(22, 22);
    m_addLongTermBtn->setCursor(Qt::PointingHandCursor);
    m_addLongTermBtn->setToolTip("Dodaj projekt długoterminowy");
    connect(m_addLongTermBtn, &QPushButton::clicked, this, &GoalsPanel::onAddLongTermGoal);
    ltHeader->addWidget(m_addLongTermBtn);

    cl->addLayout(ltHeader);

    m_longTermContent = new QWidget(this);
    m_longTermContent->setVisible(false);
    m_longTermLayout  = new QVBoxLayout(m_longTermContent);
    m_longTermLayout->setContentsMargins(0, 4, 0, 0);
    m_longTermLayout->setSpacing(6);
    cl->addWidget(m_longTermContent);

    cl->addStretch();

    scroll->setWidget(content);
    outerLayout->addWidget(scroll);
}

void GoalsPanel::clearContainer(QVBoxLayout* layout)
{
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->setParent(nullptr);
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void GoalsPanel::populateDayGoals()
{
    clearContainer(m_dayLayout);

    const QString dateKey = m_date.toString("yyyy-MM-dd");
    const QList<Goal> goals = DatabaseManager::instance().getGoalsForDate(dateKey);

    if (goals.isEmpty()) {
        auto* lbl = new QLabel(
            "Brak celów na ten dzień.\nKliknij '+ Dodaj cel'.", m_dayContent);
        lbl->setObjectName("goalsEmpty");
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setWordWrap(true);
        m_dayLayout->addWidget(lbl);
        return;
    }

    for (const Goal& g : goals) {
        auto* item = new GoalItemWidget(g, m_dayContent);
        connect(item, &GoalItemWidget::deleted,      this, [this](int)    { refreshGoals(); });
        connect(item, &GoalItemWidget::editRequested, this, &GoalsPanel::onEditGoal);
        m_dayLayout->addWidget(item);
    }
}

void GoalsPanel::populateWeekGoals()
{
    clearContainer(m_weekLayout);

    const int   dow    = m_date.dayOfWeek();
    const QDate monday = m_date.addDays(-(dow - 1));
    const QDate sunday = monday.addDays(6);

    const QList<Goal> goals = DatabaseManager::instance().getGoalsForDateRange(
        monday.toString("yyyy-MM-dd"),
        sunday.toString("yyyy-MM-dd"),
        m_date.toString("yyyy-MM-dd"));

    m_weekGoalCount = goals.size();

    const bool visible = m_weekGoalCount > 0;
    m_weekSep->setVisible(visible);
    m_weekBtn->setVisible(visible);
    m_weekContent->setVisible(m_weekExpanded && visible);

    if (visible)
        m_weekBtn->setText(
            QString("%1  Cele na ten tydzień (%2)")
            .arg(m_weekExpanded ? "▼" : "▶")
            .arg(m_weekGoalCount));

    for (const Goal& g : goals) {
        auto* item = new GoalItemWidget(g, m_weekContent);
        connect(item, &GoalItemWidget::deleted,      this, [this](int)    { refreshGoals(); });
        connect(item, &GoalItemWidget::editRequested, this, &GoalsPanel::onEditGoal);
        m_weekLayout->addWidget(item);
    }
}

void GoalsPanel::populateLongTermGoals()
{
    clearContainer(m_longTermLayout);

    const int   dow    = m_date.dayOfWeek();
    const QDate monday = m_date.addDays(-(dow - 1));
    const QDate sunday = monday.addDays(6);

    const QList<Goal> goals = DatabaseManager::instance().getLongTermGoals(
        sunday.toString("yyyy-MM-dd"));
    m_longTermGoalCount = goals.size();

    const bool visible = m_longTermGoalCount > 0;
    m_longTermSep->setVisible(visible);
    m_longTermBtn->setVisible(visible);
    m_longTermContent->setVisible(m_longTermExpanded && visible);

    if (visible)
        m_longTermBtn->setText(
            QString("%1  Projekty długoterminowe (%2)")
            .arg(m_longTermExpanded ? "▼" : "▶")
            .arg(m_longTermGoalCount));

    for (const Goal& g : goals) {
        auto* item = new GoalItemWidget(g, m_longTermContent);
        connect(item, &GoalItemWidget::deleted,      this, [this](int)    { refreshGoals(); });
        connect(item, &GoalItemWidget::editRequested, this, &GoalsPanel::onEditGoal);
        m_longTermLayout->addWidget(item);
    }
}

void GoalsPanel::refreshGoals()
{
    QString ds = m_date.toString("dddd, d MMMM yyyy");
    ds[0] = ds[0].toUpper();
    m_dateLabel->setText(ds);

    populateDayGoals();
    populateWeekGoals();
    populateLongTermGoals();
}

void GoalsPanel::onAddGoal()
{
    AddGoalDialog dlg(m_date, this);
    if (dlg.exec() == QDialog::Accepted) {
        Goal g = dlg.getGoal();
        if (!DatabaseManager::instance().addGoal(g))
            QMessageBox::warning(this, "Błąd", "Nie udało się zapisać celu.");
        else
            refreshGoals();
    }
}

void GoalsPanel::onAddLongTermGoal()
{
    // Domyślny termin: koniec bieżącego roku
    const QDate defaultDate(QDate::currentDate().year(), 12, 31);
    AddGoalDialog dlg(defaultDate, this);
    if (dlg.exec() == QDialog::Accepted) {
        Goal g = dlg.getGoal();
        if (!DatabaseManager::instance().addGoal(g))
            QMessageBox::warning(this, "Błąd", "Nie udało się zapisać projektu.");
        else {
            m_longTermExpanded = true;
            refreshGoals();
        }
    }
}

void GoalsPanel::onEditGoal(int goalId)
{
    // Znajdź cel po ID
    const QList<Goal> all = DatabaseManager::instance().getAllGoals();
    Goal found;
    bool ok = false;
    for (const Goal& g : all) {
        if (g.id == goalId) { found = g; ok = true; break; }
    }
    if (!ok) return;

    AddGoalDialog dlg(found, this);
    if (dlg.exec() == QDialog::Accepted) {
        Goal updated = dlg.getGoal();
        updated.id = goalId;
        if (!DatabaseManager::instance().updateGoal(updated))
            QMessageBox::warning(this, "Błąd", "Nie udało się zapisać zmian.");
        else
            refreshGoals();
    }
}
