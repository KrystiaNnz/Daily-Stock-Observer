#pragma once

#include <QLineEdit>
#include <QProcess>

class QCompleter;
class QStandardItemModel;
class QTimer;
class QNetworkAccessManager;
class QNetworkReply;

// Role w QStandardItem
enum TickerDataRole {
    TickerSymbolRole  = Qt::UserRole,
    CompanyNameRole   = Qt::UserRole + 1,
    ExchangeRole      = Qt::UserRole + 2,
};

// QLineEdit z podpowiadaniem tickerów (search.py + QCompleter)
// Po wyborze emituje tickerSelected i logoReady.
class TickerSearchEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit TickerSearchEdit(QWidget* parent = nullptr);

    QString ticker() const      { return m_ticker; }
    QString companyName() const { return m_name; }

signals:
    void tickerSelected(const QString& ticker, const QString& name);
    void logoReady(const QPixmap& logo);

private slots:
    void onTextEdited(const QString& text);
    void onDebounceTimeout();
    void onSearchFinished(int exitCode, QProcess::ExitStatus status);
    void onCompleterActivated(const QModelIndex& index);
    void onLogoReplyFinished(QNetworkReply* reply);

private:
    void startSearch(const QString& query);
    void fetchLogo(const QString& companyName);
    void populateModel(const QByteArray& json);

    QTimer*               m_debounce   = nullptr;
    QProcess*             m_process    = nullptr;
    QStandardItemModel*   m_model      = nullptr;
    QCompleter*           m_completer  = nullptr;
    QNetworkAccessManager* m_network   = nullptr;

    QString m_ticker;
    QString m_name;
};
