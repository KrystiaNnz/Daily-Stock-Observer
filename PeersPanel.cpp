#include "PeersPanel.h"

#include "WorldHeatmapWidget.h"

#include <QLabel>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>

// ── Country flag emoji helpers ────────────────────────────────────────────────

static QString flagEmoji(const QString& code)
{
    // Regional indicator symbols: A = U+1F1E6, offset by code point
    if (code.length() != 2) return "";
    uint32_t a = 0x1F1E6 + (code[0].unicode() - 'A');
    uint32_t b = 0x1F1E6 + (code[1].unicode() - 'A');
    return QString::fromUcs4(&a, 1) + QString::fromUcs4(&b, 1);
}

static QString countryName(const QString& code)
{
    static const QMap<QString,QString> names = {
        {"US","USA"},{"CA","Kanada"},{"GB","Wielka Brytania"},{"DE","Niemcy"},
        {"FR","Francja"},{"JP","Japonia"},{"CN","Chiny"},{"KR","Korea Pd."},
        {"TW","Tajwan"},{"PL","Polska"},{"NL","Holandia"},{"SE","Szwecja"},
        {"CH","Szwajcaria"},{"DK","Dania"},{"NO","Norwegia"},{"IT","Włochy"},
        {"ES","Hiszpania"},{"PT","Portugalia"},{"BE","Belgia"},{"AT","Austria"},
        {"FI","Finlandia"},{"HU","Węgry"},{"RO","Rumunia"},{"CZ","Czechy"},
        {"IL","Izrael"},{"IN","Indie"},{"AU","Australia"},{"BR","Brazylia"},
        {"AR","Argentyna"},{"MX","Meksyk"},{"SG","Singapur"},{"HK","Hongkong"},
        {"RU","Rosja"},{"ZA","Płd. Afryka"},{"SA","Arabia Saudyjska"},
        {"AE","ZEA"},{"TR","Turcja"},{"ID","Indonezja"},{"TH","Tajlandia"},
        {"MY","Malezja"},{"VN","Wietnam"},{"NG","Nigeria"},{"EG","Egipt"},
    };
    return names.value(code, code);
}

// ── PeersPanel ────────────────────────────────────────────────────────────────

PeersPanel::PeersPanel(QWidget* parent)
    : QWidget(parent), m_fetcher(this)
{
    setupUi();
    connect(&m_fetcher, &PeersFetcher::peersReady,  this, &PeersPanel::onPeersReady);
    connect(&m_fetcher, &PeersFetcher::fetchError,  this, &PeersPanel::onFetchError);
}

void PeersPanel::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    // ── Header bar ─────────────────────────────────────────────
    auto* hbar = new QWidget(this);
    hbar->setStyleSheet("background:#f9fafb;border-bottom:1px solid #e5e7eb;");
    auto* hl = new QHBoxLayout(hbar);
    hl->setContentsMargins(16,8,16,8);
    hl->setSpacing(8);

    m_titleLabel = new QLabel("Mapa Rynku", this);
    m_titleLabel->setStyleSheet("font-size:14px;font-weight:bold;color:#111827;");

    m_subLabel = new QLabel("Wybierz spółkę w panelu Portfel", this);
    m_subLabel->setStyleSheet("font-size:12px;color:#9ca3af;");

    hl->addWidget(m_titleLabel);
    hl->addWidget(m_subLabel);
    hl->addStretch();
    root->addWidget(hbar);

    // ── Status label ────────────────────────────────────────────
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color:#9ca3af;font-size:13px;padding:24px;");
    m_statusLabel->setText("Wybierz spółkę z panelu Portfel, aby zobaczyć mapę peerów.");
    root->addWidget(m_statusLabel, 1);

    // ── Main splitter (map | table) ─────────────────────────────
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(4);

    m_heatmap = new WorldHeatmapWidget(m_splitter);
    m_splitter->addWidget(m_heatmap);

    m_table = new QTableWidget(0, 4, m_splitter);
    m_table->setHorizontalHeaderLabels({"Kraj", "Ticker", "Nazwa", "Typ"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->hide();
    m_table->setStyleSheet("font-size:12px;");
    m_splitter->addWidget(m_table);

    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 2);
    m_splitter->setSizes({450, 300});
    m_splitter->hide();
    root->addWidget(m_splitter, 1);

    connect(m_heatmap, &WorldHeatmapWidget::countryClicked,
            this, &PeersPanel::onCountryClicked);
}

void PeersPanel::clear()
{
    m_currentTicker.clear();
    m_titleLabel->setText("Mapa Rynku");
    m_subLabel->setText("Wybierz spółkę w panelu Portfel");
    m_statusLabel->setText("Wybierz spółkę z panelu Portfel, aby zobaczyć mapę peerów.");
    m_statusLabel->setStyleSheet("color:#9ca3af;font-size:13px;padding:24px;");
    m_statusLabel->show();
    m_splitter->hide();
    m_heatmap->clear();
    m_table->setRowCount(0);
}

void PeersPanel::loadPeers(const QString& ticker)
{
    m_currentTicker = ticker;
    setLoading(ticker);
    m_fetcher.fetchPeers(ticker);
}

void PeersPanel::setLoading(const QString& ticker)
{
    m_titleLabel->setText(ticker);
    m_subLabel->setText("Pobieranie danych...");
    m_statusLabel->setStyleSheet("color:#3b82f6;font-size:13px;padding:24px;");
    m_statusLabel->setText("⏳  Wyszukiwanie peerów dla " + ticker + "...");
    m_statusLabel->show();
    m_splitter->hide();
}

void PeersPanel::onPeersReady(const PeersData& data)
{
    m_statusLabel->hide();
    populate(data);
    m_splitter->show();
}

void PeersPanel::onFetchError(const QString& msg)
{
    m_subLabel->setText("Błąd");
    m_statusLabel->setStyleSheet("color:#ef4444;font-size:13px;padding:24px;");
    m_statusLabel->setText("Błąd: " + msg);
    m_statusLabel->show();
    m_splitter->hide();
}

void PeersPanel::populate(const PeersData& data)
{
    int industryCount = data.peersIndustry.size();
    int sectorCount   = data.peersSector.size();
    int total         = industryCount + sectorCount;

    m_titleLabel->setText(data.ticker);
    m_subLabel->setText(
        data.industry + "  |  " +
        QString::number(industryCount) + " branżowych, " +
        QString::number(sectorCount)   + " sektorowych  (" +
        QString::number(total)         + " łącznie)");

    m_heatmap->setPeers(data);
    populateTable(data);
}

void PeersPanel::populateTable(const PeersData& data)
{
    m_table->setRowCount(0);

    auto addRow = [&](const PeerCompany& c, bool isIndustry) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        QString flag = flagEmoji(c.country);
        auto* countryItem = new QTableWidgetItem(flag + " " + countryName(c.country));
        auto* tickerItem  = new QTableWidgetItem(c.ticker);
        auto* nameItem    = new QTableWidgetItem(c.name);
        auto* typeItem    = new QTableWidgetItem(isIndustry ? "branża" : "sektor");

        tickerItem->setFont([]{ QFont f; f.setBold(true); return f; }());

        QColor typeColor = isIndustry ? QColor(59,130,246) : QColor(139,92,246);
        typeItem->setForeground(typeColor);

        m_table->setItem(row, 0, countryItem);
        m_table->setItem(row, 1, tickerItem);
        m_table->setItem(row, 2, nameItem);
        m_table->setItem(row, 3, typeItem);
    };

    for (const auto& p : data.peersIndustry) addRow(p, true);
    for (const auto& p : data.peersSector)   addRow(p, false);
}

void PeersPanel::onCountryClicked(const QString& code)
{
    // Scroll table to first row with this country
    for (int row = 0; row < m_table->rowCount(); row++) {
        QTableWidgetItem* it = m_table->item(row, 0);
        if (it && it->text().contains(countryName(code))) {
            m_table->scrollToItem(it);
            m_table->selectRow(row);
            break;
        }
    }
}
