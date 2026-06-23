#pragma once

#include <QWidget>
#include <QMap>
#include "PortfolioFetcher.h"

class QTableWidget;
class QLabel;
class QPushButton;
class QComboBox;
class QSplitter;
class QTabWidget;
class CompanyAnalysisPanel;
class PeersPanel;

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

private:
    void setupUi();
    void applyLivePrices(const QMap<QString, PortfolioQuote>& quotes);
    void updateSummary();
    void setRefreshing(bool active);
    void applyFilter();
    void rebuildFilterCombo();

    // Kolumny tabeli
    enum Col { Ticker, Nazwa, Kategoria, Ilosc, CenaZakupu, KursLive, Wartosc, PnlPln, PnlPct, ColCount };

    QTableWidget*        m_table          = nullptr;
    QLabel*              m_summaryLabel   = nullptr;
    QLabel*              m_statusLabel    = nullptr;
    QPushButton*         m_addBtn         = nullptr;
    QPushButton*         m_deleteBtn      = nullptr;
    QPushButton*         m_refreshBtn     = nullptr;
    QComboBox*           m_filterCombo    = nullptr;
    QSplitter*            m_splitter       = nullptr;
    QTabWidget*           m_bottomTabs    = nullptr;
    CompanyAnalysisPanel* m_analysisPanel = nullptr;
    PeersPanel*           m_peersPanel    = nullptr;

    PortfolioFetcher m_fetcher;
};
