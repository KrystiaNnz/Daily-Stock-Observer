#include "SettingsDialog.h"
#include "DialogUtils.h"
#include "PatternGen.h"
#include "ProfileManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QColorDialog>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>

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

SettingsDialog::SettingsDialog(const AppColors&       colors,
                               const DisplaySettings& display,
                               const ProfileSettings& profile,
                               QWidget* parent)
    : QDialog(parent), m_colors(colors), m_display(display), m_profile(profile)
{
    setWindowTitle("Ustawienia - Daily Stock Observer");
    setMinimumWidth(440);
    setupUi();
    applyContrastStyle();
    DialogUtils::constrainToParent(this);
}

// ── Kolory ────────────────────────────────────────────────────────

void SettingsDialog::addColorRow(QFormLayout*  form,
                                 const QString& label,
                                 QColor*        colorPtr,
                                 QPushButton*&  outBtn,
                                 QLabel*&       outHex)
{
    auto* row = new QHBoxLayout();
    row->setSpacing(10);

    outBtn = new QPushButton(this);
    outBtn->setFixedSize(44, 28);
    outBtn->setCursor(Qt::PointingHandCursor);
    applyBtnStyle(outBtn, *colorPtr);

    outHex = new QLabel(colorPtr->name().toUpper(), this);
    outHex->setObjectName("settingsHex");

    row->addWidget(outBtn);
    row->addWidget(outHex);
    row->addStretch();
    form->addRow(label, row);

    connect(outBtn, &QPushButton::clicked, this,
        [this, colorPtr, outBtn, outHex]() {
            QColor chosen = QColorDialog::getColor(*colorPtr, this, "Wybierz kolor");
            if (chosen.isValid()) {
                *colorPtr = chosen;
                applyBtnStyle(outBtn, chosen);
                outHex->setText(chosen.name().toUpper());
            }
        });
}

void SettingsDialog::applyBtnStyle(QPushButton* btn, const QColor& color)
{
    btn->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  border: 2px solid rgba(0,0,0,0.18);"
        "  border-radius: 5px;"
        "}"
        "QPushButton:hover { border-color: rgba(0,0,0,0.4); }"
    ).arg(color.name()));
    btn->setToolTip(color.name().toUpper());
}

void SettingsDialog::resetToDefaults()
{
    m_colors = AppColors::defaults();

    applyBtnStyle(m_accentBtn, m_colors.accent);
    m_accentHex->setText(m_colors.accent.name().toUpper());

    applyBtnStyle(m_leftBtn, m_colors.leftPanel);
    m_leftHex->setText(m_colors.leftPanel.name().toUpper());

    applyBtnStyle(m_bgBtn, m_colors.appBg);
    m_bgHex->setText(m_colors.appBg.name().toUpper());

    applyBtnStyle(m_surfaceBtn, m_colors.surface);
    m_surfaceHex->setText(m_colors.surface.name().toUpper());

    applyBtnStyle(m_borderBtn, m_colors.border);
    m_borderHex->setText(m_colors.border.name().toUpper());

    applyBtnStyle(m_textBtn, m_colors.text);
    m_textHex->setText(m_colors.text.name().toUpper());
    if (m_autoTextContrastCheck)
        m_autoTextContrastCheck->setChecked(m_colors.autoTextContrast);
    updateTextControls();

    m_patternGroup->button(static_cast<int>(PatternType::None))->setChecked(true);
    m_colors.patternMark = PatternGen::autoMark(m_colors.leftPanel);
    applyBtnStyle(m_patternMarkBtn, m_colors.patternMark);
    m_patternMarkHex->setText(m_colors.patternMark.name().toUpper());

    m_display = DisplaySettings::defaults();
    m_radioNormal->setChecked(true);
    m_widthSpin->setValue(m_display.width);
    m_heightSpin->setValue(m_display.height);
    updateSizeEnabled();
}

void SettingsDialog::updateSizeEnabled()
{
    const bool editable = m_radioNormal->isChecked();
    m_widthSpin->setEnabled(editable);
    m_heightSpin->setEnabled(editable);
}

void SettingsDialog::updateTextControls()
{
    const bool manualText = !m_colors.autoTextContrast;
    if (m_textBtn)
        m_textBtn->setEnabled(manualText);
    if (m_textHex)
        m_textHex->setEnabled(manualText);
}

void SettingsDialog::applyContrastStyle()
{
    const QColor surface = m_colors.surface.isValid() ? m_colors.surface : QColor("#f5f5f6");
    const QColor background = m_colors.appBg.isValid() ? m_colors.appBg : surface;
    const QColor border = m_colors.border.isValid() ? m_colors.border : QColor("#d3d7dd");
    const QColor accent = m_colors.accent.isValid() ? m_colors.accent : QColor("#3a2d97");
    const QColor text = m_colors.autoTextContrast
        ? contrastTextFor(surface)
        : (m_colors.text.isValid() ? m_colors.text : QColor("#000000"));
    const QColor bgText = m_colors.autoTextContrast ? contrastTextFor(background) : text;
    const QColor muted = blendColors(text, surface, 0.62);
    const QColor fieldBg = surface.lightness() < 128 ? surface.lighter(118) : QColor("#ffffff");
    const QColor fieldText = contrastTextFor(fieldBg);

    setStyleSheet(
        "SettingsDialog { background: " + background.name() + "; color: " + bgText.name() + "; }\n"
        "SettingsDialog QScrollArea, SettingsDialog QScrollArea > QWidget > QWidget { background: transparent; }\n"
        "SettingsDialog QGroupBox { background: " + surface.name() + "; color: " + text.name() + ";"
        " border: 1px solid " + border.name() + "; border-radius: 8px; margin-top: 10px; }\n"
        "SettingsDialog QGroupBox::title { subcontrol-origin: margin; left: 10px;"
        " padding: 0 4px; color: " + text.name() + "; }\n"
        "SettingsDialog QLabel { color: " + text.name() + "; }\n"
        "SettingsDialog QLabel#settingsHeading { color: " + bgText.name() + ";"
        " font-size: 15px; font-weight: bold; }\n"
        "SettingsDialog QLabel#settingsStrong { color: " + text.name() + "; font-weight: 600; }\n"
        "SettingsDialog QLabel#settingsHint, SettingsDialog QLabel#settingsHex { color: " + muted.name() + "; }\n"
        "SettingsDialog QLabel#settingsHex { font-family: Consolas, monospace; font-size: 13px; }\n"
        "SettingsDialog QLineEdit, SettingsDialog QComboBox, SettingsDialog QSpinBox {"
        " background: " + fieldBg.name() + "; color: " + fieldText.name() + ";"
        " border: 1px solid " + border.name() + "; border-radius: 6px; padding: 5px 7px; }\n"
        "SettingsDialog QSpinBox { padding-right: 28px; }\n"
        "SettingsDialog QSpinBox::up-button, SettingsDialog QSpinBox::down-button {"
        " subcontrol-origin: border; width: 20px; border-left: 1px solid " + border.name() + "; }\n"
        "SettingsDialog QSpinBox::up-button { subcontrol-position: top right; }\n"
        "SettingsDialog QSpinBox::down-button { subcontrol-position: bottom right; }\n"
        "SettingsDialog QCheckBox, SettingsDialog QRadioButton { color: " + text.name() + "; }\n"
        "SettingsDialog QDialogButtonBox QPushButton { background: " + accent.name() + ";"
        " color: " + contrastTextFor(accent).name() + "; border: none; border-radius: 6px;"
        " padding: 7px 14px; font-weight: 600; }\n");
}

// ── UI ────────────────────────────────────────────────────────────

void SettingsDialog::setupUi()
{
    auto* outer = new QVBoxLayout(this);
    outer->setSpacing(10);
    outer->setContentsMargins(12, 12, 12, 12);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget(scroll);
    auto* main = new QVBoxLayout(content);
    main->setSpacing(16);
    main->setContentsMargins(8, 8, 8, 8);
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);

    // ── Sekcja: Kolory ────────────────────────────────────────────
    auto* colorHeading = new QLabel("Paleta kolorów", this);
    colorHeading->setObjectName("settingsHeading");
    main->addWidget(colorHeading);

    auto* colorGroup = new QGroupBox("Kolory interfejsu", this);
    auto* form       = new QFormLayout(colorGroup);
    form->setSpacing(14);
    form->setContentsMargins(16, 20, 16, 16);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    addColorRow(form, "Kolor akcentu", &m_colors.accent,    m_accentBtn, m_accentHex);
    addColorRow(form, "Lewy panel",    &m_colors.leftPanel,  m_leftBtn,   m_leftHex);
    addColorRow(form, "Tło aplikacji", &m_colors.appBg,      m_bgBtn,     m_bgHex);

    addColorRow(form, "Powierzchnie",   &m_colors.surface,    m_surfaceBtn, m_surfaceHex);
    addColorRow(form, "Obramowania",    &m_colors.border,     m_borderBtn,  m_borderHex);
    addColorRow(form, "Tekst",          &m_colors.text,       m_textBtn,    m_textHex);

    m_autoTextContrastCheck = new QCheckBox("Automatyczny kontrast tekstu", this);
    m_autoTextContrastCheck->setChecked(m_colors.autoTextContrast);
    form->addRow("", m_autoTextContrastCheck);
    connect(m_autoTextContrastCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_colors.autoTextContrast = checked;
        updateTextControls();
    });
    updateTextControls();

    auto* notarityBtn = new QPushButton("Preset Notarity", this);
    notarityBtn->setCursor(Qt::PointingHandCursor);
    notarityBtn->setStyleSheet(
        "QPushButton { background: #3a2d97; color: white; border: none;"
        " border-radius: 6px; padding: 7px 12px; font-weight: 600; }"
        "QPushButton:hover { background: #2f247d; }");
    form->addRow("", notarityBtn);
    connect(notarityBtn, &QPushButton::clicked, this, [this]() {
        m_colors.accent = QColor("#3a2d97");
        m_colors.leftPanel = QColor("#d8e1e6");
        m_colors.appBg = QColor("#f5f5f6");
        m_colors.surface = QColor("#f5f5f6");
        m_colors.border = QColor("#d3d7dd");
        m_colors.text = QColor("#000000");
        m_colors.autoTextContrast = true;
        m_colors.pattern = PatternType::None;
        m_colors.patternMark = QColor("#adadc2");

        applyBtnStyle(m_accentBtn, m_colors.accent);
        m_accentHex->setText(m_colors.accent.name().toUpper());
        applyBtnStyle(m_leftBtn, m_colors.leftPanel);
        m_leftHex->setText(m_colors.leftPanel.name().toUpper());
        applyBtnStyle(m_bgBtn, m_colors.appBg);
        m_bgHex->setText(m_colors.appBg.name().toUpper());
        applyBtnStyle(m_surfaceBtn, m_colors.surface);
        m_surfaceHex->setText(m_colors.surface.name().toUpper());
        applyBtnStyle(m_borderBtn, m_colors.border);
        m_borderHex->setText(m_colors.border.name().toUpper());
        applyBtnStyle(m_textBtn, m_colors.text);
        m_textHex->setText(m_colors.text.name().toUpper());
        if (m_autoTextContrastCheck)
            m_autoTextContrastCheck->setChecked(m_colors.autoTextContrast);
        updateTextControls();
        applyBtnStyle(m_patternMarkBtn, m_colors.patternMark);
        m_patternMarkHex->setText(m_colors.patternMark.name().toUpper());
        if (m_patternGroup && m_patternGroup->button(static_cast<int>(PatternType::None)))
            m_patternGroup->button(static_cast<int>(PatternType::None))->setChecked(true);
    });

    main->addWidget(colorGroup);

    // ── Sekcja: Wzory ─────────────────────────────────────────────
    auto* patHeading = new QLabel("Wzory tła", this);
    patHeading->setObjectName("settingsHeading");
    main->addWidget(patHeading);

    auto* patGroup = new QGroupBox("Wzór paska i panelu bocznego", this);
    auto* patLayout = new QVBoxLayout(patGroup);
    patLayout->setContentsMargins(16, 16, 16, 16);
    patLayout->setSpacing(12);

    // Inicjalizuj kolor wzoru — jeśli brak w ustawieniach, pochodna od lewego panelu
    if (!m_colors.patternMark.isValid())
        m_colors.patternMark = PatternGen::autoMark(m_colors.leftPanel);

    m_patternGroup = new QButtonGroup(this);
    m_patternGroup->setExclusive(true);

    struct PatEntry { PatternType type; QString name; };
    const QList<PatEntry> patEntries = {
        { PatternType::None,      "Brak"     },
        { PatternType::Zebra,     "Zebra"    },
        { PatternType::Leopard,   "Panterka" },
        { PatternType::Butterfly, "Motylki"  },
        { PatternType::Dots,      "Kropki"   },
        { PatternType::Stars,     "Gwiazdki" },
    };

    auto* patRow = new QHBoxLayout();
    patRow->setSpacing(8);

    const QString accentHex = m_colors.accent.name();
    const QString btnStyle =
        "QToolButton {"
        "  border: 2px solid #ddd; border-radius: 8px;"
        "  background: #f4f6f9; padding: 4px; font-size: 11px; }"
        "QToolButton:hover {"
        "  border-color: " + accentHex + "; background: #eef4ff; }"
        "QToolButton:checked {"
        "  border: 2.5px solid " + accentHex + "; background: #dde8ff; }";

    for (const auto& e : patEntries) {
        QPixmap prev = PatternGen::preview(
            e.type, m_colors.leftPanel, m_colors.patternMark, QSize(62, 44));

        auto* btn = new QToolButton(this);
        btn->setCheckable(true);
        btn->setChecked(e.type == m_colors.pattern);
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setIcon(QIcon(prev));
        btn->setIconSize(QSize(62, 44));
        btn->setText(e.name);
        btn->setFixedSize(78, 76);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(btnStyle);

        m_patternGroup->addButton(btn, static_cast<int>(e.type));
        patRow->addWidget(btn);
    }
    patRow->addStretch();
    patLayout->addLayout(patRow);

    // Kolor wzoru
    auto* markRow = new QHBoxLayout();
    markRow->setSpacing(10);
    markRow->addWidget(new QLabel("Kolor wzoru:", this));

    m_patternMarkBtn = new QPushButton(this);
    m_patternMarkBtn->setFixedSize(44, 28);
    m_patternMarkBtn->setCursor(Qt::PointingHandCursor);
    applyBtnStyle(m_patternMarkBtn, m_colors.patternMark);

    m_patternMarkHex = new QLabel(m_colors.patternMark.name().toUpper(), this);
    m_patternMarkHex->setObjectName("settingsHex");

    auto* autoMarkBtn = new QPushButton("Auto ↺", this);
    autoMarkBtn->setFlat(true);
    autoMarkBtn->setCursor(Qt::PointingHandCursor);
    autoMarkBtn->setStyleSheet(
        "QPushButton { color: #666; font-size: 11px; text-decoration: underline;"
        " border: none; background: none; padding: 2px; }"
        "QPushButton:hover { color: #333; }");

    markRow->addWidget(m_patternMarkBtn);
    markRow->addWidget(m_patternMarkHex);
    markRow->addWidget(autoMarkBtn);
    markRow->addStretch();
    patLayout->addLayout(markRow);

    connect(m_patternMarkBtn, &QPushButton::clicked, this, [this]() {
        QColor chosen = QColorDialog::getColor(m_colors.patternMark, this, "Kolor wzoru");
        if (chosen.isValid()) {
            m_colors.patternMark = chosen;
            applyBtnStyle(m_patternMarkBtn, chosen);
            m_patternMarkHex->setText(chosen.name().toUpper());
        }
    });

    connect(autoMarkBtn, &QPushButton::clicked, this, [this]() {
        QColor auto_c = PatternGen::autoMark(m_colors.leftPanel);
        m_colors.patternMark = auto_c;
        applyBtnStyle(m_patternMarkBtn, auto_c);
        m_patternMarkHex->setText(auto_c.name().toUpper());
    });

    connect(m_patternGroup, &QButtonGroup::idClicked, this, [this](int id) {
        m_colors.pattern = static_cast<PatternType>(id);
    });

    main->addWidget(patGroup);

    // ── Sekcja: Wyświetlanie ──────────────────────────────────────
    auto* dispHeading = new QLabel("Wyświetlanie", this);
    dispHeading->setObjectName("settingsHeading");
    main->addWidget(dispHeading);

    auto* dispGroup  = new QGroupBox("Tryb okna", this);
    auto* dispLayout = new QVBoxLayout(dispGroup);
    dispLayout->setSpacing(10);
    dispLayout->setContentsMargins(16, 20, 16, 16);

    // Radio buttons trybu
    auto* modeGroup = new QButtonGroup(this);

    m_radioNormal     = new QRadioButton("Okno (rozmiar niestandardowy)", this);
    m_radioMaximized  = new QRadioButton("Zmaksymalizowane",               this);
    m_radioFullScreen = new QRadioButton("Pełny ekran",                    this);

    modeGroup->addButton(m_radioNormal,     0);
    modeGroup->addButton(m_radioMaximized,  1);
    modeGroup->addButton(m_radioFullScreen, 2);

    switch (m_display.mode) {
        case WindowMode::Normal:     m_radioNormal->setChecked(true);     break;
        case WindowMode::Maximized:  m_radioMaximized->setChecked(true);  break;
        case WindowMode::FullScreen: m_radioFullScreen->setChecked(true); break;
    }

    dispLayout->addWidget(m_radioNormal);
    dispLayout->addWidget(m_radioMaximized);
    dispLayout->addWidget(m_radioFullScreen);

    // Rozmiar okna (tylko dla trybu Normal)
    auto* sizeRow = new QHBoxLayout();
    sizeRow->setSpacing(6);

    auto* sizeLabel = new QLabel("Rozmiar:", this);
    m_widthSpin  = new QSpinBox(this);
    m_heightSpin = new QSpinBox(this);

    m_widthSpin->setRange(640, 3840);
    m_widthSpin->setValue(m_display.width);
    m_widthSpin->setSuffix(" px");
    m_widthSpin->setFixedWidth(112);

    m_heightSpin->setRange(480, 2160);
    m_heightSpin->setValue(m_display.height);
    m_heightSpin->setSuffix(" px");
    m_heightSpin->setFixedWidth(112);

    sizeRow->addWidget(sizeLabel);
    sizeRow->addWidget(m_widthSpin);
    sizeRow->addWidget(new QLabel("×", this));
    sizeRow->addWidget(m_heightSpin);
    sizeRow->addStretch();

    dispLayout->addLayout(sizeRow);

    // Przyciski preset
    auto* presetRow = new QHBoxLayout();
    presetRow->setSpacing(6);
    presetRow->addWidget(new QLabel("Presets:", this));

    struct Preset { int w, h; };
    const QList<Preset> presets = {
        {960, 660}, {1280, 800}, {1440, 900}, {1920, 1080}
    };
    for (const Preset& p : presets) {
        auto* btn = new QPushButton(
            QString("%1×%2").arg(p.w).arg(p.h), this);
        btn->setFixedHeight(24);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { font-size: 11px; padding: 2px 6px; }"
            "QPushButton:hover { background: #e0e8f5; }");
        const int w = p.w, h = p.h;
        connect(btn, &QPushButton::clicked, this, [this, w, h]() {
            m_radioNormal->setChecked(true);
            m_widthSpin->setValue(w);
            m_heightSpin->setValue(h);
            updateSizeEnabled();
        });
        presetRow->addWidget(btn);
    }
    presetRow->addStretch();
    dispLayout->addLayout(presetRow);

    main->addWidget(dispGroup);

    auto* profileHeading = new QLabel("Profil danych", this);
    profileHeading->setObjectName("settingsHeading");
    main->addWidget(profileHeading);

    auto* profileGroup = new QGroupBox("Dane uzytkownika", this);
    auto* profileLayout = new QVBoxLayout(profileGroup);
    profileLayout->setSpacing(10);
    profileLayout->setContentsMargins(16, 18, 16, 16);

    const DataProfile activeProfile = ProfileManager::profile(m_profile.activeProfileId);
    auto* activeLabel = new QLabel("Aktywny profil: " + activeProfile.name, this);
    activeLabel->setObjectName("settingsStrong");
    profileLayout->addWidget(activeLabel);

    auto* profileRow = new QHBoxLayout();
    profileRow->setSpacing(8);
    profileRow->addWidget(new QLabel("Profil po restarcie:", this));
    m_profileCombo = new QComboBox(this);
    for (const DataProfile& profile : ProfileManager::profiles())
        m_profileCombo->addItem(profile.name, profile.id);
    const int profileIndex = m_profileCombo->findData(m_profile.selectedProfileId);
    m_profileCombo->setCurrentIndex(profileIndex >= 0 ? profileIndex : 0);
    profileRow->addWidget(m_profileCombo, 1);
    profileLayout->addLayout(profileRow);

    auto* languageRow = new QHBoxLayout();
    languageRow->setSpacing(8);
    languageRow->addWidget(new QLabel("Język aplikacji:", this));
    m_languageCombo = new QComboBox(this);
    for (const AppLanguage& language : ProfileManager::languages())
        m_languageCombo->addItem(language.name, language.code);
    const int languageIndex = m_languageCombo->findData(
        ProfileManager::normalizeLanguageCode(m_profile.selectedLanguageCode));
    m_languageCombo->setCurrentIndex(languageIndex >= 0 ? languageIndex : 0);
    languageRow->addWidget(m_languageCombo, 1);
    profileLayout->addLayout(languageRow);

    m_askProfileAtStartup = new QCheckBox("Pytaj o profil przy starcie", this);
    m_askProfileAtStartup->setChecked(m_profile.askAtStartup);
    profileLayout->addWidget(m_askProfileAtStartup);

    auto* profileHint = new QLabel(
        "Zmiana profilu zostanie zastosowana po ponownym uruchomieniu aplikacji.",
        this);
    profileHint->setWordWrap(true);
    profileHint->setObjectName("settingsHint");
    profileLayout->addWidget(profileHint);

    main->addWidget(profileGroup);

    // Aktualizuj aktywność spinboxów przy zmianie trybu
    connect(modeGroup, &QButtonGroup::buttonClicked, this,
            [this](QAbstractButton*) { updateSizeEnabled(); });
    updateSizeEnabled();

    // ── Reset + OK/Anuluj ────────────────────────────────────────
    auto* resetBtn = new QPushButton("↺  Przywróć domyślne", this);
    resetBtn->setFlat(true);
    resetBtn->setCursor(Qt::PointingHandCursor);
    resetBtn->setStyleSheet(
        "QPushButton { color: #666; font-size: 12px; text-decoration: underline; "
        "border: none; background: none; padding: 2px; }"
        "QPushButton:hover { color: #333; }");
    connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::resetToDefaults);
    outer->addWidget(resetBtn, 0, Qt::AlignLeft);

    auto* btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setText("Zastosuj");
    btns->button(QDialogButtonBox::Cancel)->setText("Anuluj");
    connect(btns, &QDialogButtonBox::accepted, this, [this]() {
        // Zbierz DisplaySettings z UI przed zamknięciem
        if      (m_radioNormal->isChecked())     m_display.mode = WindowMode::Normal;
        else if (m_radioMaximized->isChecked())   m_display.mode = WindowMode::Maximized;
        else                                       m_display.mode = WindowMode::FullScreen;
        m_display.width  = m_widthSpin->value();
        m_display.height = m_heightSpin->value();
        if (m_profileCombo)
            m_profile.selectedProfileId = m_profileCombo->currentData().toString();
        if (m_languageCombo)
            m_profile.selectedLanguageCode = m_languageCombo->currentData().toString();
        if (m_askProfileAtStartup)
            m_profile.askAtStartup = m_askProfileAtStartup->isChecked();
        accept();
    });
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    outer->addWidget(btns);
}
