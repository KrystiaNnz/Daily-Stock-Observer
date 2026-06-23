#include "PeersFetcher.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

PeersFetcher::PeersFetcher(QObject* parent) : QObject(parent) {}

bool PeersFetcher::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void PeersFetcher::fetchPeers(const QString& ticker)
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(500);
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    connect(m_process, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PeersFetcher::onFinished);
    connect(m_process, &QProcess::errorOccurred, this, &PeersFetcher::onError);

    QString script = QCoreApplication::applicationDirPath() + "/python/peers.py";
    m_process->start("python", {script, ticker});
}

void PeersFetcher::onFinished(int exitCode, QProcess::ExitStatus)
{
    QByteArray raw = m_process->readAllStandardOutput();
    m_process->deleteLater();
    m_process = nullptr;

    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        emit fetchError("JSON parse error: " + pe.errorString());
        return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("error")) {
        emit fetchError(obj["error"].toString());
        return;
    }

    auto parseList = [](const QJsonArray& arr) {
        QList<PeerCompany> list;
        for (const QJsonValue& v : arr) {
            QJsonObject o = v.toObject();
            PeerCompany c;
            c.ticker   = o["ticker"].toString();
            c.name     = o["name"].toString();
            c.country  = o["country"].toString();
            c.sector   = o["sector"].toString();
            c.industry = o["industry"].toString();
            list.append(c);
        }
        return list;
    };

    PeersData data;
    data.ticker        = obj["ticker"].toString();
    data.sector        = obj["sector"].toString();
    data.industry      = obj["industry"].toString();
    data.peersIndustry = parseList(obj["peers_industry"].toArray());
    data.peersSector   = parseList(obj["peers_sector"].toArray());

    emit peersReady(data);
}

void PeersFetcher::onError(QProcess::ProcessError)
{
    QString msg = m_process ? m_process->errorString() : "Nieznany błąd procesu";
    if (m_process) { m_process->deleteLater(); m_process = nullptr; }
    emit fetchError(msg);
}
