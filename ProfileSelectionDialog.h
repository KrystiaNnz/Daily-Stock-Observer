#pragma once

#include <QDialog>
#include <QString>

class QButtonGroup;
class QCheckBox;
class QComboBox;

class ProfileSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileSelectionDialog(QWidget* parent = nullptr);

    QString selectedProfileId() const;
    QString selectedLanguageCode() const;
    bool askAtStartup() const;

private:
    void setupUi();
    void applyContrastStyle();

    QButtonGroup* m_profileGroup = nullptr;
    QComboBox* m_languageCombo = nullptr;
    QCheckBox* m_askAtStartupCheck = nullptr;
};
