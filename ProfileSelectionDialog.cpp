#include "ProfileSelectionDialog.h"
#include "DialogUtils.h"
#include "ProfileManager.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace {
QColor contrastTextFor(const QColor& background)
{
    const double luminance =
        0.2126 * background.redF() +
        0.7152 * background.greenF() +
        0.0722 * background.blueF();
    return luminance < 0.52 ? QColor("#ffffff") : QColor("#000000");
}

QColor blendColors(const QColor& foreground, const QColor& background, double foregroundWeight)
{
    const double bgWeight = 1.0 - foregroundWeight;
    return QColor(
        qBound(0, qRound(foreground.red() * foregroundWeight + background.red() * bgWeight), 255),
        qBound(0, qRound(foreground.green() * foregroundWeight + background.green() * bgWeight), 255),
        qBound(0, qRound(foreground.blue() * foregroundWeight + background.blue() * bgWeight), 255));
}
}

ProfileSelectionDialog::ProfileSelectionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Wybierz profil danych");
    setMinimumWidth(430);
    setupUi();
    applyContrastStyle();
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

QString ProfileSelectionDialog::selectedLanguageCode() const
{
    if (!m_languageCombo)
        return ProfileManager::defaultLanguageCode();
    return ProfileManager::normalizeLanguageCode(m_languageCombo->currentData().toString());
}

void ProfileSelectionDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 22, 22, 18);
    root->setSpacing(14);

    auto* title = new QLabel("Wybierz profil", this);
    title->setObjectName("startupTitle");
    root->addWidget(title);

    auto* subtitle = new QLabel(
        "Profile rozdzielaja baze danych, ustawienia i notatki. Testowy profil jest do eksperymentow.",
        this);
    subtitle->setWordWrap(true);
    subtitle->setObjectName("startupSubtitle");
    root->addWidget(subtitle);

    m_profileGroup = new QButtonGroup(this);
    m_profileGroup->setExclusive(true);

    const QString activeId = ProfileManager::activeProfileId();
    for (const DataProfile& profile : ProfileManager::profiles()) {
        auto* frame = new QFrame(this);
        frame->setObjectName("profileCard");

        auto* layout = new QVBoxLayout(frame);
        layout->setContentsMargins(14, 12, 14, 12);
        layout->setSpacing(5);

        auto* radio = new QRadioButton(profile.name, frame);
        radio->setProperty("profileId", profile.id);
        radio->setObjectName("profileRadio");
        radio->setChecked(profile.id == activeId);
        m_profileGroup->addButton(radio);
        layout->addWidget(radio);

        auto* description = new QLabel(profile.description, frame);
        description->setWordWrap(true);
        description->setObjectName("profileDescription");
        layout->addWidget(description);

        root->addWidget(frame);
    }

    auto* languageFrame = new QFrame(this);
    languageFrame->setObjectName("profileCard");
    auto* languageLayout = new QVBoxLayout(languageFrame);
    languageLayout->setContentsMargins(14, 12, 14, 12);
    languageLayout->setSpacing(7);

    auto* languageLabel = new QLabel("Język aplikacji", languageFrame);
    languageLabel->setObjectName("profileRadio");
    languageLayout->addWidget(languageLabel);

    m_languageCombo = new QComboBox(languageFrame);
    for (const AppLanguage& language : ProfileManager::languages())
        m_languageCombo->addItem(language.name, language.code);
    const int languageIndex = m_languageCombo->findData(ProfileManager::languageCode());
    m_languageCombo->setCurrentIndex(languageIndex >= 0 ? languageIndex : 0);
    languageLayout->addWidget(m_languageCombo);

    auto* languageHint = new QLabel("Wybierz język, w którym aplikacja ma być obsługiwana.", languageFrame);
    languageHint->setWordWrap(true);
    languageHint->setObjectName("profileDescription");
    languageLayout->addWidget(languageHint);
    root->addWidget(languageFrame);

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

void ProfileSelectionDialog::applyContrastStyle()
{
    const QColor background("#f5f5f6");
    const QColor surface("#ffffff");
    const QColor card("#f7f8fb");
    const QColor border("#d3d7dd");
    const QColor accent("#3a2d97");
    const QColor bgText = contrastTextFor(background);
    const QColor surfaceText = contrastTextFor(surface);
    const QColor cardText = contrastTextFor(card);
    const QColor muted = blendColors(cardText, card, 0.62);
    const QColor fieldBg("#ffffff");
    const QColor fieldText = contrastTextFor(fieldBg);

    setStyleSheet(
        "ProfileSelectionDialog {"
        " background: " + background.name() + ";"
        " color: " + bgText.name() + "; }\n"

        "ProfileSelectionDialog QLabel { color: " + cardText.name() + "; }\n"
        "ProfileSelectionDialog QLabel#startupTitle {"
        " color: " + bgText.name() + ";"
        " font-size: 21px;"
        " font-weight: bold; }\n"
        "ProfileSelectionDialog QLabel#startupSubtitle {"
        " color: " + blendColors(bgText, background, 0.68).name() + ";"
        " font-size: 13px; }\n"

        "ProfileSelectionDialog QFrame#profileCard {"
        " background: " + card.name() + ";"
        " border: 1px solid " + border.name() + ";"
        " border-radius: 8px; }\n"
        "ProfileSelectionDialog QLabel#profileDescription {"
        " color: " + muted.name() + ";"
        " font-size: 12px; }\n"
        "ProfileSelectionDialog QRadioButton#profileRadio,"
        " ProfileSelectionDialog QLabel#profileRadio {"
        " color: " + cardText.name() + ";"
        " font-size: 14px;"
        " font-weight: bold; }\n"
        "ProfileSelectionDialog QCheckBox {"
        " color: " + bgText.name() + ";"
        " font-size: 13px; }\n"

        "ProfileSelectionDialog QComboBox {"
        " background: " + fieldBg.name() + ";"
        " color: " + fieldText.name() + ";"
        " border: 1px solid " + border.name() + ";"
        " border-radius: 6px;"
        " padding: 6px 28px 6px 8px; }\n"
        "ProfileSelectionDialog QComboBox::drop-down {"
        " border: none;"
        " width: 24px; }\n"
        "ProfileSelectionDialog QComboBox QAbstractItemView {"
        " background: " + surface.name() + ";"
        " color: " + surfaceText.name() + ";"
        " border: 1px solid " + border.name() + ";"
        " selection-background-color: " + accent.name() + ";"
        " selection-color: " + contrastTextFor(accent).name() + "; }\n"

        "ProfileSelectionDialog QDialogButtonBox QPushButton {"
        " background: " + accent.name() + ";"
        " color: " + contrastTextFor(accent).name() + ";"
        " border: none;"
        " border-radius: 6px;"
        " padding: 7px 14px;"
        " font-weight: 600; }\n"
        "ProfileSelectionDialog QDialogButtonBox QPushButton:hover {"
        " background: " + accent.darker(112).name() + "; }\n"
        "ProfileSelectionDialog QDialogButtonBox QPushButton:pressed {"
        " background: " + accent.darker(130).name() + "; }\n");
}
