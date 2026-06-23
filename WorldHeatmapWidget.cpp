#include "WorldHeatmapWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QToolTip>
#include <QFont>
#include <cmath>

// ── Country centroids (lat, lon) ──────────────────────────────────────────────

const WorldHeatmapWidget::Centroid WorldHeatmapWidget::CENTROIDS[] = {
    {"US", 39.5, -98.4}, {"CA", 60.1, -95.0}, {"MX", 23.6,-102.6},
    {"BR",-10.8, -51.9}, {"AR",-34.0, -64.0}, {"CL",-35.7, -71.5},
    {"CO",  4.6, -74.3}, {"PE", -9.2, -74.5},
    {"GB", 55.4,  -3.4}, {"FR", 46.2,   2.2}, {"DE", 51.2,  10.5},
    {"IT", 42.8,  12.8}, {"ES", 40.5,  -3.7}, {"PT", 39.4,  -8.2},
    {"NL", 52.1,   5.3}, {"BE", 50.5,   4.5}, {"SE", 62.0,  17.0},
    {"NO", 65.5,  17.0}, {"DK", 56.3,   9.5}, {"FI", 61.9,  25.7},
    {"PL", 51.9,  19.1}, {"CZ", 49.8,  15.5}, {"AT", 47.5,  14.6},
    {"CH", 46.8,   8.2}, {"HU", 47.2,  19.5}, {"RO", 45.9,  24.9},
    {"RU", 61.5,  90.0}, {"UA", 49.0,  32.0}, {"TR", 39.0,  35.0},
    {"IL", 31.5,  34.8}, {"SA", 24.0,  45.0}, {"AE", 23.4,  53.8},
    {"IN", 21.1,  78.9}, {"CN", 35.9, 104.2}, {"JP", 36.2, 138.3},
    {"KR", 37.6, 127.0}, {"TW", 23.7, 121.0}, {"HK", 22.3, 114.2},
    {"SG",  1.4, 103.8}, {"AU",-25.3, 133.8}, {"NZ",-40.9, 172.7},
    {"ZA",-29.0,  25.1}, {"NG",  9.0,   8.0}, {"EG", 26.8,  30.8},
    {"ID", -0.8, 113.9}, {"MY",  4.2, 109.7}, {"TH", 15.9, 100.9},
};
const int WorldHeatmapWidget::CENTROID_COUNT =
    static_cast<int>(sizeof(CENTROIDS) / sizeof(CENTROIDS[0]));

// ── Constructor ───────────────────────────────────────────────────────────────

WorldHeatmapWidget::WorldHeatmapWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(300, 160);
}

// ── Projection ────────────────────────────────────────────────────────────────

QPointF WorldHeatmapWidget::project(double lat, double lon) const
{
    // Equirectangular: map (-180..180) x (85..-60) → widget rect
    const double LON_MIN = -180.0, LON_MAX = 180.0;
    const double LAT_MIN =  -60.0, LAT_MAX =  85.0;

    double px = (lon - LON_MIN) / (LON_MAX - LON_MIN) * width();
    double py = (LAT_MAX - lat) / (LAT_MAX - LAT_MIN) * height();
    return {px, py};
}

// ── Build bubbles from peers data ─────────────────────────────────────────────

void WorldHeatmapWidget::buildBubbles()
{
    m_bubbles.clear();

    QMap<QString, Bubble> byCountry;

    auto addPeer = [&](const PeerCompany& c, bool isIndustry) {
        Bubble& b = byCountry[c.country];
        b.country = c.country;
        if (isIndustry) b.industryCount++;
        else            b.sectorCount++;
        b.names.append(c.ticker + "  " + c.name);
    };

    for (const auto& p : m_data.peersIndustry) addPeer(p, true);
    for (const auto& p : m_data.peersSector)   addPeer(p, false);

    for (auto& b : byCountry) {
        // Find centroid
        bool found = false;
        for (int i = 0; i < CENTROID_COUNT; i++) {
            if (CENTROIDS[i].code == b.country) {
                b.center = project(CENTROIDS[i].lat, CENTROIDS[i].lon);
                found = true;
                break;
            }
        }
        if (found) m_bubbles.append(b);
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

int WorldHeatmapWidget::bubbleRadius(int total) const
{
    if (total <= 1) return 9;
    if (total <= 3) return 12;
    if (total <= 6) return 16;
    return 20;
}

// ── Continent polygon data ────────────────────────────────────────────────────

static QVector<QPolygonF> buildContinents(
    std::function<QPointF(double lat, double lon)> proj)
{
    QVector<QPolygonF> out;

    auto poly = [&](std::initializer_list<std::pair<double,double>> pts) {
        QPolygonF p;
        for (auto [lat, lon] : pts) p << proj(lat, lon);
        out.append(p);
    };

    // North America
    poly({{71,-168},{71,-52},{47,-52},{45,-67},{25,-80},{15,-87},
          {7,-77},{8,-90},{15,-105},{22,-118},{32,-117},{48,-124},
          {55,-133},{62,-165},{71,-168}});

    // Greenland
    poly({{84,-72},{84,-18},{60,-18},{56,-48},{60,-72},{84,-72}});

    // South America
    poly({{12,-82},{12,-62},{-5,-35},{-55,-37},{-56,-65},{-15,-73},{0,-82},{12,-82}});

    // Europe (simplified)
    poly({{36,-10},{56,-8},{58,5},{71,10},{70,25},{65,30},{60,25},
          {56,22},{54,20},{51,5},{51,2},{43,5},{43,-5},{36,-6},{36,-10}});

    // Scandinavia
    poly({{71,10},{71,35},{65,28},{56,22},{60,25},{65,30},{70,25},{71,10}});

    // Africa
    poly({{15,-18},{12,50},{-12,50},{-35,22},{-30,-18},{15,-18}});

    // Asia mainland
    poly({{75,60},{75,180},{25,180},{25,145},{10,105},{0,100},
          {8,80},{8,45},{25,35},{27,36},{36,40},{45,55},{75,60}});

    // Indian subcontinent bump
    poly({{28,67},{28,88},{8,80},{28,67}});

    // Japan (schematic)
    poly({{41,141},{32,131},{34,130},{36,136},{38,141},{41,141}});

    // Australia
    poly({{-14,113},{-14,154},{-44,151},{-44,114},{-14,113}});

    // New Zealand (schematic)
    poly({{-36,174},{-46,168},{-46,171},{-36,174}});

    return out;
}

// ── Paint ─────────────────────────────────────────────────────────────────────

void WorldHeatmapWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Ocean
    p.fillRect(rect(), QColor(180, 210, 235));

    // Continents
    auto projFn = [this](double lat, double lon) { return project(lat, lon); };
    const QColor landColor(210, 220, 205);
    const QColor landBorder(160, 175, 155);

    p.setPen(QPen(landBorder, 0.5));
    p.setBrush(landColor);
    for (const QPolygonF& poly : buildContinents(projFn))
        p.drawPolygon(poly);

    drawBubbles(p);
    drawLegend(p);
}

void WorldHeatmapWidget::drawBubbles(QPainter& p) const
{
    const QColor colIndustry(59, 130, 246);      // blue
    const QColor colSector(139, 92, 246);        // purple
    const QColor colMixed(16, 185, 129);         // teal

    for (int i = 0; i < m_bubbles.size(); i++) {
        const Bubble& b = m_bubbles[i];
        int total  = b.industryCount + b.sectorCount;
        int radius = bubbleRadius(total);
        QRectF r(b.center.x() - radius, b.center.y() - radius,
                 radius*2, radius*2);

        QColor fill;
        if (b.industryCount > 0 && b.sectorCount > 0)
            fill = colMixed;
        else if (b.industryCount > 0)
            fill = colIndustry;
        else
            fill = colSector;

        // Hover highlight
        if (i == m_hoverIdx) {
            p.setPen(QPen(fill.darker(150), 2));
            fill = fill.lighter(130);
        } else {
            p.setPen(QPen(Qt::white, 1.5));
        }

        fill.setAlpha(210);
        p.setBrush(fill);
        p.drawEllipse(r);

        // Count label
        p.setPen(Qt::white);
        QFont f; f.setPixelSize(radius > 12 ? 10 : 8); f.setBold(true);
        p.setFont(f);
        p.drawText(r, Qt::AlignCenter, QString::number(total));
    }
}

void WorldHeatmapWidget::drawLegend(QPainter& p) const
{
    if (m_data.ticker.isEmpty()) return;

    const int margin = 8, bh = 14, bw = 14, gap = 5, lineH = 18;
    const int panelW = 190, panelH = 72;
    QRect panel(margin, height() - panelH - margin, panelW, panelH);

    p.setPen(Qt::NoPen);
    QColor bg(255,255,255,200);
    p.setBrush(bg);
    p.drawRoundedRect(panel, 5, 5);

    auto row = [&](int y, QColor c, const QString& txt) {
        p.setBrush(c); p.setPen(Qt::NoPen);
        p.drawRoundedRect(panel.left()+6, y, bw, bh, 3, 3);
        p.setPen(QColor(55,65,81));
        QFont f; f.setPixelSize(10); p.setFont(f);
        p.drawText(panel.left()+6+bw+gap, y+1, panelW-30, bh,
                   Qt::AlignVCenter|Qt::AlignLeft, txt);
    };

    int y0 = panel.top() + 6;
    row(y0,            QColor(59,130,246), "Ta sama branża");
    row(y0+lineH,      QColor(139,92,246), "Ten sam sektor");
    row(y0+lineH*2,    QColor(16,185,129), "Branża + sektor");
    row(y0+lineH*3+2,  QColor(200,200,200), "Liczba = ilość spółek");
}

// ── Mouse ─────────────────────────────────────────────────────────────────────

void WorldHeatmapWidget::mouseMoveEvent(QMouseEvent* ev)
{
    int prev = m_hoverIdx;
    m_hoverIdx = -1;

    for (int i = 0; i < m_bubbles.size(); i++) {
        const Bubble& b = m_bubbles[i];
        int r = bubbleRadius(b.industryCount + b.sectorCount);
        if ((ev->pos() - b.center.toPoint()).manhattanLength() <= r + 4) {
            m_hoverIdx = i;
            // Build tooltip
            QString tip = "<b>" + b.country + "</b><br>";
            for (const QString& n : b.names)
                tip += n + "<br>";
            QToolTip::showText(ev->globalPosition().toPoint(), tip.trimmed(), this);
            break;
        }
    }

    if (prev != m_hoverIdx) update();
}

void WorldHeatmapWidget::mousePressEvent(QMouseEvent*)
{
    if (m_hoverIdx >= 0)
        emit countryClicked(m_bubbles[m_hoverIdx].country);
}

void WorldHeatmapWidget::leaveEvent(QEvent*)
{
    m_hoverIdx = -1;
    update();
}

// ── Public API ────────────────────────────────────────────────────────────────

void WorldHeatmapWidget::setPeers(const PeersData& data)
{
    m_data = data;
    buildBubbles();
    update();
}

void WorldHeatmapWidget::clear()
{
    m_data  = {};
    m_bubbles.clear();
    update();
}
