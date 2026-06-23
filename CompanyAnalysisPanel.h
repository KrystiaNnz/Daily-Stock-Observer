#pragma once

#include <QWidget>
#include "CompanyAnalyzer.h"

class QLabel;
class QScrollArea;
class QGridLayout;
class QVBoxLayout;
class QPushButton;
class QTableWidget;

class CompanyAnalysisPanel : public QWidget
{
    Q_OBJECT

public:
    explicit CompanyAnalysisPanel(QWidget* parent = nullptr);

    void loadCompany(const QString& ticker);
    void clear();

private slots:
    void onAnalysisReady(const CompanyAnalysis& analysis);
    void onAnalysisError(const QString& message);

private:
    void setupUi();
    void setLoading(const QString& ticker);
    void populate(const CompanyAnalysis& a);
    void addMetricRow(QGridLayout* grid, int row,
                      const QString& label, const QString& value,
                      bool highlight = false);

    // Header
    QLabel*      m_headerLabel   = nullptr;
    QLabel*      m_subLabel      = nullptr;
    QPushButton* m_refreshBtn    = nullptr;

    // Content area
    QLabel*      m_statusLabel   = nullptr;
    QScrollArea* m_scroll        = nullptr;
    QWidget*     m_content       = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;

    CompanyAnalyzer m_analyzer;
    QString         m_currentTicker;
};
