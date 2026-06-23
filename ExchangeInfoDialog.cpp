#include "ExchangeInfoDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>

ExchangeInfoDialog::ExchangeInfoDialog(const ExchangeMarker& ex, QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint)
{
    setWindowTitle("Informacje o giełdzie");
    setMinimumWidth(480);
    setMaximumWidth(640);

    const QSettings settings("Kryst", "PlanerDnia");
    const QString textColor = settings.value("colors/dialogText", "#1a2535").toString();

    auto* root = new QVBoxLayout(this);
    root->setSpacing(0);
    root->setContentsMargins(0, 0, 0, 0);

    // ── Header ────────────────────────────────────────────────────────────
    auto* header = new QWidget(this);
    header->setStyleSheet("QWidget { background: #0d1a2e; }");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(22, 18, 22, 18);
    headerLayout->setSpacing(14);

    auto* iconLbl = new QLabel("🏛", header);
    iconLbl->setStyleSheet("font-size: 26px; background: transparent;");
    iconLbl->setFixedSize(38, 38);
    iconLbl->setAlignment(Qt::AlignCenter);

    auto* nameLbl = new QLabel(ex.name, header);
    nameLbl->setStyleSheet(
        "color: #e0ecfc; font-size: 15px; font-weight: bold; background: transparent;");
    nameLbl->setWordWrap(true);

    headerLayout->addWidget(iconLbl);
    headerLayout->addWidget(nameLbl, 1);
    root->addWidget(header);

    // ── Badges: type + region ─────────────────────────────────────────────
    if (!ex.type.isEmpty() || !ex.region.isEmpty()) {
        auto* badgeRow = new QWidget(this);
        badgeRow->setStyleSheet("QWidget { background: #111f35; }");
        auto* badgeLayout = new QHBoxLayout(badgeRow);
        badgeLayout->setContentsMargins(22, 8, 22, 8);
        badgeLayout->setSpacing(8);

        auto makeBadge = [&](const QString& text, const QString& bg, const QString& fg) {
            auto* lbl = new QLabel(text, badgeRow);
            lbl->setStyleSheet(QString(
                "background: %1; color: %2; border-radius: 10px;"
                " padding: 2px 11px; font-size: 11px; font-weight: bold;").arg(bg, fg));
            return lbl;
        };

        if (!ex.type.isEmpty())
            badgeLayout->addWidget(makeBadge(ex.type, "#1a3a5c", "#7cc8f8"));
        if (!ex.region.isEmpty())
            badgeLayout->addWidget(makeBadge(ex.region, "#1a3d28", "#7dcea0"));

        badgeLayout->addStretch();
        root->addWidget(badgeRow);
    }

    // ── Separator ─────────────────────────────────────────────────────────
    auto* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("color: #dde3ec;");
    root->addWidget(sep1);

    // ── Info body ─────────────────────────────────────────────────────────
    auto* body = new QWidget(this);
    body->setStyleSheet("QWidget { background: #ffffff; }");
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(22, 16, 22, 16);
    bodyLayout->setSpacing(11);

    auto addRow = [&](const QString& icon, const QString& label, const QString& value) {
        if (value.isEmpty()) return;
        auto* row = new QHBoxLayout();
        row->setSpacing(10);

        auto* iconL = new QLabel(icon, body);
        iconL->setFixedWidth(22);
        iconL->setStyleSheet("font-size: 15px; background: transparent;");
        iconL->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

        auto* labelL = new QLabel(label + ":", body);
        labelL->setFixedWidth(72);
        labelL->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        labelL->setStyleSheet("color: #888; font-size: 12px; padding-top: 1px;"
                              " background: transparent;");

        auto* valueL = new QLabel(value, body);
        valueL->setStyleSheet(
            QString("color: %1; font-size: 13px; background: transparent;")
                .arg(textColor));
        valueL->setWordWrap(true);

        row->addWidget(iconL);
        row->addWidget(labelL);
        row->addWidget(valueL, 1);
        bodyLayout->addLayout(row);
    };

    addRow("🌍", "Kraj",    ex.country);
    addRow("🏙", "Miasto",  ex.city);
    if (ex.foundedYear > 0)
        addRow("📅", "Założona",
               QString::number(ex.foundedYear) + " r.");
    addRow("🏢", "Spółki",
           ex.listedCompanies > 0
               ? QString("~%L1").arg(ex.listedCompanies)
               : QString("Liczba do uzupełnienia"));
    addRow("📍", "Adres",   ex.address);
    addRow("📐", "Wsp.",
           QString("%1°N   %2°E")
               .arg(ex.lat, 0, 'f', 4)
               .arg(ex.lon, 0, 'f', 4));

    // Website jako klikalny link
    if (!ex.website.isEmpty()) {
        auto* row = new QHBoxLayout();
        row->setSpacing(10);

        auto* iconL = new QLabel("🌐", body);
        iconL->setFixedWidth(22);
        iconL->setStyleSheet("font-size: 15px;");
        iconL->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

        auto* labelL = new QLabel("Strona:", body);
        labelL->setFixedWidth(72);
        labelL->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        labelL->setStyleSheet("color: #888; font-size: 12px; padding-top: 1px;");

        auto* linkL = new QLabel(
            QString("<a href=\"%1\" style=\"color:#1a6fb5; text-decoration:none;\">%1</a>")
                .arg(ex.website.toHtmlEscaped()),
            body);
        linkL->setOpenExternalLinks(true);
        linkL->setTextFormat(Qt::RichText);
        linkL->setWordWrap(true);
        linkL->setStyleSheet("font-size: 13px; background: transparent;");

        row->addWidget(iconL);
        row->addWidget(labelL);
        row->addWidget(linkL, 1);
        bodyLayout->addLayout(row);
    }

    root->addWidget(body, 1);

    // ── Separator ─────────────────────────────────────────────────────────
    auto* sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("color: #dde3ec;");
    root->addWidget(sep2);

    // ── Footer buttons ────────────────────────────────────────────────────
    auto* footer = new QWidget(this);
    footer->setStyleSheet("QWidget { background: #ffffff; }");
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(22, 12, 22, 14);
    footerLayout->setSpacing(8);
    footerLayout->addStretch();

    if (!ex.website.isEmpty()) {
        const QString url = ex.website;
        auto* visitBtn = new QPushButton("🌐  Odwiedź stronę", footer);
        visitBtn->setStyleSheet(
            "QPushButton { background: #1a6fb5; color: white; border: none;"
            " border-radius: 6px; padding: 8px 18px; font-size: 13px; }"
            "QPushButton:hover { background: #155fa0; }"
            "QPushButton:pressed { background: #104c85; }");
        connect(visitBtn, &QPushButton::clicked, this, [url]() {
            QDesktopServices::openUrl(QUrl(url));
        });
        footerLayout->addWidget(visitBtn);
    }

    auto* closeBtn = new QPushButton("Zamknij", footer);
    closeBtn->setStyleSheet(
        "QPushButton { background: #f0f2f5; color: #333; border: none;"
        " border-radius: 6px; padding: 8px 18px; font-size: 13px; }"
        "QPushButton:hover { background: #e2e5ea; }"
        "QPushButton:pressed { background: #d4d8de; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    footerLayout->addWidget(closeBtn);

    root->addWidget(footer);

    adjustSize();
}
