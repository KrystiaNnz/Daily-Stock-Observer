#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QFrame;
class QProgressBar;

class MainHubPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MainHubPanel(QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void navigateRequested(int index);

private:
    QFrame* createModuleCard(const QString& title,
                             const QString& body,
                             const QString& buttonText,
                             int targetIndex);
    void setupUi();

    QLabel* m_dateLabel = nullptr;
    QLabel* m_profileLabel = nullptr;
    QLabel* m_eventsMetric = nullptr;
    QLabel* m_goalsMetric = nullptr;
    QLabel* m_assetsMetric = nullptr;
    QLabel* m_portfolioCostMetric = nullptr;
    QLabel* m_levelLabel = nullptr;
    QLabel* m_xpLabel = nullptr;
    QLabel* m_streakLabel = nullptr;
    QLabel* m_completedStepsLabel = nullptr;
    QProgressBar* m_xpProgress = nullptr;
};
