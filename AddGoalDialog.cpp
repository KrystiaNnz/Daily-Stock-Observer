#include "AddGoalDialog.h"
#include "DialogUtils.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

AddGoalDialog::AddGoalDialog(const QDate& defaultDate, QWidget* parent)
    : QDialog(parent), m_editId(0)
{
    setWindowTitle("Nowy cel");
    setMinimumWidth(420);
    setupUi({}, {}, defaultDate, /*showSteps=*/true);
    DialogUtils::constrainToParent(this);
}

AddGoalDialog::AddGoalDialog(const Goal& existing, QWidget* parent)
    : QDialog(parent), m_editId(existing.id)
{
    setWindowTitle("Edytuj cel");
    setMinimumWidth(420);
    QDate date = existing.dueDate.isEmpty()
        ? QDate::currentDate()
        : QDate::fromString(existing.dueDate, "yyyy-MM-dd");
    setupUi(existing.title, existing.description, date, /*showSteps=*/false);
    DialogUtils::constrainToParent(this);
}

void AddGoalDialog::setupUi(const QString& titleVal, const QString& descVal,
                             const QDate& date, bool showSteps)
{
    auto* main = new QVBoxLayout(this);
    main->setSpacing(12);
    main->setContentsMargins(20, 20, 20, 20);

    auto* form = new QFormLayout();
    form->setSpacing(8);

    // Tytuł
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("np. Licencja maklerska KNF");
    m_titleEdit->setText(titleVal);
    form->addRow("Cel *", m_titleEdit);

    // Termin (date picker)
    m_dateEdit = new QDateEdit(date, this);
    m_dateEdit->setDisplayFormat("dd.MM.yyyy");
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setMinimumDate(QDate(2020, 1, 1));
    form->addRow("Termin", m_dateEdit);

    // Opis
    m_descEdit = new QTextEdit(this);
    m_descEdit->setPlaceholderText("Opcjonalny opis…");
    m_descEdit->setMaximumHeight(70);
    m_descEdit->setPlainText(descVal);
    form->addRow("Opis", m_descEdit);

    // Kroki — tylko przy tworzeniu
    if (showSteps) {
        m_stepsEdit = new QPlainTextEdit(this);
        m_stepsEdit->setPlaceholderText(
            "Wpisz kroki (każdy w nowej linii)…\n"
            "np.\n"
            "Przeczytać notatki\n"
            "Zebrać slajdy\n"
            "Przećwiczyć");
        m_stepsEdit->setMaximumHeight(130);
        form->addRow("Kroki", m_stepsEdit);
    }

    main->addLayout(form);

    if (m_editId > 0) {
        auto* hint = new QLabel(
            "<small><i>Kroki edytujesz bezpośrednio na karcie celu.</i></small>", this);
        hint->setAlignment(Qt::AlignLeft);
        main->addWidget(hint);
    }

    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setText(
        m_editId > 0 ? "Zapisz zmiany" : "Utwórz cel");
    btns->button(QDialogButtonBox::Cancel)->setText("Anuluj");

    connect(btns, &QDialogButtonBox::accepted, this, [this]() {
        if (m_titleEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Błąd", "Tytuł celu jest wymagany.");
            return;
        }
        accept();
    });
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);

    main->addWidget(btns);
}

Goal AddGoalDialog::getGoal() const
{
    Goal g;
    g.id          = m_editId;
    g.title       = m_titleEdit->text().trimmed();
    g.description = m_descEdit->toPlainText().trimmed();
    g.dueDate     = m_dateEdit->date().toString("yyyy-MM-dd");

    if (m_stepsEdit) {
        const QStringList lines = m_stepsEdit->toPlainText().split('\n');
        int pos = 0;
        for (const QString& line : lines) {
            const QString t = line.trimmed();
            if (!t.isEmpty()) {
                Step s;
                s.label    = t;
                s.position = pos++;
                g.steps.append(s);
            }
        }
    }

    return g;
}
