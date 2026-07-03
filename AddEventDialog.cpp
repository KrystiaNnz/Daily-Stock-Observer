#include "AddEventDialog.h"
#include "DialogUtils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QTimeEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QTime>

AddEventDialog::AddEventDialog(const QDate& date, QWidget* parent)
    : QDialog(parent), m_date(date)
{
    setupUi(date);
    setWindowTitle("Dodaj wydarzenie");
    setMinimumWidth(380);
    DialogUtils::constrainToParent(this);
}

void AddEventDialog::setupUi(const QDate& date)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Nagłówek z datą
    auto* dateLabel = new QLabel(
        "<b>Data:</b> " + date.toString("dddd, d MMMM yyyy"), this);
    mainLayout->addWidget(dateLabel);

    // Formularz
    auto* form = new QFormLayout();
    form->setSpacing(8);

    // Tytuł (wymagany)
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("np. Spotkanie z zespołem");
    form->addRow("Tytuł *", m_titleEdit);

    // Godzina (opcjonalna)
    auto* timeRow = new QHBoxLayout();
    m_timeCheck = new QCheckBox("Ustaw godzinę", this);
    m_timeEdit  = new QTimeEdit(QTime::currentTime(), this);
    m_timeEdit->setDisplayFormat("HH:mm");
    m_timeEdit->setEnabled(false);
    connect(m_timeCheck, &QCheckBox::toggled, m_timeEdit, &QTimeEdit::setEnabled);
    timeRow->addWidget(m_timeCheck);
    timeRow->addWidget(m_timeEdit);
    timeRow->addStretch();
    form->addRow("Godzina", timeRow);

    // Opis (opcjonalny)
    m_descEdit = new QTextEdit(this);
    m_descEdit->setPlaceholderText("Opcjonalny opis...");
    m_descEdit->setMaximumHeight(100);
    form->addRow("Opis", m_descEdit);

    mainLayout->addLayout(form);

    // Przyciski OK / Anuluj
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText("Zapisz");
    buttons->button(QDialogButtonBox::Cancel)->setText("Anuluj");

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (m_titleEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Błąd", "Tytuł jest wymagany.");
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttons);
}

Event AddEventDialog::getEvent() const
{
    Event e;
    e.title       = m_titleEdit->text().trimmed();
    e.date        = m_date.toString("yyyy-MM-dd");
    e.time        = m_timeCheck->isChecked()
                        ? m_timeEdit->time().toString("HH:mm")
                        : QString();
    e.description = m_descEdit->toPlainText().trimmed();
    return e;
}
