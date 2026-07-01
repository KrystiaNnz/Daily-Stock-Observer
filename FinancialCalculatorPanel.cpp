#include "FinancialCalculatorPanel.h"
#include "DatabaseManager.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QtMath>

FinancialCalculatorPanel::FinancialCalculatorPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void FinancialCalculatorPanel::setupUi()
{
    setObjectName("financialCalculatorPanel");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(14);

    auto* title = new QLabel("Kalkulator finansowy", this);
    title->setObjectName("calcTitle");
    root->addWidget(title);

    auto* subtitle = new QLabel("Stopa zwrotu z inwestycji oraz realna stopa zwrotu po uwzglednieniu inflacji.", this);
    subtitle->setObjectName("calcSubtitle");
    root->addWidget(subtitle);

    auto* mainRow = new QHBoxLayout();
    mainRow->setSpacing(16);
    root->addLayout(mainRow);

    auto* inputFrame = new QFrame(this);
    inputFrame->setObjectName("calcInputFrame");
    auto* inputLayout = new QVBoxLayout(inputFrame);
    inputLayout->setContentsMargins(18, 18, 18, 18);
    inputLayout->setSpacing(12);
    mainRow->addWidget(inputFrame, 1);

    inputLayout->addWidget(new QLabel("Zrodlo danych:", this));
    m_sourceCombo = new QComboBox(this);
    m_sourceCombo->addItem("Wpisz recznie", "manual");
    m_sourceCombo->addItem("Zaciagnij z portfolio", "portfolio");
    inputLayout->addWidget(m_sourceCombo);

    auto* portfolioRow = new QHBoxLayout();
    portfolioRow->setSpacing(8);
    m_portfolioCombo = new QComboBox(this);
    m_portfolioCombo->setMinimumWidth(220);
    m_loadPortfolioBtn = new QPushButton("Pobierz", this);
    portfolioRow->addWidget(m_portfolioCombo, 1);
    portfolioRow->addWidget(m_loadPortfolioBtn);
    inputLayout->addLayout(portfolioRow);

    m_portfolioStatusLabel = new QLabel(this);
    m_portfolioStatusLabel->setObjectName("calcSourceStatus");
    inputLayout->addWidget(m_portfolioStatusLabel);

    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem("Inwestycja ogolna: r = Z / I", "investment");
    m_modeCombo->addItem("Akcja: r = ((C_s - C_k) + D) / C_k", "stock");
    inputLayout->addWidget(m_modeCombo);

    m_inputsStack = new QStackedWidget(this);
    inputLayout->addWidget(m_inputsStack);

    auto* investmentPage = new QWidget(this);
    auto* investmentForm = new QFormLayout(investmentPage);

    m_profitSpin = new QDoubleSpinBox(this);
    m_profitSpin->setRange(-1000000000.0, 1000000000.0);
    m_profitSpin->setDecimals(2);
    m_profitSpin->setSingleStep(100.0);
    m_profitSpin->setSuffix(" PLN");
    investmentForm->addRow("Zysk (Z):", m_profitSpin);

    m_investmentSpin = new QDoubleSpinBox(this);
    m_investmentSpin->setRange(0.01, 1000000000.0);
    m_investmentSpin->setDecimals(2);
    m_investmentSpin->setSingleStep(100.0);
    m_investmentSpin->setValue(1000.0);
    m_investmentSpin->setSuffix(" PLN");
    investmentForm->addRow("Naklad inwestycyjny (I):", m_investmentSpin);
    m_inputsStack->addWidget(investmentPage);

    auto* stockPage = new QWidget(this);
    auto* stockForm = new QFormLayout(stockPage);

    m_buyPriceSpin = new QDoubleSpinBox(this);
    m_buyPriceSpin->setRange(0.01, 1000000000.0);
    m_buyPriceSpin->setDecimals(4);
    m_buyPriceSpin->setSingleStep(1.0);
    m_buyPriceSpin->setValue(100.0);
    m_buyPriceSpin->setSuffix(" PLN");
    stockForm->addRow("Cena zakupu (C_k):", m_buyPriceSpin);

    m_sellPriceSpin = new QDoubleSpinBox(this);
    m_sellPriceSpin->setRange(0.0, 1000000000.0);
    m_sellPriceSpin->setDecimals(4);
    m_sellPriceSpin->setSingleStep(1.0);
    m_sellPriceSpin->setValue(110.0);
    m_sellPriceSpin->setSuffix(" PLN");
    stockForm->addRow("Cena sprzedazy (C_s):", m_sellPriceSpin);

    m_dividendSpin = new QDoubleSpinBox(this);
    m_dividendSpin->setRange(0.0, 1000000000.0);
    m_dividendSpin->setDecimals(4);
    m_dividendSpin->setSingleStep(0.1);
    m_dividendSpin->setSuffix(" PLN");
    stockForm->addRow("Dywidenda (D):", m_dividendSpin);
    m_inputsStack->addWidget(stockPage);

    inputLayout->addWidget(new QLabel("Inflacja w analizowanym okresie:", this));
    m_inflationSpin = new QDoubleSpinBox(this);
    m_inflationSpin->setRange(-99.99, 100000.0);
    m_inflationSpin->setDecimals(2);
    m_inflationSpin->setSingleStep(0.25);
    m_inflationSpin->setSuffix(" %");
    inputLayout->addWidget(m_inflationSpin);

    m_calculateBtn = new QPushButton("Oblicz", this);
    m_calculateBtn->setObjectName("calcPrimaryBtn");
    inputLayout->addWidget(m_calculateBtn);
    inputLayout->addStretch();

    auto* resultFrame = new QFrame(this);
    resultFrame->setObjectName("calcResultFrame");
    auto* resultLayout = new QVBoxLayout(resultFrame);
    resultLayout->setContentsMargins(18, 18, 18, 18);
    resultLayout->setSpacing(12);
    mainRow->addWidget(resultFrame, 1);

    auto* resultTitle = new QLabel("Wynik", this);
    resultTitle->setObjectName("calcSectionTitle");
    resultLayout->addWidget(resultTitle);

    m_nominalResult = new QLabel("Nominalna stopa zwrotu: -", this);
    m_nominalResult->setObjectName("calcResultMain");
    resultLayout->addWidget(m_nominalResult);

    m_realResult = new QLabel("Realna stopa zwrotu: -", this);
    m_realResult->setObjectName("calcResultMain");
    resultLayout->addWidget(m_realResult);

    m_formulaLabel = new QLabel("r_real = (1 + r_nom) / (1 + r_inf) - 1", this);
    m_formulaLabel->setObjectName("calcFormula");
    resultLayout->addWidget(m_formulaLabel);

    m_detailsBox = new QTextEdit(this);
    m_detailsBox->setObjectName("calcDetails");
    m_detailsBox->setReadOnly(true);
    m_detailsBox->setText("Wpisz dane i kliknij Oblicz. Skrypt Python zwroci JSON z wynikiem.");
    resultLayout->addWidget(m_detailsBox, 1);

    auto* exerciseFrame = new QFrame(this);
    exerciseFrame->setObjectName("calcExerciseFrame");
    auto* exerciseLayout = new QVBoxLayout(exerciseFrame);
    exerciseLayout->setContentsMargins(18, 18, 18, 18);
    exerciseLayout->setSpacing(10);
    root->addWidget(exerciseFrame, 1);

    auto* exerciseTitle = new QLabel("Zadania treningowe", this);
    exerciseTitle->setObjectName("calcSectionTitle");
    exerciseLayout->addWidget(exerciseTitle);

    auto* exerciseTopRow = new QHBoxLayout();
    exerciseTopRow->setSpacing(8);
    m_exerciseDepartmentCombo = new QComboBox(this);
    populateExerciseDepartments(false);
    m_detailedExerciseTypesCheck = new QCheckBox("Pokaz konkretne typy", this);
    m_generateExerciseBtn = new QPushButton("Generuj zadanie", this);
    m_generateExerciseBtn->setObjectName("calcPrimaryBtn");
    exerciseTopRow->addWidget(m_exerciseDepartmentCombo, 1);
    exerciseTopRow->addWidget(m_detailedExerciseTypesCheck);
    exerciseTopRow->addWidget(m_generateExerciseBtn);
    exerciseLayout->addLayout(exerciseTopRow);

    m_exerciseTopicLabel = new QLabel("Wybierz dzial i wygeneruj zadanie.", this);
    m_exerciseTopicLabel->setObjectName("calcSourceStatus");
    exerciseLayout->addWidget(m_exerciseTopicLabel);

    m_exerciseQuestionBox = new QTextEdit(this);
    m_exerciseQuestionBox->setObjectName("calcDetails");
    m_exerciseQuestionBox->setReadOnly(true);
    m_exerciseQuestionBox->setMinimumHeight(90);
    m_exerciseQuestionBox->setText("Tu pojawi sie tresc zadania.");
    exerciseLayout->addWidget(m_exerciseQuestionBox);

    m_exerciseChoicesFrame = new QFrame(this);
    m_exerciseChoicesFrame->setObjectName("calcInputFrame");
    auto* choicesLayout = new QVBoxLayout(m_exerciseChoicesFrame);
    choicesLayout->setContentsMargins(12, 10, 12, 10);
    choicesLayout->setSpacing(6);
    m_exerciseChoiceGroup = new QButtonGroup(this);
    m_exerciseChoiceGroup->setExclusive(true);
    m_choiceA = new QRadioButton("A", this);
    m_choiceB = new QRadioButton("B", this);
    m_choiceC = new QRadioButton("C", this);
    m_choiceD = new QRadioButton("D", this);
    m_exerciseChoiceGroup->addButton(m_choiceA);
    m_exerciseChoiceGroup->addButton(m_choiceB);
    m_exerciseChoiceGroup->addButton(m_choiceC);
    m_exerciseChoiceGroup->addButton(m_choiceD);
    choicesLayout->addWidget(m_choiceA);
    choicesLayout->addWidget(m_choiceB);
    choicesLayout->addWidget(m_choiceC);
    choicesLayout->addWidget(m_choiceD);
    exerciseLayout->addWidget(m_exerciseChoicesFrame);
    m_exerciseChoicesFrame->setVisible(false);

    auto* answerRow = new QHBoxLayout();
    answerRow->setSpacing(8);
    m_exerciseNumericAnswerLabel = new QLabel("Odpowiedz liczbowa:", this);
    answerRow->addWidget(m_exerciseNumericAnswerLabel);
    m_exerciseAnswerSpin = new QDoubleSpinBox(this);
    m_exerciseAnswerSpin->setRange(-100000.0, 100000.0);
    m_exerciseAnswerSpin->setDecimals(2);
    m_exerciseAnswerSpin->setSingleStep(0.1);
    m_exerciseAnswerSpin->setSuffix(" %");
    answerRow->addWidget(m_exerciseAnswerSpin);
    m_submitExerciseBtn = new QPushButton("Sprawdz", this);
    m_submitExerciseBtn->setObjectName("calcPrimaryBtn");
    m_submitExerciseBtn->setEnabled(false);
    answerRow->addWidget(m_submitExerciseBtn);
    answerRow->addStretch();
    exerciseLayout->addLayout(answerRow);

    m_exerciseFeedbackLabel = new QLabel(this);
    m_exerciseFeedbackLabel->setObjectName("calcSourceStatus");
    exerciseLayout->addWidget(m_exerciseFeedbackLabel);

    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FinancialCalculatorPanel::onModeChanged);
    connect(m_sourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FinancialCalculatorPanel::onSourceChanged);
    connect(m_loadPortfolioBtn, &QPushButton::clicked,
            this, &FinancialCalculatorPanel::loadSelectedPortfolioAsset);
    connect(m_calculateBtn, &QPushButton::clicked,
            this, &FinancialCalculatorPanel::calculate);
    connect(m_generateExerciseBtn, &QPushButton::clicked,
            this, &FinancialCalculatorPanel::generateExercise);
    connect(m_submitExerciseBtn, &QPushButton::clicked,
            this, &FinancialCalculatorPanel::submitExerciseAnswer);
    connect(m_detailedExerciseTypesCheck, &QCheckBox::toggled,
            this, &FinancialCalculatorPanel::populateExerciseDepartments);

    refreshPortfolioAssets();
    onSourceChanged(0);
}

void FinancialCalculatorPanel::populateExerciseDepartments(bool detailed)
{
    if (!m_exerciseDepartmentCombo)
        return;

    const QString previous = m_exerciseDepartmentCombo->currentData().toString();
    m_exerciseDepartmentCombo->blockSignals(true);
    m_exerciseDepartmentCombo->clear();

    if (!detailed) {
        m_exerciseDepartmentCombo->addItem("MATEMATYKA FINANSOWA", "financial_math_mix");
        m_exerciseDepartmentCombo->addItem("INSTRUMENTY DLUZNE", "debt_instruments_mix");
        m_exerciseDepartmentCombo->addItem("TECHNIKI NOTOWAN GIELDOWYCH", "exchange_trading_techniques_mix");
        m_exerciseDepartmentCombo->addItem("INSTRUMENTY POCHODNE", "derivatives_mix");
        m_exerciseDepartmentCombo->addItem("ANALIZA PORTFELOWA", "portfolio_analysis_mix");
        m_exerciseDepartmentCombo->addItem("ANALIZA WSKAZNIKOWA I RACHUNKOWOSC", "ratio_accounting_mix");
    } else {
        m_exerciseDepartmentCombo->addItem("Stopa zwrotu z inwestycji", "investment_return");
        m_exerciseDepartmentCombo->addItem("Stopa zwrotu z akcji", "stock_return");
        m_exerciseDepartmentCombo->addItem("Realna stopa zwrotu", "real_return");
        m_exerciseDepartmentCombo->addItem("Nominalna stopa zwrotu", "nominal_from_real_inflation");
        m_exerciseDepartmentCombo->addItem("Efektywna stopa roczna", "effective_annual_rate");
        m_exerciseDepartmentCombo->addItem("Wartosc przyszla", "future_value");
        m_exerciseDepartmentCombo->addItem("Wartosc biezaca", "present_value");
        m_exerciseDepartmentCombo->addItem("Wartosc przyszla renty", "annuity_future_value");
        m_exerciseDepartmentCombo->addItem("Wartosc biezaca renty", "annuity_present_value");
        m_exerciseDepartmentCombo->addItem("Renta wieczysta", "perpetuity_present_value");
        m_exerciseDepartmentCombo->addItem("Platnosc renty wieczystej", "perpetuity_payment");
        m_exerciseDepartmentCombo->addItem("NPV", "npv");
        m_exerciseDepartmentCombo->addItem("PI", "profitability_index");
        m_exerciseDepartmentCombo->addItem("IRR", "single_cashflow_irr");
        m_exerciseDepartmentCombo->addItem("MIRR", "mirr");
        m_exerciseDepartmentCombo->addItem("EAA", "equivalent_annual_annuity");
        m_exerciseDepartmentCombo->addItem("Zdyskontowany okres zwrotu", "discounted_payback");
        m_exerciseDepartmentCombo->addItem("Kredyt - rowne raty kapitalowe", "debt_equal_principal_interest");
        m_exerciseDepartmentCombo->addItem("Kredyt - rowne platnosci", "debt_annuity_loan_payment");
        m_exerciseDepartmentCombo->addItem("Obligacja zero - cena", "debt_zero_coupon_price");
        m_exerciseDepartmentCombo->addItem("Obligacja zero - nominal", "debt_zero_coupon_nominal");
        m_exerciseDepartmentCombo->addItem("Obligacja zero - YTM", "debt_zero_coupon_ytm");
        m_exerciseDepartmentCombo->addItem("Obligacja kuponowa - cena", "debt_coupon_bond_price");
        m_exerciseDepartmentCombo->addItem("Duration Macaulaya", "debt_macaulay_duration");
        m_exerciseDepartmentCombo->addItem("Modified duration - zmiana ceny", "debt_modified_duration_price_change");
        m_exerciseDepartmentCombo->addItem("Convexity - zmiana ceny", "debt_convexity_price_change");
        m_exerciseDepartmentCombo->addItem("TNG - kurs fixingowy", "tng_fixing_price");
        m_exerciseDepartmentCombo->addItem("TNG - teoretyczny wolumen", "tng_fixing_volume");
        m_exerciseDepartmentCombo->addItem("TNG - realizacja kupna", "tng_continuous_buy_execution_price");
        m_exerciseDepartmentCombo->addItem("TNG - realizacja sprzedazy", "tng_continuous_sell_execution_price");
        m_exerciseDepartmentCombo->addItem("TNG - krok notowan", "tng_tick_size_validation");
        m_exerciseDepartmentCombo->addItem("Pochodne - long forward", "derivative_forward_long_payoff");
        m_exerciseDepartmentCombo->addItem("Pochodne - short forward", "derivative_forward_short_payoff");
        m_exerciseDepartmentCombo->addItem("Pochodne - payoff call", "derivative_call_payoff");
        m_exerciseDepartmentCombo->addItem("Pochodne - payoff put", "derivative_put_payoff");
        m_exerciseDepartmentCombo->addItem("Pochodne - protective put", "derivative_protective_put_payoff");
        m_exerciseDepartmentCombo->addItem("Pochodne - parytet call", "derivative_put_call_parity_call");
        m_exerciseDepartmentCombo->addItem("Pochodne - parytet put", "derivative_put_call_parity_put");
        m_exerciseDepartmentCombo->addItem("Pochodne - delta i cena", "derivative_delta_new_option_price");
        m_exerciseDepartmentCombo->addItem("Pochodne - estymacja delty", "derivative_delta_estimation");
        m_exerciseDepartmentCombo->addItem("Analiza portfela - stopa zwrotu", "portfolio_expected_return");
        m_exerciseDepartmentCombo->addItem("Analiza portfela - ryzyko 2 aktywow", "portfolio_two_asset_std");
        m_exerciseDepartmentCombo->addItem("Analiza portfela - beta z kowariancji", "portfolio_beta_covariance");
        m_exerciseDepartmentCombo->addItem("Analiza portfela - beta z korelacji", "portfolio_beta_correlation");
        m_exerciseDepartmentCombo->addItem("Analiza portfela - CAPM", "portfolio_capm_expected_return");
        m_exerciseDepartmentCombo->addItem("Analiza portfela - WACC", "portfolio_wacc");
        m_exerciseDepartmentCombo->addItem("Analiza portfela - Sharpe", "portfolio_sharpe_ratio");
        m_exerciseDepartmentCombo->addItem("Analiza portfela - Treynor", "portfolio_treynor_ratio");
        m_exerciseDepartmentCombo->addItem("Analiza portfela - Jensen alpha", "portfolio_jensen_alpha");
        m_exerciseDepartmentCombo->addItem("Wskazniki - sredni okres inkasa", "ratio_average_collection_period");
        m_exerciseDepartmentCombo->addItem("Wskazniki - okres obrotu zapasami", "ratio_inventory_processing_period");
        m_exerciseDepartmentCombo->addItem("Wskazniki - cykl konwersji gotowki", "ratio_cash_conversion_cycle");
        m_exerciseDepartmentCombo->addItem("DuPont - obrot aktywami", "ratio_dupont_asset_turnover");
        m_exerciseDepartmentCombo->addItem("DuPont - marza netto", "ratio_dupont_net_margin");
        m_exerciseDepartmentCombo->addItem("Rentownosc - marza brutto", "ratio_gross_profit_margin");
        m_exerciseDepartmentCombo->addItem("Gordon - trailing P/E", "ratio_gordon_trailing_pe");
        m_exerciseDepartmentCombo->addItem("Gordon - wymagana stopa zwrotu", "ratio_gordon_required_return");
        m_exerciseDepartmentCombo->addItem("Gordon - stopa zyskow zatrzymanych", "ratio_gordon_retention_rate");
        m_exerciseDepartmentCombo->addItem("Dzwignie - DOL", "ratio_dol");
        m_exerciseDepartmentCombo->addItem("Dzwignie - koszty stale z DOL", "ratio_fixed_costs_from_dol");
        m_exerciseDepartmentCombo->addItem("Dzwignie - DTL", "ratio_dtl");
        m_exerciseDepartmentCombo->addItem("Rentownosc - koszt dlugu z ROC", "ratio_cost_of_debt_from_roc");
    }

    const int restoreIndex = previous.isEmpty() ? -1 : m_exerciseDepartmentCombo->findData(previous);
    m_exerciseDepartmentCombo->setCurrentIndex(restoreIndex >= 0 ? restoreIndex : 0);
    m_exerciseDepartmentCombo->blockSignals(false);
}

void FinancialCalculatorPanel::refreshPortfolioAssets()
{
    const int previousId = m_portfolioCombo
        ? m_portfolioCombo->currentData().toInt()
        : 0;

    m_portfolioCombo->blockSignals(true);
    m_portfolioCombo->clear();

    const QList<PortfolioAsset> assets = DatabaseManager::instance().getAllAssets();
    for (const PortfolioAsset& asset : assets) {
        QString label = asset.ticker;
        if (!asset.name.trimmed().isEmpty())
            label += " - " + asset.name.trimmed();
        label += QString(" | %1 %2 @ %3 %4")
            .arg(QString::number(asset.quantity, 'f', 4))
            .arg(asset.ticker)
            .arg(QString::number(asset.avgBuyPrice, 'f', 4))
            .arg(asset.currency.isEmpty() ? "PLN" : asset.currency);
        m_portfolioCombo->addItem(label, asset.id);
    }

    int restoreIndex = previousId > 0 ? m_portfolioCombo->findData(previousId) : -1;
    m_portfolioCombo->setCurrentIndex(restoreIndex >= 0 ? restoreIndex : (assets.isEmpty() ? -1 : 0));
    m_portfolioCombo->blockSignals(false);

    const bool hasAssets = !assets.isEmpty();
    m_portfolioCombo->setEnabled(hasAssets);
    m_loadPortfolioBtn->setEnabled(hasAssets && m_sourceCombo->currentData().toString() == "portfolio");
    if (!hasAssets)
        m_portfolioStatusLabel->setText("Portfel jest pusty. Dodaj aktywo w zakladce Portfel.");
    else if (m_portfolioStatusLabel->text().isEmpty())
        m_portfolioStatusLabel->setText("Wybierz aktywo i pobierz dane do kalkulatora.");
}

void FinancialCalculatorPanel::onSourceChanged(int)
{
    const bool portfolioMode = m_sourceCombo->currentData().toString() == "portfolio";
    m_portfolioCombo->setVisible(portfolioMode);
    m_loadPortfolioBtn->setVisible(portfolioMode);
    m_portfolioStatusLabel->setVisible(portfolioMode);
    m_loadPortfolioBtn->setEnabled(portfolioMode && m_portfolioCombo->count() > 0);

    if (portfolioMode) {
        refreshPortfolioAssets();
        if (m_portfolioCombo->count() > 0)
            m_portfolioStatusLabel->setText("Wybierz aktywo i pobierz srednia cene zakupu z portfela.");
    }
}

void FinancialCalculatorPanel::onModeChanged(int index)
{
    m_inputsStack->setCurrentIndex(index);
}

void FinancialCalculatorPanel::loadSelectedPortfolioAsset()
{
    const int selectedId = m_portfolioCombo->currentData().toInt();
    if (selectedId <= 0) {
        m_portfolioStatusLabel->setText("Brak aktywa do pobrania.");
        return;
    }

    const QList<PortfolioAsset> assets = DatabaseManager::instance().getAllAssets();
    for (const PortfolioAsset& asset : assets) {
        if (asset.id != selectedId)
            continue;

        m_modeCombo->setCurrentIndex(m_modeCombo->findData("stock"));
        m_buyPriceSpin->setValue(asset.avgBuyPrice);
        if (m_sellPriceSpin->value() <= 0.0)
            m_sellPriceSpin->setValue(asset.avgBuyPrice);

        m_portfolioStatusLabel->setText(
            QString("Pobrano %1: C_k = %2 %3, ilosc = %4. C_s i dywidende wpisz recznie.")
                .arg(asset.ticker)
                .arg(QString::number(asset.avgBuyPrice, 'f', 4))
                .arg(asset.currency.isEmpty() ? "PLN" : asset.currency)
                .arg(QString::number(asset.quantity, 'f', 4)));
        return;
    }

    m_portfolioStatusLabel->setText("Nie znaleziono wybranego aktywa w portfelu.");
}

QString FinancialCalculatorPanel::scriptPath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("python/financial_calculator.py");
}

void FinancialCalculatorPanel::setBusy(bool busy)
{
    m_calculateBtn->setEnabled(!busy);
    m_calculateBtn->setText(busy ? "Licze..." : "Oblicz");
}

void FinancialCalculatorPanel::setExerciseBusy(bool busy)
{
    m_generateExerciseBtn->setEnabled(!busy);
    m_generateExerciseBtn->setText(busy ? "Generuje..." : "Generuj zadanie");
}

void FinancialCalculatorPanel::calculate()
{
    if (m_process) {
        m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
    }

    const QString mode = m_modeCombo->currentData().toString();
    QStringList args;
    args << scriptPath() << mode;

    if (mode == "investment") {
        args << QString::number(m_profitSpin->value(), 'f', 8)
             << QString::number(m_investmentSpin->value(), 'f', 8)
             << QString::number(m_inflationSpin->value(), 'f', 8);
    } else {
        args << QString::number(m_buyPriceSpin->value(), 'f', 8)
             << QString::number(m_sellPriceSpin->value(), 'f', 8)
             << QString::number(m_dividendSpin->value(), 'f', 8)
             << QString::number(m_inflationSpin->value(), 'f', 8);
    }

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FinancialCalculatorPanel::onFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &FinancialCalculatorPanel::onProcessError);

    setBusy(true);
    m_pendingOperation = PendingOperation::Calculation;
    m_process->start("cmd.exe", QStringList{"/C", "python"} + args);
}

void FinancialCalculatorPanel::generateExercise()
{
    if (m_process) {
        m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
    }

    const QString department = m_exerciseDepartmentCombo->currentData().toString();
    QStringList args;
    args << scriptPath() << "exercise" << department;

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FinancialCalculatorPanel::onFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &FinancialCalculatorPanel::onProcessError);

    m_hasActiveExercise = false;
    m_submitExerciseBtn->setEnabled(false);
    m_exerciseFeedbackLabel->setText("");
    if (m_exerciseChoiceGroup) {
        m_exerciseChoiceGroup->setExclusive(false);
        for (QAbstractButton* button : m_exerciseChoiceGroup->buttons())
            button->setChecked(false);
        m_exerciseChoiceGroup->setExclusive(true);
    }
    setExerciseBusy(true);
    m_pendingOperation = PendingOperation::ExerciseGeneration;
    m_process->start("cmd.exe", QStringList{"/C", "python"} + args);
}

void FinancialCalculatorPanel::submitExerciseAnswer()
{
    if (!m_hasActiveExercise)
        return;

    if (m_exerciseAnswerMode == "choice") {
        QAbstractButton* selectedChoice = m_exerciseChoiceGroup
            ? m_exerciseChoiceGroup->checkedButton()
            : nullptr;
        if (!selectedChoice) {
            m_exerciseFeedbackLabel->setText("Wybierz odpowiedz A/B/C/D.");
            return;
        }

        const QString selectedKey = selectedChoice->property("choiceKey").toString();
        if (selectedKey == m_exerciseCorrectChoice) {
            if (DatabaseManager::instance().awardExperience("calculator_exercise", m_exerciseXpReward, m_exerciseReference)) {
                m_exerciseFeedbackLabel->setText(
                    QString("Poprawnie. +%1 XP\n%2")
                        .arg(m_exerciseXpReward)
                        .arg(m_exerciseExplanation));
                m_submitExerciseBtn->setEnabled(false);
                m_hasActiveExercise = false;
            } else {
                m_exerciseFeedbackLabel->setText("Odpowiedz poprawna, ale nie udalo sie zapisac XP.");
            }
        } else {
            m_exerciseFeedbackLabel->setText(
                QString("Jeszcze nie. Zaznaczono %1, poprawna odpowiedz to %2.")
                    .arg(selectedKey)
                    .arg(m_exerciseCorrectChoice));
        }
        return;
    }

    const double userAnswer = m_exerciseAnswerSpin->value();
    const double diff = qAbs(userAnswer - m_exerciseAnswer);
    if (diff <= m_exerciseTolerance) {
        if (DatabaseManager::instance().awardExperience("calculator_exercise", m_exerciseXpReward, m_exerciseReference)) {
            m_exerciseFeedbackLabel->setText(
                QString("Poprawnie. +%1 XP\n%2")
                    .arg(m_exerciseXpReward)
                    .arg(m_exerciseExplanation));
            m_submitExerciseBtn->setEnabled(false);
            m_hasActiveExercise = false;
        } else {
            m_exerciseFeedbackLabel->setText("Odpowiedz poprawna, ale nie udalo sie zapisac XP.");
        }
    } else {
        m_exerciseFeedbackLabel->setText(
            QString("Jeszcze nie. Twoj wynik: %1%2, roznica: %3%2.")
                .arg(QString::number(userAnswer, 'f', 2))
                .arg(m_exerciseAnswerUnit)
                .arg(QString::number(diff, 'f', 2)));
    }
}

QString FinancialCalculatorPanel::formatPercent(double value) const
{
    return QLocale(QLocale::Polish, QLocale::Poland).toString(value, 'f', 2) + "%";
}

void FinancialCalculatorPanel::onFinished(int, QProcess::ExitStatus)
{
    QByteArray out = m_process->readAllStandardOutput();
    QByteArray err = m_process->readAllStandardError();

    m_process->deleteLater();
    m_process = nullptr;
    const PendingOperation operation = m_pendingOperation;
    m_pendingOperation = PendingOperation::None;
    if (operation == PendingOperation::Calculation)
        setBusy(false);
    if (operation == PendingOperation::ExerciseGeneration)
        setExerciseBusy(false);

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(out, &parseError);
    if (!doc.isObject()) {
        QString msg = err.isEmpty() ? parseError.errorString() : QString::fromUtf8(err.left(300));
        QMessageBox::warning(this, "Blad kalkulatora", "Nieprawidlowa odpowiedz Python: " + msg);
        return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("error")) {
        QMessageBox::warning(this, "Blad kalkulatora", obj.value("error").toString());
        return;
    }

    if (operation == PendingOperation::ExerciseGeneration) {
        m_exerciseTopicLabel->setText(obj.value("topic").toString());
        m_exerciseQuestionBox->setText(obj.value("question").toString());
        m_exerciseAnswer = obj.contains("answer_value")
            ? obj.value("answer_value").toDouble()
            : obj.value("answer_pct").toDouble();
        m_exerciseTolerance = obj.value("tolerance").toDouble(
            obj.value("tolerance_pct").toDouble(0.05));
        m_exerciseAnswerUnit = obj.value("answer_unit").toString("%");
        m_exerciseExplanation = obj.value("explanation").toString();
        m_exerciseReference = obj.value("exercise_id").toString();
        m_exerciseCorrectChoice = obj.value("correct_choice").toString();
        m_exerciseAnswerMode = obj.value("answer_mode").toString("numeric");
        m_exerciseXpReward = obj.value("xp_reward").toInt(5);
        QRadioButton* choiceButtons[] = {m_choiceA, m_choiceB, m_choiceC, m_choiceD};
        if (m_exerciseChoiceGroup) {
            m_exerciseChoiceGroup->setExclusive(false);
            for (QRadioButton* button : choiceButtons) {
                button->setChecked(false);
                button->setText("");
                button->setProperty("choiceKey", "");
                button->setVisible(false);
            }
            m_exerciseChoiceGroup->setExclusive(true);
        }
        const QJsonArray choices = obj.value("choices").toArray();
        for (int i = 0; i < choices.size() && i < 4; ++i) {
            const QJsonObject choice = choices.at(i).toObject();
            const QString key = choice.value("key").toString(QString(QChar('A' + i)));
            const QString label = choice.value("label").toString();
            choiceButtons[i]->setText(QString("%1. %2").arg(key, label));
            choiceButtons[i]->setProperty("choiceKey", key);
            choiceButtons[i]->setVisible(true);
        }
        m_exerciseAnswerSpin->setValue(0.0);
        m_exerciseAnswerSpin->setSuffix(" " + m_exerciseAnswerUnit);
        const bool choiceMode = m_exerciseAnswerMode == "choice";
        if (m_exerciseChoicesFrame)
            m_exerciseChoicesFrame->setVisible(choiceMode);
        if (m_exerciseNumericAnswerLabel)
            m_exerciseNumericAnswerLabel->setVisible(!choiceMode);
        m_exerciseAnswerSpin->setVisible(!choiceMode);
        m_exerciseTopicLabel->setText(
            obj.value("topic").toString()
            + (choiceMode ? " | tryb odpowiedzi: A/B/C/D" : " | tryb odpowiedzi: liczbowo"));
        m_submitExerciseBtn->setText(QString("Sprawdz (+%1 XP)").arg(m_exerciseXpReward));
        m_submitExerciseBtn->setEnabled(true);
        m_hasActiveExercise = true;
        return;
    }

    const double nominalPct = obj.value("nominal_return_pct").toDouble();
    const double realPct = obj.value("real_return_pct").toDouble();

    m_nominalResult->setText("Nominalna stopa zwrotu: " + formatPercent(nominalPct));
    m_realResult->setText("Realna stopa zwrotu: " + formatPercent(realPct));
    m_formulaLabel->setText(obj.value("formula").toString());
    m_detailsBox->setText(obj.value("explanation").toString());
}

void FinancialCalculatorPanel::onProcessError(QProcess::ProcessError)
{
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
    setBusy(false);
    setExerciseBusy(false);
    m_pendingOperation = PendingOperation::None;
    QMessageBox::warning(this, "Blad kalkulatora",
        "Nie mozna uruchomic financial_calculator.py. Sprawdz, czy Python jest dostepny w PATH.");
}
