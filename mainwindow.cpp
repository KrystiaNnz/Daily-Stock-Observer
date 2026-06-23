#include "mainwindow.h"
#include "DatabaseManager.h"
#include "AddEventDialog.h"
#include "SettingsDialog.h"
#include "PatternGen.h"
#include "GoalsPanel.h"
#include "PortfolioPanel.h"
#include "MapPanel.h"

#include <QCalendarWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QStackedWidget>
#include <QMessageBox>
#include <QDate>
#include <QLocale>
#include <QFont>
#include <QCoreApplication>
#include <QSettings>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    if (!DatabaseManager::instance().init()) {
        QMessageBox::critical(nullptr, "Błąd bazy danych",
            "Nie udało się otworzyć bazy danych. Aplikacja zostanie zamknięta.");
        QCoreApplication::quit();
        return;
    }
    DatabaseManager::instance().seedGoals();

    m_selectedDate = QDate::currentDate();
    loadColors();
    loadDisplaySettings();

    setupUi();
    setupConnections();
    applyStyle();
    refreshEvents();
    applyDisplaySettings();
}

void MainWindow::setupUi()
{
    setWindowTitle("Daily Stock Observer");
    setMinimumSize(760, 540);
    resize(960, 660);

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── TAB BAR ──────────────────────────────────────────
    auto* tabBar = new QWidget(this);
    tabBar->setObjectName("tabBar");
    m_tabBarWidget = tabBar;
    tabBar->setFixedHeight(52);
    auto* tabLayout = new QHBoxLayout(tabBar);
    tabLayout->setContentsMargins(16, 0, 16, 0);
    tabLayout->setSpacing(2);

    auto* appTitle = new QLabel("Daily Stock Observer", this);
    appTitle->setObjectName("appTitle");
    tabLayout->addWidget(appTitle);
    tabLayout->addSpacing(20);

    const QList<QString> tabLabels = {"📅  Planer", "🎯  Cele", "📈  Portfel", "🗺  Mapa"};
    for (int i = 0; i < tabLabels.size(); ++i) {
        auto* btn = new QPushButton(tabLabels[i], this);
        btn->setObjectName("tabBtn");
        btn->setCheckable(true);
        btn->setChecked(i == 0);
        m_tabBtns.append(btn);
        tabLayout->addWidget(btn);
    }

    tabLayout->addStretch();

    m_settingsBtn = new QPushButton("⚙  Ustawienia", this);
    m_settingsBtn->setObjectName("settingsTabBtn");
    tabLayout->addWidget(m_settingsBtn);

    rootLayout->addWidget(tabBar);

    // Cienka linia pod tab barem
    auto* topLine = new QWidget(this);
    topLine->setFixedHeight(1);
    topLine->setObjectName("topLine");
    rootLayout->addWidget(topLine);

    // ── STACKED WIDGET ────────────────────────────────────
    m_stack = new QStackedWidget(this);
    m_stack->setObjectName("mainStack");
    rootLayout->addWidget(m_stack, 1);

    // ── PAGE 0: Planer (Kalendarz + Wydarzenia) ──────────
    auto* planerPage = new QWidget(this);
    auto* planerLayout = new QHBoxLayout(planerPage);
    planerLayout->setContentsMargins(0, 0, 0, 0);
    planerLayout->setSpacing(0);

    // Lewy: Kalendarz
    auto* calPanel = new QWidget(this);
    calPanel->setObjectName("leftPanel");
    m_leftPanelWidget = calPanel;
    calPanel->setFixedWidth(300);
    auto* calLayout = new QVBoxLayout(calPanel);
    calLayout->setContentsMargins(12, 16, 12, 16);
    calLayout->setSpacing(8);

    m_calendar = new QCalendarWidget(this);
    m_calendar->setLocale(QLocale(QLocale::Polish, QLocale::Poland));
    m_calendar->setGridVisible(true);
    m_calendar->setFirstDayOfWeek(Qt::Monday);
    m_calendar->setSelectedDate(m_selectedDate);
    calLayout->addWidget(m_calendar);
    calLayout->addStretch();

    // Separator pionowy
    auto* calSep = new QWidget(this);
    calSep->setFixedWidth(1);
    calSep->setObjectName("separator");

    // Prawy: Lista wydarzeń
    auto* eventsPanel = new QWidget(this);
    eventsPanel->setObjectName("rightPanel");
    auto* eventsLayout = new QVBoxLayout(eventsPanel);
    eventsLayout->setContentsMargins(20, 20, 20, 16);
    eventsLayout->setSpacing(10);

    m_dateLabel = new QLabel(this);
    m_dateLabel->setObjectName("dateLabel");
    eventsLayout->addWidget(m_dateLabel);

    m_eventList = new QListWidget(this);
    m_eventList->setObjectName("eventList");
    m_eventList->setAlternatingRowColors(true);
    m_eventList->setSelectionMode(QAbstractItemView::SingleSelection);
    eventsLayout->addWidget(m_eventList, 1);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    m_addBtn = new QPushButton("+ Dodaj wydarzenie", this);
    m_addBtn->setObjectName("addBtn");

    m_deleteBtn = new QPushButton("Usuń zaznaczone", this);
    m_deleteBtn->setObjectName("deleteBtn");
    m_deleteBtn->setEnabled(false);

    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_deleteBtn);
    eventsLayout->addLayout(btnLayout);

    planerLayout->addWidget(calPanel);
    planerLayout->addWidget(calSep);
    planerLayout->addWidget(eventsPanel, 1);

    m_stack->addWidget(planerPage);          // index 0

    // ── PAGE 1: Cele ──────────────────────────────────────
    m_goalsPanel = new GoalsPanel(this);
    m_goalsPanel->setDate(m_selectedDate);
    m_stack->addWidget(m_goalsPanel);        // index 1

    // ── PAGE 2: Portfel ───────────────────────────────────
    m_portfolioPanel = new PortfolioPanel(this);
    m_stack->addWidget(m_portfolioPanel);    // index 2

    // ── PAGE 3: Mapa ──────────────────────────────────────
    m_mapPanel = new MapPanel(this);
    m_stack->addWidget(m_mapPanel);          // index 3

    m_tabBarWidget->installEventFilter(this);
    m_leftPanelWidget->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Paint &&
        (watched == m_tabBarWidget || watched == m_leftPanelWidget))
    {
        auto* w = static_cast<QWidget*>(watched);
        QPainter painter(w);

        // Solid base color first
        painter.fillRect(w->rect(), m_colors.leftPanel);

        // Tile pattern on top (if any)
        if (m_colors.pattern != PatternType::None) {
            QColor mark = m_colors.patternMark.isValid()
                ? m_colors.patternMark
                : PatternGen::autoMark(m_colors.leftPanel);
            QPixmap tile = PatternGen::generate(m_colors.pattern, m_colors.leftPanel, mark);
            painter.fillRect(w->rect(), QBrush(tile));
        }

        return true;   // skip widget's own paintEvent (handles background)
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupConnections()
{
    // Przełączanie zakładek
    for (int i = 0; i < m_tabBtns.size(); ++i) {
        connect(m_tabBtns[i], &QPushButton::clicked, this, [this, i]() {
            switchTab(i);
        });
    }

    // Zmiana daty w kalendarzu
    connect(m_calendar, &QCalendarWidget::selectionChanged, this, [this]() {
        onDateSelected(m_calendar->selectedDate());
    });

    // Dodaj wydarzenie
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddEvent);

    // Usuń wydarzenie
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteEvent);

    // Aktywuj przycisk Usuń tylko gdy coś zaznaczono
    connect(m_eventList, &QListWidget::itemSelectionChanged, this, [this]() {
        m_deleteBtn->setEnabled(!m_eventList->selectedItems().isEmpty());
    });

    // Podwójne kliknięcie = dodaj (wygodny skrót)
    connect(m_eventList, &QListWidget::itemDoubleClicked, this, &MainWindow::onAddEvent);

    // Ustawienia
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::onSettings);
}

void MainWindow::switchTab(int index)
{
    for (int i = 0; i < m_tabBtns.size(); ++i)
        m_tabBtns[i]->setChecked(i == index);

    m_stack->setCurrentIndex(index);

    if (index == 2)
        m_portfolioPanel->refreshTable();
}

void MainWindow::onDateSelected(const QDate& date)
{
    m_selectedDate = date;
    refreshEvents();
    m_goalsPanel->setDate(date);
}

void MainWindow::onAddEvent()
{
    AddEventDialog dialog(m_selectedDate, this);
    if (dialog.exec() == QDialog::Accepted) {
        Event e = dialog.getEvent();
        if (DatabaseManager::instance().addEvent(e)) {
            refreshEvents();
        } else {
            QMessageBox::warning(this, "Błąd", "Nie udało się zapisać wydarzenia.");
        }
    }
}

void MainWindow::onDeleteEvent()
{
    auto* item = m_eventList->currentItem();
    if (!item) return;

    int id = item->data(Qt::UserRole).toInt();
    auto reply = QMessageBox::question(this, "Usuń wydarzenie",
        "Czy na pewno chcesz usunąć:\n\"" + item->text() + "\"?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        DatabaseManager::instance().deleteEvent(id);
        refreshEvents();
    }
}

void MainWindow::refreshEvents()
{
    QString dateStr = m_selectedDate.toString("dddd, d MMMM yyyy");
    dateStr[0] = dateStr[0].toUpper();
    m_dateLabel->setText(dateStr);

    m_eventList->clear();
    m_deleteBtn->setEnabled(false);

    QString dateKey = m_selectedDate.toString("yyyy-MM-dd");
    QList<Event> events = DatabaseManager::instance().getEventsForDate(dateKey);

    if (events.isEmpty()) {
        auto* placeholder = new QListWidgetItem("(brak wydarzeń — kliknij '+ Dodaj')");
        placeholder->setFlags(Qt::NoItemFlags);
        placeholder->setForeground(Qt::gray);
        m_eventList->addItem(placeholder);
        return;
    }

    for (const Event& e : events) {
        QString label;
        if (!e.time.isEmpty())
            label = e.time + "  –  " + e.title;
        else
            label = e.title;

        if (!e.description.isEmpty())
            label += "\n    " + e.description;

        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, e.id);
        m_eventList->addItem(item);
    }
}

void MainWindow::loadColors()
{
    QSettings s("Kryst", "DailyStockObserver");
    AppColors def = AppColors::defaults();
    m_colors.accent    = QColor(s.value("colors/accent",    def.accent.name()).toString());
    m_colors.leftPanel = QColor(s.value("colors/leftPanel", def.leftPanel.name()).toString());
    m_colors.appBg     = QColor(s.value("colors/appBg",     def.appBg.name()).toString());
    m_colors.pattern   = static_cast<PatternType>(s.value("colors/pattern", 0).toInt());

    QString markStr    = s.value("colors/patternMark", "").toString();
    m_colors.patternMark = markStr.isEmpty()
        ? PatternGen::autoMark(m_colors.leftPanel)
        : QColor(markStr);

    if (!m_colors.accent.isValid())    m_colors.accent    = def.accent;
    if (!m_colors.leftPanel.isValid()) m_colors.leftPanel = def.leftPanel;
    if (!m_colors.appBg.isValid())     m_colors.appBg     = def.appBg;
}

void MainWindow::saveColors()
{
    QSettings s("Kryst", "DailyStockObserver");
    s.setValue("colors/accent",    m_colors.accent.name());
    s.setValue("colors/leftPanel", m_colors.leftPanel.name());
    s.setValue("colors/appBg",     m_colors.appBg.name());
    s.setValue("colors/pattern",     static_cast<int>(m_colors.pattern));
    s.setValue("colors/patternMark", m_colors.patternMark.isValid()
                                     ? m_colors.patternMark.name() : "");
}

void MainWindow::loadDisplaySettings()
{
    QSettings s("Kryst", "DailyStockObserver");
    m_display.mode   = static_cast<WindowMode>(s.value("display/mode", 0).toInt());
    m_display.width  = s.value("display/width",  960).toInt();
    m_display.height = s.value("display/height", 660).toInt();
}

void MainWindow::saveDisplaySettings()
{
    QSettings s("Kryst", "DailyStockObserver");
    s.setValue("display/mode",   static_cast<int>(m_display.mode));
    s.setValue("display/width",  m_display.width);
    s.setValue("display/height", m_display.height);
}

void MainWindow::applyDisplaySettings()
{
    switch (m_display.mode) {
        case WindowMode::Normal:
            showNormal();
            resize(m_display.width, m_display.height);
            break;
        case WindowMode::Maximized:
            showMaximized();
            break;
        case WindowMode::FullScreen:
            showFullScreen();
            break;
    }
}

void MainWindow::onSettings()
{
    SettingsDialog dlg(m_colors, m_display, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_colors  = dlg.colors();
        m_display = dlg.display();
        saveColors();
        saveDisplaySettings();
        applyStyle();
        applyDisplaySettings();
    }
}

void MainWindow::applyStyle()
{
    const QColor& acc = m_colors.accent;
    const QColor& lp  = m_colors.leftPanel;
    const QColor& bg  = m_colors.appBg;

    QString accHex   = acc.name();
    QString accHover = acc.darker(112).name();
    QString accPress = acc.darker(130).name();

    int hue = acc.hsvHue();
    QString selBg   = (hue >= 0)
        ? QColor::fromHsv(hue, 35, 255).name()
        : QColor(235, 235, 235).name();
    QString selText = (hue >= 0)
        ? QColor::fromHsv(hue, qMin(acc.hsvSaturation() + 60, 255),
                               qMax(acc.value() - 55, 80)).name()
        : QColor(60, 60, 60).name();
    QString altRow  = (hue >= 0)
        ? QColor::fromHsv(hue, 18, 252).name()
        : QColor(248, 248, 248).name();

    QString lpHex   = lp.name();
    QString calNav  = lp.darker(125).name();
    QString calGrid = lp.lighter(118).name();
    QString calLine = lp.lighter(148).name();
    QString tabLine = lp.darker(115).name();
    // Tło tabBar i leftPanel malowane przez eventFilter — nie potrzeba CSS background dla nich

    QString bgHex    = bg.name();
    QString sepHex   = bg.darker(115).name();
    QString listBord = bg.darker(108).name();

    bool lpDark = lp.lightness() < 128;
    bool bgDark = bg.lightness() < 128;

    QString goalItemBg   = bgDark ? bg.lighter(115).name() : "#ffffff";
    QString goalItemBord = bgDark ? bg.lighter(140).name() : listBord;

    QString lpText       = lpDark ? "#ffffff"               : "#222222";
    QString lpSubText    = lpDark ? "rgba(255,255,255,0.55)" : "rgba(0,0,0,0.50)";
    QString lpBtnHoverBg = lpDark ? "rgba(255,255,255,0.10)" : "rgba(0,0,0,0.07)";

    QString dateText = bgDark ? "#e8eaf0" : "#1a2535";
    QString evText   = bgDark ? "#dde1e8" : "#2d3748";

    setStyleSheet(
        "QMainWindow { background: " + bgHex + "; }\n"

        // ── Tab bar — tło malowane przez eventFilter ──────
        "#tabBar { }\n"

        "#appTitle { color: " + lpText + "; font-size: 15px;"
        " font-weight: bold; padding: 0 4px; }\n"

        "QPushButton#tabBtn {"
        " background: transparent;"
        " color: " + lpSubText + ";"
        " border: none;"
        " border-bottom: 3px solid transparent;"
        " border-radius: 0;"
        " padding: 14px 18px 11px 18px;"
        " font-size: 13px;"
        " font-weight: 500;"
        " min-width: 90px; }\n"

        "QPushButton#tabBtn:hover {"
        " color: " + lpText + ";"
        " background: " + lpBtnHoverBg + "; }\n"

        "QPushButton#tabBtn:checked {"
        " color: " + lpText + ";"
        " border-bottom: 3px solid " + accHex + ";"
        " font-weight: bold; }\n"

        "QPushButton#settingsTabBtn {"
        " background: transparent;"
        " color: " + lpSubText + ";"
        " border: 1px solid rgba(128,128,128,0.25);"
        " border-radius: 6px;"
        " padding: 6px 12px;"
        " font-size: 12px; }\n"
        "QPushButton#settingsTabBtn:hover {"
        " color: " + lpText + ";"
        " background: " + lpBtnHoverBg + "; }\n"

        "#topLine { background: " + tabLine + "; }\n"

        // ── Lewy panel — tło malowane przez eventFilter ───
        "#leftPanel { }\n"

        "QCalendarWidget { background: " + lpHex + "; color: " + lpText + ";"
        " border-radius: 8px; }\n"

        "QCalendarWidget QAbstractItemView {"
        " background: "                 + calGrid + ";"
        " color: "                      + lpText  + ";"
        " selection-background-color: " + accHex  + ";"
        " selection-color: #ffffff;"
        " gridline-color: "             + calLine + "; }\n"

        "QCalendarWidget QWidget#qt_calendar_navigationbar { background: " + calNav + "; }\n"

        "QCalendarWidget QToolButton { color: " + lpText + ";"
        " background: transparent; font-weight: bold; }\n"

        "QCalendarWidget QMenu { background: " + calGrid + "; color: " + lpText + "; }\n"

        "QCalendarWidget QSpinBox { color: " + lpText + "; background: " + calGrid + "; }\n"

        "#separator { background: " + sepHex + "; }\n"

        // ── Prawy panel (wydarzenia) ───────────────────────
        "#rightPanel { background: " + bgHex + "; }\n"

        "#dateLabel { font-size: 20px; font-weight: bold; color: " + dateText + ";"
        " padding-bottom: 4px; border-bottom: 2px solid " + accHex + "; }\n"

        "#eventList { background: #ffffff; border: 1px solid " + listBord + ";"
        " border-radius: 8px; font-size: 14px; color: " + evText + "; padding: 4px; }\n"

        "#eventList::item { padding: 10px 12px;"
        " border-bottom: 1px solid " + bgHex + "; border-radius: 4px; }\n"

        "#eventList::item:selected { background: " + selBg + "; color: " + selText + "; }\n"

        "#eventList::item:alternate { background: " + altRow + "; }\n"

        "#addBtn { background: " + accHex + "; color: white; border: none;"
        " border-radius: 6px; padding: 10px 18px; font-size: 13px; font-weight: bold; }\n"
        "#addBtn:hover   { background: " + accHover + "; }\n"
        "#addBtn:pressed { background: " + accPress  + "; }\n"

        "#deleteBtn { background: #ffffff; color: #e53e3e;"
        " border: 1.5px solid #e53e3e; border-radius: 6px;"
        " padding: 10px 18px; font-size: 13px; }\n"
        "#deleteBtn:hover    { background: #fff5f5; }\n"
        "#deleteBtn:pressed  { background: #fed7d7; }\n"
        "#deleteBtn:disabled { color: #aaa; border-color: #ccc; }\n"

        // ── Goals Panel ───────────────────────────────────
        "#goalsPanel { background: " + bgHex + "; }\n"
        "#goalsPanelScroll { background: " + bgHex + "; }\n"

        "#goalsPanelTitle { font-size: 15px; font-weight: bold;"
        " color: " + dateText + "; }\n"

        "#goalsDayLabel { font-size: 12px; color: #999;"
        " padding: 0 0 6px 0; border-bottom: 1px solid " + listBord + "; }\n"

        "#goalsAddBtn { background: " + accHex + "; color: white; border: none;"
        " border-radius: 6px; padding: 6px 14px; font-size: 12px;"
        " font-weight: bold; }\n"
        "#goalsAddBtn:hover   { background: " + accHover + "; }\n"
        "#goalsAddBtn:pressed { background: " + accPress  + "; }\n"

        "QFrame#goalItem { background: " + goalItemBg + ";"
        " border: 1px solid " + goalItemBord + "; border-radius: 8px; }\n"

        "#goalTitle { font-size: 13px; font-weight: 600;"
        " color: " + evText + "; }\n"

        "#goalProgressLabel { font-size: 11px; color: #999;"
        " font-family: Consolas, monospace; }\n"

        "QProgressBar#goalProgressBar { border: none; border-radius: 2px;"
        " background: " + altRow + "; max-height: 4px; min-height: 4px; }\n"
        "QProgressBar#goalProgressBar::chunk { background: " + accHex + ";"
        " border-radius: 2px; }\n"

        "#goalExpandBtn { background: transparent; border: none;"
        " color: #bbb; font-size: 11px; font-weight: bold; }\n"
        "#goalExpandBtn:hover { color: " + evText + "; }\n"

        "#goalDeleteBtn { background: transparent; border: none;"
        " color: #ddd; font-size: 12px; font-weight: bold; }\n"
        "#goalDeleteBtn:hover { color: #e53e3e; }\n"

        "#stepsScroll { background: " + goalItemBg + ";"
        " border: 1px solid " + listBord + "; border-radius: 6px; }\n"

        "#newStepEdit { border: 1px solid " + listBord + "; border-radius: 4px;"
        " padding: 3px 7px; font-size: 12px; background: " + goalItemBg + ";"
        " color: " + evText + "; }\n"
        "#newStepEdit:focus { border-color: " + accHex + "; }\n"

        "#addStepBtn { background: " + accHex + "; color: white; border: none;"
        " border-radius: 4px; font-size: 14px; font-weight: bold; }\n"
        "#addStepBtn:hover   { background: " + accHover + "; }\n"
        "#addStepBtn:pressed { background: " + accPress  + "; }\n"

        "#goalsSep { background: " + listBord + "; }\n"

        "#weekToggle { background: transparent; border: none;"
        " color: #999; font-size: 12px; font-weight: 600;"
        " padding: 6px 4px; text-align: left; }\n"
        "#weekToggle:hover { color: " + evText + "; }\n"

        "#goalsEmpty { color: #bbb; font-size: 13px; padding: 20px; }\n"
    );

    // Wymuś przerysowanie widgetów obsługiwanych przez eventFilter
    if (m_tabBarWidget)    m_tabBarWidget->update();
    if (m_leftPanelWidget) m_leftPanelWidget->update();
}
