#pragma once

#include <QDialog>
#include <QColor>
#include <QString>
#include "PatternGen.h"   // defines PatternType

class QPushButton;
class QToolButton;
class QLabel;
class QFormLayout;
class QRadioButton;
class QSpinBox;
class QButtonGroup;
class QComboBox;
class QCheckBox;

// ── Kolory aplikacji ───────────────────────────────────────────────
struct AppColors {
    QColor      accent      { "#3a2d97" };
    QColor      leftPanel   { "#d8e1e6" };
    QColor      appBg       { "#f5f5f6" };
    QColor      surface     { "#f5f5f6" };
    QColor      border      { "#d3d7dd" };
    QColor      text        { "#000000" };
    bool        autoTextContrast = true;
    PatternType pattern     { PatternType::None };
    QColor      patternMark { "#adadc2" };   // invalid = auto-derive from leftPanel

    static AppColors defaults() { return {}; }
};

// ── Ustawienia wyświetlania ────────────────────────────────────────
enum class WindowMode { Normal, Maximized, FullScreen };

struct DisplaySettings {
    WindowMode mode   = WindowMode::Normal;
    int        width  = 960;
    int        height = 660;

    static DisplaySettings defaults() { return {}; }
};

struct ProfileSettings {
    QString activeProfileId;
    QString selectedProfileId;
    bool askAtStartup = false;
};

// ── Dialog ustawień ───────────────────────────────────────────────
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(const AppColors&      colors,
                            const DisplaySettings& display,
                            const ProfileSettings& profile,
                            QWidget* parent = nullptr);

    AppColors       colors()  const { return m_colors;  }
    DisplaySettings display() const { return m_display; }
    ProfileSettings profile() const { return m_profile; }

private:
    void setupUi();
    void addColorRow(QFormLayout*  form,
                     const QString& label,
                     QColor*        colorPtr,
                     QPushButton*&  outBtn,
                     QLabel*&       outHex);
    static void applyBtnStyle(QPushButton* btn, const QColor& color);
    void resetToDefaults();
    void updateSizeEnabled();    // włącz/wyłącz spinboxy wg trybu
    void updateTextControls();
    void applyContrastStyle();

    AppColors       m_colors;
    DisplaySettings m_display;
    ProfileSettings m_profile;

    // Kolory
    QPushButton* m_accentBtn = nullptr;
    QLabel*      m_accentHex = nullptr;
    QPushButton* m_leftBtn   = nullptr;
    QLabel*      m_leftHex   = nullptr;
    QPushButton* m_bgBtn     = nullptr;
    QLabel*      m_bgHex     = nullptr;
    QPushButton* m_surfaceBtn = nullptr;
    QLabel*      m_surfaceHex = nullptr;
    QPushButton* m_borderBtn  = nullptr;
    QLabel*      m_borderHex  = nullptr;
    QPushButton* m_textBtn    = nullptr;
    QLabel*      m_textHex    = nullptr;
    QCheckBox*   m_autoTextContrastCheck = nullptr;

    // Wzory
    QButtonGroup* m_patternGroup       = nullptr;
    QPushButton*  m_patternMarkBtn     = nullptr;
    QLabel*       m_patternMarkHex     = nullptr;

    // Wyświetlanie
    QRadioButton* m_radioNormal    = nullptr;
    QRadioButton* m_radioMaximized = nullptr;
    QRadioButton* m_radioFullScreen= nullptr;
    QSpinBox*     m_widthSpin      = nullptr;
    QSpinBox*     m_heightSpin     = nullptr;

    // Profil danych
    QComboBox* m_profileCombo = nullptr;
    QCheckBox* m_askProfileAtStartup = nullptr;
};
