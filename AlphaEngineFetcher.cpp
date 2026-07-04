#include "AlphaEngineFetcher.h"
#include "ProfileManager.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

AlphaEngineFetcher::AlphaEngineFetcher(QObject* parent)
    : QObject(parent)
{}

QString AlphaEngineFetcher::scriptPath() const
{
    return QCoreApplication::applicationDirPath() + "/python/edge_engine.py";
}

void AlphaEngineFetcher::computeFactorLinks(const QString& ticker,
                                            int top,
                                            int windowDays,
                                            int lagPeriods)
{
    if (ticker.trimmed().isEmpty() || m_process)
        return;

    m_ticker = ticker.trimmed().toUpper();
    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished,
            this, &AlphaEngineFetcher::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &AlphaEngineFetcher::onProcessError);

    m_process->start("cmd.exe", {
        "/C", "python", scriptPath(), "compute-factor-links",
        ProfileManager::databasePath(), m_ticker,
        "--top", QString::number(top),
        "--save-limit", QString::number(qMax(top, 100)),
        "--window-days", QString::number(windowDays),
        "--lag-periods", QString::number(lagPeriods)
    });
}

void AlphaEngineFetcher::onProcessFinished(int /*exitCode*/, QProcess::ExitStatus /*status*/)
{
    const QByteArray out = m_process->readAllStandardOutput();
    const QByteArray err = m_process->readAllStandardError();
    m_process->deleteLater();
    m_process = nullptr;

    QJsonDocument doc = QJsonDocument::fromJson(out);
    if (!doc.isObject()) {
        const QString raw = QString::fromUtf8(out).left(240);
        emit factorLinksError(raw.isEmpty()
            ? "edge_engine.py nie zwrocil danych"
            : "Nieoczekiwana odpowiedz edge_engine.py: " + raw);
        return;
    }

    const QJsonObject obj = doc.object();
    const QString error = obj.value("error").toString();
    if (!error.isEmpty()) {
        emit factorLinksError(error);
        return;
    }

    QList<AssetFactorCovariance> rows;
    const QJsonArray arr = obj.value("rows").toArray();
    for (const QJsonValue& value : arr) {
        const QJsonObject item = value.toObject();
        AssetFactorCovariance row;
        row.ticker = obj.value("ticker").toString(m_ticker).toUpper();
        row.factorCode = item.value("factor_code").toString();
        row.factorType = item.value("factor_type").toString();
        row.windowDays = item.value("window_days").toInt();
        row.lagPeriods = item.value("lag_periods").toInt();
        row.covariance = item.value("covariance").toDouble();
        row.correlation = item.value("correlation").toDouble();
        row.observations = item.value("observations").toInt();
        row.stabilityScore = item.value("stability_score").toDouble();
        rows.append(row);
    }

    if (!err.isEmpty()) {
        // Python dependencies can write warnings to stderr; valid JSON stdout wins.
    }

    emit factorLinksFinished(obj.value("ticker").toString(m_ticker).toUpper(),
                             obj.value("candidate_rows").toInt(),
                             obj.value("saved_rows").toInt(),
                             rows);
}

void AlphaEngineFetcher::onProcessError()
{
    m_process->deleteLater();
    m_process = nullptr;
    emit factorLinksError("Nie mozna uruchomic edge_engine.py - sprawdz Python i lokalna baze danych.");
}
