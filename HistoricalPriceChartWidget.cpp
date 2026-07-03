#include "HistoricalPriceChartWidget.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>
#include <algorithm>
#include <cmath>
#include <limits>

HistoricalPriceChartWidget::HistoricalPriceChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumHeight(220);
}

void HistoricalPriceChartWidget::setBars(const QList<MarketPriceBar>& bars,
                                         const QString& ticker,
                                         const QString& interval,
                                         const QString& currency)
{
    m_bars = bars;
    m_ticker = ticker;
    m_interval = interval.isEmpty() ? "1d" : interval;
    m_currency = currency;
    m_hoverIndex = -1;
    update();
}

void HistoricalPriceChartWidget::setChartType(ChartType type)
{
    if (m_chartType == type)
        return;
    m_chartType = type;
    m_hoverIndex = -1;
    update();
}

void HistoricalPriceChartWidget::clearChart()
{
    m_bars.clear();
    m_ticker.clear();
    m_currency.clear();
    m_hoverIndex = -1;
    update();
}

QRectF HistoricalPriceChartWidget::plotRect() const
{
    return rect().adjusted(58, 34, -20, -38);
}

double HistoricalPriceChartWidget::yForPrice(double price,
                                             const QRectF& rect,
                                             double minPrice,
                                             double maxPrice) const
{
    const double range = std::max(maxPrice - minPrice, 0.000001);
    return rect.bottom() - ((price - minPrice) / range) * rect.height();
}

QPointF HistoricalPriceChartWidget::closePointForIndex(int index,
                                                       const QRectF& rect,
                                                       double minPrice,
                                                       double maxPrice) const
{
    if (m_bars.isEmpty())
        return {};

    const double xStep = (m_bars.size() > 1)
        ? rect.width() / double(m_bars.size() - 1)
        : 0.0;
    const double x = rect.left() + xStep * index;
    const double y = yForPrice(m_bars[index].close, rect, minPrice, maxPrice);
    return {x, y};
}

int HistoricalPriceChartWidget::nearestIndexAtX(double x, const QRectF& rect) const
{
    if (m_bars.isEmpty() || !rect.contains(QPointF(x, rect.center().y())))
        return -1;
    if (m_bars.size() == 1)
        return 0;

    const double ratio = (x - rect.left()) / rect.width();
    const int index = int(std::round(ratio * (m_bars.size() - 1)));
    return std::clamp(index, 0, int(m_bars.size()) - 1);
}

QString HistoricalPriceChartWidget::chartTypeLabel() const
{
    switch (m_chartType) {
    case ChartType::BarClose:
        return "slupki close";
    case ChartType::Candlestick:
        return "swiece OHLC";
    case ChartType::LineClose:
    default:
        return "linia close";
    }
}

void HistoricalPriceChartWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), palette().base());

    const QColor textColor = palette().text().color();
    const QColor mutedText = palette().mid().color();
    const QColor gridColor = palette().midlight().color();
    const QColor lineColor = palette().highlight().color();

    const QRectF plot = plotRect();
    p.setPen(QPen(gridColor, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(plot);

    p.setPen(textColor);
    QFont titleFont = p.font();
    titleFont.setBold(true);
    p.setFont(titleFont);
    const QString title = m_ticker.isEmpty()
        ? "Wykres ceny zamkniecia"
        : QString("%1 - %2 (%3)").arg(m_ticker, chartTypeLabel(), m_interval);
    p.drawText(QRectF(12, 8, width() - 24, 20), Qt::AlignLeft | Qt::AlignVCenter, title);
    titleFont.setBold(false);
    p.setFont(titleFont);

    if (m_bars.isEmpty()) {
        p.setPen(mutedText);
        p.drawText(plot, Qt::AlignCenter, "Brak danych do wykresu. Zaladuj tabele cen historycznych.");
        return;
    }

    double minPrice = std::numeric_limits<double>::max();
    double maxPrice = std::numeric_limits<double>::lowest();
    for (const MarketPriceBar& bar : m_bars) {
        if (m_chartType == ChartType::Candlestick) {
            minPrice = std::min(minPrice, std::min(bar.low, bar.open));
            maxPrice = std::max(maxPrice, std::max(bar.high, bar.open));
        } else {
            minPrice = std::min(minPrice, bar.close);
            maxPrice = std::max(maxPrice, bar.close);
        }
    }
    const double padding = std::max((maxPrice - minPrice) * 0.06, maxPrice * 0.002);
    minPrice = std::max(0.0, minPrice - padding);
    maxPrice += padding;

    p.setPen(QPen(gridColor, 1));
    for (int i = 1; i < 4; ++i) {
        const double y = plot.top() + plot.height() * i / 4.0;
        p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
    }

    p.setPen(textColor);
    for (int i = 0; i <= 4; ++i) {
        const double value = maxPrice - (maxPrice - minPrice) * i / 4.0;
        const double y = plot.top() + plot.height() * i / 4.0;
        p.drawText(QRectF(4, y - 8, 50, 16), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(value, 'f', 2));
    }

    const double xStep = (m_bars.size() > 1)
        ? plot.width() / double(m_bars.size() - 1)
        : plot.width();
    const double bodyWidth = std::clamp(xStep * 0.55, 3.0, 16.0);

    if (m_chartType == ChartType::LineClose) {
        QPainterPath path;
        for (int i = 0; i < m_bars.size(); ++i) {
            const QPointF point = closePointForIndex(i, plot, minPrice, maxPrice);
            if (i == 0)
                path.moveTo(point);
            else
                path.lineTo(point);
        }

        p.setPen(QPen(lineColor, 2.2));
        p.drawPath(path);
    } else if (m_chartType == ChartType::BarClose) {
        p.setPen(Qt::NoPen);
        p.setBrush(lineColor);
        for (int i = 0; i < m_bars.size(); ++i) {
            const QPointF point = closePointForIndex(i, plot, minPrice, maxPrice);
            const QRectF barRect(point.x() - bodyWidth / 2.0,
                                 point.y(),
                                 bodyWidth,
                                 plot.bottom() - point.y());
            p.drawRoundedRect(barRect, 1.5, 1.5);
        }
    } else {
        const QColor upColor("#16a34a");
        const QColor downColor("#dc2626");
        for (int i = 0; i < m_bars.size(); ++i) {
            const MarketPriceBar& bar = m_bars[i];
            const double x = closePointForIndex(i, plot, minPrice, maxPrice).x();
            const double yHigh = yForPrice(bar.high, plot, minPrice, maxPrice);
            const double yLow = yForPrice(bar.low, plot, minPrice, maxPrice);
            const double yOpen = yForPrice(bar.open, plot, minPrice, maxPrice);
            const double yClose = yForPrice(bar.close, plot, minPrice, maxPrice);
            const bool isUp = bar.close >= bar.open;
            const QColor candleColor = isUp ? upColor : downColor;

            p.setPen(QPen(candleColor, 1.2));
            p.drawLine(QPointF(x, yHigh), QPointF(x, yLow));

            const double top = std::min(yOpen, yClose);
            const double bottom = std::max(yOpen, yClose);
            QRectF body(x - bodyWidth / 2.0, top, bodyWidth, std::max(1.0, bottom - top));
            p.setBrush(isUp ? palette().base() : candleColor);
            p.drawRect(body);
        }
    }

    const QList<int> labelIndexes = m_bars.size() == 1
        ? QList<int>{0}
        : QList<int>{0, int(m_bars.size()) / 2, int(m_bars.size()) - 1};
    p.setPen(textColor);
    for (int index : labelIndexes) {
        const QPointF point = closePointForIndex(index, plot, minPrice, maxPrice);
        p.drawText(QRectF(point.x() - 48, plot.bottom() + 8, 96, 18),
                   Qt::AlignCenter,
                   m_bars[index].tradingDate);
    }

    if (m_hoverIndex >= 0 && m_hoverIndex < m_bars.size()) {
        const QPointF point = closePointForIndex(m_hoverIndex, plot, minPrice, maxPrice);
        p.setPen(QPen(mutedText, 1, Qt::DashLine));
        p.drawLine(QPointF(point.x(), plot.top()), QPointF(point.x(), plot.bottom()));
        p.setPen(QPen(lineColor, 2));
        p.setBrush(palette().base());
        p.drawEllipse(point, 4, 4);
    }

    if (!m_currency.isEmpty()) {
        p.setPen(mutedText);
        p.drawText(QRectF(width() - 120, 8, 108, 20), Qt::AlignRight | Qt::AlignVCenter, m_currency);
    }
}

void HistoricalPriceChartWidget::mouseMoveEvent(QMouseEvent* event)
{
    const QRectF plot = plotRect();
    const int index = nearestIndexAtX(event->position().x(), plot);
    if (index != m_hoverIndex) {
        m_hoverIndex = index;
        update();
    }

    if (index >= 0 && index < m_bars.size()) {
        const MarketPriceBar& bar = m_bars[index];
        const QString suffix = m_currency.isEmpty() ? "" : " " + m_currency;
        QToolTip::showText(event->globalPosition().toPoint(),
                           QString("%1\nOpen: %2%6\nHigh: %3%6\nLow: %4%6\nClose: %5%6")
                               .arg(bar.tradingDate)
                               .arg(QString::number(bar.open, 'f', 4))
                               .arg(QString::number(bar.high, 'f', 4))
                               .arg(QString::number(bar.low, 'f', 4))
                               .arg(QString::number(bar.close, 'f', 4))
                               .arg(suffix),
                           this);
    } else {
        QToolTip::hideText();
    }
}

void HistoricalPriceChartWidget::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    m_hoverIndex = -1;
    QToolTip::hideText();
    update();
}
