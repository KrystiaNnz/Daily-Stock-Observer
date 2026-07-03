#include "AddAssetDialog.h"
#include "DialogUtils.h"
#include "TickerSearchEdit.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QDate>
#include <QPixmap>

AddAssetDialog::AddAssetDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Dodaj aktywo");
    setMinimumWidth(420);
    setupUi();
    DialogUtils::constrainToParent(this);
}

void AddAssetDialog::setupUi()
{
    auto* outer = new QVBoxLayout(this);
    outer->setSpacing(10);
    outer->setContentsMargins(12, 12, 12, 12);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget(scroll);
    auto* main = new QVBoxLayout(content);
    main->setSpacing(12);
    main->setContentsMargins(8, 8, 8, 8);
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);

    auto* form = new QFormLayout();
    form->setSpacing(10);

    // ── Ticker + Logo ─────────────────────────────────
    m_tickerEdit = new TickerSearchEdit(this);

    m_logoLabel = new QLabel(this);
    m_logoLabel->setFixedSize(32, 32);
    m_logoLabel->setStyleSheet(
        "border: 1px solid #e2e8f0; border-radius: 6px; background: #f8fafc;");
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_logoLabel->setToolTip("Logo firmy");

    auto* tickerRow = new QHBoxLayout();
    tickerRow->setSpacing(8);
    tickerRow->addWidget(m_logoLabel);
    tickerRow->addWidget(m_tickerEdit, 1);
    form->addRow("Ticker *", tickerRow);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItems({
        "Akcja",
        "ETF",
        "Obligacja rządowa",
        "Obligacja korporacyjna",
        "Kryptowaluta",
        "Kontrakt na surowce",
        "Inne",
    });
    form->addRow("Typ instrumentu", m_typeCombo);

    // ── Nazwa (auto-wypełniana po wyborze) ────────────
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("wypełni się automatycznie po wyborze tickera");
    form->addRow("Nazwa", m_nameEdit);

    // ── Ilość ─────────────────────────────────────────
    m_qtyEdit = new QDoubleSpinBox(this);
    m_qtyEdit->setRange(0.0001, 1'000'000);
    m_qtyEdit->setDecimals(4);
    m_qtyEdit->setSingleStep(1.0);
    m_qtyEdit->setValue(1.0);
    form->addRow("Ilo\u015b\u0107 *", m_qtyEdit);

    // ── Cena zakupu ───────────────────────────────────
    m_priceEdit = new QDoubleSpinBox(this);
    m_priceEdit->setRange(0.0001, 10'000'000);
    m_priceEdit->setDecimals(4);
    m_priceEdit->setSingleStep(1.0);
    m_priceEdit->setValue(0.0);
    form->addRow("Cena zakupu *", m_priceEdit);

    // ── Waluta ────────────────────────────────────────
    m_currencyCombo = new QComboBox(this);
    m_currencyCombo->addItems({"PLN", "USD", "EUR", "GBP", "CHF"});
    form->addRow("Waluta", m_currencyCombo);

    // ── Kategoria ─────────────────────────────────────
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->setEditable(true);
    m_categoryCombo->setInsertPolicy(QComboBox::NoInsert);
    m_categoryCombo->lineEdit()->setPlaceholderText("Wybierz lub wpisz własną…");
    m_categoryCombo->addItems({
        // Technologia
        "Gry wideo i e-sport",
        "Oprogramowanie i SaaS",
        "Półprzewodniki",
        "Chmura obliczeniowa i AI",
        "Cyberbezpieczeństwo",
        "Sprzęt komputerowy",
        "Internet i e-commerce",
        "Media społecznościowe",
        // Energia
        "Energia odnawialna (OZE)",
        "Paliwa kopalne (ropa, gaz)",
        "Energetyka jądrowa",
        "Elektroenergetyka i sieci",
        // Przemysł i infrastruktura
        "Infrastruktura drogowa i autostrady",
        "Budownictwo i materiały budowlane",
        "Transport i logistyka",
        "Lotnictwo i kosmonautyka",
        "Kolej",
        // Finanse
        "Banki i usługi kredytowe",
        "Ubezpieczenia",
        "Fintech i kryptowaluty",
        // Opieka zdrowotna
        "Farmaceutyki i biotechnologia",
        "Urządzenia medyczne",
        "Usługi medyczne",
        // Dobra konsumpcyjne
        "Handel detaliczny",
        "Żywność i napoje",
        "Dobra luksusowe i moda",
        "Media i streaming",
        // Surowce i materiały
        "Metale szlachetne",
        "Metale przemysłowe",
        "Chemia i tworzywa sztuczne",
        // Nieruchomości
        "REIT i nieruchomości komercyjne",
        "Deweloperzy mieszkaniowi",
        // Motoryzacja
        "Pojazdy elektryczne (EV)",
        "Pojazdy spalinowe i hybrydowe",
        "Części i akcesoria motoryzacyjne",
        // Inne
        "Konglomeraty",
        "Inne",
    });
    m_categoryCombo->setCurrentIndex(-1);
    form->addRow("Kategoria", m_categoryCombo);

    main->addLayout(form);

    // Hint
    m_hintLabel = new QLabel(this);
    m_hintLabel->setTextFormat(Qt::RichText);
    m_hintLabel->setWordWrap(true);
    main->addWidget(m_hintLabel);

    // Przyciski
    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setText("Dodaj");
    btns->button(QDialogButtonBox::Cancel)->setText("Anuluj");

    connect(btns, &QDialogButtonBox::accepted, this, [this]() {
        if (m_tickerEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "B\u0142\u0105d", "Ticker jest wymagany.");
            return;
        }
        if (m_priceEdit->value() <= 0.0) {
            QMessageBox::warning(this, "B\u0142\u0105d",
                "Cena zakupu musi by\u0107 wi\u0119ksza od 0.");
            return;
        }
        accept();
    });
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    outer->addWidget(btns);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddAssetDialog::updateInstrumentHints);

    // Sygnały TickerSearchEdit
    connect(m_tickerEdit, &TickerSearchEdit::tickerSelected,
            this, &AddAssetDialog::onTickerSelected);
    connect(m_tickerEdit, &TickerSearchEdit::logoReady,
            this, &AddAssetDialog::onLogoReady);

    updateInstrumentHints();
}

void AddAssetDialog::updateInstrumentHints()
{
    const QString type = m_typeCombo->currentText();
    QString placeholder = "Wpisz nazwę lub ticker, np. cdprojekt, AAPL...";
    QString hint =
        "Ticker GPW: sufiks <b>.WA</b> (np. <b>CDR.WA</b>) &nbsp;|&nbsp; "
        "US: bez sufiksu (np. <b>AAPL</b>)";

    if (type == "ETF") {
        placeholder = "Wpisz ETF, np. VWCE.DE, CSPX.L, SPY...";
        hint = "ETF-y zwykle działają jak akcje: wpisz ticker z giełdy, np. <b>VWCE.DE</b>, <b>CSPX.L</b>, <b>SPY</b>.";
    } else if (type == "Kryptowaluta") {
        placeholder = "Wpisz krypto, np. BTC-USD, ETH-USD...";
        hint = "Krypto w Yahoo Finance najczęściej ma format <b>BTC-USD</b>, <b>ETH-USD</b>, <b>SOL-USD</b>.";
        int usd = m_currencyCombo->findText("USD");
        if (usd >= 0)
            m_currencyCombo->setCurrentIndex(usd);
    } else if (type == "Obligacja rządowa" || type == "Obligacja korporacyjna") {
        placeholder = "Wpisz ticker, ISIN albo własny identyfikator obligacji...";
        hint = "Dla pojedynczych obligacji możesz wpisać ISIN lub własny symbol. Jeśli nie ma live tickera, pozycja zostanie w portfelu z ceną zakupu bez kursu live.";
    } else if (type == "Kontrakt na surowce") {
        placeholder = "Wpisz kontrakt, np. GC=F, CL=F, SI=F...";
        hint = "Kontrakty surowcowe w Yahoo Finance zwykle kończą się <b>=F</b>, np. złoto <b>GC=F</b>, ropa WTI <b>CL=F</b>, srebro <b>SI=F</b>.";
        int usd = m_currencyCombo->findText("USD");
        if (usd >= 0)
            m_currencyCombo->setCurrentIndex(usd);
    } else if (type == "Inne") {
        placeholder = "Wpisz ticker, ISIN albo własny identyfikator...";
        hint = "Możesz dodać instrument niestandardowy. Live kurs zadziała tylko wtedy, gdy symbol rozpozna dostawca danych.";
    }

    m_tickerEdit->setPlaceholderText(placeholder);
    m_hintLabel->setText("<small style='color:#999'>" + hint + "</small>");
}

void AddAssetDialog::onTickerSelected(const QString& /*ticker*/, const QString& name)
{
    if (!name.isEmpty())
        m_nameEdit->setText(name);

    // Waluta: GPW → PLN, inne → USD
    QString t = m_tickerEdit->ticker();
    int idx = t.endsWith(".WA") ? 0 : 1;   // 0=PLN, 1=USD
    m_currencyCombo->setCurrentIndex(idx);
}

void AddAssetDialog::onLogoReady(const QPixmap& logo)
{
    m_logoLabel->setPixmap(logo);
    m_logoLabel->setStyleSheet(
        "border: 1px solid #e2e8f0; border-radius: 6px; background: white;");
}

PortfolioAsset AddAssetDialog::getAsset() const
{
    PortfolioAsset a;
    a.ticker      = m_tickerEdit->text().trimmed().toUpper();
    a.name        = m_nameEdit->text().trimmed();
    a.assetType   = m_typeCombo->currentText();
    a.quantity    = m_qtyEdit->value();
    a.avgBuyPrice = m_priceEdit->value();
    a.currency    = m_currencyCombo->currentText();
    a.category    = m_categoryCombo->currentText().trimmed();
    return a;
}

PortfolioTransaction AddAssetDialog::getTransaction() const
{
    PortfolioTransaction tx;
    tx.ticker   = m_tickerEdit->text().trimmed().toUpper();
    tx.type     = "buy";
    tx.quantity = m_qtyEdit->value();
    tx.price    = m_priceEdit->value();
    tx.date     = QDate::currentDate().toString("yyyy-MM-dd");
    return tx;
}
