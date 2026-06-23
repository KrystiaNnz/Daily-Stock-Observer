#pragma once

#include <QDialog>
#include <QDate>
#include "DatabaseManager.h"

class QLineEdit;
class QTextEdit;
class QTimeEdit;
class QCheckBox;

// Dialog do dodawania nowego wydarzenia
class AddEventDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddEventDialog(const QDate& date, QWidget* parent = nullptr);

    // Zwraca wypełnioną strukturę Event po zamknięciu dialogu
    Event getEvent() const;

private:
    void setupUi(const QDate& date);

    QLineEdit* m_titleEdit;
    QTimeEdit* m_timeEdit;
    QCheckBox* m_timeCheck;
    QTextEdit* m_descEdit;
    QDate      m_date;
};
