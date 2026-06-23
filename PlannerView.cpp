#include "PlannerView.h"
#include "DatabaseManager.h"
#include "GoalsPanel.h"

#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QSplitter>
#include <QTimer>
#include <QMap>

// Rozmiary slotów w minutach i odpowiadające im etykiety przycisków
static const int   SLOT_SIZES[]  = { 30, 60, 120, 240, 480, 720, 1440 };
static const char* SLOT_LABELS[] = { "30 min", "1 h", "2 h", "4 h", "8 h", "12 h", "24 h" };
static const int   SLOT_COUNT    = 7;
static const int   DEFAULT_SLOT_ID = 1; // 1h

PlannerView::PlannerView(QWidget* parent)
    : QWidget(parent)
    , m_date(QDate::currentDate())
{
    setupUi();
    rebuildGrid();
}

void PlannerView::setDate(const QDate& date)
{
    m_date = date;
    QString dateStr = date.toString("dddd, d MMMM yyyy");
    dateStr[0] = dateStr[0].toUpper();
    m_dateLabel->setText(dateStr);
    m_goalsPanel->setDate(date);
    rebuildGrid();
}

void PlannerView::onSlotSizeChanged(int id)
{
    m_slotMinutes = SLOT_SIZES[id];
    rebuildGrid();
}

void PlannerView::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Górny pasek: data + filtry slotów ─────────────────
    auto* topBar = new QWidget(this);
    topBar->setObjectName("plannerTopBar");
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(16, 8, 16, 8);
    topLayout->setSpacing(6);

    m_dateLabel = new QLabel(this);
    m_dateLabel->setObjectName("plannerDateLabel");
    topLayout->addWidget(m_dateLabel);
    topLayout->addStretch();

    // Przyciski filtrów slotów
    m_slotGroup = new QButtonGroup(this);
    m_slotGroup->setExclusive(true);
    for (int i = 0; i < SLOT_COUNT; ++i) {
        auto* btn = new QPushButton(SLOT_LABELS[i], this);
        btn->setObjectName("plannerSlotBtn");
        btn->setCheckable(true);
        if (i == DEFAULT_SLOT_ID) btn->setChecked(true);
        m_slotGroup->addButton(btn, i);
        topLayout->addWidget(btn);
    }

    mainLayout->addWidget(topBar);

    // Separator
    auto* sep = new QWidget(this);
    sep->setObjectName("separator");
    sep->setFixedHeight(1);
    mainLayout->addWidget(sep);

    // ── Zawartość: siatka godzin | panel celów ─────────────
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName("contentSplitter");
    splitter->setHandleWidth(1);

    // Siatka godzin (QScrollArea)
    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName("plannerScroll");
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_gridWidget = new QWidget();
    m_gridWidget->setObjectName("plannerGrid");
    m_gridLayout = new QVBoxLayout(m_gridWidget);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setSpacing(0);
    m_scroll->setWidget(m_gridWidget);

    // Panel celów po prawej
    m_goalsPanel = new GoalsPanel(this);

    splitter->addWidget(m_scroll);
    splitter->addWidget(m_goalsPanel);
    splitter->setStretchFactor(0, 7);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({700, 300});

    mainLayout->addWidget(splitter, 1);

    connect(m_slotGroup, &QButtonGroup::idClicked,
            this, &PlannerView::onSlotSizeChanged);
}

void PlannerView::rebuildGrid()
{
    // Wyczyść siatkę
    while (QLayoutItem* li = m_gridLayout->takeAt(0)) {
        if (QWidget* w = li->widget()) {
            w->setParent(nullptr);
            delete w;
        }
        delete li;
    }

    QString dateKey = m_date.toString("yyyy-MM-dd");
    QList<Event> events = DatabaseManager::instance().getEventsForDate(dateKey);

    // Przypisz każde wydarzenie do slotu (w minutach od północy)
    QMap<int, QList<Event>> slotEvents;
    for (const Event& e : events) {
        if (e.time.isEmpty()) {
            slotEvents[-1].append(e);   // bez czasu → całodniowe
        } else {
            QStringList parts = e.time.split(':');
            if (parts.size() >= 2) {
                int totalMin = parts[0].toInt() * 60 + parts[1].toInt();
                int slotStart = (totalMin / m_slotMinutes) * m_slotMinutes;
                slotEvents[slotStart].append(e);
            } else {
                slotEvents[-1].append(e);
            }
        }
    }

    // Rząd "całodniowe" (jeśli są wydarzenia bez czasu)
    if (slotEvents.contains(-1)) {
        auto* frame = new QFrame(m_gridWidget);
        frame->setObjectName("plannerSlotFrameHour");
        auto* row = new QHBoxLayout(frame);
        row->setContentsMargins(8, 4, 16, 4);
        row->setSpacing(0);

        auto* timeLabel = new QLabel("ca\u0142odz.", frame);
        timeLabel->setObjectName("plannerTimeLabel");
        timeLabel->setFixedWidth(60);
        timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(timeLabel);

        auto* line = new QWidget(frame);
        line->setObjectName("plannerSlotLine");
        line->setFixedWidth(1);
        row->addWidget(line);

        auto* content = new QWidget(frame);
        auto* cl = new QVBoxLayout(content);
        cl->setContentsMargins(8, 2, 0, 2);
        cl->setSpacing(2);
        for (const Event& e : slotEvents[-1]) {
            auto* chip = new QLabel(e.title, content);
            chip->setObjectName("plannerEventChip");
            chip->setWordWrap(true);
            cl->addWidget(chip);
        }
        row->addWidget(content, 1);
        m_gridLayout->addWidget(frame);
    }

    // Rzędy slotów czasowych
    int totalSlots = 1440 / m_slotMinutes;
    int scrollTarget = -1;  // slot nr 8:00

    for (int i = 0; i < totalSlots; ++i) {
        int startMin = i * m_slotMinutes;
        int h = startMin / 60;
        int m = startMin % 60;

        // Ramka wiersza — wyraźniejsza dla pełnych godzin przy widoku < 2h
        bool isHourBoundary = (m == 0);
        auto* frame = new QFrame(m_gridWidget);
        frame->setObjectName(isHourBoundary ? "plannerSlotFrameHour" : "plannerSlotFrame");
        frame->setMinimumHeight(36);

        auto* row = new QHBoxLayout(frame);
        row->setContentsMargins(8, 2, 16, 2);
        row->setSpacing(0);

        // Etykieta czasu
        auto* timeLabel = new QLabel(
            QString("%1:%2").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')),
            frame);
        timeLabel->setObjectName("plannerTimeLabel");
        timeLabel->setFixedWidth(60);
        timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(timeLabel);

        // Pionowa linia oddzielająca
        auto* line = new QWidget(frame);
        line->setObjectName("plannerSlotLine");
        line->setFixedWidth(1);
        row->addWidget(line);

        // Obszar treści (wydarzenia w tym slocie)
        auto* content = new QWidget(frame);
        auto* cl = new QVBoxLayout(content);
        cl->setContentsMargins(8, 1, 0, 1);
        cl->setSpacing(2);

        const QList<Event>& evs = slotEvents.value(startMin);
        for (const Event& e : evs) {
            QString label = e.time.isEmpty() ? e.title : e.time + "  " + e.title;
            auto* chip = new QLabel(label, content);
            chip->setObjectName("plannerEventChip");
            chip->setWordWrap(true);
            cl->addWidget(chip);
        }
        row->addWidget(content, 1);

        m_gridLayout->addWidget(frame);

        // Zapamiętaj pozycję 8:00 do późniejszego przewinięcia
        if (h == 8 && m == 0) scrollTarget = i;
    }

    m_gridLayout->addStretch();

    // Przewiń do 8:00 po wyrenderowaniu
    if (scrollTarget >= 0) {
        int targetSlot = scrollTarget;
        QTimer::singleShot(0, m_scroll, [this, targetSlot, totalSlots]() {
            int maxScroll = m_scroll->verticalScrollBar()->maximum();
            int pos = (maxScroll * targetSlot) / qMax(totalSlots, 1);
            m_scroll->verticalScrollBar()->setValue(pos);
        });
    }
}
