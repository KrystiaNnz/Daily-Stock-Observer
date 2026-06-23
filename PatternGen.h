#pragma once
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QtMath>

enum class PatternType { None=0, Zebra=1, Leopard=2, Butterfly=3, Dots=4, Stars=5 };

namespace PatternGen {

inline QColor autoMark(const QColor& base)
{
    if (base.lightness() < 128)
        return QColor::fromHsv(
            base.hsvHue() < 0 ? 0 : base.hsvHue(),
            qMax(base.hsvSaturation() - 30, 0),
            qMin(base.value() + 65, 255));
    else
        return QColor::fromHsv(
            base.hsvHue() < 0 ? 0 : base.hsvHue(),
            qMin(base.hsvSaturation() + 20, 255),
            qMax(base.value() - 60, 0));
}

// ── Zebra: poziome faliste paski ─────────────────────────────────────
inline QPixmap makeZebra(const QColor& base, const QColor& mark)
{
    const int W = 100, H = 44;
    QPixmap pm(W, H);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(pm.rect(), mark);       // cały kafelek kolorem mark

    // Pasek base z falistą dolną krawędzią (bezier, wejście i wyjście na y=H/2)
    QPainterPath band;
    band.moveTo(0, 0);
    band.lineTo(W, 0);
    band.lineTo(W, H / 2);
    band.cubicTo(W * 0.70, H / 2 - 11,
                 W * 0.30, H / 2 + 11,
                 0,        H / 2);
    band.closeSubpath();

    p.setPen(Qt::NoPen);
    p.fillPath(band, base);
    return pm;
}

// ── Panterka: plamy-rozetki ───────────────────────────────────────────
inline QPixmap makeLeopard(const QColor& base, const QColor& mark)
{
    const int T = 64;
    QPixmap pm(T, T);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(pm.rect(), base);
    p.setPen(Qt::NoPen);
    p.setBrush(mark);

    struct Spot { float cx, cy, rx, ry, angle; };
    const QList<Spot> spots {
        {13, 11,  9, 6,  15},
        {47, 10,  7, 5, -20},
        {27, 38,  8, 5,  40},
        {55, 45, 10, 6,   5},
        { 7, 53,  6, 4,  30},
        {56, 25,  5, 3, -10},
    };
    for (const auto& s : spots) {
        p.save();
        p.translate(s.cx, s.cy);
        p.rotate(s.angle);
        p.drawEllipse(QPointF(0, 0), (double)s.rx, (double)s.ry);
        p.restore();
    }
    return pm;
}

// ── Motylki: gładkie skrzydła bezier ─────────────────────────────────
inline QPixmap makeButterfly(const QColor& base, const QColor& mark)
{
    const int T = 64;
    QPixmap pm(T, T);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(pm.rect(), base);
    p.setPen(Qt::NoPen);
    p.setBrush(mark);

    const double cx = T / 2.0, cy = T / 2.0;

    QPainterPath wings;

    // Górne lewe skrzydło
    wings.moveTo(cx, cy - 2);
    wings.cubicTo(cx - 5, cy - 15,  cx - 22, cy - 13,  cx - 19, cy - 3);
    wings.cubicTo(cx - 16, cy + 4,  cx - 5,  cy + 2,   cx,      cy - 2);

    // Dolne lewe skrzydło
    wings.moveTo(cx, cy + 2);
    wings.cubicTo(cx - 3, cy + 9,   cx - 15, cy + 14,  cx - 13, cy + 6);
    wings.cubicTo(cx - 10, cy + 2,  cx - 3,  cy + 2,   cx,      cy + 2);

    // Górne prawe skrzydło (lustrzane)
    wings.moveTo(cx, cy - 2);
    wings.cubicTo(cx + 5,  cy - 15, cx + 22, cy - 13, cx + 19, cy - 3);
    wings.cubicTo(cx + 16, cy + 4,  cx + 5,  cy + 2,  cx,      cy - 2);

    // Dolne prawe skrzydło (lustrzane)
    wings.moveTo(cx, cy + 2);
    wings.cubicTo(cx + 3,  cy + 9,  cx + 15, cy + 14, cx + 13, cy + 6);
    wings.cubicTo(cx + 10, cy + 2,  cx + 3,  cy + 2,  cx,      cy + 2);

    p.fillPath(wings, mark);

    // Tułów
    p.setBrush(mark.darker(150));
    p.drawEllipse(QPointF(cx, cy), 1.5, 6.0);
    // Czułki
    p.setPen(QPen(mark.darker(150), 1.0));
    p.drawLine(QPointF(cx, cy - 7), QPointF(cx - 6, cy - 15));
    p.drawLine(QPointF(cx, cy - 7), QPointF(cx + 6, cy - 15));
    p.setPen(Qt::NoPen);
    p.setBrush(mark.darker(150));
    p.drawEllipse(QPointF(cx - 6, cy - 15), 1.5, 1.5);
    p.drawEllipse(QPointF(cx + 6, cy - 15), 1.5, 1.5);

    return pm;
}

// ── Kropki: polka dots ────────────────────────────────────────────────
inline QPixmap makeDots(const QColor& base, const QColor& mark)
{
    const int T = 22;
    QPixmap pm(T, T);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(pm.rect(), base);
    p.setPen(Qt::NoPen);
    p.setBrush(mark);
    p.drawEllipse(QPoint(T / 2, T / 2), 4, 4);
    return pm;
}

// ── Gwiazdki: pięcioramienne gwiazdy ─────────────────────────────────
inline QPixmap makeStars(const QColor& base, const QColor& mark)
{
    const int T = 46;
    QPixmap pm(T, T);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(pm.rect(), base);
    p.setPen(Qt::NoPen);

    const double cx = T / 2.0, cy = T / 2.0;
    const double outer = 12.0, inner = 5.0;

    QPainterPath star;
    for (int i = 0; i < 10; ++i) {
        double angle = (i * M_PI / 5.0) - M_PI_2;
        double r = (i % 2 == 0) ? outer : inner;
        double x = cx + r * qCos(angle);
        double y = cy + r * qSin(angle);
        if (i == 0) star.moveTo(x, y);
        else         star.lineTo(x, y);
    }
    star.closeSubpath();
    p.fillPath(star, mark);
    return pm;
}

// ── Generowanie kafelka ───────────────────────────────────────────────
inline QPixmap generate(PatternType type, const QColor& base, const QColor& markOverride = {})
{
    if (type == PatternType::None) return {};
    QColor mark = markOverride.isValid() ? markOverride : autoMark(base);
    switch (type) {
        case PatternType::Zebra:     return makeZebra(base, mark);
        case PatternType::Leopard:   return makeLeopard(base, mark);
        case PatternType::Butterfly: return makeButterfly(base, mark);
        case PatternType::Dots:      return makeDots(base, mark);
        case PatternType::Stars:     return makeStars(base, mark);
        default:                     return {};
    }
}

// Miniaturka do podglądu w ustawieniach
inline QPixmap preview(PatternType type, const QColor& base,
                       const QColor& markOverride, QSize size)
{
    QPixmap pm(size);
    QPainter p(&pm);
    if (type == PatternType::None) {
        p.fillRect(pm.rect(), base);
    } else {
        QPixmap tile = generate(type, base, markOverride);
        p.fillRect(pm.rect(), QBrush(tile));
    }
    p.setPen(QPen(QColor(0, 0, 0, 25), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(pm.rect().adjusted(0, 0, -1, -1));
    return pm;
}

} // namespace PatternGen
