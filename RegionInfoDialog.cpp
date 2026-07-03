#include "RegionInfoDialog.h"
#include "DialogUtils.h"

#include <QDialogButtonBox>
#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

RegionInfoDialog::RegionInfoDialog(const SubdivisionInfo& region, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(region.name);
    setMinimumSize(420, 260);
    DialogUtils::constrainToParent(this);
    setStyleSheet(
        "RegionInfoDialog { background: #ffffff; color: #1f2937; }"
        "QLabel { background: transparent; }"
        "QDialogButtonBox QPushButton { background: #f8fafc; color: #1f2937;"
        " border: 1px solid #cbd5e1; border-radius: 5px; padding: 6px 14px; }"
        "QDialogButtonBox QPushButton:hover { background: #e2e8f0; }");

    auto* outer = new QVBoxLayout(this);
    outer->setSpacing(10);
    outer->setContentsMargins(12, 12, 12, 12);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget(scroll);
    auto* root = new QVBoxLayout(content);
    root->setContentsMargins(12, 8, 12, 8);
    root->setSpacing(12);
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);

    auto* title = new QLabel(region.name, this);
    title->setStyleSheet("font-size:22px; font-weight:bold; color:#111827;");
    title->setWordWrap(true);
    root->addWidget(title);

    QString subtitle;
    if (!region.type.isEmpty())
        subtitle += region.type;
    if (!region.code.isEmpty()) {
        if (!subtitle.isEmpty())
            subtitle += " | ";
        subtitle += region.code;
    }
    if (!subtitle.isEmpty()) {
        auto* meta = new QLabel(subtitle, this);
        meta->setStyleSheet("color:#64748b; font-size:12px;");
        root->addWidget(meta);
    }

    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#e5e7eb;");
    root->addWidget(sep);

    root->addStretch();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    outer->addWidget(buttons);
}
