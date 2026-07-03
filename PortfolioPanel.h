#pragma once

#include <QWidget>
#include <QMap>
#include <QPair>
#include "PortfolioFetcher.h"
#include "HistoricalPriceFetcher.h"

class QTableWidget;
class QLabel;
class QPushButton;
class QComboBox;
class QDateEdit;
class QSplitter;
class QTabWidget;
class CompanyAnalysisPanel;
class PeersPanel;
class HistoricalPriceChartWidget;

class PortfolioPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PortfolioPanel(QWidget* parent = nullptr);

public slots:
    void refreshTable();

private slots:
    void onAddAsset();
    void onDeleteAsset();
    void onRefreshPrices();
    void onFetchFinished(const QMap<QString, PortfolioQuote>& quotes);
    void onFetchError(const QString& message);
    void onSelectionChanged();
    void onFilterChanged();
    void onLoadHistoricalPrices();
    void onHistoryChartTypeChanged(int index);
    void onOpenHistoryAnalysis();
    void onHistoryFetchFinished(const QString& ticker,
                                const QString& interval,
                                const QString& source,
                                const QString& currency,
                                const QList<MarketPriceBar>& bars);
    void onHistoryFetchError(const QString& message);

private:
    void setupUi();
    QWidget* createReportsTab();
    void applyLivePrices(const QMap<QString, PortfolioQuote>& quotes);
    void updateSummary();
    void setRefreshing(bool active);
    void applyFilter();
    void rebuildFilterCombos();
    void rebuildHistoryTickerCombo();
    void queueMissingHistoryRanges();
    void startNextHistoryFetch();
    void loadHistoricalTable();
    void setHistoryBusy(bool active);
    QString historyStartTimestamp() const;
    QString historyEndTimestamp() const;

    // Kolumny tabeli
    enum Col { Ticker, Nazwa, Typ, Kategoria, Ilosc, CenaZakupu, KursLive, Wartosc, PnlPln, PnlPct, ColCount };

    QTableWidget*        m_table          = nullptr;
    QLabel*              m_summaryLabel   = nullptr;
    QLabel*              m_statusLabel    = nullptr;
    QPushButton*         m_addBtn         = nullptr;
    QPushButton*         m_deleteBtn      = nullptr;
    QPushButton*         m_refreshBtn     = nullptr;
    QComboBox*           m_typeFilterCombo = nullptr;
    QComboBox*           m_filterCombo    = nullptr;
    QSplitter*            m_splitter       = nullptr;
    QTabWidget*           m_bottomTabs    = nullptr;
    CompanyAnalysisPanel* m_analysisPanel = nullptr;
    PeersPanel*           m_peersPanel    = nullptr;
    QComboBox*            m_historyTickerCombo = nullptr;
    QDateEdit*            m_historyStartEdit = nullptr;
    QDateEdit*            m_historyEndEdit = nullptr;
    QComboBox*            m_historyChartTypeCombo = nullptr;
    QPushButton*          m_historyLoadBtn = nullptr;
    QPushButton*          m_historyExpandBtn = nullptr;
    QLabel*               m_historyStatusLabel = nullptr;
    HistoricalPriceChartWidget* m_historyChart = nullptr;
    QTableWidget*         m_historyTable = nullptr;
    QList<MarketPriceBar> m_historyRows;
    QList<QPair<QString, QString>> m_pendingHistoryRanges;
    QString               m_historyTicker;
    QString               m_historyInterval = "1d";
    QString               m_historySource = "yfinance";
    QString               m_historyCurrency;

    PortfolioFetcher m_fetcher;
    HistoricalPriceFetcher m_historyFetcher;
};
