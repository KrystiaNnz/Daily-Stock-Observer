#pragma once

#include <QDialog>
#include <QString>

class QButtonGroup;
class QCheckBox;

class ProfileSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileSelectionDialog(QWidget* parent = nullptr);

    QString selectedProfileId() const;
    bool askAtStartup() const;

private:
    void setupUi();

    QButtonGroup* m_profileGroup = nullptr;
    QCheckBox* m_askAtStartupCheck = nullptr;
};
