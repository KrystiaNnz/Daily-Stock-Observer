#pragma once

#include <QDialog>
#include "DatabaseManager.h"
#include "HistoricalPriceChartWidget.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QPushButton;
class QTableWidget;

class HistoricalPriceAnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HistoricalPriceAnalysisDialog(const QList<MarketPriceBar>& bars,
                                           const QString& ticker,
                                           const QString& interval,
                                           const QString& currency,
                                           HistoricalPriceChartWidget::ChartType chartType,
                                           QWidget* parent = nullptr);

private slots:
    void applyFilters();
    void onChartTypeChanged(int index);
    void copyFilteredRows();
    void exportFilteredRows();

private:
    void setupUi();
    QList<MarketPriceBar> filteredBars() const;
    void updateStats(const QList<MarketPriceBar>& rows);
    void updateTable(const QList<MarketPriceBar>& rows);
    QString rowsToCsv(const QList<MarketPriceBar>& rows) const;

    QList<MarketPriceBar> m_allBars;
    QString m_ticker;
    QString m_interval;
    QString m_currency;

    HistoricalPriceChartWidget* m_chart = nullptr;
    QComboBox* m_chartTypeCombo = nullptr;
    QDateEdit* m_startEdit = nullptr;
    QDateEdit* m_endEdit = nullptr;
    QLabel* m_statsLabel = nullptr;
    QTableWidget* m_table = nullptr;
    QPushButton* m_copyBtn = nullptr;
    QPushButton* m_exportBtn = nullptr;
};
