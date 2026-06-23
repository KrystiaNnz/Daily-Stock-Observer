#pragma once

#include <QWidget>
#include "PeersFetcher.h"

class WorldHeatmapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WorldHeatmapWidget(QWidget* parent = nullptr);

    void setPeers(const PeersData& data);
    void clear();

    QSize sizeHint() const override { return {600, 320}; }
    QSize minimumSizeHint() const override { return {300, 160}; }

signals:
    void countryClicked(const QString& countryCode);

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    struct Bubble {
        QString country;
        QPointF center;         // pixel coords
        int industryCount = 0;
        int sectorCount   = 0;
        QStringList names;
    };

    QPointF project(double lat, double lon) const;
    void    buildBubbles();
    void    drawContinents(QPainter& p) const;
    void    drawBubbles(QPainter& p) const;
    void    drawLegend(QPainter& p) const;
    int     bubbleRadius(int total) const;

    PeersData         m_data;
    QString           m_selectedCountry;   // query company's country (not in db)
    QList<Bubble>     m_bubbles;
    int               m_hoverIdx = -1;

    // Country centroids: {code, lat, lon}
    static const struct Centroid { const char* code; double lat, lon; } CENTROIDS[];
    static const int CENTROID_COUNT;
};
