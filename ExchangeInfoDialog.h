#pragma once

#include <QDialog>
#include "MapPanel.h"

class ExchangeInfoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ExchangeInfoDialog(const ExchangeMarker& exchange, QWidget* parent = nullptr);
};
