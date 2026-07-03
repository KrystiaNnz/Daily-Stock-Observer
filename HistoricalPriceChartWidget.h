#pragma once

#include <QWidget>
#include "DatabaseManager.h"

class HistoricalPriceChartWidget : public QWidget
{
public:
    enum class ChartType {
        LineClose,
        BarClose,
        Candlestick
    };

    explicit HistoricalPriceChartWidget(QWidget* parent = nullptr);

    void setBars(const QList<MarketPriceBar>& bars,
                 const QString& ticker,
                 const QString& interval,
                 const QString& currency);
    void setChartType(ChartType type);
    void clearChart();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QRectF plotRect() const;
    QPointF closePointForIndex(int index, const QRectF& rect, double minPrice, double maxPrice) const;
    double yForPrice(double price, const QRectF& rect, double minPrice, double maxPrice) const;
    int nearestIndexAtX(double x, const QRectF& rect) const;
    QString chartTypeLabel() const;

    QList<MarketPriceBar> m_bars;
    QString m_ticker;
    QString m_interval = "1d";
    QString m_currency;
    ChartType m_chartType = ChartType::LineClose;
    int m_hoverIndex = -1;
};
