#pragma once

#include <QObject>
#include <QProcess>
#include <QList>

struct AnalysisFiling {
    QString form;
    QString filingDate;
    QString description;
    QString url;
};

struct CompanyMetrics {
    QString peTtm;
    QString peForward;
    QString evEbitda;
    QString pBook;
    QString grossMargin;
    QString operatingMargin;
    QString netMargin;
    QString roe;
    QString roa;
    QString debtEquity;
    QString currentRatio;
    QString quickRatio;
    QString fcf;
    QString revenueGrowth;
    QString earningsGrowth;
};

struct CompanyAnalysis {
    QString ticker;
    QString name;
    QString sector;
    QString industry;
    QString description;
    QString currency;
    QString marketCap;
    QString enterpriseValue;
    CompanyMetrics metrics;
    QString analystRecommendation;
    double  analystTargetPrice = 0.0;
    int     analystCount       = 0;
    QList<AnalysisFiling> filings;
};

class CompanyAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit CompanyAnalyzer(QObject* parent = nullptr);

    void analyze(const QString& ticker);
    bool isRunning() const;

signals:
    void analysisReady(const CompanyAnalysis& analysis);
    void analysisError(const QString& message);

private slots:
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

private:
    QProcess* m_process = nullptr;
};
