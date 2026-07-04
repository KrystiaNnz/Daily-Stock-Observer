#pragma once

#include <QObject>
#include <QProcess>
#include <QList>
#include "DatabaseManager.h"

class AlphaEngineFetcher : public QObject
{
    Q_OBJECT

public:
    explicit AlphaEngineFetcher(QObject* parent = nullptr);

    bool isRunning() const { return m_process != nullptr; }
    void computeFactorLinks(const QString& ticker,
                            int top = 25,
                            int windowDays = 756,
                            int lagPeriods = 0);

signals:
    void factorLinksFinished(const QString& ticker,
                             int candidateRows,
                             int savedRows,
                             const QList<AssetFactorCovariance>& rows);
    void factorLinksError(const QString& message);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError();

private:
    QString scriptPath() const;

    QProcess* m_process = nullptr;
    QString m_ticker;
};
