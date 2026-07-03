#include "DialogUtils.h"

#include <QApplication>
#include <QDialog>
#include <QGuiApplication>
#include <QRect>
#include <QScreen>
#include <QTimer>
#include <QWidget>

namespace {
QRect boundsForDialog(QDialog* dialog)
{
    if (dialog && dialog->parentWidget())
        return dialog->parentWidget()->window()->frameGeometry();

    if (QScreen* screen = QGuiApplication::primaryScreen())
        return screen->availableGeometry();

    return QRect(0, 0, 960, 660);
}
}

void DialogUtils::constrainToParent(QDialog* dialog)
{
    if (!dialog)
        return;

    QTimer::singleShot(0, dialog, [dialog]() {
        const QRect bounds = boundsForDialog(dialog);
        const int maxW = qMax(360, int(bounds.width() * 0.92));
        const int maxH = qMax(320, int(bounds.height() * 0.92));

        dialog->setMaximumSize(maxW, maxH);
        dialog->adjustSize();

        QSize size = dialog->size();
        size.setWidth(qMin(size.width(), maxW));
        size.setHeight(qMin(size.height(), maxH));
        dialog->resize(size);

        const int x = bounds.left() + (bounds.width() - dialog->width()) / 2;
        const int y = bounds.top() + (bounds.height() - dialog->height()) / 2;
        dialog->move(qMax(bounds.left(), x), qMax(bounds.top(), y));
    });
}
