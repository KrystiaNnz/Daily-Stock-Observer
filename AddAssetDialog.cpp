#include "AddAssetDialog.h"
#include "TickerSearchEdit.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QDate>
#include <QPixmap>

AddAssetDialog::AddAssetDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Dodaj aktywo");
    setMinimumWidth(420);
    setupUi();
}

void AddAssetDialog::setupUi()
{
    auto* main = new QVBoxLayout(this);
    main->setSpacing(12);
    main->setContentsMargins(20, 20, 20, 20);

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
    auto* hint = new QLabel(
        "<small style='color:#999'>"
        "Ticker GPW: sufiks <b>.WA</b> (np. <b>CDR.WA</b>) &nbsp;|&nbsp; "
        "US: bez sufiksu (np. <b>AAPL</b>) &nbsp;|&nbsp; "
        "Crypto: sufiks <b>-USD</b> (np. <b>BTC-USD</b>)"
        "</small>", this);
    hint->setTextFormat(Qt::RichText);
    main->addWidget(hint);

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
    main->addWidget(btns);

    // Sygnały TickerSearchEdit
    connect(m_tickerEdit, &TickerSearchEdit::tickerSelected,
            this, &AddAssetDialog::onTickerSelected);
    connect(m_tickerEdit, &TickerSearchEdit::logoReady,
            this, &AddAssetDialog::onLogoReady);
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
