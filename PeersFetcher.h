#pragma once

#include <QObject>
#include <QProcess>
#include <QList>

struct PeerCompany {
    QString ticker;
    QString name;
    QString country;   // ISO-2
    QString sector;
    QString industry;
};

struct PeersData {
    QString ticker;
    QString sector;
    QString industry;
    QList<PeerCompany> peersIndustry;   // same industry
    QList<PeerCompany> peersSector;     // same sector, different industry
};

class PeersFetcher : public QObject
{
    Q_OBJECT

public:
    explicit PeersFetcher(QObject* parent = nullptr);

    void fetchPeers(const QString& ticker);
    bool isRunning() const;

signals:
    void peersReady(const PeersData& data);
    void fetchError(const QString& message);

private slots:
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onError(QProcess::ProcessError err);

private:
    QProcess* m_process = nullptr;
};
