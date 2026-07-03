#include "CountryInfoDialog.h"
#include "CountryData.h"
#include "DialogUtils.h"
#include "LocalDataSources.h"
#include "ProfileManager.h"

#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QTextEdit>
#include <QVBoxLayout>

CountryInfoDialog::CountryInfoDialog(const QString& name, const QString& isoCode,
                                     QWidget* parent)
    : QDialog(parent)
{
    const CountryInfo info = CountryData::byIso(isoCode, name);
    m_isoCode = info.isoCode;

    setWindowTitle(info.name);
    setMinimumWidth(460);
    setMaximumWidth(560);
    DialogUtils::constrainToParent(this);
    setStyleSheet(
        "CountryInfoDialog { background: #ffffff; color: #1f2937; }"
        "QLabel { background: transparent; }"
        "QDialogButtonBox QPushButton { background: #f8fafc; color: #1f2937;"
        " border: 1px solid #cbd5e1; border-radius: 5px; padding: 6px 14px; }"
        "QDialogButtonBox QPushButton:hover { background: #e2e8f0; }"
        "QPushButton#centralBankBtn { background: #eff6ff; color: #1d4ed8;"
        " border: 1px solid #bfdbfe; border-radius: 6px; padding: 7px 12px;"
        " font-weight: 600; }"
        "QPushButton#centralBankBtn:hover { background: #dbeafe; }");

    auto* outer = new QVBoxLayout(this);
    outer->setSpacing(10);
    outer->setContentsMargins(12, 12, 12, 12);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget(scroll);
    auto* root = new QVBoxLayout(content);
    root->setSpacing(12);
    root->setContentsMargins(12, 8, 12, 8);
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);

    auto* nameLabel = new QLabel(info.name, this);
    nameLabel->setStyleSheet("font-size:22px; font-weight:bold; color:#111827;");
    root->addWidget(nameLabel);

    if (!m_isoCode.isEmpty()) {
        auto* isoLabel = new QLabel("Kod ISO: " + m_isoCode, this);
        isoLabel->setStyleSheet("color:#64748b; font-size:12px;");
        root->addWidget(isoLabel);
    }

    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#e5e7eb;");
    root->addWidget(sep);

    addInfoRow(root, "Stolica", info.capital);
    addInfoRow(root, "Populacja", info.population);
    addInfoRow(root, "PKB", info.gdp);
    addInfoRow(root, "Waluta", info.currency);
    addInfoRow(root, "Jezyki urzedowe", info.languages);
    addInfoRow(root, "Kontynent", info.continent);
    addInfoRow(root, "Powierzchnia", info.area);

    if (!info.centralBankName.isEmpty()) {
        auto* bankSep = new QFrame(this);
        bankSep->setFrameShape(QFrame::HLine);
        bankSep->setStyleSheet("color:#e5e7eb;");
        root->addWidget(bankSep);

        auto* bankTitle = new QLabel("Bank centralny", this);
        bankTitle->setStyleSheet("font-weight:700; color:#334155; font-size:14px; padding-top:4px;");
        root->addWidget(bankTitle);

        addInfoRow(root, "Nazwa", info.centralBankName);
        addInfoRow(root, "Miasto", info.centralBankCity);
        addLinkRow(root, "Strona", info.centralBankUrl, info.centralBankUrl);

        if (info.hasCentralBankLocation) {
            auto* showBankBtn = new QPushButton("Pokaz bank centralny na mapie", this);
            showBankBtn->setObjectName("centralBankBtn");
            root->addWidget(showBankBtn);
            connect(showBankBtn, &QPushButton::clicked, this, [this, info]() {
                saveNotes();
                emit centralBankRequested(info.centralBankName,
                                          info.centralBankLat,
                                          info.centralBankLon);
                accept();
            });
        }
    }

    const LocalDataSourceInfo dataSource = LocalDataSources::sourceForIso(m_isoCode);
    if (!dataSource.iso.isEmpty()) {
        auto* dataSep = new QFrame(this);
        dataSep->setFrameShape(QFrame::HLine);
        dataSep->setStyleSheet("color:#e5e7eb;");
        root->addWidget(dataSep);

        auto* dataTitle = new QLabel("Dane lokalne", this);
        dataTitle->setStyleSheet("font-weight:700; color:#334155; font-size:14px; padding-top:4px;");
        root->addWidget(dataTitle);

        addInfoRow(root, "Zrodlo", dataSource.sourceName);
        addInfoRow(root, "Status API", dataSource.apiStatus);
        addInfoRow(root, "Typ API", dataSource.apiType);
        addLinkRow(root, "Baza danych", dataSource.websiteUrl, dataSource.websiteUrl);
        addLinkRow(root, "Endpoint API", dataSource.apiUrl, dataSource.apiUrl);
    }

    auto* noteTitle = new QLabel("Notatki", this);
    noteTitle->setStyleSheet("font-weight:600; color:#334155; font-size:13px; padding-top:8px;");
    root->addWidget(noteTitle);

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMinimumHeight(90);
    m_notesEdit->setPlaceholderText("Twoje notatki o kraju...");
    m_notesEdit->setStyleSheet(
        "QTextEdit { border:1px solid #cbd5e1; border-radius:6px; padding:8px;"
        " background:#fbfcfe; color:#1f2937; }");
    root->addWidget(m_notesEdit);
    loadNotes();

    auto* hint = new QLabel(
        "Dane kraju pochodza z lokalnej bazy aplikacji. Notatki sa zapisywane lokalnie w ustawieniach.",
        this);
    hint->setStyleSheet("color:#64748b; font-size:11px; font-style:italic; padding-top:4px;");
    hint->setWordWrap(true);
    root->addWidget(hint);

    root->addStretch();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, [this]() {
        saveNotes();
        reject();
    });
    outer->addWidget(buttons);
}

void CountryInfoDialog::addInfoRow(QVBoxLayout* root, const QString& label, const QString& value)
{
    auto* rowW = new QWidget(this);
    auto* rowL = new QHBoxLayout(rowW);
    rowL->setContentsMargins(0, 2, 0, 2);
    rowL->setSpacing(8);

    auto* lbl = new QLabel(label + ":", rowW);
    lbl->setFixedWidth(130);
    lbl->setStyleSheet("font-weight:600; color:#334155; font-size:13px;");

    auto* val = new QLabel(value.isEmpty() ? "-" : value, rowW);
    val->setWordWrap(true);
    val->setStyleSheet("color:#111827; font-size:13px;");

    rowL->addWidget(lbl);
    rowL->addWidget(val, 1);
    root->addWidget(rowW);
}

void CountryInfoDialog::addLinkRow(QVBoxLayout* root, const QString& label,
                                   const QString& text, const QString& url)
{
    if (url.trimmed().isEmpty()) {
        addInfoRow(root, label, text);
        return;
    }

    auto* rowW = new QWidget(this);
    auto* rowL = new QHBoxLayout(rowW);
    rowL->setContentsMargins(0, 2, 0, 2);
    rowL->setSpacing(8);

    auto* lbl = new QLabel(label + ":", rowW);
    lbl->setFixedWidth(130);
    lbl->setStyleSheet("font-weight:600; color:#334155; font-size:13px;");

    const QString href = url.trimmed().toHtmlEscaped();
    const QString caption = (text.trimmed().isEmpty() ? url.trimmed() : text.trimmed()).toHtmlEscaped();
    auto* val = new QLabel(QString("<a href=\"%1\">%2</a>").arg(href, caption), rowW);
    val->setWordWrap(true);
    val->setTextFormat(Qt::RichText);
    val->setTextInteractionFlags(Qt::TextBrowserInteraction);
    val->setOpenExternalLinks(true);
    val->setStyleSheet("color:#1d4ed8; font-size:13px;");

    rowL->addWidget(lbl);
    rowL->addWidget(val, 1);
    root->addWidget(rowW);
}

void CountryInfoDialog::loadNotes()
{
    if (!m_notesEdit || m_isoCode.isEmpty()) return;

    QSettings settings("Kryst", ProfileManager::profileSettingsAppName());
    m_notesEdit->setPlainText(settings.value("countryNotes/" + m_isoCode).toString());
}

void CountryInfoDialog::saveNotes()
{
    if (!m_notesEdit || m_isoCode.isEmpty()) return;

    QSettings settings("Kryst", ProfileManager::profileSettingsAppName());
    settings.setValue("countryNotes/" + m_isoCode, m_notesEdit->toPlainText());
}
