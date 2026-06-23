#pragma once

#include <QDialog>
#include "DatabaseManager.h"

class TickerSearchEdit;
class QLineEdit;
class QDoubleSpinBox;
class QComboBox;
class QLabel;

class AddAssetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddAssetDialog(QWidget* parent = nullptr);

    PortfolioAsset      getAsset()       const;
    PortfolioTransaction getTransaction() const;

private slots:
    void onTickerSelected(const QString& ticker, const QString& name);
    void onLogoReady(const QPixmap& logo);

private:
    void setupUi();

    TickerSearchEdit* m_tickerEdit     = nullptr;
    QLabel*           m_logoLabel      = nullptr;
    QLineEdit*        m_nameEdit       = nullptr;
    QDoubleSpinBox*   m_qtyEdit        = nullptr;
    QDoubleSpinBox*   m_priceEdit      = nullptr;
    QComboBox*        m_currencyCombo  = nullptr;
    QComboBox*        m_categoryCombo  = nullptr;
};
