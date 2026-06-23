#include "EdgarFetcher.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDir>

EdgarFetcher::EdgarFetcher(QObject* parent)
    : QObject(parent)
{}

bool EdgarFetcher::isFetching() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void EdgarFetcher::fetchFilings(const QString& ticker,
                                const QString& formType,
                                int            limit)
{
    if (isFetching()) return;

    if (!m_process) {
        m_process = new QProcess(this);
        connect(m_process, &QProcess::finished,
                this, &EdgarFetcher::onFinished);
        connect(m_process, &QProcess::errorOccurred,
                this, &EdgarFetcher::onProcessError);
    }

    QString scriptPath = QDir(QCoreApplication::applicationDirPath())
                             .filePath("python/edgar.py");

    m_process->start("python", {scriptPath, ticker, formType, QString::number(limit)});
}

void EdgarFetcher::onFinished(int exitCode, QProcess::ExitStatus)
{
    Q_UNUSED(exitCode)

    QByteArray out = m_process->readAllStandardOutput();
    QByteArray err = m_process->readAllStandardError();

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(out, &parseErr);

    if (doc.isNull()) {
        QString detail = err.isEmpty()
            ? parseErr.errorString()
            : QString::fromUtf8(err.left(200));
        emit fetchError("Błąd parsowania odpowiedzi edgar.py: " + detail);
        return;
    }

    if (doc.isObject()) {
        QString msg = doc.object().value("error").toString();
        emit fetchError(msg.isEmpty() ? QString::fromUtf8(err) : msg);
        return;
    }

    if (!doc.isArray()) {
        emit fetchError("Nieprawidłowy format odpowiedzi edgar.py");
        return;
    }

    QList<EdgarFiling> filings;
    for (const QJsonValue& v : doc.array()) {
        const QJsonObject o = v.toObject();
        EdgarFiling f;
        f.ticker      = o["ticker"].toString();
        f.company     = o["company"].toString();
        f.cik         = o["cik"].toString();
        f.form        = o["form"].toString();
        f.filingDate  = o["filingDate"].toString();
        f.reportDate  = o["reportDate"].toString();
        f.accession   = o["accession"].toString();
        f.document    = o["document"].toString();
        f.description = o["description"].toString();
        f.url         = o["url"].toString();
        filings.append(f);
    }

    emit filingsReady(filings);
}

void EdgarFetcher::onProcessError(QProcess::ProcessError)
{
    emit fetchError("Nie można uruchomić skryptu edgar.py — sprawdź instalację Pythona.");
}
