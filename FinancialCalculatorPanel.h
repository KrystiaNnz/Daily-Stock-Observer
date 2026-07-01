#pragma once

#include <QWidget>
#include <QProcess>

class QComboBox;
class QCheckBox;
class QButtonGroup;
class QDoubleSpinBox;
class QFrame;
class QLabel;
class QPushButton;
class QRadioButton;
class QStackedWidget;
class QTextEdit;

class FinancialCalculatorPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FinancialCalculatorPanel(QWidget* parent = nullptr);

public slots:
    void refreshPortfolioAssets();

private slots:
    void onSourceChanged(int index);
    void onModeChanged(int index);
    void loadSelectedPortfolioAsset();
    void calculate();
    void generateExercise();
    void submitExerciseAnswer();
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

private:
    void setupUi();
    void populateExerciseDepartments(bool detailed);
    void setBusy(bool busy);
    void setExerciseBusy(bool busy);
    QString scriptPath() const;
    QString formatPercent(double value) const;

    enum class PendingOperation { None, Calculation, ExerciseGeneration };

    QComboBox*      m_sourceCombo = nullptr;
    QComboBox*      m_portfolioCombo = nullptr;
    QPushButton*    m_loadPortfolioBtn = nullptr;
    QLabel*         m_portfolioStatusLabel = nullptr;
    QComboBox*      m_modeCombo = nullptr;
    QStackedWidget* m_inputsStack = nullptr;

    QDoubleSpinBox* m_profitSpin = nullptr;
    QDoubleSpinBox* m_investmentSpin = nullptr;
    QDoubleSpinBox* m_buyPriceSpin = nullptr;
    QDoubleSpinBox* m_sellPriceSpin = nullptr;
    QDoubleSpinBox* m_dividendSpin = nullptr;
    QDoubleSpinBox* m_inflationSpin = nullptr;

    QPushButton* m_calculateBtn = nullptr;
    QLabel*      m_nominalResult = nullptr;
    QLabel*      m_realResult = nullptr;
    QLabel*      m_formulaLabel = nullptr;
    QTextEdit*   m_detailsBox = nullptr;

    QComboBox*      m_exerciseDepartmentCombo = nullptr;
    QCheckBox*      m_detailedExerciseTypesCheck = nullptr;
    QPushButton*    m_generateExerciseBtn = nullptr;
    QLabel*         m_exerciseTopicLabel = nullptr;
    QTextEdit*      m_exerciseQuestionBox = nullptr;
    QFrame*         m_exerciseChoicesFrame = nullptr;
    QButtonGroup*   m_exerciseChoiceGroup = nullptr;
    QRadioButton*   m_choiceA = nullptr;
    QRadioButton*   m_choiceB = nullptr;
    QRadioButton*   m_choiceC = nullptr;
    QRadioButton*   m_choiceD = nullptr;
    QLabel*         m_exerciseNumericAnswerLabel = nullptr;
    QDoubleSpinBox* m_exerciseAnswerSpin = nullptr;
    QPushButton*    m_submitExerciseBtn = nullptr;
    QLabel*         m_exerciseFeedbackLabel = nullptr;

    PendingOperation m_pendingOperation = PendingOperation::None;
    double  m_exerciseAnswer = 0.0;
    double  m_exerciseTolerance = 0.01;
    QString m_exerciseAnswerUnit;
    QString m_exerciseExplanation;
    QString m_exerciseReference;
    QString m_exerciseCorrectChoice;
    QString m_exerciseAnswerMode = "numeric";
    int     m_exerciseXpReward = 5;
    bool    m_hasActiveExercise = false;

    QProcess* m_process = nullptr;
};
