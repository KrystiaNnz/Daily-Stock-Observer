#pragma once

#include <QMainWindow>
#include <QDate>
#include <QList>
#include "SettingsDialog.h"   // dostarcza też AppColors

class QCalendarWidget;
class QListWidget;
class QLabel;
class QPushButton;
class QStackedWidget;
class GoalsPanel;
class PortfolioPanel;
class MapPanel;
class FinancialCalculatorPanel;
class MainHubPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onDateSelected(const QDate& date);
    void onAddEvent();
    void onDeleteEvent();
    void onSettings();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUi();
    void setupConnections();
    void switchTab(int index);
    void refreshEvents();
    void applyStyle();
    void loadColors();
    void saveColors();
    void loadDisplaySettings();
    void saveDisplaySettings();
    void applyDisplaySettings();

    QStackedWidget*      m_stack         = nullptr;
    QList<QPushButton*>  m_tabBtns;
    QWidget*             m_tabBarWidget  = nullptr;
    QWidget*             m_leftPanelWidget = nullptr;

    QCalendarWidget* m_calendar      = nullptr;
    QListWidget*     m_eventList     = nullptr;
    QLabel*          m_dateLabel     = nullptr;
    QPushButton*     m_addBtn        = nullptr;
    QPushButton*     m_deleteBtn     = nullptr;
    QPushButton*     m_settingsBtn   = nullptr;
    MainHubPanel*    m_mainHubPanel  = nullptr;
    GoalsPanel*      m_goalsPanel    = nullptr;
    PortfolioPanel*  m_portfolioPanel = nullptr;
    MapPanel*        m_mapPanel       = nullptr;
    FinancialCalculatorPanel* m_calculatorPanel = nullptr;

    QDate           m_selectedDate;
    AppColors       m_colors;
    DisplaySettings m_display;
};
