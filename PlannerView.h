#pragma once

#include <QWidget>
#include <QDate>

class QLabel;
class QPushButton;
class QButtonGroup;
class QScrollArea;
class QVBoxLayout;
class GoalsPanel;

class PlannerView : public QWidget
{
    Q_OBJECT

public:
    explicit PlannerView(QWidget* parent = nullptr);
    void setDate(const QDate& date);

private slots:
    void onSlotSizeChanged(int id);

private:
    void setupUi();
    void rebuildGrid();

    QDate         m_date;
    int           m_slotMinutes  = 60;   // domyślnie 1h

    QLabel*       m_dateLabel    = nullptr;
    QButtonGroup* m_slotGroup    = nullptr;
    QScrollArea*  m_scroll       = nullptr;
    QWidget*      m_gridWidget   = nullptr;
    QVBoxLayout*  m_gridLayout   = nullptr;
    GoalsPanel*   m_goalsPanel   = nullptr;
};
