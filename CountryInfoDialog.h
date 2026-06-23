#pragma once

#include <QDialog>
#include <QString>

class QVBoxLayout;
class QTextEdit;

class CountryInfoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CountryInfoDialog(const QString& name, const QString& isoCode,
                               QWidget* parent = nullptr);

signals:
    void centralBankRequested(const QString& name, double lat, double lon);

private:
    void addInfoRow(QVBoxLayout* root, const QString& label, const QString& value);
    void addLinkRow(QVBoxLayout* root, const QString& label, const QString& text, const QString& url);
    void loadNotes();
    void saveNotes();

    QString    m_isoCode;
    QTextEdit* m_notesEdit = nullptr;
};
