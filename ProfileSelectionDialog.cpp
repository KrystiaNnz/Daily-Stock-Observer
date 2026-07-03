#include "ProfileSelectionDialog.h"
#include "DialogUtils.h"
#include "ProfileManager.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

ProfileSelectionDialog::ProfileSelectionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Wybierz profil danych");
    setMinimumWidth(430);
    setupUi();
    DialogUtils::constrainToParent(this);
}

QString ProfileSelectionDialog::selectedProfileId() const
{
    if (!m_profileGroup || !m_profileGroup->checkedButton())
        return ProfileManager::defaultProfileId();
    return m_profileGroup->checkedButton()->property("profileId").toString();
}

bool ProfileSelectionDialog::askAtStartup() const
{
    return m_askAtStartupCheck && m_askAtStartupCheck->isChecked();
}

void ProfileSelectionDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 22, 22, 18);
    root->setSpacing(14);

    auto* title = new QLabel("Wybierz profil", this);
    title->setStyleSheet("font-size: 21px; font-weight: bold; color: #182235;");
    root->addWidget(title);

    auto* subtitle = new QLabel(
        "Profile rozdzielaja baze danych, ustawienia i notatki. Testowy profil jest do eksperymentow.",
        this);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet("color: #5d6675; font-size: 13px;");
    root->addWidget(subtitle);

    m_profileGroup = new QButtonGroup(this);
    m_profileGroup->setExclusive(true);

    const QString activeId = ProfileManager::activeProfileId();
    for (const DataProfile& profile : ProfileManager::profiles()) {
        auto* frame = new QFrame(this);
        frame->setObjectName("profileCard");
        frame->setStyleSheet(
            "QFrame#profileCard { background: #f7f8fb; border: 1px solid #d9dee8;"
            " border-radius: 8px; }");

        auto* layout = new QVBoxLayout(frame);
        layout->setContentsMargins(14, 12, 14, 12);
        layout->setSpacing(5);

        auto* radio = new QRadioButton(profile.name, frame);
        radio->setProperty("profileId", profile.id);
        radio->setStyleSheet("font-weight: bold; font-size: 14px; color: #182235;");
        radio->setChecked(profile.id == activeId);
        m_profileGroup->addButton(radio);
        layout->addWidget(radio);

        auto* description = new QLabel(profile.description, frame);
        description->setWordWrap(true);
        description->setStyleSheet("color: #5d6675; font-size: 12px;");
        layout->addWidget(description);

        root->addWidget(frame);
    }

    m_askAtStartupCheck = new QCheckBox("Pytaj o profil przy starcie", this);
    m_askAtStartupCheck->setChecked(ProfileManager::askAtStartup());
    root->addWidget(m_askAtStartupCheck);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText("Otworz profil");
    buttons->button(QDialogButtonBox::Cancel)->setText("Anuluj");
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}
