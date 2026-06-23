#include "CompanyAnalyzer.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDir>

CompanyAnalyzer::CompanyAnalyzer(QObject* parent)
    : QObject(parent)
{}

bool CompanyAnalyzer::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void CompanyAnalyzer::analyze(const QString& ticker)
{
    if (isRunning()) {
        m_process->kill();
        m_process->waitForFinished(500);
    }

    if (!m_process) {
        m_process = new QProcess(this);
        connect(m_process, &QProcess::finished,
                this, &CompanyAnalyzer::onFinished);
        connect(m_process, &QProcess::errorOccurred,
                this, &CompanyAnalyzer::onProcessError);
    }

    QString script = QDir(QCoreApplication::applicationDirPath())
                         .filePath("python/analyze.py");

    m_process->start("python", {script, ticker});
}

void CompanyAnalyzer::onFinished(int, QProcess::ExitStatus)
{
    QByteArray out = m_process->readAllStandardOutput();
    QByteArray err = m_process->readAllStandardError();

    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(out, &pe);

    if (doc.isNull()) {
        emit analysisError("Błąd parsowania analyze.py: " +
                           (err.isEmpty() ? pe.errorString() : QString::fromUtf8(err.left(200))));
        return;
    }

    if (doc.isObject() && doc.object().contains("error")) {
        emit analysisError(doc.object()["error"].toString());
        return;
    }

    if (!doc.isObject()) {
        emit analysisError("Nieprawidłowy format odpowiedzi analyze.py");
        return;
    }

    const QJsonObject o = doc.object();
    const QJsonObject m = o["metrics"].toObject();
    const QJsonObject a = o["analyst"].toObject();

    CompanyMetrics metrics;
    metrics.peTtm           = m["pe_ttm"].toString();
    metrics.peForward       = m["pe_forward"].toString();
    metrics.evEbitda        = m["ev_ebitda"].toString();
    metrics.pBook           = m["p_book"].toString();
    metrics.grossMargin     = m["gross_margin"].toString();
    metrics.operatingMargin = m["operating_margin"].toString();
    metrics.netMargin       = m["net_margin"].toString();
    metrics.roe             = m["roe"].toString();
    metrics.roa             = m["roa"].toString();
    metrics.debtEquity      = m["debt_equity"].toString();
    metrics.currentRatio    = m["current_ratio"].toString();
    metrics.quickRatio      = m["quick_ratio"].toString();
    metrics.fcf             = m["fcf"].toString();
    metrics.revenueGrowth   = m["revenue_growth"].toString();
    metrics.earningsGrowth  = m["earnings_growth"].toString();

    CompanyAnalysis analysis;
    analysis.ticker          = o["ticker"].toString();
    analysis.name            = o["name"].toString();
    analysis.sector          = o["sector"].toString();
    analysis.industry        = o["industry"].toString();
    analysis.description     = o["description"].toString();
    analysis.currency        = o["currency"].toString();
    analysis.marketCap       = o["market_cap"].toString();
    analysis.enterpriseValue = o["enterprise_value"].toString();
    analysis.metrics         = metrics;

    analysis.analystRecommendation = a["recommendation"].toString();
    analysis.analystTargetPrice    = a["target_price"].toDouble();
    analysis.analystCount          = a["num_analysts"].toInt();

    for (const QJsonValue& v : o["edgar_filings"].toArray()) {
        const QJsonObject f = v.toObject();
        analysis.filings.append({
            f["form"].toString(),
            f["filingDate"].toString(),
            f["description"].toString(),
            f["url"].toString(),
        });
    }

    emit analysisReady(analysis);
}

void CompanyAnalyzer::onProcessError(QProcess::ProcessError)
{
    emit analysisError("Nie można uruchomić analyze.py — sprawdź instalację Pythona.");
}
