#pragma once

#include <QWidget>

class QLineEdit;
class QVBoxLayout;

class GoalsListView : public QWidget
{
    Q_OBJECT

public:
    explicit GoalsListView(QWidget* parent = nullptr);
    void refresh();

private slots:
    void onSearch(const QString& text);

private:
    void setupUi();
    void populate(const QString& filter = {});

    QLineEdit*   m_searchEdit    = nullptr;
    QWidget*     m_listContainer = nullptr;
    QVBoxLayout* m_listLayout    = nullptr;
};
