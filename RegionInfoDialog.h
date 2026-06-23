#pragma once

#include "CountrySubdivisions.h"

#include <QDialog>

class RegionInfoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RegionInfoDialog(const SubdivisionInfo& region, QWidget* parent = nullptr);
};
