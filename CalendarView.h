#pragma once

#include <QWidget>
#include <QDate>

class QCalendarWidget;
class QListWidget;
class QLabel;
class QPushButton;

class CalendarView : public QWidget
{
    Q_OBJECT

public:
    explicit CalendarView(QWidget* parent = nullptr);
    void setDate(const QDate& date);
    QDate selectedDate() const { return m_selectedDate; }

signals:
    void dateChanged(const QDate& date);

private slots:
    void onDateSelected();
    void onAddEvent();
    void onDeleteEvent();

private:
    void setupUi();
    void setupConnections();
    void refreshEvents();

    QCalendarWidget* m_calendar  = nullptr;
    QListWidget*     m_eventList = nullptr;
    QLabel*          m_dateLabel = nullptr;
    QPushButton*     m_addBtn    = nullptr;
    QPushButton*     m_deleteBtn = nullptr;

    QDate m_selectedDate;
};
