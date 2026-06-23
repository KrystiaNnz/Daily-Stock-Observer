#pragma once

#include <QDialog>
#include <QColor>
#include "PatternGen.h"   // defines PatternType

class QPushButton;
class QToolButton;
class QLabel;
class QFormLayout;
class QRadioButton;
class QSpinBox;
class QButtonGroup;

// ── Kolory aplikacji ───────────────────────────────────────────────
struct AppColors {
    QColor      accent      { "#3a7bd5" };
    QColor      leftPanel   { "#1e2a3a" };
    QColor      appBg       { "#f8f9fb" };
    QColor      dialogText  { "#1a2535" };  // kolor tekstu w oknach dialogowych
    PatternType pattern     { PatternType::None };
    QColor      patternMark {};   // invalid = auto-derive from leftPanel

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

// ── Dialog ustawień ───────────────────────────────────────────────
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(const AppColors&      colors,
                            const DisplaySettings& display,
                            QWidget* parent = nullptr);

    AppColors       colors()  const { return m_colors;  }
    DisplaySettings display() const { return m_display; }

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

    AppColors       m_colors;
    DisplaySettings m_display;

    // Kolory
    QPushButton* m_accentBtn = nullptr;
    QLabel*      m_accentHex = nullptr;
    QPushButton* m_leftBtn   = nullptr;
    QLabel*      m_leftHex   = nullptr;
    QPushButton* m_bgBtn     = nullptr;
    QLabel*      m_bgHex     = nullptr;
    QPushButton* m_textBtn   = nullptr;
    QLabel*      m_textHex   = nullptr;

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
};
