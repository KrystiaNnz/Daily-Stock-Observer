#include "MainHubPanel.h"
#include "DatabaseManager.h"
#include "ProfileManager.h"

#include <QDate>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

MainHubPanel::MainHubPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    refresh();
}

void MainHubPanel::setupUi()
{
    setObjectName("mainHubPanel");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);

    auto* headerRow = new QHBoxLayout();
    headerRow->setSpacing(12);

    auto* titleBlock = new QVBoxLayout();
    titleBlock->setSpacing(4);

    auto* title = new QLabel("Panel glowny", this);
    title->setObjectName("hubTitle");
    titleBlock->addWidget(title);

    m_dateLabel = new QLabel(this);
    m_dateLabel->setObjectName("hubSubtitle");
    titleBlock->addWidget(m_dateLabel);

    headerRow->addLayout(titleBlock);
    headerRow->addStretch();

    m_profileLabel = new QLabel(this);
    m_profileLabel->setObjectName("hubProfileBadge");
    headerRow->addWidget(m_profileLabel);

    auto* xpFrame = new QFrame(this);
    xpFrame->setObjectName("hubXpCard");
    xpFrame->setMinimumHeight(72);
    auto* xpLayout = new QVBoxLayout(xpFrame);
    xpLayout->setContentsMargins(16, 10, 16, 10);
    xpLayout->setSpacing(6);

    auto* xpHeader = new QHBoxLayout();
    auto* xpTitle = new QLabel("Rozwoj / XP", xpFrame);
    xpTitle->setObjectName("hubCardTitle");
    xpHeader->addWidget(xpTitle);
    xpHeader->addStretch();
    m_levelLabel = new QLabel("Level 1", xpFrame);
    m_levelLabel->setObjectName("hubLevelBadge");
    xpHeader->addWidget(m_levelLabel);
    xpLayout->addLayout(xpHeader);

    m_xpProgress = new QProgressBar(xpFrame);
    m_xpProgress->setObjectName("hubXpProgress");
    m_xpProgress->setTextVisible(false);
    m_xpProgress->setRange(0, 100);
    xpLayout->addWidget(m_xpProgress);

    auto* xpMeta = new QHBoxLayout();
    m_xpLabel = new QLabel("-", xpFrame);
    m_xpLabel->setObjectName("hubCardBody");
    m_streakLabel = new QLabel("-", xpFrame);
    m_streakLabel->setObjectName("hubCardBody");
    m_completedStepsLabel = new QLabel("-", xpFrame);
    m_completedStepsLabel->setObjectName("hubCardBody");
    xpMeta->addWidget(m_xpLabel);
    xpMeta->addStretch();
    xpMeta->addWidget(m_streakLabel);
    xpMeta->addSpacing(12);
    xpMeta->addWidget(m_completedStepsLabel);
    xpLayout->addLayout(xpMeta);

    headerRow->addWidget(xpFrame, 1);
    root->addLayout(headerRow);

    auto* metricsGrid = new QGridLayout();
    metricsGrid->setHorizontalSpacing(12);
    metricsGrid->setVerticalSpacing(12);

    auto makeMetric = [this](const QString& label, QLabel** valueLabel) {
        auto* frame = new QFrame(this);
        frame->setObjectName("hubMetricCard");
        auto* layout = new QVBoxLayout(frame);
        layout->setContentsMargins(16, 14, 16, 14);
        layout->setSpacing(4);

        auto* caption = new QLabel(label, frame);
        caption->setObjectName("hubMetricCaption");
        layout->addWidget(caption);

        *valueLabel = new QLabel("-", frame);
        (*valueLabel)->setObjectName("hubMetricValue");
        layout->addWidget(*valueLabel);
        return frame;
    };

    metricsGrid->addWidget(makeMetric("Wydarzenia dzisiaj", &m_eventsMetric), 0, 0);
    metricsGrid->addWidget(makeMetric("Cele dzisiaj", &m_goalsMetric), 0, 1);
    metricsGrid->addWidget(makeMetric("Aktywa w portfelu", &m_assetsMetric), 0, 2);
    metricsGrid->addWidget(makeMetric("Koszt portfela", &m_portfolioCostMetric), 0, 3);
    root->addLayout(metricsGrid);

    auto* modulesGrid = new QGridLayout();
    modulesGrid->setHorizontalSpacing(12);
    modulesGrid->setVerticalSpacing(12);

    modulesGrid->addWidget(createModuleCard(
        "Planer",
        "Kalendarz, wydarzenia i szybkie dodawanie zadan na wybrany dzien.",
        "Otworz planer",
        1), 0, 0);

    modulesGrid->addWidget(createModuleCard(
        "Cele",
        "Cele dnia, cele tygodniowe i projekty dlugoterminowe.",
        "Otworz cele",
        2), 0, 1);

    modulesGrid->addWidget(createModuleCard(
        "Portfel",
        "Aktywa, kursy live, analiza spolki i mapa rynku.",
        "Otworz portfel",
        3), 0, 2);

    modulesGrid->addWidget(createModuleCard(
        "Mapa",
        "Kraje, regiony administracyjne i lokalne zrodla danych.",
        "Otworz mape",
        4), 1, 0);

    modulesGrid->addWidget(createModuleCard(
        "Kalkulator",
        "Stopa zwrotu, realna stopa zwrotu i dane z portfela.",
        "Otworz kalkulator",
        5), 1, 1);

    auto* spacer = new QFrame(this);
    spacer->setObjectName("hubModuleCard");
    auto* spacerLayout = new QVBoxLayout(spacer);
    spacerLayout->setContentsMargins(16, 14, 16, 14);
    auto* spacerTitle = new QLabel("Nastepne centrum pracy", spacer);
    spacerTitle->setObjectName("hubCardTitle");
    auto* spacerBody = new QLabel("Tutaj mozna pozniej podpiac alerty, watchlisty i szybkie akcje z danych rynkowych.", spacer);
    spacerBody->setObjectName("hubCardBody");
    spacerBody->setWordWrap(true);
    spacerLayout->addWidget(spacerTitle);
    spacerLayout->addWidget(spacerBody);
    spacerLayout->addStretch();
    modulesGrid->addWidget(spacer, 1, 2);

    root->addLayout(modulesGrid, 1);
}

QFrame* MainHubPanel::createModuleCard(const QString& title,
                                       const QString& body,
                                       const QString& buttonText,
                                       int targetIndex)
{
    auto* frame = new QFrame(this);
    frame->setObjectName("hubModuleCard");

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);

    auto* titleLabel = new QLabel(title, frame);
    titleLabel->setObjectName("hubCardTitle");
    layout->addWidget(titleLabel);

    auto* bodyLabel = new QLabel(body, frame);
    bodyLabel->setObjectName("hubCardBody");
    bodyLabel->setWordWrap(true);
    layout->addWidget(bodyLabel, 1);

    auto* button = new QPushButton(buttonText, frame);
    button->setObjectName("hubNavBtn");
    connect(button, &QPushButton::clicked, this, [this, targetIndex]() {
        emit navigateRequested(targetIndex);
    });
    layout->addWidget(button);

    return frame;
}

void MainHubPanel::refresh()
{
    const DataProfile profile = ProfileManager::activeProfile();
    if (m_profileLabel) {
        m_profileLabel->setText(profile.isTest ? "Profil testowy" : "Profil prywatny");
        m_profileLabel->setProperty("testProfile", profile.isTest);
        m_profileLabel->style()->unpolish(m_profileLabel);
        m_profileLabel->style()->polish(m_profileLabel);
    }

    const QDate today = QDate::currentDate();
    const QString dateKey = today.toString("yyyy-MM-dd");
    QString dateText = QLocale(QLocale::Polish, QLocale::Poland)
        .toString(today, "dddd, d MMMM yyyy");
    if (!dateText.isEmpty())
        dateText[0] = dateText[0].toUpper();
    m_dateLabel->setText(dateText);

    const QList<Event> events = DatabaseManager::instance().getEventsForDate(dateKey);
    const QList<Goal> goals = DatabaseManager::instance().getGoalsForDate(dateKey);
    const QList<PortfolioAsset> assets = DatabaseManager::instance().getAllAssets();
    const ExperienceProfile xp = DatabaseManager::instance().getExperienceProfile();

    double portfolioCost = 0.0;
    for (const PortfolioAsset& asset : assets)
        portfolioCost += asset.quantity * asset.avgBuyPrice;

    m_eventsMetric->setText(QString::number(events.size()));
    m_goalsMetric->setText(QString::number(goals.size()));
    m_assetsMetric->setText(QString::number(assets.size()));
    m_portfolioCostMetric->setText(QString::number(portfolioCost, 'f', 2) + " PLN");

    m_levelLabel->setText(QString("Level %1").arg(xp.level));
    m_xpLabel->setText(QString("%1 / %2 XP do nastepnego poziomu")
        .arg(xp.xpIntoLevel)
        .arg(xp.xpForNextLevel));
    m_xpProgress->setValue(xp.xpForNextLevel > 0
        ? xp.xpIntoLevel * 100 / xp.xpForNextLevel
        : 0);
    m_streakLabel->setText(QString("Streak: %1 dni").arg(xp.streakDays));
    m_completedStepsLabel->setText(QString("Dzisiaj: %1 krokow").arg(xp.completedStepsToday));
}
