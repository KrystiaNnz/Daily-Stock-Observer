#pragma once

#include <QDialog>
#include <QDate>
#include "DatabaseManager.h"

class QLineEdit;
class QTextEdit;
class QPlainTextEdit;
class QDateEdit;

class AddGoalDialog : public QDialog
{
    Q_OBJECT

public:
    // Tryb tworzenia — domyślna data w pickerze
    explicit AddGoalDialog(const QDate& defaultDate, QWidget* parent = nullptr);
    // Tryb edycji — pre-fill z istniejącego celu (bez pola kroków)
    explicit AddGoalDialog(const Goal& existing, QWidget* parent = nullptr);

    Goal getGoal() const;

private:
    void setupUi(const QString& titleVal, const QString& descVal,
                 const QDate& date, bool showSteps);

    QLineEdit*      m_titleEdit = nullptr;
    QTextEdit*      m_descEdit  = nullptr;
    QPlainTextEdit* m_stepsEdit = nullptr;   // nullptr w trybie edycji
    QDateEdit*      m_dateEdit  = nullptr;
    int             m_editId    = 0;         // 0 = tryb tworzenia
};
