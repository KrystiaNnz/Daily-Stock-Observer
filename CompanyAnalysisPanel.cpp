#include "CompanyAnalysisPanel.h"

#include <QLabel>
#include <QScrollArea>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>
#include <QFont>

// ── Pomocnicze ────────────────────────────────────────────────

static QLabel* sectionHeader(const QString& text, QWidget* parent)
{
    auto* lbl = new QLabel(text, parent);
    lbl->setStyleSheet(
        "font-size: 11px; font-weight: bold; color: #6b7280;"
        " letter-spacing: 0.8px; padding: 10px 0 4px 0;");
    return lbl;
}

static QFrame* hLine(QWidget* parent)
{
    auto* line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #e5e7eb;");
    return line;
}

// ── CompanyAnalysisPanel ──────────────────────────────────────

CompanyAnalysisPanel::CompanyAnalysisPanel(QWidget* parent)
    : QWidget(parent), m_analyzer(this)
{
    setupUi();
    connect(&m_analyzer, &CompanyAnalyzer::analysisReady,
            this, &CompanyAnalysisPanel::onAnalysisReady);
    connect(&m_analyzer, &CompanyAnalyzer::analysisError,
            this, &CompanyAnalysisPanel::onAnalysisError);
}

void CompanyAnalysisPanel::setupUi()
{
    setMinimumHeight(120);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Pasek nagłówka ─────────────────────────────────────
    auto* headerBar = new QWidget(this);
    headerBar->setStyleSheet("background: #f9fafb; border-bottom: 1px solid #e5e7eb;");
    auto* headerLayout = new QHBoxLayout(headerBar);
    headerLayout->setContentsMargins(16, 8, 16, 8);
    headerLayout->setSpacing(8);

    m_headerLabel = new QLabel("Analiza spółki", this);
    m_headerLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #111827;");

    m_subLabel = new QLabel("Wybierz spółkę z tabeli", this);
    m_subLabel->setStyleSheet("font-size: 12px; color: #9ca3af;");

    m_refreshBtn = new QPushButton("Odśwież", this);
    m_refreshBtn->setStyleSheet(
        "QPushButton { font-size: 12px; color: #3b82f6; background: transparent;"
        " border: 1px solid #3b82f6; border-radius: 4px; padding: 3px 10px; }"
        "QPushButton:hover { background: #eff6ff; }"
        "QPushButton:disabled { color: #9ca3af; border-color: #d1d5db; }");
    m_refreshBtn->setEnabled(false);

    headerLayout->addWidget(m_headerLabel);
    headerLayout->addWidget(m_subLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_refreshBtn);
    root->addWidget(headerBar);

    // ── Status label (loading / error) ─────────────────────
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #9ca3af; font-size: 13px; padding: 24px;");
    m_statusLabel->hide();
    root->addWidget(m_statusLabel);

    // ── Scroll area (content) ───────────────────────────────
    m_scroll = new QScrollArea(this);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->hide();

    m_content = new QWidget();
    m_content->setStyleSheet("background: white;");
    m_contentLayout = new QVBoxLayout(m_content);
    m_contentLayout->setContentsMargins(16, 8, 16, 16);
    m_contentLayout->setSpacing(0);
    m_contentLayout->addStretch();

    m_scroll->setWidget(m_content);
    root->addWidget(m_scroll, 1);

    connect(m_refreshBtn, &QPushButton::clicked, this, [this]() {
        if (!m_currentTicker.isEmpty())
            loadCompany(m_currentTicker);
    });
}

void CompanyAnalysisPanel::clear()
{
    m_currentTicker.clear();
    m_headerLabel->setText("Analiza spółki");
    m_subLabel->setText("Wybierz spółkę z tabeli");
    m_refreshBtn->setEnabled(false);
    m_statusLabel->hide();
    m_scroll->hide();
}

void CompanyAnalysisPanel::loadCompany(const QString& ticker)
{
    m_currentTicker = ticker;
    setLoading(ticker);
    m_analyzer.analyze(ticker);
}

void CompanyAnalysisPanel::setLoading(const QString& ticker)
{
    m_headerLabel->setText(ticker);
    m_subLabel->setText("Pobieranie danych...");
    m_refreshBtn->setEnabled(false);
    m_scroll->hide();
    m_statusLabel->setStyleSheet("color: #3b82f6; font-size: 13px; padding: 24px;");
    m_statusLabel->setText("⏳  Pobieranie analizy dla " + ticker + "...");
    m_statusLabel->show();
}

void CompanyAnalysisPanel::onAnalysisReady(const CompanyAnalysis& a)
{
    m_refreshBtn->setEnabled(true);
    m_statusLabel->hide();
    populate(a);
    m_scroll->show();
}

void CompanyAnalysisPanel::onAnalysisError(const QString& msg)
{
    m_subLabel->setText("Błąd");
    m_refreshBtn->setEnabled(true);
    m_statusLabel->setStyleSheet("color: #ef4444; font-size: 13px; padding: 24px;");
    m_statusLabel->setText("Błąd: " + msg);
    m_statusLabel->show();
    m_scroll->hide();
}

// ── Populate ──────────────────────────────────────────────────

void CompanyAnalysisPanel::addMetricRow(QGridLayout* grid, int row,
                                        const QString& label, const QString& value,
                                        bool highlight)
{
    auto* lbl = new QLabel(label + ":", m_content);
    lbl->setStyleSheet("font-size: 12px; color: #6b7280;");

    auto* val = new QLabel(value, m_content);
    QString style = "font-size: 12px; font-weight: 600; color: ";
    style += highlight ? "#111827;" : "#374151;";
    val->setStyleSheet(style);
    val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    grid->addWidget(lbl, row, 0);
    grid->addWidget(val, row, 1);
}

void CompanyAnalysisPanel::populate(const CompanyAnalysis& a)
{
    // Wyczyść poprzednią zawartość (wszystko oprócz końcowego stretch)
    while (m_contentLayout->count() > 1) {
        QLayoutItem* li = m_contentLayout->takeAt(0);
        if (QWidget* w = li->widget()) {
            w->setParent(nullptr);
            delete w;
        }
        delete li;
    }

    int idx = 0;

    // ── Nagłówek spółki ────────────────────────────────────
    auto* nameLabel = new QLabel(a.name, m_content);
    nameLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #111827; padding-top: 4px;");
    nameLabel->setWordWrap(true);
    m_contentLayout->insertWidget(idx++, nameLabel);

    auto* metaLabel = new QLabel(
        a.sector + "  ·  " + a.industry +
        "  |  Market Cap: " + a.marketCap +
        "  |  EV: " + a.enterpriseValue, m_content);
    metaLabel->setStyleSheet("font-size: 11px; color: #9ca3af; padding-bottom: 4px;");
    metaLabel->setWordWrap(true);
    m_contentLayout->insertWidget(idx++, metaLabel);

    m_headerLabel->setText(a.ticker);
    m_subLabel->setText(a.name);

    // ── Opis biznesowy ─────────────────────────────────────
    if (!a.description.isEmpty()) {
        m_contentLayout->insertWidget(idx++, hLine(m_content));
        auto* descLabel = new QLabel(a.description, m_content);
        descLabel->setStyleSheet("font-size: 11px; color: #6b7280; padding: 6px 0;");
        descLabel->setWordWrap(true);
        m_contentLayout->insertWidget(idx++, descLabel);
    }

    // ── Helper: tworzy sekcję metryczną (dwie kolumny) ────
    auto addSection = [&](const QString& title,
                          std::initializer_list<std::pair<QString,QString>> rows,
                          bool highlightAll = false) {
        m_contentLayout->insertWidget(idx++, hLine(m_content));
        m_contentLayout->insertWidget(idx++, sectionHeader(title, m_content));

        auto* container = new QWidget(m_content);
        auto* grid = new QGridLayout(container);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setVerticalSpacing(3);
        grid->setColumnStretch(0, 1);
        grid->setColumnStretch(1, 0);

        int r = 0;
        for (const auto& [lbl, val] : rows)
            addMetricRow(grid, r++, lbl, val, highlightAll);

        m_contentLayout->insertWidget(idx++, container);
    };

    const auto& m = a.metrics;

    // ── Wycena rynkowa ─────────────────────────────────────
    addSection("WYCENA RYNKOWA", {
        {"P/E (TTM)",    m.peTtm},
        {"Forward P/E",  m.peForward},
        {"EV/EBITDA",    m.evEbitda},
        {"P/BV",         m.pBook},
    });

    // ── Rentowność ─────────────────────────────────────────
    addSection("RENTOWNOŚĆ", {
        {"Marża brutto",    m.grossMargin},
        {"Marża operacyjna",m.operatingMargin},
        {"Marża netto",     m.netMargin},
        {"ROE",             m.roe},
        {"ROA",             m.roa},
    });

    // ── Dźwignia i płynność ────────────────────────────────
    addSection("DŹWIGNIA I PŁYNNOŚĆ", {
        {"Debt/Equity",   m.debtEquity},
        {"Current Ratio", m.currentRatio},
        {"Quick Ratio",   m.quickRatio},
        {"FCF",           m.fcf},
    });

    // ── Wzrost ────────────────────────────────────────────
    addSection("WZROST (YoY)", {
        {"Przychody", m.revenueGrowth},
        {"Zysk",      m.earningsGrowth},
    });

    // ── Rekomendacje analityków ────────────────────────────
    if (!a.analystRecommendation.isEmpty() && a.analystRecommendation != "N/A") {
        QString rec = a.analystRecommendation.toUpper();
        QString tp  = a.analystTargetPrice > 0
            ? QString::number(a.analystTargetPrice, 'f', 2) + " " + a.currency
            : "N/A";
        addSection("REKOMENDACJE ANALITYKÓW", {
            {"Konsensus",        rec + " (" + QString::number(a.analystCount) + " analityków)"},
            {"Cena docelowa",    tp},
        });
    }

    // ── Filingi EDGAR ──────────────────────────────────────
    if (!a.filings.isEmpty()) {
        m_contentLayout->insertWidget(idx++, hLine(m_content));
        m_contentLayout->insertWidget(idx++, sectionHeader("FILINGI SEC EDGAR", m_content));

        auto* filingsContainer = new QWidget(m_content);
        auto* fl = new QVBoxLayout(filingsContainer);
        fl->setContentsMargins(0, 0, 0, 0);
        fl->setSpacing(2);

        for (const AnalysisFiling& f : a.filings) {
            auto* row = new QWidget(filingsContainer);
            auto* rl  = new QHBoxLayout(row);
            rl->setContentsMargins(0, 2, 0, 2);
            rl->setSpacing(8);

            // Badge typu formularza
            auto* badge = new QLabel(f.form, row);
            badge->setFixedWidth(42);
            badge->setAlignment(Qt::AlignCenter);
            QString badgeColor = (f.form == "10-K") ? "#1d4ed8"
                               : (f.form == "10-Q") ? "#0369a1"
                               : (f.form == "8-K")  ? "#7c3aed"
                                                     : "#374151";
            badge->setStyleSheet(
                QString("font-size: 10px; font-weight: bold; color: white;"
                        " background: %1; border-radius: 3px; padding: 1px 2px;")
                    .arg(badgeColor));

            auto* dateLbl = new QLabel(f.filingDate, row);
            dateLbl->setStyleSheet("font-size: 11px; color: #9ca3af;");
            dateLbl->setFixedWidth(80);

            auto* descLbl = new QLabel(
                f.description.isEmpty() ? f.form + " Filing" : f.description, row);
            descLbl->setStyleSheet("font-size: 11px; color: #374151;");

            rl->addWidget(badge);
            rl->addWidget(dateLbl);
            rl->addWidget(descLbl, 1);

            if (!f.url.isEmpty()) {
                auto* openBtn = new QPushButton("Otwórz", row);
                openBtn->setStyleSheet(
                    "QPushButton { font-size: 10px; color: #3b82f6; background: transparent;"
                    " border: none; padding: 0; }"
                    "QPushButton:hover { text-decoration: underline; }");
                const QString url = f.url;
                connect(openBtn, &QPushButton::clicked, this, [url]() {
                    QDesktopServices::openUrl(QUrl(url));
                });
                rl->addWidget(openBtn);
            }

            fl->addWidget(row);
        }

        m_contentLayout->insertWidget(idx++, filingsContainer);
    } else {
        m_contentLayout->insertWidget(idx++, hLine(m_content));
        m_contentLayout->insertWidget(idx++, sectionHeader("FILINGI SEC EDGAR", m_content));
        auto* noEdgar = new QLabel("Brak danych EDGAR — spółka może być spoza US.", m_content);
        noEdgar->setStyleSheet("font-size: 11px; color: #9ca3af; padding: 4px 0;");
        m_contentLayout->insertWidget(idx++, noEdgar);
    }
}
