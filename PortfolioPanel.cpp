#include "PortfolioPanel.h"
#include "AddAssetDialog.h"
#include "DatabaseManager.h"
#include "CompanyAnalysisPanel.h"
#include "PeersPanel.h"
#include "HistoricalPriceChartWidget.h"
#include "HistoricalPriceAnalysisDialog.h"
#include "TickerSearchEdit.h"

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QColor>
#include <QSet>
#include <QSplitter>
#include <QTabWidget>
#include <QDate>
#include <QDateEdit>
#include <QLineEdit>

// ── Pomocnicze: item tylko do odczytu ─────────────────
static QTableWidgetItem* roItem(const QString& text)
{
    auto* it = new QTableWidgetItem(text);
    it->setFlags(it->flags() & ~Qt::ItemIsEditable);
    return it;
}

static QTableWidgetItem* roItemNum(double val, int decimals = 2)
{
    auto* it = new QTableWidgetItem(QString::number(val, 'f', decimals));
    it->setFlags(it->flags() & ~Qt::ItemIsEditable);
    it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return it;
}

// ══════════════════════════════════════════════════════
// PortfolioPanel
// ══════════════════════════════════════════════════════

PortfolioPanel::PortfolioPanel(QWidget* parent)
    : QWidget(parent), m_fetcher(this), m_historyFetcher(this), m_alphaFetcher(this)
{
    setupUi();

    connect(&m_fetcher, &PortfolioFetcher::fetchFinished,
            this, &PortfolioPanel::onFetchFinished);
    connect(&m_fetcher, &PortfolioFetcher::fetchError,
            this, &PortfolioPanel::onFetchError);
    connect(&m_historyFetcher, &HistoricalPriceFetcher::fetchFinished,
            this, &PortfolioPanel::onHistoryFetchFinished);
    connect(&m_historyFetcher, &HistoricalPriceFetcher::fetchError,
            this, &PortfolioPanel::onHistoryFetchError);
    connect(&m_alphaFetcher, &AlphaEngineFetcher::factorLinksFinished,
            this, &PortfolioPanel::onAlphaFactorLinksFinished);
    connect(&m_alphaFetcher, &AlphaEngineFetcher::factorLinksError,
            this, &PortfolioPanel::onAlphaFactorLinksError);
}

void PortfolioPanel::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 12);
    root->setSpacing(10);

    // ── Pasek narzędziowy ─────────────────────────────
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(8);

    auto* title = new QLabel("Portfel inwestycyjny", this);
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    toolbar->addWidget(title);
    toolbar->addStretch();

    m_statusLabel = new QLabel("Brak danych", this);
    m_statusLabel->setStyleSheet("color: #999; font-size: 12px;");
    toolbar->addWidget(m_statusLabel);

    // ── Filtr kategorii ───────────────────────────────
    m_typeFilterCombo = new QComboBox(this);
    m_typeFilterCombo->addItem("Wszystkie typy");
    m_typeFilterCombo->setMinimumWidth(160);
    toolbar->addWidget(m_typeFilterCombo);

    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItem("Wszystkie kategorie");
    m_filterCombo->setMinimumWidth(180);
    toolbar->addWidget(m_filterCombo);

    m_refreshBtn = new QPushButton("Odśwież kursy", this);
    m_deleteBtn  = new QPushButton("Usuń zaznaczone", this);
    m_addBtn     = new QPushButton("+ Dodaj aktywo", this);

    m_deleteBtn->setEnabled(false);

    m_addBtn->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border: none;"
        " border-radius: 6px; padding: 7px 14px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }"
        "QPushButton:pressed { background: #1d4ed8; }");

    m_deleteBtn->setStyleSheet(
        "QPushButton { color: #e53e3e; border: 1.5px solid #e53e3e;"
        " border-radius: 6px; padding: 7px 14px; background: white; }"
        "QPushButton:hover { background: #fff5f5; }"
        "QPushButton:disabled { color: #aaa; border-color: #ccc; }");

    toolbar->addWidget(m_refreshBtn);
    toolbar->addWidget(m_deleteBtn);
    toolbar->addWidget(m_addBtn);

    root->addLayout(toolbar);

    // ── Tabela ────────────────────────────────────────
    m_table = new QTableWidget(0, ColCount, this);
    m_table->setHorizontalHeaderLabels({
        "Ticker", "Nazwa", "Typ", "Kategoria", "Ilość", "Cena zakupu",
        "Kurs live", "Wartość", "P&L (PLN)", "P&L (%)"
    });

    m_table->horizontalHeader()->setSectionResizeMode(Nazwa,      QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(Typ,        QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Kategoria,  QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Ticker,     QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Ilosc,      QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSortingEnabled(true);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(true);

    // ── Dolny panel: zakładki Analiza / Mapa Rynku ───────
    m_analysisPanel = new CompanyAnalysisPanel(this);
    m_peersPanel    = new PeersPanel(this);

    m_bottomTabs = new QTabWidget(this);
    m_bottomTabs->setTabPosition(QTabWidget::North);
    m_bottomTabs->setDocumentMode(true);
    m_bottomTabs->addTab(m_analysisPanel, "Analiza spółki");
    m_bottomTabs->addTab(m_peersPanel,    "Mapa Rynku");
    m_bottomTabs->addTab(createReportsTab(), "Raporty");
    m_bottomTabs->addTab(createAlphaTab(), "Alpha");

    // ── Splitter: tabela (góra) + zakładki (dół) ─────────
    m_splitter = new QSplitter(Qt::Vertical, this);
    m_splitter->setHandleWidth(4);
    m_splitter->addWidget(m_table);
    m_splitter->addWidget(m_bottomTabs);
    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 2);
    m_splitter->setSizes({300, 250});

    root->addWidget(m_splitter, 1);

    // ── Podsumowanie ──────────────────────────────────
    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setStyleSheet("font-size: 13px; color: #444; padding: 4px 0;");
    root->addWidget(m_summaryLabel);

    // ── Połączenia ────────────────────────────────────
    connect(m_addBtn,     &QPushButton::clicked, this, &PortfolioPanel::onAddAsset);
    connect(m_deleteBtn,  &QPushButton::clicked, this, &PortfolioPanel::onDeleteAsset);
    connect(m_refreshBtn, &QPushButton::clicked, this, &PortfolioPanel::onRefreshPrices);
    connect(m_typeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PortfolioPanel::onFilterChanged);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PortfolioPanel::onFilterChanged);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &PortfolioPanel::onSelectionChanged);
}

// ── Dane ──────────────────────────────────────────────

QWidget* PortfolioPanel::createReportsTab()
{
    auto* page = new QWidget(this);
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    auto* controls = new QHBoxLayout();
    controls->setSpacing(8);

    controls->addWidget(new QLabel("Ticker:", page));
    m_historyTickerCombo = new QComboBox(page);
    m_historyTickerCombo->setEditable(true);
    m_historyTickerCombo->setLineEdit(new TickerSearchEdit(m_historyTickerCombo));
    m_historyTickerCombo->setInsertPolicy(QComboBox::NoInsert);
    m_historyTickerCombo->lineEdit()->setPlaceholderText("Wpisz nazwę lub ticker...");
    m_historyTickerCombo->setMinimumWidth(140);
    controls->addWidget(m_historyTickerCombo);

    controls->addWidget(new QLabel("Od:", page));
    m_historyStartEdit = new QDateEdit(QDate::currentDate().addYears(-1), page);
    m_historyStartEdit->setCalendarPopup(true);
    m_historyStartEdit->setDisplayFormat("yyyy-MM-dd");
    controls->addWidget(m_historyStartEdit);

    controls->addWidget(new QLabel("Do:", page));
    m_historyEndEdit = new QDateEdit(QDate::currentDate(), page);
    m_historyEndEdit->setCalendarPopup(true);
    m_historyEndEdit->setDisplayFormat("yyyy-MM-dd");
    controls->addWidget(m_historyEndEdit);

    controls->addWidget(new QLabel("Wykres:", page));
    m_historyChartTypeCombo = new QComboBox(page);
    m_historyChartTypeCombo->addItem("Linia", int(HistoricalPriceChartWidget::ChartType::LineClose));
    m_historyChartTypeCombo->addItem("Slupki", int(HistoricalPriceChartWidget::ChartType::BarClose));
    m_historyChartTypeCombo->addItem("Swiece", int(HistoricalPriceChartWidget::ChartType::Candlestick));
    m_historyChartTypeCombo->setMinimumWidth(110);
    controls->addWidget(m_historyChartTypeCombo);

    m_historyLoadBtn = new QPushButton("Załaduj tabelę", page);
    controls->addWidget(m_historyLoadBtn);
    m_historyExpandBtn = new QPushButton("Rozszerz", page);
    m_historyExpandBtn->setEnabled(false);
    controls->addWidget(m_historyExpandBtn);
    controls->addStretch();
    root->addLayout(controls);

    m_historyStatusLabel = new QLabel("Wybierz ticker i zakres dat.", page);
    m_historyStatusLabel->setStyleSheet("color: #666; font-size: 12px;");
    root->addWidget(m_historyStatusLabel);

    m_historyChart = new HistoricalPriceChartWidget(page);

    m_historyTable = new QTableWidget(0, 8, page);
    m_historyTable->setHorizontalHeaderLabels({
        "Data", "Open", "High", "Low", "Close", "Adj Close", "Volume", "Źródło"
    });
    m_historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setSortingEnabled(true);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->verticalHeader()->setVisible(false);

    auto* historySplitter = new QSplitter(Qt::Vertical, page);
    historySplitter->setHandleWidth(4);
    historySplitter->addWidget(m_historyChart);
    historySplitter->addWidget(m_historyTable);
    historySplitter->setStretchFactor(0, 2);
    historySplitter->setStretchFactor(1, 3);
    historySplitter->setSizes({240, 320});
    root->addWidget(historySplitter, 1);

    connect(m_historyLoadBtn, &QPushButton::clicked,
            this, &PortfolioPanel::onLoadHistoricalPrices);
    connect(m_historyChartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PortfolioPanel::onHistoryChartTypeChanged);
    connect(m_historyExpandBtn, &QPushButton::clicked,
            this, &PortfolioPanel::onOpenHistoryAnalysis);

    return page;
}

QWidget* PortfolioPanel::createAlphaTab()
{
    auto* page = new QWidget(this);
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    auto* controls = new QHBoxLayout();
    controls->setSpacing(8);

    controls->addWidget(new QLabel("Ticker:", page));
    m_alphaTickerCombo = new QComboBox(page);
    m_alphaTickerCombo->setEditable(true);
    m_alphaTickerCombo->setLineEdit(new TickerSearchEdit(m_alphaTickerCombo));
    m_alphaTickerCombo->setInsertPolicy(QComboBox::NoInsert);
    m_alphaTickerCombo->lineEdit()->setPlaceholderText("Wpisz nazwę lub ticker...");
    m_alphaTickerCombo->setMinimumWidth(140);
    controls->addWidget(m_alphaTickerCombo);

    m_alphaComputeBtn = new QPushButton("Policz czynniki", page);
    controls->addWidget(m_alphaComputeBtn);
    controls->addStretch();
    root->addLayout(controls);

    m_alphaStatusLabel = new QLabel(
        "Alpha MVP: korelacje i kowariancje tickera z lokalnymi czynnikami w bazie.",
        page);
    m_alphaStatusLabel->setStyleSheet("color: #666; font-size: 12px;");
    root->addWidget(m_alphaStatusLabel);

    m_alphaTable = new QTableWidget(0, 7, page);
    m_alphaTable->setHorizontalHeaderLabels({
        "Czynnik", "Typ", "Korelacja", "Kowariancja", "Obserwacje", "Stabilność", "Okno"
    });
    m_alphaTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_alphaTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_alphaTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_alphaTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_alphaTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_alphaTable->setSortingEnabled(true);
    m_alphaTable->setAlternatingRowColors(true);
    m_alphaTable->verticalHeader()->setVisible(false);
    root->addWidget(m_alphaTable, 1);

    connect(m_alphaComputeBtn, &QPushButton::clicked,
            this, &PortfolioPanel::onComputeAlphaFactors);

    return page;
}

void PortfolioPanel::refreshTable()
{
    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);

    const QList<PortfolioAsset> assets = DatabaseManager::instance().getAllAssets();

    for (const PortfolioAsset& a : assets) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        m_table->setItem(row, Ticker,     roItem(a.ticker));
        m_table->setItem(row, Nazwa,      roItem(a.name));
        m_table->setItem(row, Typ,        roItem(a.assetType.isEmpty() ? "Akcja" : a.assetType));
        m_table->setItem(row, Kategoria,  roItem(a.category));
        m_table->setItem(row, Ilosc,      roItemNum(a.quantity, 4));
        m_table->setItem(row, CenaZakupu, roItemNum(a.avgBuyPrice, 4));
        m_table->setItem(row, KursLive,   roItem("—"));
        m_table->setItem(row, Wartosc,    roItem("—"));
        m_table->setItem(row, PnlPln,     roItem("—"));
        m_table->setItem(row, PnlPct,     roItem("—"));

        // Przechowaj asset.id i walutę zakupu w tickerze
        m_table->item(row, Ticker)->setData(Qt::UserRole,     a.id);
        m_table->item(row, Ticker)->setData(Qt::UserRole + 1, a.currency);
    }

    m_table->setSortingEnabled(true);
    rebuildFilterCombos();
    rebuildHistoryTickerCombo();
    rebuildAlphaTickerCombo();
    applyFilter();
    updateSummary();

    // Pobierz kursy jeśli cache jest nieaktualny
    if (!m_fetcher.isCacheValid() && m_table->rowCount() > 0)
        onRefreshPrices();
    else if (m_fetcher.isCacheValid())
        applyLivePrices(m_fetcher.cachedQuotes());
}

void PortfolioPanel::onAddAsset()
{
    AddAssetDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    PortfolioAsset      asset = dlg.getAsset();
    PortfolioTransaction tx   = dlg.getTransaction();

    if (!DatabaseManager::instance().addAsset(asset)) {
        QMessageBox::warning(this, "Błąd",
            "Nie udało się dodać aktywa.\n"
            "Ticker " + asset.ticker + " może już istnieć w portfelu.");
        return;
    }
    DatabaseManager::instance().addTransaction(tx);

    refreshTable();
}

void PortfolioPanel::onDeleteAsset()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    QString ticker = m_table->item(row, Ticker)->text();
    int     id     = m_table->item(row, Ticker)->data(Qt::UserRole).toInt();

    auto reply = QMessageBox::question(this, "Usuń aktywo",
        "Czy na pewno chcesz usunąć " + ticker + " z portfela?\n"
        "Historia transakcji zostanie zachowana.",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        DatabaseManager::instance().deleteAsset(id);
        refreshTable();
    }
}

void PortfolioPanel::onRefreshPrices()
{
    if (m_fetcher.isFetching()) return;

    QStringList tickers;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QString ticker   = m_table->item(r, Ticker)->text();
        QString currency = m_table->item(r, Ticker)->data(Qt::UserRole + 1).toString();
        QString assetType = m_table->item(r, Typ)->text();
        if (assetType == "Obligacja rządowa" || assetType == "Obligacja korporacyjna")
            continue;
        tickers << (currency.isEmpty() ? ticker : ticker + ":" + currency);
    }

    if (tickers.isEmpty()) return;

    setRefreshing(true);
    m_fetcher.fetchPrices(tickers);
}

void PortfolioPanel::onFetchFinished(const QMap<QString, PortfolioQuote>& quotes)
{
    setRefreshing(false);
    applyLivePrices(quotes);

    if (!quotes.isEmpty()) {
        QString ts = quotes.first().timestamp;
        m_statusLabel->setText("Kurs z: " + ts.replace("T", " ").left(16));
        m_statusLabel->setStyleSheet("color: #666; font-size: 12px;");
    }
}

void PortfolioPanel::onFetchError(const QString& message)
{
    setRefreshing(false);
    m_statusLabel->setText("Błąd: " + message);
    m_statusLabel->setStyleSheet("color: #e53e3e; font-size: 12px;");
}

void PortfolioPanel::onSelectionChanged()
{
    int row = m_table->currentRow();
    m_deleteBtn->setEnabled(row >= 0);

    if (row >= 0 && !m_table->isRowHidden(row)) {
        QString ticker = m_table->item(row, Ticker)->text();
        if (!ticker.isEmpty()) {
            m_analysisPanel->loadCompany(ticker);
            m_peersPanel->loadPeers(ticker);
        }
    } else {
        m_analysisPanel->clear();
        m_peersPanel->clear();
    }
}

// ── Prywatne ─────────────────────────────────────────

void PortfolioPanel::applyLivePrices(const QMap<QString, PortfolioQuote>& quotes)
{
    m_table->setSortingEnabled(false);

    for (int r = 0; r < m_table->rowCount(); ++r) {
        QString ticker = m_table->item(r, Ticker)->text();
        if (!quotes.contains(ticker)) continue;

        const PortfolioQuote& q = quotes[ticker];
        if (!q.valid) {
            m_table->item(r, KursLive)->setText("N/A");
            continue;
        }

        double qty      = m_table->item(r, Ilosc)->text().toDouble();
        double buyPrice = m_table->item(r, CenaZakupu)->text().toDouble();
        double livePrice = (q.priceConverted > 0) ? q.priceConverted : q.price;
        double value    = qty * livePrice;
        double pnlPln   = (livePrice - buyPrice) * qty;
        double pnlPct   = (buyPrice > 0) ? (livePrice - buyPrice) / buyPrice * 100.0 : 0.0;

        // Aktualizuj nazwę jeśli była pusta
        if (m_table->item(r, Nazwa)->text().isEmpty() && !q.name.isEmpty())
            m_table->item(r, Nazwa)->setText(q.name);

        QString kursTxt = QString::number(livePrice, 'f', 4);
        if (q.liveCurrency != q.currency && !q.liveCurrency.isEmpty())
            kursTxt += QString(" (%1→%2 %3)")
                .arg(q.liveCurrency).arg(q.currency)
                .arg(QString::number(q.exchangeRate, 'f', 4));
        auto* kursItem = new QTableWidgetItem(kursTxt);
        kursItem->setFlags(kursItem->flags() & ~Qt::ItemIsEditable);
        kursItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, KursLive, kursItem);
        m_table->setItem(r, Wartosc,  roItemNum(value,   2));
        m_table->setItem(r, PnlPln,   roItemNum(pnlPln,  2));
        m_table->setItem(r, PnlPct,   roItemNum(pnlPct,  2));

        // Kolorowanie P&L
        QColor pnlColor = (pnlPln >= 0) ? QColor("#15803d") : QColor("#b91c1c");
        m_table->item(r, PnlPln)->setForeground(pnlColor);
        m_table->item(r, PnlPct)->setForeground(pnlColor);
    }

    m_table->setSortingEnabled(true);
    updateSummary();
}

void PortfolioPanel::updateSummary()
{
    double totalValue = 0.0;
    double totalPnl   = 0.0;
    bool   hasLive    = false;

    for (int r = 0; r < m_table->rowCount(); ++r) {
        QString valStr = m_table->item(r, Wartosc)->text();
        QString pnlStr = m_table->item(r, PnlPln)->text();
        if (valStr == "—" || valStr == "N/A") continue;
        hasLive    = true;
        totalValue += valStr.toDouble();
        totalPnl   += pnlStr.toDouble();
    }

    if (!hasLive || m_table->rowCount() == 0) {
        m_summaryLabel->setText(
            QString("Aktywa: %1").arg(m_table->rowCount()));
        return;
    }

    double totalCost = totalValue - totalPnl;
    double pnlPct    = (totalCost > 0) ? (totalPnl / totalCost * 100.0) : 0.0;

    QString pnlSign  = (totalPnl >= 0) ? "+" : "";
    QString pnlColor = (totalPnl >= 0) ? "#15803d" : "#b91c1c";

    m_summaryLabel->setText(
        QString("Aktywa: %1  |  Wartość portfela: <b>%2 PLN</b>  |  "
                "P&amp;L: <span style='color:%3'><b>%4%5 PLN (%6%7%)</b></span>")
        .arg(m_table->rowCount())
        .arg(QString::number(totalValue, 'f', 2))
        .arg(pnlColor)
        .arg(pnlSign)
        .arg(QString::number(totalPnl, 'f', 2))
        .arg(pnlSign)
        .arg(QString::number(pnlPct, 'f', 2)));
    m_summaryLabel->setTextFormat(Qt::RichText);
}

void PortfolioPanel::setRefreshing(bool active)
{
    m_refreshBtn->setEnabled(!active);
    m_refreshBtn->setText(active ? "Pobieranie..." : "Odśwież kursy");
    if (active) {
        m_statusLabel->setText("Pobieranie kursów...");
        m_statusLabel->setStyleSheet("color: #3b82f6; font-size: 12px;");
    }
}

void PortfolioPanel::rebuildFilterCombos()
{
    QString currentType = m_typeFilterCombo->currentText();
    QString currentCat = m_filterCombo->currentText();

    QSet<QString> types;
    QSet<QString> cats;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QString type = m_table->item(r, Typ)->text().trimmed();
        QString cat = m_table->item(r, Kategoria)->text().trimmed();
        if (!type.isEmpty())
            types.insert(type);
        if (!cat.isEmpty())
            cats.insert(cat);
    }

    m_typeFilterCombo->blockSignals(true);
    m_typeFilterCombo->clear();
    m_typeFilterCombo->addItem("Wszystkie typy");
    QStringList sortedTypes = QStringList(types.begin(), types.end());
    sortedTypes.sort();
    m_typeFilterCombo->addItems(sortedTypes);

    int typeIdx = m_typeFilterCombo->findText(currentType);
    m_typeFilterCombo->setCurrentIndex(typeIdx >= 0 ? typeIdx : 0);
    m_typeFilterCombo->blockSignals(false);

    m_filterCombo->blockSignals(true);
    m_filterCombo->clear();
    m_filterCombo->addItem("Wszystkie kategorie");
    QStringList sorted = QStringList(cats.begin(), cats.end());
    sorted.sort();
    m_filterCombo->addItems(sorted);

    int idx = m_filterCombo->findText(currentCat);
    m_filterCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    m_filterCombo->blockSignals(false);
}

void PortfolioPanel::applyFilter()
{
    QString typeFilter = m_typeFilterCombo->currentText();
    QString catFilter = m_filterCombo->currentText();
    bool showAllTypes = (typeFilter == "Wszystkie typy" || m_typeFilterCombo->currentIndex() == 0);
    bool showAllCats = (catFilter == "Wszystkie kategorie" || m_filterCombo->currentIndex() == 0);

    for (int r = 0; r < m_table->rowCount(); ++r) {
        bool matchType = showAllTypes ||
                         m_table->item(r, Typ)->text().trimmed() == typeFilter;
        bool matchCat = showAllCats ||
                        m_table->item(r, Kategoria)->text().trimmed() == catFilter;
        bool match = matchType && matchCat;
        m_table->setRowHidden(r, !match);
    }
    updateSummary();
}

void PortfolioPanel::onFilterChanged()
{
    applyFilter();
}

QString PortfolioPanel::historyStartTimestamp() const
{
    return m_historyStartEdit->date().toString("yyyy-MM-dd") + "T00:00:00";
}

QString PortfolioPanel::historyEndTimestamp() const
{
    return m_historyEndEdit->date().toString("yyyy-MM-dd") + "T23:59:59";
}

void PortfolioPanel::rebuildHistoryTickerCombo()
{
    if (!m_historyTickerCombo) return;

    QString current = m_historyTickerCombo->currentText().trimmed();
    QSet<QString> tickers;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QString ticker = m_table->item(r, Ticker)->text().trimmed().toUpper();
        if (!ticker.isEmpty())
            tickers.insert(ticker);
    }

    m_historyTickerCombo->blockSignals(true);
    m_historyTickerCombo->clear();
    QStringList sorted = QStringList(tickers.begin(), tickers.end());
    sorted.sort();
    m_historyTickerCombo->addItems(sorted);
    if (!current.isEmpty()) {
        int idx = m_historyTickerCombo->findText(current, Qt::MatchFixedString);
        if (idx >= 0)
            m_historyTickerCombo->setCurrentIndex(idx);
        else
            m_historyTickerCombo->setEditText(current);
    }
    m_historyTickerCombo->blockSignals(false);
}

void PortfolioPanel::rebuildAlphaTickerCombo()
{
    if (!m_alphaTickerCombo) return;

    QString current = m_alphaTickerCombo->currentText().trimmed();
    QSet<QString> tickers;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QString ticker = m_table->item(r, Ticker)->text().trimmed().toUpper();
        if (!ticker.isEmpty())
            tickers.insert(ticker);
    }

    m_alphaTickerCombo->blockSignals(true);
    m_alphaTickerCombo->clear();
    QStringList sorted = QStringList(tickers.begin(), tickers.end());
    sorted.sort();
    m_alphaTickerCombo->addItems(sorted);
    if (!current.isEmpty()) {
        int idx = m_alphaTickerCombo->findText(current, Qt::MatchFixedString);
        if (idx >= 0)
            m_alphaTickerCombo->setCurrentIndex(idx);
        else
            m_alphaTickerCombo->setEditText(current);
    }
    m_alphaTickerCombo->blockSignals(false);
}

void PortfolioPanel::setAlphaBusy(bool active)
{
    m_alphaComputeBtn->setEnabled(!active);
    m_alphaComputeBtn->setText(active ? "Liczenie..." : "Policz czynniki");
    if (active) {
        m_alphaStatusLabel->setText("Liczenie kowariancji i korelacji na lokalnej bazie...");
        m_alphaStatusLabel->setStyleSheet("color: #3b82f6; font-size: 12px;");
    }
}

void PortfolioPanel::updateAlphaTable(const QList<AssetFactorCovariance>& rows)
{
    m_alphaTable->setSortingEnabled(false);
    m_alphaTable->setRowCount(0);

    for (const AssetFactorCovariance& item : rows) {
        const int row = m_alphaTable->rowCount();
        m_alphaTable->insertRow(row);
        m_alphaTable->setItem(row, 0, roItem(item.factorCode));
        m_alphaTable->setItem(row, 1, roItem(item.factorType));
        m_alphaTable->setItem(row, 2, roItemNum(item.correlation, 4));
        m_alphaTable->setItem(row, 3, roItemNum(item.covariance, 8));
        m_alphaTable->setItem(row, 4, roItemNum(item.observations, 0));
        m_alphaTable->setItem(row, 5, roItemNum(item.stabilityScore, 2));
        m_alphaTable->setItem(row, 6, roItem(QString::number(item.windowDays) + " dni"));

        QColor corrColor = (item.correlation >= 0.0) ? QColor("#15803d") : QColor("#b91c1c");
        m_alphaTable->item(row, 2)->setForeground(corrColor);
    }

    m_alphaTable->setSortingEnabled(true);
}

void PortfolioPanel::onComputeAlphaFactors()
{
    if (m_alphaFetcher.isRunning())
        return;

    const QString ticker = m_alphaTickerCombo->currentText().trimmed().toUpper();
    if (ticker.isEmpty()) {
        QMessageBox::warning(this, "Brak tickera", "Podaj ticker do analizy Alpha.");
        return;
    }

    setAlphaBusy(true);
    updateAlphaTable({});
    m_alphaFetcher.computeFactorLinks(ticker, 25, 756, 0);
}

void PortfolioPanel::onAlphaFactorLinksFinished(const QString& ticker,
                                                int candidateRows,
                                                int savedRows,
                                                const QList<AssetFactorCovariance>& rows)
{
    setAlphaBusy(false);
    updateAlphaTable(rows);

    m_alphaStatusLabel->setText(
        QString("Alpha MVP %1: pokazano %2 czynnikow, zapisano %3, kandydaci %4.")
            .arg(ticker)
            .arg(rows.size())
            .arg(savedRows)
            .arg(candidateRows));
    m_alphaStatusLabel->setStyleSheet("color: #666; font-size: 12px;");
}

void PortfolioPanel::onAlphaFactorLinksError(const QString& message)
{
    setAlphaBusy(false);
    updateAlphaTable({});

    QString guidance = message;
    if (message.contains("Za malo danych cenowych", Qt::CaseInsensitive)) {
        const QString ticker = m_alphaTickerCombo
            ? m_alphaTickerCombo->currentText().trimmed().toUpper()
            : QString();
        if (m_historyTickerCombo && !ticker.isEmpty())
            m_historyTickerCombo->setEditText(ticker);
        guidance += " Najpierw wybierz poprawny ticker z listy podpowiedzi i pobierz jego historie cen w zakladce Raporty.";
    }

    m_alphaStatusLabel->setText("Blad Alpha: " + guidance);
    m_alphaStatusLabel->setStyleSheet("color: #e53e3e; font-size: 12px;");
}

void PortfolioPanel::onLoadHistoricalPrices()
{
    if (m_historyFetcher.isFetching()) return;

    QString ticker = m_historyTickerCombo->currentText().trimmed().toUpper();
    if (ticker.isEmpty()) {
        m_historyChart->clearChart();
        m_historyRows.clear();
        m_historyExpandBtn->setEnabled(false);
        QMessageBox::warning(this, "Brak tickera", "Podaj ticker do pobrania historii cen.");
        return;
    }
    if (m_historyStartEdit->date() > m_historyEndEdit->date()) {
        m_historyChart->clearChart();
        m_historyRows.clear();
        m_historyExpandBtn->setEnabled(false);
        QMessageBox::warning(this, "Błędny zakres", "Data początkowa musi być wcześniejsza niż data końcowa.");
        return;
    }

    m_historyTicker = ticker;
    m_historyInterval = "1d";
    m_historySource = "yfinance";
    queueMissingHistoryRanges();

    if (m_pendingHistoryRanges.isEmpty()) {
        loadHistoricalTable();
        return;
    }

    setHistoryBusy(true);
    startNextHistoryFetch();
}

void PortfolioPanel::onHistoryChartTypeChanged(int index)
{
    if (!m_historyChartTypeCombo || !m_historyChart)
        return;

    bool ok = false;
    const int rawType = m_historyChartTypeCombo->itemData(index).toInt(&ok);
    if (!ok)
        return;

    m_historyChart->setChartType(static_cast<HistoricalPriceChartWidget::ChartType>(rawType));
}

void PortfolioPanel::onOpenHistoryAnalysis()
{
    if (m_historyRows.isEmpty()) {
        QMessageBox::information(this, "Analiza wykresu", "Najpierw zaladuj dane historyczne dla wybranego tickera.");
        return;
    }

    const auto chartType = static_cast<HistoricalPriceChartWidget::ChartType>(
        m_historyChartTypeCombo->currentData().toInt());
    HistoricalPriceAnalysisDialog dialog(
        m_historyRows,
        m_historyTicker,
        m_historyInterval,
        m_historyCurrency,
        chartType,
        this);
    dialog.exec();
}

void PortfolioPanel::queueMissingHistoryRanges()
{
    m_pendingHistoryRanges.clear();

    const QDate requestedStart = m_historyStartEdit->date();
    const QDate requestedEnd = m_historyEndEdit->date();
    const QString reqStartTs = historyStartTimestamp();
    const QString reqEndTs = historyEndTimestamp();

    MarketPriceCacheMeta meta = DatabaseManager::instance().getMarketPriceCacheMeta(
        m_historyTicker, m_historyInterval, m_historySource);

    if (!meta.exists || meta.firstTimestamp.isEmpty() || meta.lastTimestamp.isEmpty()) {
        m_pendingHistoryRanges.append({requestedStart.toString("yyyy-MM-dd"),
                                       requestedEnd.toString("yyyy-MM-dd")});
        return;
    }

    QDate cachedFirst = QDate::fromString(meta.firstTimestamp.left(10), "yyyy-MM-dd");
    QDate cachedLast = QDate::fromString(meta.lastTimestamp.left(10), "yyyy-MM-dd");
    if (!cachedFirst.isValid() || !cachedLast.isValid()) {
        m_pendingHistoryRanges.append({requestedStart.toString("yyyy-MM-dd"),
                                       requestedEnd.toString("yyyy-MM-dd")});
        return;
    }

    if (reqStartTs < meta.firstTimestamp) {
        QDate endBeforeCache = cachedFirst.addDays(-1);
        if (requestedStart <= endBeforeCache)
            m_pendingHistoryRanges.append({requestedStart.toString("yyyy-MM-dd"),
                                           endBeforeCache.toString("yyyy-MM-dd")});
    }

    if (reqEndTs > meta.lastTimestamp) {
        QDate startAfterCache = cachedLast.addDays(1);
        if (startAfterCache <= requestedEnd)
            m_pendingHistoryRanges.append({startAfterCache.toString("yyyy-MM-dd"),
                                           requestedEnd.toString("yyyy-MM-dd")});
    }
}

void PortfolioPanel::startNextHistoryFetch()
{
    if (m_pendingHistoryRanges.isEmpty()) {
        setHistoryBusy(false);
        loadHistoricalTable();
        return;
    }

    QPair<QString, QString> range = m_pendingHistoryRanges.takeFirst();
    m_historyStatusLabel->setText(
        QString("Pobieranie %1: %2 - %3...")
            .arg(m_historyTicker, range.first, range.second));
    m_historyFetcher.fetch(m_historyTicker, range.first, range.second, m_historyInterval);
}

void PortfolioPanel::onHistoryFetchFinished(const QString& ticker,
                                            const QString& interval,
                                            const QString& source,
                                            const QString& currency,
                                            const QList<MarketPriceBar>& bars)
{
    Q_UNUSED(ticker);
    Q_UNUSED(interval);
    Q_UNUSED(source);

    if (!bars.isEmpty()) {
        DatabaseManager::instance().upsertMarketPriceBars(bars);
        m_historyCurrency = currency;
    }
    DatabaseManager::instance().refreshMarketPriceCacheMeta(
        m_historyTicker, m_historyInterval, m_historySource, m_historyCurrency, "ok");

    startNextHistoryFetch();
}

void PortfolioPanel::onHistoryFetchError(const QString& message)
{
    m_historyChart->clearChart();
    m_historyRows.clear();
    m_historyExpandBtn->setEnabled(false);
    DatabaseManager::instance().refreshMarketPriceCacheMeta(
        m_historyTicker, m_historyInterval, m_historySource, m_historyCurrency, "error", message);
    setHistoryBusy(false);
    m_historyStatusLabel->setText("Błąd pobierania: " + message);
    m_historyStatusLabel->setStyleSheet("color: #e53e3e; font-size: 12px;");
}

void PortfolioPanel::loadHistoricalTable()
{
    QList<MarketPriceBar> rows = DatabaseManager::instance().getMarketPriceBars(
        m_historyTicker, m_historyInterval, historyStartTimestamp(), historyEndTimestamp(), m_historySource);
    m_historyRows = rows;

    m_historyTable->setSortingEnabled(false);
    m_historyTable->setRowCount(0);

    for (const MarketPriceBar& bar : rows) {
        int row = m_historyTable->rowCount();
        m_historyTable->insertRow(row);
        m_historyTable->setItem(row, 0, roItem(bar.tradingDate));
        m_historyTable->setItem(row, 1, roItemNum(bar.open, 4));
        m_historyTable->setItem(row, 2, roItemNum(bar.high, 4));
        m_historyTable->setItem(row, 3, roItemNum(bar.low, 4));
        m_historyTable->setItem(row, 4, roItemNum(bar.close, 4));
        m_historyTable->setItem(row, 5, roItemNum(bar.adjClose, 4));
        m_historyTable->setItem(row, 6, roItemNum(bar.volume, 0));
        m_historyTable->setItem(row, 7, roItem(bar.source));
    }

    m_historyTable->setSortingEnabled(true);

    MarketPriceCacheMeta meta = DatabaseManager::instance().getMarketPriceCacheMeta(
        m_historyTicker, m_historyInterval, m_historySource);
    if (m_historyCurrency.isEmpty())
        m_historyCurrency = meta.currency;
    m_historyChart->setBars(rows, m_historyTicker, m_historyInterval, m_historyCurrency);
    m_historyExpandBtn->setEnabled(!rows.isEmpty());

    QString cacheInfo;
    if (meta.exists && !meta.firstTimestamp.isEmpty() && !meta.lastTimestamp.isEmpty()) {
        cacheInfo = QString(" Cache: %1 - %2.")
            .arg(meta.firstTimestamp.left(10), meta.lastTimestamp.left(10));
    }
    m_historyStatusLabel->setText(
        QString("Tabela %1 (%2): %3 wierszy.%4")
            .arg(m_historyTicker, m_historyInterval)
            .arg(rows.size())
            .arg(cacheInfo));
    m_historyStatusLabel->setStyleSheet("color: #666; font-size: 12px;");
}

void PortfolioPanel::setHistoryBusy(bool active)
{
    m_historyLoadBtn->setEnabled(!active);
    m_historyLoadBtn->setText(active ? "Pobieranie..." : "Załaduj tabelę");
    if (active)
        m_historyStatusLabel->setStyleSheet("color: #3b82f6; font-size: 12px;");
}
