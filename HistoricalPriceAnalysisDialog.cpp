#include "HistoricalPriceAnalysisDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <limits>

namespace {
QTableWidgetItem* readonlyItem(const QString& text)
{
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

QTableWidgetItem* readonlyNumber(double value, int decimals = 4)
{
    auto* item = readonlyItem(QString::number(value, 'f', decimals));
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}
}

HistoricalPriceAnalysisDialog::HistoricalPriceAnalysisDialog(
    const QList<MarketPriceBar>& bars,
    const QString& ticker,
    const QString& interval,
    const QString& currency,
    HistoricalPriceChartWidget::ChartType chartType,
    QWidget* parent)
    : QDialog(parent),
      m_allBars(bars),
      m_ticker(ticker),
      m_interval(interval),
      m_currency(currency)
{
    setWindowTitle(QString("Analiza wykresu - %1").arg(m_ticker));
    setModal(true);
    setupUi();
    m_chartTypeCombo->setCurrentIndex(m_chartTypeCombo->findData(int(chartType)));
    applyFilters();

    QTimer::singleShot(0, this, [this]() {
        if (parentWidget() && parentWidget()->window()) {
            QRect target = parentWidget()->window()->geometry().adjusted(8, 8, -8, -8);
            setGeometry(target);
        } else if (QScreen* screen = QApplication::primaryScreen()) {
            setGeometry(screen->availableGeometry().adjusted(24, 24, -24, -24));
        }
    });
}

void HistoricalPriceAnalysisDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    auto* controls = new QHBoxLayout();
    controls->setSpacing(8);

    auto* title = new QLabel(QString("%1 - analiza historycznych cen").arg(m_ticker), this);
    title->setStyleSheet("font-size: 16px; font-weight: bold;");
    controls->addWidget(title);
    controls->addStretch();

    controls->addWidget(new QLabel("Od:", this));
    m_startEdit = new QDateEdit(this);
    m_startEdit->setCalendarPopup(true);
    m_startEdit->setDisplayFormat("yyyy-MM-dd");
    controls->addWidget(m_startEdit);

    controls->addWidget(new QLabel("Do:", this));
    m_endEdit = new QDateEdit(this);
    m_endEdit->setCalendarPopup(true);
    m_endEdit->setDisplayFormat("yyyy-MM-dd");
    controls->addWidget(m_endEdit);

    controls->addWidget(new QLabel("Wykres:", this));
    m_chartTypeCombo = new QComboBox(this);
    m_chartTypeCombo->addItem("Linia", int(HistoricalPriceChartWidget::ChartType::LineClose));
    m_chartTypeCombo->addItem("Slupki", int(HistoricalPriceChartWidget::ChartType::BarClose));
    m_chartTypeCombo->addItem("Swiece", int(HistoricalPriceChartWidget::ChartType::Candlestick));
    controls->addWidget(m_chartTypeCombo);

    auto* applyBtn = new QPushButton("Filtruj", this);
    controls->addWidget(applyBtn);

    m_copyBtn = new QPushButton("Kopiuj CSV", this);
    m_exportBtn = new QPushButton("Eksport CSV", this);
    auto* closeBtn = new QPushButton("Zamknij", this);
    controls->addWidget(m_copyBtn);
    controls->addWidget(m_exportBtn);
    controls->addWidget(closeBtn);
    root->addLayout(controls);

    m_statsLabel = new QLabel(this);
    m_statsLabel->setStyleSheet("font-size: 12px; color: palette(mid);");
    root->addWidget(m_statsLabel);

    m_chart = new HistoricalPriceChartWidget(this);
    m_chart->setMinimumHeight(360);

    m_table = new QTableWidget(0, 8, this);
    m_table->setHorizontalHeaderLabels({
        "Data", "Open", "High", "Low", "Close", "Adj Close", "Volume", "Zrodlo"
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);

    auto* splitter = new QSplitter(Qt::Vertical, this);
    splitter->setHandleWidth(5);
    splitter->addWidget(m_chart);
    splitter->addWidget(m_table);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({520, 300});
    root->addWidget(splitter, 1);

    if (!m_allBars.isEmpty()) {
        const QDate first = QDate::fromString(m_allBars.first().tradingDate, "yyyy-MM-dd");
        const QDate last = QDate::fromString(m_allBars.last().tradingDate, "yyyy-MM-dd");
        m_startEdit->setDate(first.isValid() ? first : QDate::currentDate().addYears(-1));
        m_endEdit->setDate(last.isValid() ? last : QDate::currentDate());
    } else {
        m_startEdit->setDate(QDate::currentDate().addYears(-1));
        m_endEdit->setDate(QDate::currentDate());
    }

    connect(applyBtn, &QPushButton::clicked, this, &HistoricalPriceAnalysisDialog::applyFilters);
    connect(m_startEdit, &QDateEdit::dateChanged, this, &HistoricalPriceAnalysisDialog::applyFilters);
    connect(m_endEdit, &QDateEdit::dateChanged, this, &HistoricalPriceAnalysisDialog::applyFilters);
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HistoricalPriceAnalysisDialog::onChartTypeChanged);
    connect(m_copyBtn, &QPushButton::clicked, this, &HistoricalPriceAnalysisDialog::copyFilteredRows);
    connect(m_exportBtn, &QPushButton::clicked, this, &HistoricalPriceAnalysisDialog::exportFilteredRows);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

QList<MarketPriceBar> HistoricalPriceAnalysisDialog::filteredBars() const
{
    QList<MarketPriceBar> rows;
    const QString start = m_startEdit->date().toString("yyyy-MM-dd");
    const QString end = m_endEdit->date().toString("yyyy-MM-dd");

    for (const MarketPriceBar& bar : m_allBars) {
        if (bar.tradingDate >= start && bar.tradingDate <= end)
            rows.append(bar);
    }
    return rows;
}

void HistoricalPriceAnalysisDialog::applyFilters()
{
    const QList<MarketPriceBar> rows = filteredBars();
    const auto type = static_cast<HistoricalPriceChartWidget::ChartType>(
        m_chartTypeCombo->currentData().toInt());
    m_chart->setChartType(type);
    m_chart->setBars(rows, m_ticker, m_interval, m_currency);
    updateStats(rows);
    updateTable(rows);
}

void HistoricalPriceAnalysisDialog::onChartTypeChanged(int /*index*/)
{
    applyFilters();
}

void HistoricalPriceAnalysisDialog::updateStats(const QList<MarketPriceBar>& rows)
{
    if (rows.isEmpty()) {
        m_statsLabel->setText("Brak danych w wybranym zakresie.");
        return;
    }

    double minClose = std::numeric_limits<double>::max();
    double maxClose = std::numeric_limits<double>::lowest();
    double sumClose = 0.0;
    double sumVolume = 0.0;
    for (const MarketPriceBar& bar : rows) {
        minClose = std::min(minClose, bar.close);
        maxClose = std::max(maxClose, bar.close);
        sumClose += bar.close;
        sumVolume += bar.volume;
    }

    const double firstClose = rows.first().close;
    const double lastClose = rows.last().close;
    const double changePct = firstClose > 0.0 ? (lastClose - firstClose) / firstClose * 100.0 : 0.0;
    const double avgClose = sumClose / rows.size();

    m_statsLabel->setText(
        QString("Wiersze: %1 | Close min/max: %2 / %3 | Srednia close: %4 | Zmiana: %5% | Wolumen lacznie: %6")
            .arg(rows.size())
            .arg(QString::number(minClose, 'f', 4))
            .arg(QString::number(maxClose, 'f', 4))
            .arg(QString::number(avgClose, 'f', 4))
            .arg(QString::number(changePct, 'f', 2))
            .arg(QString::number(sumVolume, 'f', 0)));
}

void HistoricalPriceAnalysisDialog::updateTable(const QList<MarketPriceBar>& rows)
{
    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);

    for (const MarketPriceBar& bar : rows) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, readonlyItem(bar.tradingDate));
        m_table->setItem(row, 1, readonlyNumber(bar.open, 4));
        m_table->setItem(row, 2, readonlyNumber(bar.high, 4));
        m_table->setItem(row, 3, readonlyNumber(bar.low, 4));
        m_table->setItem(row, 4, readonlyNumber(bar.close, 4));
        m_table->setItem(row, 5, readonlyNumber(bar.adjClose, 4));
        m_table->setItem(row, 6, readonlyNumber(bar.volume, 0));
        m_table->setItem(row, 7, readonlyItem(bar.source));
    }

    m_table->setSortingEnabled(true);
}

QString HistoricalPriceAnalysisDialog::rowsToCsv(const QList<MarketPriceBar>& rows) const
{
    QString csv;
    QTextStream out(&csv);
    out << "date,open,high,low,close,adj_close,volume,source\n";
    for (const MarketPriceBar& bar : rows) {
        out << bar.tradingDate << ','
            << QString::number(bar.open, 'f', 6) << ','
            << QString::number(bar.high, 'f', 6) << ','
            << QString::number(bar.low, 'f', 6) << ','
            << QString::number(bar.close, 'f', 6) << ','
            << QString::number(bar.adjClose, 'f', 6) << ','
            << QString::number(bar.volume, 'f', 0) << ','
            << bar.source << '\n';
    }
    return csv;
}

void HistoricalPriceAnalysisDialog::copyFilteredRows()
{
    QApplication::clipboard()->setText(rowsToCsv(filteredBars()));
    QMessageBox::information(this, "CSV", "Dane z wybranego zakresu skopiowano do schowka.");
}

void HistoricalPriceAnalysisDialog::exportFilteredRows()
{
    const QString suggested = QString("%1_%2_%3.csv")
        .arg(m_ticker, m_startEdit->date().toString("yyyyMMdd"), m_endEdit->date().toString("yyyyMMdd"));
    const QString path = QFileDialog::getSaveFileName(this, "Eksport CSV", suggested, "CSV (*.csv)");
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "CSV", "Nie udalo sie zapisac pliku CSV.");
        return;
    }

    QTextStream out(&file);
    out << rowsToCsv(filteredBars());
    QMessageBox::information(this, "CSV", "Zapisano plik CSV.");
}
