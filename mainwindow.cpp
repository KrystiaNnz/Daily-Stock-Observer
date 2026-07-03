#include "mainwindow.h"
#include "DatabaseManager.h"
#include "AddEventDialog.h"
#include "SettingsDialog.h"
#include "PatternGen.h"
#include "GoalsPanel.h"
#include "PortfolioPanel.h"
#include "MapPanel.h"
#include "FinancialCalculatorPanel.h"
#include "MainHubPanel.h"
#include "ProfileManager.h"

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
#include <QApplication>
#include <QSettings>

namespace {
constexpr int TabHome = 0;
constexpr int TabPlanner = 1;
constexpr int TabGoals = 2;
constexpr int TabPortfolio = 3;
constexpr int TabMap = 4;
constexpr int TabCalculator = 5;

QColor contrastTextFor(const QColor& background)
{
    const double luminance =
        0.2126 * background.redF() +
        0.7152 * background.greenF() +
        0.0722 * background.blueF();
    return luminance < 0.52 ? QColor("#ffffff") : QColor("#000000");
}

QColor blendColors(const QColor& foreground, const QColor& background, double foregroundWeight)
{
    const double bgWeight = 1.0 - foregroundWeight;
    return QColor(
        qBound(0, qRound(foreground.red() * foregroundWeight + background.red() * bgWeight), 255),
        qBound(0, qRound(foreground.green() * foregroundWeight + background.green() * bgWeight), 255),
        qBound(0, qRound(foreground.blue() * foregroundWeight + background.blue() * bgWeight), 255));
}
}

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
    setWindowTitle(ProfileManager::activeProfile().isTest
        ? "Daily Stock Observer - TEST"
        : "Daily Stock Observer");
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

    auto* appTitle = new QLabel(ProfileManager::activeProfile().isTest
        ? "Daily Stock Observer [TEST]"
        : "Daily Stock Observer", this);
    appTitle->setObjectName("appTitle");
    tabLayout->addWidget(appTitle);
    tabLayout->addSpacing(20);

    const QList<QString> tabLabels = {"Główny", "📅  Planer", "🎯  Cele", "📈  Portfel", "🗺  Mapa", "Kalkulator"};
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

    m_mainHubPanel = new MainHubPanel(this);
    m_stack->addWidget(m_mainHubPanel);      // index 0

    // ── PAGE 1: Planer (Kalendarz + Wydarzenia) ──────────
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

    m_stack->addWidget(planerPage);          // index 1

    // ── PAGE 2: Cele ──────────────────────────────────────
    m_goalsPanel = new GoalsPanel(this);
    m_goalsPanel->setDate(m_selectedDate);
    m_stack->addWidget(m_goalsPanel);        // index 2

    // ── PAGE 3: Portfel ───────────────────────────────────
    m_portfolioPanel = new PortfolioPanel(this);
    m_stack->addWidget(m_portfolioPanel);    // index 3

    // ── PAGE 4: Mapa ──────────────────────────────────────
    m_mapPanel = new MapPanel(this);
    m_stack->addWidget(m_mapPanel);          // index 4

    m_calculatorPanel = new FinancialCalculatorPanel(this);
    m_stack->addWidget(m_calculatorPanel);   // index 5

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

    connect(m_mainHubPanel, &MainHubPanel::navigateRequested,
            this, &MainWindow::switchTab);
}

void MainWindow::switchTab(int index)
{
    for (int i = 0; i < m_tabBtns.size(); ++i)
        m_tabBtns[i]->setChecked(i == index);

    m_stack->setCurrentIndex(index);

    if (index == TabHome)
        m_mainHubPanel->refresh();
    if (index == TabPortfolio)
        m_portfolioPanel->refreshTable();
    if (index == TabCalculator)
        m_calculatorPanel->refreshPortfolioAssets();
}

void MainWindow::onDateSelected(const QDate& date)
{
    m_selectedDate = date;
    refreshEvents();
    m_goalsPanel->setDate(date);
    m_mainHubPanel->refresh();
}

void MainWindow::onAddEvent()
{
    AddEventDialog dialog(m_selectedDate, this);
    if (dialog.exec() == QDialog::Accepted) {
        Event e = dialog.getEvent();
        if (DatabaseManager::instance().addEvent(e)) {
            refreshEvents();
            m_mainHubPanel->refresh();
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
        m_mainHubPanel->refresh();
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
    QSettings s("Kryst", ProfileManager::profileSettingsAppName());
    AppColors def = AppColors::defaults();

    const bool hasExtendedPalette = s.contains("colors/surface")
        && s.contains("colors/border")
        && s.contains("colors/text");
    if (!hasExtendedPalette) {
        m_colors = def;
        return;
    }

    m_colors.accent    = QColor(s.value("colors/accent",    def.accent.name()).toString());
    m_colors.leftPanel = QColor(s.value("colors/leftPanel", def.leftPanel.name()).toString());
    m_colors.appBg     = QColor(s.value("colors/appBg",     def.appBg.name()).toString());
    m_colors.surface   = QColor(s.value("colors/surface",   def.surface.name()).toString());
    m_colors.border    = QColor(s.value("colors/border",    def.border.name()).toString());
    m_colors.text      = QColor(s.value("colors/text",      def.text.name()).toString());
    m_colors.autoTextContrast = s.value("colors/autoTextContrast", def.autoTextContrast).toBool();
    m_colors.pattern   = static_cast<PatternType>(s.value("colors/pattern", 0).toInt());

    QString markStr    = s.value("colors/patternMark", "").toString();
    m_colors.patternMark = markStr.isEmpty()
        ? PatternGen::autoMark(m_colors.leftPanel)
        : QColor(markStr);

    if (!m_colors.accent.isValid())    m_colors.accent    = def.accent;
    if (!m_colors.leftPanel.isValid()) m_colors.leftPanel = def.leftPanel;
    if (!m_colors.appBg.isValid())     m_colors.appBg     = def.appBg;
    if (!m_colors.surface.isValid())   m_colors.surface   = def.surface;
    if (!m_colors.border.isValid())    m_colors.border    = def.border;
    if (!m_colors.text.isValid())      m_colors.text      = def.text;
}

void MainWindow::saveColors()
{
    QSettings s("Kryst", ProfileManager::profileSettingsAppName());
    s.setValue("colors/accent",    m_colors.accent.name());
    s.setValue("colors/leftPanel", m_colors.leftPanel.name());
    s.setValue("colors/appBg",     m_colors.appBg.name());
    s.setValue("colors/surface",   m_colors.surface.name());
    s.setValue("colors/border",    m_colors.border.name());
    s.setValue("colors/text",      m_colors.text.name());
    s.setValue("colors/autoTextContrast", m_colors.autoTextContrast);
    s.setValue("colors/pattern",     static_cast<int>(m_colors.pattern));
    s.setValue("colors/patternMark", m_colors.patternMark.isValid()
                                     ? m_colors.patternMark.name() : "");
}

void MainWindow::loadDisplaySettings()
{
    QSettings s("Kryst", ProfileManager::profileSettingsAppName());
    m_display.mode   = static_cast<WindowMode>(s.value("display/mode", 0).toInt());
    m_display.width  = s.value("display/width",  960).toInt();
    m_display.height = s.value("display/height", 660).toInt();
}

void MainWindow::saveDisplaySettings()
{
    QSettings s("Kryst", ProfileManager::profileSettingsAppName());
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
    ProfileSettings profileSettings;
    profileSettings.activeProfileId = ProfileManager::activeProfileId();
    profileSettings.selectedProfileId = ProfileManager::activeProfileId();
    profileSettings.askAtStartup = ProfileManager::askAtStartup();

    SettingsDialog dlg(m_colors, m_display, profileSettings, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_colors  = dlg.colors();
        m_display = dlg.display();
        const ProfileSettings nextProfile = dlg.profile();
        saveColors();
        saveDisplaySettings();
        ProfileManager::setAskAtStartup(nextProfile.askAtStartup);
        if (ProfileManager::normalizeProfileId(nextProfile.selectedProfileId) != ProfileManager::activeProfileId()) {
            ProfileManager::setStartupProfile(nextProfile.selectedProfileId);
            const DataProfile selected = ProfileManager::profile(nextProfile.selectedProfileId);
            QMessageBox::information(this, "Profil danych",
                "Profil \"" + selected.name + "\" zostanie uzyty po ponownym uruchomieniu aplikacji.");
        }
        applyStyle();
        applyDisplaySettings();
    }
}

void MainWindow::applyStyle()
{
    const QColor& acc = m_colors.accent;
    const QColor& lp  = m_colors.leftPanel;
    const QColor& bg  = m_colors.appBg;
    const QColor& surface = m_colors.surface;
    const QColor& border = m_colors.border;
    const QColor& text = m_colors.text;

    QString accHex   = acc.name();
    QString accHover = acc.darker(112).name();
    QString accPress = acc.darker(130).name();

    int hue = acc.hsvHue();
    QColor selBgColor = (hue >= 0)
        ? QColor::fromHsv(hue, 35, acc.lightness() < 128 ? 95 : 255)
        : QColor(235, 235, 235);
    QString selBg = selBgColor.name();
    QString selText = contrastTextFor(selBgColor).name();

    QString lpHex   = lp.name();
    QString calNav  = lp.darker(125).name();
    QString calGrid = lp.lighter(118).name();
    QString calLine = lp.lighter(148).name();
    QString tabLine = lp.darker(115).name();
    // Tło tabBar i leftPanel malowane przez eventFilter — nie potrzeba CSS background dla nich

    QString bgHex      = bg.name();
    QString surfaceHex = surface.name();
    QString borderHex  = border.name();
    const QColor bgTextColor = m_colors.autoTextContrast ? contrastTextFor(bg) : text;
    const QColor surfaceTextColor = m_colors.autoTextContrast ? contrastTextFor(surface) : text;
    const QColor leftTextColor = m_colors.autoTextContrast ? contrastTextFor(lp) : text;
    QString textHex    = surfaceTextColor.name();
    QString sepHex     = borderHex;
    QString listBord   = borderHex;

    bool lpDark = lp.lightness() < 128;
    bool bgDark = bg.lightness() < 128;
    bool surfaceDark = surface.lightness() < 128;

    QString goalItemBg   = surfaceHex;
    QString goalItemBord = borderHex;

    QString lpText       = leftTextColor.name();
    QString lpSubText    = blendColors(leftTextColor, lp, 0.58).name();
    QString lpBtnHoverBg = lpDark ? "rgba(255,255,255,0.10)" : "rgba(0,0,0,0.07)";

    QString dateText = bgTextColor.name();
    QString evText   = surfaceTextColor.name();
    QString bgMutedText = blendColors(bgTextColor, bg, 0.62).name();
    QString bgSubtleText = blendColors(bgTextColor, bg, 0.46).name();
    QString surfaceMutedText = blendColors(surfaceTextColor, surface, 0.62).name();
    QString surfaceSubtleText = blendColors(surfaceTextColor, surface, 0.46).name();
    QString disabledText = blendColors(surfaceTextColor, surface, 0.35).name();
    QString fieldBg = surfaceDark ? surface.lighter(118).name() : "#ffffff";
    QString fieldText = contrastTextFor(QColor(fieldBg)).name();
    QString fieldMutedText = blendColors(QColor(fieldText), QColor(fieldBg), 0.54).name();
    QString fieldDisabledBg = surfaceDark ? surface.lighter(108).name() : surface.darker(103).name();
    QString popupBg = surfaceDark ? surface.lighter(112).name() : "#ffffff";
    QString popupText = contrastTextFor(QColor(popupBg)).name();
    QString altRow = (hue >= 0 && !surfaceDark)
        ? QColor::fromHsv(hue, 18, 252).name()
        : (surfaceDark ? surface.lighter(125).name() : surface.darker(104).name());

    const QString appStyle =
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

        "QLineEdit, QTextEdit, QPlainTextEdit, QComboBox, QAbstractSpinBox {"
        " background: " + fieldBg + ";"
        " color: " + fieldText + ";"
        " border: 1px solid " + listBord + ";"
        " border-radius: 6px;"
        " padding: 5px 7px;"
        " selection-background-color: " + accHex + ";"
        " selection-color: " + contrastTextFor(acc).name() + "; }\n"

        "QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled,"
        " QComboBox:disabled, QAbstractSpinBox:disabled {"
        " color: " + fieldMutedText + ";"
        " background: " + fieldDisabledBg + "; }\n"

        "QAbstractSpinBox { padding-right: 28px; }\n"
        "QAbstractSpinBox::up-button, QAbstractSpinBox::down-button {"
        " subcontrol-origin: border;"
        " width: 20px;"
        " border-left: 1px solid " + listBord + "; }\n"
        "QAbstractSpinBox::up-button { subcontrol-position: top right; }\n"
        "QAbstractSpinBox::down-button { subcontrol-position: bottom right; }\n"

        "QComboBox::drop-down { border: none; width: 24px; }\n"

        "QComboBox QAbstractItemView {"
        " background: " + popupBg + ";"
        " color: " + popupText + ";"
        " border: 1px solid " + listBord + ";"
        " selection-background-color: " + selBg + ";"
        " selection-color: " + selText + "; }\n"

        "QCheckBox, QRadioButton, QLabel { color: " + evText + "; }\n"

        "QScrollArea { background: transparent; border: none; }\n"
        "QScrollArea > QWidget > QWidget { background: transparent; }\n"

        "QTableWidget, QTableView {"
        " background: " + surfaceHex + ";"
        " color: " + evText + ";"
        " alternate-background-color: " + altRow + ";"
        " gridline-color: " + listBord + ";"
        " border: 1px solid " + listBord + ";"
        " border-radius: 6px;"
        " selection-background-color: " + selBg + ";"
        " selection-color: " + selText + "; }\n"

        "QHeaderView::section {"
        " background: " + (surfaceDark ? surface.lighter(112).name() : surface.darker(104).name()) + ";"
        " color: " + evText + ";"
        " border: none;"
        " border-right: 1px solid " + listBord + ";"
        " border-bottom: 1px solid " + listBord + ";"
        " padding: 5px 7px;"
        " font-weight: 600; }\n"

        "QTabWidget::pane {"
        " border: 1px solid " + listBord + ";"
        " background: " + surfaceHex + ";"
        " border-radius: 6px; }\n"
        "QTabBar::tab {"
        " background: " + (surfaceDark ? surface.lighter(110).name() : surface.darker(103).name()) + ";"
        " color: " + surfaceMutedText + ";"
        " padding: 7px 12px;"
        " border: 1px solid " + listBord + ";"
        " border-bottom: none; }\n"
        "QTabBar::tab:selected {"
        " background: " + surfaceHex + ";"
        " color: " + evText + ";"
        " font-weight: 600; }\n"

        // ── Prawy panel (wydarzenia) ───────────────────────
        "#rightPanel { background: " + bgHex + "; }\n"

        "#dateLabel { font-size: 20px; font-weight: bold; color: " + dateText + ";"
        " padding-bottom: 4px; border-bottom: 2px solid " + accHex + "; }\n"

        "#eventList { background: " + surfaceHex + "; border: 1px solid " + listBord + ";"
        " border-radius: 8px; font-size: 14px; color: " + evText + "; padding: 4px; }\n"

        "#eventList::item { padding: 10px 12px;"
        " border-bottom: 1px solid " + bgHex + "; border-radius: 4px; }\n"

        "#eventList::item:selected { background: " + selBg + "; color: " + selText + "; }\n"

        "#eventList::item:alternate { background: " + altRow + "; }\n"

        "#addBtn { background: " + accHex + "; color: white; border: none;"
        " border-radius: 6px; padding: 10px 18px; font-size: 13px; font-weight: bold; }\n"
        "#addBtn:hover   { background: " + accHover + "; }\n"
        "#addBtn:pressed { background: " + accPress  + "; }\n"

        "#deleteBtn { background: " + surfaceHex + "; color: #e53e3e;"
        " border: 1.5px solid #e53e3e; border-radius: 6px;"
        " padding: 10px 18px; font-size: 13px; }\n"
        "#deleteBtn:hover    { background: #fff5f5; }\n"
        "#deleteBtn:pressed  { background: #fed7d7; }\n"
        "#deleteBtn:disabled { color: " + disabledText + "; border-color: " + borderHex + "; }\n"

        // ── Goals Panel ───────────────────────────────────
        "#goalsPanel { background: " + bgHex + "; }\n"
        "#goalsPanelScroll { background: " + bgHex + "; }\n"

        "#goalsPanelTitle { font-size: 15px; font-weight: bold;"
        " color: " + dateText + "; }\n"

        "#goalsDayLabel { font-size: 12px; color: " + bgMutedText + ";"
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

        "#goalProgressLabel { font-size: 11px; color: " + surfaceMutedText + ";"
        " font-family: Consolas, monospace; }\n"

        "QProgressBar#goalProgressBar { border: none; border-radius: 2px;"
        " background: " + altRow + "; max-height: 4px; min-height: 4px; }\n"
        "QProgressBar#goalProgressBar::chunk { background: " + accHex + ";"
        " border-radius: 2px; }\n"

        "#goalExpandBtn { background: transparent; border: none;"
        " color: " + surfaceSubtleText + "; font-size: 11px; font-weight: bold; }\n"
        "#goalExpandBtn:hover { color: " + evText + "; }\n"

        "#goalDeleteBtn { background: transparent; border: none;"
        " color: " + surfaceSubtleText + "; font-size: 12px; font-weight: bold; }\n"
        "#goalDeleteBtn:hover { color: #e53e3e; }\n"

        "#goalEditBtn { background: transparent; border: none;"
        " color: " + surfaceSubtleText + "; font-size: 12px; font-weight: bold; }\n"
        "#goalEditBtn:hover { color: " + evText + "; }\n"

        "#stepsScroll { background: " + goalItemBg + ";"
        " border: 1px solid " + listBord + "; border-radius: 6px; }\n"

        "#stepsEmpty { color: " + surfaceSubtleText + "; font-size: 11px; padding: 4px 2px; }\n"

        "QCheckBox#stepCheckbox { color: " + evText + "; font-size: 12px; padding: 3px 2px; }\n"
        "QCheckBox#stepCheckbox[done=\"true\"] { color: " + surfaceSubtleText + "; text-decoration: line-through; }\n"

        "#stepDeleteBtn { background: transparent; border: none; color: " + surfaceSubtleText + "; font-size: 10px; }\n"
        "#stepDeleteBtn:hover { color: #e53e3e; }\n"

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
        " color: " + bgMutedText + "; font-size: 12px; font-weight: 600;"
        " padding: 6px 4px; text-align: left; }\n"
        "#weekToggle:hover { color: " + evText + "; }\n"

        "#goalsEmpty { color: " + bgSubtleText + "; font-size: 13px; padding: 20px; }\n"

        "#goalsListScroll, #goalsListContainer { background: " + bgHex + "; }\n"
        "#goalsSearchEdit { background: " + fieldBg + "; color: " + fieldText + "; }\n"
        "#goalsDateHeader { color: " + bgMutedText + "; font-size: 12px;"
        " font-weight: 700; padding: 10px 2px 4px 2px; }\n"

        "#mainHubPanel { background: " + bgHex + "; }\n"
        "#hubTitle { font-size: 24px; font-weight: bold; color: " + dateText + "; }\n"
        "#hubSubtitle { font-size: 13px; color: " + bgMutedText + "; }\n"
        "#hubProfileBadge { color: " + evText + "; background: " + surfaceHex + ";"
        " border: 1px solid " + borderHex + "; border-radius: 6px;"
        " padding: 5px 10px; font-size: 12px; font-weight: 600; }\n"
        "#hubProfileBadge[testProfile=\"true\"] { color: #7f1d1d; background: #fee2e2;"
        " border-color: #fecaca; }\n"
        "QFrame#hubMetricCard, QFrame#hubModuleCard, QFrame#hubXpCard { background: " + goalItemBg + ";"
        " border: 1px solid " + goalItemBord + "; border-radius: 8px; }\n"
        "#hubMetricCaption { color: " + surfaceMutedText + "; font-size: 12px; }\n"
        "#hubMetricValue { color: " + evText + "; font-size: 22px; font-weight: bold; }\n"
        "#hubCardTitle { color: " + dateText + "; font-size: 16px; font-weight: bold; }\n"
        "#hubCardBody { color: " + evText + "; font-size: 13px; }\n"
        "#hubLevelBadge { color: white; background: " + accHex + "; border-radius: 6px;"
        " padding: 4px 10px; font-weight: bold; }\n"
        "QProgressBar#hubXpProgress { border: none; border-radius: 5px;"
        " background: " + altRow + "; min-height: 10px; max-height: 10px; }\n"
        "QProgressBar#hubXpProgress::chunk { background: " + accHex + "; border-radius: 5px; }\n"
        "#hubNavBtn { background: " + accHex + "; color: white; border: none;"
        " border-radius: 6px; padding: 9px 14px; font-size: 13px; font-weight: bold; }\n"
        "#hubNavBtn:hover { background: " + accHover + "; }\n"
        "#hubNavBtn:pressed { background: " + accPress + "; }\n"

        "#financialCalculatorPanel { background: " + bgHex + "; }\n"
        "#calcTitle { font-size: 22px; font-weight: bold; color: " + dateText + "; }\n"
        "#calcSubtitle { font-size: 13px; color: " + bgMutedText + "; padding-bottom: 6px; }\n"
        "QFrame#calcInputFrame, QFrame#calcResultFrame, QFrame#calcExerciseFrame { background: " + goalItemBg + ";"
        " border: 1px solid " + goalItemBord + "; border-radius: 8px; }\n"
        "#calcSectionTitle { font-size: 15px; font-weight: bold; color: " + dateText + "; }\n"
        "#calcResultMain { font-size: 18px; font-weight: bold; color: " + evText + "; }\n"
        "#calcFormula { font-family: Consolas, monospace; color: " + surfaceMutedText + "; padding: 8px 0; }\n"
        "#calcSourceStatus { color: " + surfaceMutedText + "; font-size: 12px; }\n"
        "#calcDetails { background: " + surfaceHex + "; border: 1px solid " + listBord + ";"
        " border-radius: 6px; color: " + evText + "; font-size: 13px; }\n"
        "#calcPrimaryBtn { background: " + accHex + "; color: white; border: none;"
        " border-radius: 6px; padding: 10px 18px; font-size: 13px; font-weight: bold; }\n"
        "#calcPrimaryBtn:hover { background: " + accHover + "; }\n"
        "#calcPrimaryBtn:pressed { background: " + accPress + "; }\n"
        "#calcPrimaryBtn:disabled { background: " + disabledText + "; }\n";

    qApp->setStyleSheet(appStyle);
    setStyleSheet(appStyle);

    // Wymuś przerysowanie widgetów obsługiwanych przez eventFilter
    if (m_tabBarWidget)    m_tabBarWidget->update();
    if (m_leftPanelWidget) m_leftPanelWidget->update();
}
