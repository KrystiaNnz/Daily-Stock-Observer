#include "TickerSearchEdit.h"

#include <QCompleter>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QTimer>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>
#include <QUrl>
#include <QAbstractItemView>
#include <QDebug>

// ══════════════════════════════════════════════════════
// TickerDelegate — renderuje wiersz z awatarem literowym
// ══════════════════════════════════════════════════════

class TickerDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        return QSize(0, 52);
    }

    void paint(QPainter* p, const QStyleOptionViewItem& opt,
               const QModelIndex& idx) const override
    {
        p->save();

        // Tło zaznaczenia / hover
        if (opt.state & QStyle::State_Selected)
            p->fillRect(opt.rect, opt.palette.highlight());
        else if (opt.state & QStyle::State_MouseOver)
            p->fillRect(opt.rect, QColor(245, 247, 250));
        else
            p->fillRect(opt.rect, Qt::white);

        const int pad   = 10;
        const int aSize = 32;
        const int aTop  = opt.rect.top() + (opt.rect.height() - aSize) / 2;

        QString ticker   = idx.data(TickerSymbolRole).toString();
        QString name     = idx.data(CompanyNameRole).toString();
        QString exchange = idx.data(ExchangeRole).toString();

        // ── Awatar: kolorowe kółko z pierwszą literą ─────
        QRect avatarRect(opt.rect.left() + pad, aTop, aSize, aSize);

        QColor avatarColor = tickerColor(ticker);
        p->setRenderHint(QPainter::Antialiasing);
        p->setBrush(avatarColor);
        p->setPen(Qt::NoPen);
        p->drawEllipse(avatarRect);

        QString letter = ticker.isEmpty() ? "?" : QString(ticker[0]).toUpper();
        p->setPen(Qt::white);
        QFont lf = p->font();
        lf.setPixelSize(14);
        lf.setBold(true);
        p->setFont(lf);
        p->drawText(avatarRect, Qt::AlignCenter, letter);

        // ── Tekst po prawej stronie awatara ──────────────
        int textX  = avatarRect.right() + 10;
        int textW  = opt.rect.right() - textX - pad;
        int midY   = opt.rect.top() + opt.rect.height() / 2;

        QColor textColor = (opt.state & QStyle::State_Selected)
            ? opt.palette.highlightedText().color()
            : QColor(0x1a, 0x25, 0x35);

        QColor subColor = (opt.state & QStyle::State_Selected)
            ? opt.palette.highlightedText().color().darker(130)
            : QColor(0x71, 0x80, 0x96);

        // Ticker (bold)
        QFont tf = p->font();
        tf.setPixelSize(13);
        tf.setBold(true);
        p->setFont(tf);
        p->setPen(textColor);
        QRect tickerRect(textX, midY - 16, textW, 18);
        p->drawText(tickerRect, Qt::AlignLeft | Qt::AlignVCenter, ticker);

        // Exchange (mała odznaka)
        if (!exchange.isEmpty()) {
            QFont ef = tf;
            ef.setPixelSize(10);
            ef.setBold(false);
            p->setFont(ef);
            p->setPen(QColor(0x94, 0xa3, 0xb8));
            QFontMetrics fm(ef);
            int exW = fm.horizontalAdvance(exchange) + 10;
            QRect exRect(opt.rect.right() - pad - exW, midY - 16, exW, 16);
            p->drawText(exRect, Qt::AlignRight | Qt::AlignVCenter, exchange);
        }

        // Nazwa firmy (szary)
        QFont nf = tf;
        nf.setPixelSize(11);
        nf.setBold(false);
        p->setFont(nf);
        p->setPen(subColor);
        QRect nameRect(textX, midY + 2, textW, 16);
        p->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, name);

        p->restore();
    }

private:
    static QColor tickerColor(const QString& ticker)
    {
        // Deterministyczny kolor na podstawie tickera
        static const QColor palette[] = {
            QColor(0x3b, 0x82, 0xf6),  // blue
            QColor(0x10, 0xb9, 0x81),  // green
            QColor(0x8b, 0x5c, 0xf6),  // violet
            QColor(0xf5, 0x9e, 0x0b),  // amber
            QColor(0xef, 0x44, 0x44),  // red
            QColor(0x06, 0xb6, 0xd4),  // cyan
            QColor(0xec, 0x48, 0x99),  // pink
            QColor(0x84, 0xcc, 0x16),  // lime
        };
        uint hash = qHash(ticker);
        return palette[hash % 8];
    }
};

// ══════════════════════════════════════════════════════
// TickerSearchEdit
// ══════════════════════════════════════════════════════

TickerSearchEdit::TickerSearchEdit(QWidget* parent)
    : QLineEdit(parent)
{
    setPlaceholderText("Wpisz nazwę lub ticker, np. cdprojekt, AAPL…");

    // Model + completer
    m_model = new QStandardItemModel(this);

    m_completer = new QCompleter(m_model, this);
    m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setMaxVisibleItems(8);
    m_completer->popup()->setItemDelegate(new TickerDelegate(m_completer->popup()));
    m_completer->popup()->setMinimumWidth(420);
    m_completer->popup()->setStyleSheet(
        "QListView { border: 1px solid #e2e8f0; border-radius: 8px;"
        " background: white; outline: none; }"
        "QListView::item { border: none; }"
        "QScrollBar:vertical { width: 6px; }"
        "QScrollBar::handle:vertical { background: #cbd5e1; border-radius: 3px; }"
    );
    setCompleter(m_completer);

    // Debounce timer
    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(380);

    // Network (loga)
    m_network = new QNetworkAccessManager(this);

    connect(this,        &QLineEdit::textEdited,
            this,        &TickerSearchEdit::onTextEdited);
    connect(m_debounce,  &QTimer::timeout,
            this,        &TickerSearchEdit::onDebounceTimeout);
    connect(m_completer, QOverload<const QModelIndex&>::of(&QCompleter::activated),
            this,        &TickerSearchEdit::onCompleterActivated);
    connect(m_network,   &QNetworkAccessManager::finished,
            this,        &TickerSearchEdit::onLogoReplyFinished);
}

void TickerSearchEdit::onTextEdited(const QString& text)
{
    m_ticker.clear();
    m_name.clear();

    if (text.trimmed().length() < 2) {
        m_debounce->stop();
        m_model->clear();
        return;
    }
    m_debounce->start();   // restart przy każdej zmianie
}

void TickerSearchEdit::onDebounceTimeout()
{
    startSearch(text().trimmed());
}

void TickerSearchEdit::startSearch(const QString& query)
{
    if (query.isEmpty()) return;
    if (m_process) return;   // poprzednie zapytanie jeszcze trwa

    QString script = QCoreApplication::applicationDirPath() + "/python/search.py";

    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished,
            this, &TickerSearchEdit::onSearchFinished);
    connect(m_process, &QProcess::errorOccurred, this, [this]() {
        m_process->deleteLater();
        m_process = nullptr;
    });

    m_process->start("cmd.exe", {"/C", "python", script, query});
}

void TickerSearchEdit::onSearchFinished(int /*exitCode*/, QProcess::ExitStatus /*status*/)
{
    QByteArray out = m_process->readAllStandardOutput();
    m_process->deleteLater();
    m_process = nullptr;

    populateModel(out);

    // Jeśli użytkownik wpisał kolejną frazę podczas wyszukiwania — uruchom ponownie
    QString current = text().trimmed();
    if (current.length() >= 2)
        m_completer->complete();
}

void TickerSearchEdit::populateModel(const QByteArray& json)
{
    m_model->clear();

    QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isArray()) return;

    for (const QJsonValue& val : doc.array()) {
        QJsonObject obj = val.toObject();
        if (obj.contains("error")) continue;

        QString ticker   = obj["ticker"].toString();
        QString name     = obj["name"].toString();
        QString exchange = obj["exchange"].toString();
        if (ticker.isEmpty()) continue;

        // DisplayRole = sam ticker (to co QCompleter wstawi do pola)
        // Wizualna reprezentacja pochodzi z TickerDelegate (data roles)
        auto* item = new QStandardItem(ticker);
        item->setData(ticker,   TickerSymbolRole);
        item->setData(name,     CompanyNameRole);
        item->setData(exchange, ExchangeRole);
        m_model->appendRow(item);
    }
}

void TickerSearchEdit::onCompleterActivated(const QModelIndex& index)
{
    m_ticker = index.data(TickerSymbolRole).toString();
    m_name   = index.data(CompanyNameRole).toString();

    // Pokaż tylko ticker w polu (nie pełny tekst z modelu)
    setText(m_ticker);
    emit tickerSelected(m_ticker, m_name);

    // Pobierz logo asynchronicznie
    if (!m_name.isEmpty())
        fetchLogo(m_name);
}

void TickerSearchEdit::fetchLogo(const QString& companyName)
{
    // Clearbit autocomplete → zwraca {name, domain, logo}
    QString query = QString::fromUtf8(
        QUrl::toPercentEncoding(companyName.split(' ').first()));
    QString url = "https://autocomplete.clearbit.com/v1/companies/suggest?query=" + query;

    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    m_network->get(req);
}

void TickerSearchEdit::onLogoReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) return;

    QByteArray data = reply->readAll();
    QString urlStr  = reply->url().toString();

    // ── Krok 1: clearbit autocomplete zwrócił listę firm ──
    if (urlStr.contains("autocomplete.clearbit")) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isArray() || doc.array().isEmpty()) return;

        QString logoUrl = doc.array().first().toObject()["logo"].toString();
        if (logoUrl.isEmpty()) return;

        QNetworkRequest req(logoUrl);
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
        m_network->get(req);
        return;
    }

    // ── Krok 2: pobranie obrazka logo ─────────────────────
    QPixmap pix;
    if (pix.loadFromData(data)) {
        emit logoReady(pix.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}
