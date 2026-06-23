#pragma once

#include <QObject>
#include <QProcess>

struct EdgarFiling {
    QString ticker;
    QString company;
    QString cik;
    QString form;
    QString filingDate;
    QString reportDate;
    QString accession;
    QString document;
    QString description;
    QString url;
};

class EdgarFetcher : public QObject
{
    Q_OBJECT

public:
    explicit EdgarFetcher(QObject* parent = nullptr);

    void fetchFilings(const QString& ticker,
                      const QString& formType = "10-K",
                      int            limit    = 10);

    bool isFetching() const;

signals:
    void filingsReady(const QList<EdgarFiling>& filings);
    void fetchError(const QString& message);

private slots:
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

private:
    QProcess* m_process = nullptr;
};
