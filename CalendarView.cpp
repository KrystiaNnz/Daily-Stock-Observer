#include "CalendarView.h"
#include "DatabaseManager.h"
#include "AddEventDialog.h"

#include <QCalendarWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QLocale>

CalendarView::CalendarView(QWidget* parent)
    : QWidget(parent)
    , m_selectedDate(QDate::currentDate())
{
    setupUi();
    setupConnections();
    refreshEvents();
}

void CalendarView::setDate(const QDate& date)
{
    m_selectedDate = date;
    m_calendar->setSelectedDate(date);
    refreshEvents();
}

void CalendarView::setupUi()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ── Lewy panel: Kalendarz ──────────────────────────────
    auto* leftPanel = new QWidget(this);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setFixedWidth(310);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(12, 12, 12, 12);
    leftLayout->setSpacing(8);

    auto* appTitle = new QLabel("\xF0\x9F\x93\x85 Daily Stock Observer", this);
    appTitle->setObjectName("appTitle");
    leftLayout->addWidget(appTitle);

    m_calendar = new QCalendarWidget(this);
    m_calendar->setLocale(QLocale(QLocale::Polish, QLocale::Poland));
    m_calendar->setGridVisible(true);
    m_calendar->setFirstDayOfWeek(Qt::Monday);
    m_calendar->setSelectedDate(m_selectedDate);
    leftLayout->addWidget(m_calendar);
    leftLayout->addStretch();

    // ── Separator ──────────────────────────────────────────
    auto* sep = new QWidget(this);
    sep->setObjectName("separator");
    sep->setFixedWidth(1);

    // ── Prawy panel: Lista wydarzeń ───────────────────────
    auto* rightPanel = new QWidget(this);
    rightPanel->setObjectName("rightPanel");
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(16, 16, 16, 16);
    rightLayout->setSpacing(10);

    m_dateLabel = new QLabel(this);
    m_dateLabel->setObjectName("dateLabel");
    rightLayout->addWidget(m_dateLabel);

    m_eventList = new QListWidget(this);
    m_eventList->setObjectName("eventList");
    m_eventList->setAlternatingRowColors(true);
    m_eventList->setSelectionMode(QAbstractItemView::SingleSelection);
    rightLayout->addWidget(m_eventList, 1);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    m_addBtn = new QPushButton("+ Dodaj wydarzenie", this);
    m_addBtn->setObjectName("addBtn");

    m_deleteBtn = new QPushButton("Usu\u0144 zaznaczone", this);
    m_deleteBtn->setObjectName("deleteBtn");
    m_deleteBtn->setEnabled(false);

    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_deleteBtn);
    rightLayout->addLayout(btnLayout);

    layout->addWidget(leftPanel);
    layout->addWidget(sep);
    layout->addWidget(rightPanel, 1);
}

void CalendarView::setupConnections()
{
    connect(m_calendar, &QCalendarWidget::selectionChanged,
            this, &CalendarView::onDateSelected);

    connect(m_addBtn, &QPushButton::clicked,
            this, &CalendarView::onAddEvent);

    connect(m_deleteBtn, &QPushButton::clicked,
            this, &CalendarView::onDeleteEvent);

    connect(m_eventList, &QListWidget::itemSelectionChanged, this, [this]() {
        m_deleteBtn->setEnabled(!m_eventList->selectedItems().isEmpty());
    });

    connect(m_eventList, &QListWidget::itemDoubleClicked,
            this, &CalendarView::onAddEvent);
}

void CalendarView::onDateSelected()
{
    m_selectedDate = m_calendar->selectedDate();
    refreshEvents();
    emit dateChanged(m_selectedDate);
}

void CalendarView::onAddEvent()
{
    AddEventDialog dialog(m_selectedDate, this);
    if (dialog.exec() == QDialog::Accepted) {
        Event e = dialog.getEvent();
        if (DatabaseManager::instance().addEvent(e))
            refreshEvents();
        else
            QMessageBox::warning(this, "B\u0142\u0105d", "Nie uda\u0142o si\u0119 zapisa\u0107 wydarzenia.");
    }
}

void CalendarView::onDeleteEvent()
{
    auto* item = m_eventList->currentItem();
    if (!item) return;

    int id = item->data(Qt::UserRole).toInt();
    auto reply = QMessageBox::question(this, "Usu\u0144 wydarzenie",
        "Czy na pewno chcesz usun\u0105\u0107:\n\"" + item->text() + "\"?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        DatabaseManager::instance().deleteEvent(id);
        refreshEvents();
    }
}

void CalendarView::refreshEvents()
{
    QString dateStr = m_selectedDate.toString("dddd, d MMMM yyyy");
    dateStr[0] = dateStr[0].toUpper();
    m_dateLabel->setText(dateStr);

    m_eventList->clear();
    m_deleteBtn->setEnabled(false);

    QString dateKey = m_selectedDate.toString("yyyy-MM-dd");
    QList<Event> events = DatabaseManager::instance().getEventsForDate(dateKey);

    if (events.isEmpty()) {
        auto* ph = new QListWidgetItem("(brak wydarze\u0144 \u2014 kliknij '+ Dodaj')");
        ph->setFlags(Qt::NoItemFlags);
        ph->setForeground(Qt::gray);
        m_eventList->addItem(ph);
        return;
    }

    for (const Event& e : events) {
        QString label = e.time.isEmpty() ? e.title : e.time + "  \u2013  " + e.title;
        if (!e.description.isEmpty())
            label += "\n    " + e.description;
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, e.id);
        m_eventList->addItem(item);
    }
}
