#include "GoalsListView.h"
#include "DatabaseManager.h"
#include "GoalsPanel.h"

#include <QLineEdit>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <QDate>

GoalsListView::GoalsListView(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    populate();
}

void GoalsListView::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    // Pasek wyszukiwania
    auto* searchRow = new QHBoxLayout();
    searchRow->setSpacing(8);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName("goalsSearchEdit");
    m_searchEdit->setPlaceholderText("Szukaj cel\u00f3w...");
    m_searchEdit->setClearButtonEnabled(true);
    searchRow->addWidget(m_searchEdit);
    layout->addLayout(searchRow);

    // Przewijana lista celów
    auto* scroll = new QScrollArea(this);
    scroll->setObjectName("goalsListScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    m_listContainer = new QWidget(this);
    m_listContainer->setObjectName("goalsListContainer");
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 8, 0);
    m_listLayout->setSpacing(4);
    m_listLayout->addStretch();

    scroll->setWidget(m_listContainer);
    layout->addWidget(scroll, 1);

    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &GoalsListView::onSearch);
}

void GoalsListView::refresh()
{
    populate(m_searchEdit->text());
}

void GoalsListView::onSearch(const QString& text)
{
    populate(text);
}

void GoalsListView::populate(const QString& filter)
{
    // Usuń wszystkie widgety oprócz końcowego stretch
    while (m_listLayout->count() > 1) {
        QLayoutItem* li = m_listLayout->takeAt(0);
        if (QWidget* w = li->widget()) {
            w->setParent(nullptr);
            delete w;
        }
        delete li;
    }

    QList<Goal> goals = DatabaseManager::instance().getAllGoals(filter);

    if (goals.isEmpty()) {
        auto* empty = new QLabel(
            filter.isEmpty()
                ? "Brak cel\u00f3w. Dodaj cel w widoku Kalendarz."
                : "Brak wynik\u00f3w dla: \"\u200b" + filter + "\"",
            m_listContainer);
        empty->setObjectName("goalsEmpty");
        empty->setAlignment(Qt::AlignCenter);
        empty->setWordWrap(true);
        m_listLayout->insertWidget(0, empty);
        return;
    }

    QString lastDate;
    int idx = 0;

    for (const Goal& g : goals) {
        // Nagłówek sekcji daty (gdy zmienia się data)
        if (g.dueDate != lastDate) {
            lastDate = g.dueDate;

            QString headerText;
            if (g.dueDate.isEmpty()) {
                headerText = "Bez terminu";
            } else {
                QDate d = QDate::fromString(g.dueDate, "yyyy-MM-dd");
                headerText = d.isValid()
                    ? d.toString("dddd, d MMMM yyyy")
                    : g.dueDate;
                if (!headerText.isEmpty())
                    headerText[0] = headerText[0].toUpper();
            }

            auto* header = new QLabel(headerText, m_listContainer);
            header->setObjectName("goalsDateHeader");
            m_listLayout->insertWidget(idx++, header);
        }

        auto* goalWidget = new GoalItemWidget(g, m_listContainer);
        connect(goalWidget, &GoalItemWidget::deleted, this, [this](int) {
            QTimer::singleShot(0, this, [this]() { refresh(); });
        });
        m_listLayout->insertWidget(idx++, goalWidget);
    }
}
