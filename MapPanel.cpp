#include "MapPanel.h"
#include "TileManager.h"
#include "CountryInfoDialog.h"
#include "CountryBorders.h"
#include "CountrySubdivisions.h"
#include "LocalDataSources.h"
#include "RegionInfoDialog.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtMath>
#include <cmath>

double MapPanel::lon2tx(double lon, int z)
{
    return (lon + 180.0) / 360.0 * (1 << z);
}

double MapPanel::lat2ty(double lat, int z)
{
    double r = qDegreesToRadians(lat);
    return (1.0 - std::log(std::tan(r) + 1.0 / std::cos(r)) / M_PI) / 2.0 * (1 << z);
}

double MapPanel::tx2lon(double tx, int z)
{
    return tx / (1 << z) * 360.0 - 180.0;
}

double MapPanel::ty2lat(double ty, int z)
{
    double n = M_PI - 2.0 * M_PI * ty / (1 << z);
    return qRadiansToDegrees(std::atan(0.5 * (std::exp(n) - std::exp(-n))));
}

static bool ringContainsLonLat(const QVector<QPointF>& ring, double lon, double lat)
{
    if (ring.size() < 3) return false;

    bool inside = false;
    for (int i = 0, j = ring.size() - 1; i < ring.size(); j = i++) {
        const double xi = ring.at(i).x();
        const double yi = ring.at(i).y();
        const double xj = ring.at(j).x();
        const double yj = ring.at(j).y();

        const bool intersects = ((yi > lat) != (yj > lat)) &&
            (lon < (xj - xi) * (lat - yi) / ((yj - yi) + 1e-12) + xi);
        if (intersects)
            inside = !inside;
    }

    return inside;
}

static bool subdivisionContainsLonLat(const SubdivisionInfo& subdivision, double lon, double lat)
{
    bool inside = false;
    for (const QVector<QPointF>& ring : subdivision.rings) {
        if (ringContainsLonLat(ring, lon, lat))
            inside = !inside;
    }
    return inside;
}

MapPanel::MapPanel(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);

    m_tiles = new TileManager(this);
    m_nam   = new QNetworkAccessManager(this);

    m_status = new QLabel(this);
    m_status->setStyleSheet(
        "background: rgba(10,20,40,0.88); color: #9bc;"
        "font-size: 12px; padding: 0 14px;");

    setupControls();
    setupLocalDataPanel();
    setupFilterPanel();

    connect(m_tiles, &TileManager::tileReady, this, &MapPanel::onTileReady);

    resetView();
}

void MapPanel::setupControls()
{
    m_toolbar = new QWidget(this);
    m_toolbar->setObjectName("mapToolbar");
    m_toolbar->setStyleSheet(
        "#mapToolbar { background: rgba(255,255,255,0.94); border: 1px solid #cbd5e1;"
        " border-radius: 8px; }"
        "QLineEdit { border: 1px solid #cbd5e1; border-radius: 5px; padding: 6px 8px;"
        " background: white; color: #1f2937; }"
        "QPushButton { border: 1px solid #cbd5e1; border-radius: 5px; padding: 6px 10px;"
        " background: #f8fafc; color: #1f2937; font-weight: 600; }"
        "QPushButton:hover { background: #e2e8f0; }"
        "QPushButton:pressed { background: #cbd5e1; }");

    auto* layout = new QHBoxLayout(m_toolbar);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(6);

    m_searchEdit = new QLineEdit(m_toolbar);
    m_searchEdit->setPlaceholderText("Szukaj kraju lub miasta...");
    m_searchBtn = new QPushButton("Szukaj", m_toolbar);
    m_zoomInBtn = new QPushButton("+", m_toolbar);
    m_zoomOutBtn = new QPushButton("-", m_toolbar);
    m_resetBtn = new QPushButton("Europa", m_toolbar);
    m_filterBtn = new QPushButton("Filtry", m_toolbar);
    m_filterBtn->setCheckable(true);

    m_zoomInBtn->setFixedWidth(34);
    m_zoomOutBtn->setFixedWidth(34);
    m_resetBtn->setFixedWidth(70);
    m_filterBtn->setFixedWidth(70);

    layout->addWidget(m_searchEdit, 1);
    layout->addWidget(m_searchBtn);
    layout->addWidget(m_zoomOutBtn);
    layout->addWidget(m_zoomInBtn);
    layout->addWidget(m_resetBtn);
    layout->addWidget(m_filterBtn);

    connect(m_searchEdit, &QLineEdit::returnPressed, this, &MapPanel::onSearchRequested);
    connect(m_searchBtn, &QPushButton::clicked, this, &MapPanel::onSearchRequested);
    connect(m_zoomInBtn, &QPushButton::clicked, this, &MapPanel::zoomIn);
    connect(m_zoomOutBtn, &QPushButton::clicked, this, &MapPanel::zoomOut);
    connect(m_resetBtn, &QPushButton::clicked, this, &MapPanel::resetView);
    connect(m_filterBtn, &QPushButton::clicked, this, &MapPanel::toggleFilterPanel);
}

void MapPanel::setupLocalDataPanel()
{
    m_localDataPanel = new QWidget(this);
    m_localDataPanel->setObjectName("localDataPanel");
    m_localDataPanel->setStyleSheet(
        "#localDataPanel { background: rgba(255,255,255,0.94); border: 1px solid #cbd5e1;"
        " border-radius: 8px; }"
        "QLabel#localDataTitle { font-size: 12px; font-weight: 700; color: #334155; }"
        "QLabel#localDataDetails { font-size: 11px; color: #475569; }"
        "QComboBox { border: 1px solid #cbd5e1; border-radius: 5px; padding: 5px 8px;"
        " background: white; color: #1f2937; }"
        "QComboBox QAbstractItemView { background: white; color: #1f2937;"
        " border: 1px solid #94a3b8; selection-background-color: #dbeafe;"
        " selection-color: #0f172a; outline: 0; }");

    auto* root = new QVBoxLayout(m_localDataPanel);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(6);

    auto* title = new QLabel("Status danych lokalnych", m_localDataPanel);
    title->setObjectName("localDataTitle");
    root->addWidget(title);

    m_localDataCombo = new QComboBox(m_localDataPanel);
    m_localDataCombo->setEditable(true);
    m_localDataCombo->setInsertPolicy(QComboBox::NoInsert);
    m_localDataCombo->setMaxVisibleItems(6);
    m_localDataCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_localDataCombo->setMinimumContentsLength(26);
    if (m_localDataCombo->view()) {
        m_localDataCombo->view()->setMaximumHeight(168);
        m_localDataCombo->view()->setMinimumWidth(390);
        m_localDataCombo->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    root->addWidget(m_localDataCombo);

    m_localDataDetails = new QLabel(m_localDataPanel);
    m_localDataDetails->setObjectName("localDataDetails");
    m_localDataDetails->setWordWrap(true);
    m_localDataDetails->setTextFormat(Qt::RichText);
    m_localDataDetails->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_localDataDetails->setOpenExternalLinks(true);
    root->addWidget(m_localDataDetails);

    populateLocalDataCombo();
    connect(m_localDataCombo, &QComboBox::currentIndexChanged,
            this, &MapPanel::onLocalDataComboChanged);

    updateLocalDataPanel(QStringLiteral("PL"));
}

void MapPanel::setupFilterPanel()
{
    m_filterPanel = new QWidget(this);
    m_filterPanel->setObjectName("mapFilterPanel");
    m_filterPanel->setVisible(false);
    m_filterPanel->setStyleSheet(
        "#mapFilterPanel { background: rgba(255,255,255,0.96); border: 1px solid #cbd5e1;"
        " border-radius: 8px; }"
        "QLabel#filterTitle { font-size: 16px; font-weight: 700; color: #172033; }"
        "QLabel#filterSection { font-size: 12px; font-weight: 700; color: #64748b;"
        " padding-top: 8px; }"
        "QLabel#filterHint { color: #94a3b8; font-size: 11px; }"
        "QCheckBox { color: #1f2937; spacing: 8px; }"
        "QSpinBox { border: 1px solid #cbd5e1; border-radius: 5px; padding: 5px 8px;"
        " background: white; color: #1f2937; }");

    auto* root = new QVBoxLayout(m_filterPanel);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(10);

    auto* title = new QLabel("Filtry mapy", m_filterPanel);
    title->setObjectName("filterTitle");
    root->addWidget(title);

    auto* hint = new QLabel("Panel gotowy pod kolejne warstwy. Kontrolki sa narazie szkieletem.", m_filterPanel);
    hint->setObjectName("filterHint");
    hint->setWordWrap(true);
    root->addWidget(hint);

    auto* citySection = new QLabel("Miasta", m_filterPanel);
    citySection->setObjectName("filterSection");
    root->addWidget(citySection);

    m_showCitiesCheck = new QCheckBox("Pokaz miasta", m_filterPanel);
    m_showCitiesCheck->setChecked(false);
    root->addWidget(m_showCitiesCheck);

    auto* cityLimitRow = new QWidget(m_filterPanel);
    auto* cityLimitLayout = new QHBoxLayout(cityLimitRow);
    cityLimitLayout->setContentsMargins(0, 0, 0, 0);
    cityLimitLayout->setSpacing(8);

    auto* cityLimitLabel = new QLabel("Liczba miast", cityLimitRow);
    cityLimitLabel->setStyleSheet("color:#475569;");
    m_cityLimitSpin = new QSpinBox(cityLimitRow);
    m_cityLimitSpin->setRange(0, 200);
    m_cityLimitSpin->setValue(25);
    m_cityLimitSpin->setSuffix(" pkt");
    cityLimitLayout->addWidget(cityLimitLabel, 1);
    cityLimitLayout->addWidget(m_cityLimitSpin);
    root->addWidget(cityLimitRow);

    auto* companySection = new QLabel("Spolki", m_filterPanel);
    companySection->setObjectName("filterSection");
    root->addWidget(companySection);

    m_showCompaniesCheck = new QCheckBox("Pokaz lokalizacje spolek", m_filterPanel);
    m_showCompaniesCheck->setChecked(false);
    root->addWidget(m_showCompaniesCheck);

    auto* countrySection = new QLabel("Kraje", m_filterPanel);
    countrySection->setObjectName("filterSection");
    root->addWidget(countrySection);

    m_showCountryInfoCheck = new QCheckBox("Podbij kraje z danymi lokalnymi", m_filterPanel);
    m_showCountryInfoCheck->setChecked(false);
    root->addWidget(m_showCountryInfoCheck);

    m_showSubdivisionsCheck = new QCheckBox("Pokaz regiony administracyjne", m_filterPanel);
    m_showSubdivisionsCheck->setChecked(false);
    root->addWidget(m_showSubdivisionsCheck);

    m_pickSubdivisionsCheck = new QCheckBox("Wybieraj regiony", m_filterPanel);
    m_pickSubdivisionsCheck->setChecked(false);
    root->addWidget(m_pickSubdivisionsCheck);

    connect(m_showCountryInfoCheck, &QCheckBox::toggled, this, [this]() { update(); });
    connect(m_showSubdivisionsCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            if (m_selectedIso.isEmpty()) {
                setStatus("Wybierz kraj, aby pokazac jego regiony administracyjne.");
            } else if (m_selectedSubdivisions.isEmpty()) {
                setStatus("Brak lokalnych regionow dla: " + m_selectedCountry);
            } else {
                setStatus(QString("Regiony administracyjne: %1 (%2)")
                              .arg(m_selectedCountry)
                              .arg(m_selectedSubdivisions.size()));
            }
        }
        update();
    });
    connect(m_pickSubdivisionsCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            if (m_showSubdivisionsCheck)
                m_showSubdivisionsCheck->setChecked(true);
            if (m_selectedIso.isEmpty())
                setStatus("Wybierz kraj, a potem kliknij jego region administracyjny.");
            else if (m_selectedSubdivisions.isEmpty())
                setStatus("Brak lokalnych regionow dla: " + m_selectedCountry);
            else
                setStatus("Kliknij region administracyjny na mapie.");
        }
        update();
    });

    root->addStretch();
}

void MapPanel::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    m_status->setGeometry(0, height() - STATUS_H, width(), STATUS_H);
    positionOverlays();
    update();
}

void MapPanel::positionOverlays()
{
    if (m_toolbar)
        m_toolbar->setGeometry(16, 16, qMin(width() - 32, 600), 42);

    if (m_localDataPanel) {
        const int panelW = qMin(430, qMax(300, width() - 32));
        const int panelH = 104;
        const int y = qMax(72, height() - STATUS_H - panelH - 16);
        m_localDataPanel->setGeometry(16, y, panelW, panelH);
    }

    if (m_filterPanel) {
        const int panelW = qMin(300, qMax(240, width() - 32));
        const int panelH = qMin(390, qMax(260, height() - STATUS_H - 78));
        m_filterPanel->setGeometry(width() - panelW - 16, 70, panelW, panelH);
    }
}

void MapPanel::populateLocalDataCombo()
{
    if (!m_localDataCombo) return;

    m_localDataCombo->clear();
    const QVector<LocalDataSourceInfo> rows = LocalDataSources::allSources();
    for (const LocalDataSourceInfo& row : rows) {
        QString statusLabel = row.apiStatus;
        if (statusLabel == QStringLiteral("api"))
            statusLabel = QStringLiteral("API");
        else if (statusLabel == QStringLiteral("do_weryfikacji"))
            statusLabel = QStringLiteral("do weryfikacji");
        else if (statusLabel.isEmpty())
            statusLabel = QStringLiteral("brak statusu");

        m_localDataCombo->addItem(QString("%1 - %2").arg(row.country, statusLabel), row.iso);
    }
}

void MapPanel::updateLocalDataPanel(const QString& iso)
{
    if (!m_localDataCombo || !m_localDataDetails) return;

    const QString normalizedIso = iso.trimmed().toUpper();
    if (!normalizedIso.isEmpty()) {
        const int idx = m_localDataCombo->findData(normalizedIso);
        if (idx >= 0 && idx != m_localDataCombo->currentIndex()) {
            const QSignalBlocker blocker(m_localDataCombo);
            m_localDataCombo->setCurrentIndex(idx);
        } else if (idx < 0) {
            m_localDataDetails->setText(QString("%1: brak wpisu w lokalnym rejestrze. Fallback: World Bank Indicators API / Eurostat.")
                                            .arg(normalizedIso));
            return;
        }
    }

    LocalDataSourceInfo row = LocalDataSources::sourceForIso(
        m_localDataCombo->currentData().toString());

    if (row.iso.isEmpty()) {
        m_localDataDetails->setText("Brak wpisu w lokalnym rejestrze. Fallback: World Bank Indicators API / Eurostat.");
        return;
    }

    QString apiText = row.apiStatus;
    if (apiText == QStringLiteral("api"))
        apiText = QStringLiteral("API dostepne");
    else if (apiText == QStringLiteral("do_weryfikacji"))
        apiText = QStringLiteral("API do weryfikacji");
    else if (apiText.isEmpty())
        apiText = QStringLiteral("brak statusu API");

    const QString typeText = row.apiType.isEmpty() ? QStringLiteral("typ nieustalony") : row.apiType;
    const QString fallbackText = row.fallbackName.isEmpty() ? QStringLiteral("brak") : row.fallbackName;
    const QString sourceLink = row.websiteUrl.isEmpty()
        ? row.sourceName.toHtmlEscaped()
        : QString("<a href=\"%1\">%2</a>").arg(row.websiteUrl.toHtmlEscaped(),
                                                row.sourceName.toHtmlEscaped());
    const QString apiLink = row.apiUrl.isEmpty()
        ? QString()
        : QString(" <a href=\"%1\">Endpoint API</a>.").arg(row.apiUrl.toHtmlEscaped());

    m_localDataDetails->setText(QString("%1 -> %2 -> %3.%4 Fallback: %5.")
                                    .arg(row.country.toHtmlEscaped(),
                                         sourceLink,
                                         (apiText + " / " + typeText).toHtmlEscaped(),
                                         apiLink,
                                         fallbackText.toHtmlEscaped()));
}

void MapPanel::onLocalDataComboChanged(int)
{
    updateLocalDataPanel(QString());
}

void MapPanel::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    const double mapH = height() - STATUS_H;
    const double cx   = width() / 2.0;
    const double cy   = mapH    / 2.0;

    p.fillRect(0, 0, width(), static_cast<int>(mapH), QColor(10, 20, 40));

    const int tileCount = 1 << m_zoom;
    const int numX = static_cast<int>(std::ceil((double)width() / TILE_SIZE)) + 2;
    const int numY = static_cast<int>(std::ceil(mapH / TILE_SIZE)) + 2;

    const int startX = static_cast<int>(std::floor(m_centerTX - cx / TILE_SIZE));
    const int startY = static_cast<int>(std::floor(m_centerTY - cy / TILE_SIZE));

    for (int tx = startX; tx < startX + numX; ++tx) {
        for (int ty = startY; ty < startY + numY; ++ty) {
            const double px = (tx - m_centerTX) * TILE_SIZE + cx;
            const double py = (ty - m_centerTY) * TILE_SIZE + cy;

            const int wx = ((tx % tileCount) + tileCount) % tileCount;

            if (ty < 0 || ty >= tileCount) {
                p.fillRect(QRectF(px, py, TILE_SIZE, TILE_SIZE), QColor(20, 35, 55));
                continue;
            }

            QPixmap pm = m_tiles->getTile(m_zoom, wx, ty);
            if (!pm.isNull()) {
                p.drawPixmap(QPointF(px, py), pm);
            } else {
                p.fillRect(QRectF(px, py, TILE_SIZE, TILE_SIZE), QColor(20, 35, 55));
                p.setPen(QColor(35, 55, 75));
                p.drawRect(QRectF(px + 0.5, py + 0.5, TILE_SIZE - 1, TILE_SIZE - 1));
            }
        }
    }

    if (m_hasSelectedCountry) {
        QRectF selectedBounds;

        if (m_hasSelectedBorders) {
            QPainterPath fillPath;
            const int tileCount = 1 << m_zoom;

            for (const QVector<QPointF>& geoPolygon : m_selectedBorderPolygons) {
                if (geoPolygon.size() < 3) continue;

                QPolygonF screenPolygon;
                screenPolygon.reserve(geoPolygon.size());
                for (const QPointF& lonLat : geoPolygon) {
                    double tx = lon2tx(lonLat.x(), m_zoom);
                    while (tx - m_centerTX > tileCount / 2.0) tx -= tileCount;
                    while (m_centerTX - tx > tileCount / 2.0) tx += tileCount;

                    const double ty = lat2ty(lonLat.y(), m_zoom);
                    const double sx = (tx - m_centerTX) * TILE_SIZE + cx;
                    const double sy = (ty - m_centerTY) * TILE_SIZE + cy;
                    screenPolygon.append(QPointF(sx, sy));
                }

                if (screenPolygon.size() >= 3) {
                    fillPath.addPolygon(screenPolygon);
                    fillPath.closeSubpath();
                    selectedBounds = selectedBounds.isNull()
                        ? screenPolygon.boundingRect()
                        : selectedBounds.united(screenPolygon.boundingRect());
                }
            }

            if (!fillPath.isEmpty()) {
                p.setRenderHint(QPainter::Antialiasing, true);
                p.fillPath(fillPath, QColor(245, 158, 11, 70));
                p.setPen(QPen(QColor(245, 158, 11, 240), 2.2));
                p.drawPath(fillPath);
            }
        }

        if (selectedBounds.isNull()) {
            const double westTX  = lon2tx(m_selectedWest, m_zoom);
            const double eastTX  = lon2tx(m_selectedEast, m_zoom);
            const double northTY = lat2ty(m_selectedNorth, m_zoom);
            const double southTY = lat2ty(m_selectedSouth, m_zoom);

            double x1 = (westTX - m_centerTX) * TILE_SIZE + cx;
            double x2 = (eastTX - m_centerTX) * TILE_SIZE + cx;
            double y1 = (northTY - m_centerTY) * TILE_SIZE + cy;
            double y2 = (southTY - m_centerTY) * TILE_SIZE + cy;

            selectedBounds = QRectF(QPointF(qMin(x1, x2), qMin(y1, y2)),
                                    QPointF(qMax(x1, x2), qMax(y1, y2)));
            p.setBrush(QColor(245, 158, 11, 70));
            p.setPen(QPen(QColor(245, 158, 11, 230), 2));
            p.drawRoundedRect(selectedBounds, 8, 8);
        }

        if (selectedBounds.width() < 24) {
            selectedBounds.adjust(-(24 - selectedBounds.width()) / 2, 0,
                                  (24 - selectedBounds.width()) / 2, 0);
        }
        if (selectedBounds.height() < 24) {
            selectedBounds.adjust(0, -(24 - selectedBounds.height()) / 2,
                                  0, (24 - selectedBounds.height()) / 2);
        }

        QRectF labelBox(selectedBounds.left(), selectedBounds.top() - 28,
                        qMax(150.0, selectedBounds.width()), 24);
        if (labelBox.top() < 8)
            labelBox.moveTop(selectedBounds.bottom() + 6);
        p.setBrush(QColor(255, 255, 255, 235));
        p.setPen(QColor(245, 158, 11, 230));
        p.drawRoundedRect(labelBox, 6, 6);
        p.setPen(QColor(31, 41, 55));
        p.setFont(QFont("Segoe UI", 9, QFont::DemiBold));
        p.drawText(labelBox.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft,
                   m_selectedCountry);
    }

    if (m_showSubdivisionsCheck && m_showSubdivisionsCheck->isChecked())
        drawSubdivisionLayer(p, cx, cy, tileCount);

    p.setPen(QColor(200, 200, 200, 180));
    p.setFont(QFont("Segoe UI", 8));
    p.drawText(QRect(0, static_cast<int>(mapH) - 18, width() - 6, 16),
               Qt::AlignRight | Qt::AlignVCenter,
               "(c) CARTO  (c) OpenStreetMap contributors");
}

void MapPanel::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging = false;
        m_lastMouse = e->pos();
    }
}

void MapPanel::mouseMoveEvent(QMouseEvent* e)
{
    if (e->buttons() & Qt::LeftButton) {
        QPoint delta = e->pos() - m_lastMouse;
        if (delta.manhattanLength() > 3) m_dragging = true;
        m_centerTX -= delta.x() / static_cast<double>(TILE_SIZE);
        m_centerTY -= delta.y() / static_cast<double>(TILE_SIZE);
        m_lastMouse = e->pos();
        update();
    }
}

void MapPanel::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && !m_dragging) {
        if (e->pos().y() >= height() - STATUS_H) return;
        if (m_toolbar && m_toolbar->geometry().contains(e->pos())) return;
        if (m_localDataPanel && m_localDataPanel->geometry().contains(e->pos())) return;
        if (m_filterPanel && m_filterPanel->isVisible() && m_filterPanel->geometry().contains(e->pos())) return;

        double lat, lon;
        pixelToLatLon(e->pos(), lat, lon);

        if (m_pickSubdivisionsCheck && m_pickSubdivisionsCheck->isChecked()) {
            if (selectSubdivisionAt(lat, lon)) {
                m_dragging = false;
                return;
            }
            if (!m_selectedIso.isEmpty() && !m_selectedSubdivisions.isEmpty()) {
                setStatus("Nie trafiono w region - kliknij blizej srodka obszaru.");
                m_dragging = false;
                return;
            }
        }

        setStatus(QString("Szukam kraju... (%1, %2)").arg(lat, 0, 'f', 2).arg(lon, 0, 'f', 2));
        QApplication::setOverrideCursor(Qt::WaitCursor);

        QString url = QStringLiteral(
            "https://nominatim.openstreetmap.org/reverse"
            "?lat=%1&lon=%2&zoom=3&format=json&accept-language=pl")
            .arg(lat, 0, 'f', 6).arg(lon, 0, 'f', 6);

        QNetworkRequest req{QUrl(url)};
        req.setHeader(QNetworkRequest::UserAgentHeader,
                      "DailyStockObserver/1.0 (Qt6; educational use)");

        QNetworkReply* reply = m_nam->get(req);
        connect(reply, &QNetworkReply::finished,
                this, [this, reply]() { onNominatimReply(reply); });
    }
    m_dragging = false;
}

void MapPanel::wheelEvent(QWheelEvent* e)
{
    const int newZoom = qBound(MIN_ZOOM, m_zoom + (e->angleDelta().y() > 0 ? 1 : -1), MAX_ZOOM);
    if (newZoom == m_zoom) return;

    double lat, lon;
    pixelToLatLon(e->position(), lat, lon);

    m_zoom = newZoom;

    const double mapH = height() - STATUS_H;
    const double cx   = width() / 2.0;
    const double cy   = mapH / 2.0;
    m_centerTX = lon2tx(lon, m_zoom) - (e->position().x() - cx) / TILE_SIZE;
    m_centerTY = lat2ty(lat, m_zoom) - (e->position().y() - cy) / TILE_SIZE;

    update();
}

void MapPanel::onTileReady(int /*z*/, int /*x*/, int /*y*/)
{
    update();
}

void MapPanel::onNominatimReply(QNetworkReply* reply)
{
    QApplication::restoreOverrideCursor();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        setStatus("Blad Nominatim: " + reply->errorString());
        return;
    }

    const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
    if (obj.contains("error")) {
        setStatus("Ocean lub obszar bez danych - kliknij blizej srodka ladu.");
        return;
    }

    const QJsonObject addr = obj["address"].toObject();
    const QString country = addr["country"].toString();
    const QString iso = addr["country_code"].toString().toUpper();

    if (country.isEmpty()) {
        setStatus("Nie rozpoznano kraju - sprobuj w innym miejscu.");
        return;
    }

    setStatus("Rozpoznano: " + country);
    setSelectedCountry(country, iso, obj["boundingbox"].toArray());
    showCountryInfo(country, iso);
    setStatus("Kliknij kraj, wyszukaj miejsce albo przeciagnij mape.");
}

void MapPanel::onSearchRequested()
{
    const QString query = m_searchEdit ? m_searchEdit->text().trimmed() : QString();
    if (query.isEmpty()) {
        setStatus("Wpisz nazwe kraju lub miasta.");
        return;
    }

    setStatus("Szukam: " + query);
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QUrl url("https://nominatim.openstreetmap.org/search");
    QUrlQuery q;
    q.addQueryItem("q", query);
    q.addQueryItem("format", "json");
    q.addQueryItem("addressdetails", "1");
    q.addQueryItem("limit", "1");
    q.addQueryItem("accept-language", "pl");
    url.setQuery(q);

    QNetworkRequest req{url};
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  "DailyStockObserver/1.0 (Qt6; educational use)");

    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() { onSearchReply(reply); });
}

void MapPanel::onSearchReply(QNetworkReply* reply)
{
    QApplication::restoreOverrideCursor();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        setStatus("Blad wyszukiwania: " + reply->errorString());
        return;
    }

    const QJsonArray results = QJsonDocument::fromJson(reply->readAll()).array();
    if (results.isEmpty()) {
        setStatus("Nie znaleziono miejsca.");
        return;
    }

    const QJsonObject obj = results.first().toObject();
    const double lat = obj["lat"].toString().toDouble();
    const double lon = obj["lon"].toString().toDouble();
    const QString type = obj["type"].toString();
    const QString displayName = obj["display_name"].toString();
    const QJsonObject addr = obj["address"].toObject();
    const QString country = addr["country"].toString();
    const QString iso = addr["country_code"].toString().toUpper();

    const int targetZoom = (type == "country" || type == "administrative") ? 5 : 8;
    centerOnLatLon(lat, lon, targetZoom);
    setStatus("Znaleziono: " + displayName);

    if (!iso.isEmpty() && (type == "country" || !country.isEmpty())) {
        setSelectedCountry(country.isEmpty() ? displayName : country, iso, obj["boundingbox"].toArray());
        showCountryInfo(country.isEmpty() ? displayName : country, iso);
    }
}

void MapPanel::zoomIn()
{
    m_zoom = qMin(MAX_ZOOM, m_zoom + 1);
    update();
}

void MapPanel::zoomOut()
{
    m_zoom = qMax(MIN_ZOOM, m_zoom - 1);
    update();
}

void MapPanel::resetView()
{
    centerOnLatLon(52.0, 15.0, 4);
    setStatus("Widok: Europa. Kliknij kraj, wyszukaj miejsce albo przeciagnij mape.");
}

void MapPanel::toggleFilterPanel()
{
    if (!m_filterPanel) return;

    const bool visible = !m_filterPanel->isVisible();
    m_filterPanel->setVisible(visible);
    if (m_filterBtn)
        m_filterBtn->setChecked(visible);
    positionOverlays();
    setStatus(visible ? "Panel filtrow mapy jest otwarty." :
                        "Panel filtrow mapy jest ukryty.");
}

void MapPanel::showCountryInfo(const QString& country, const QString& iso)
{
    if (m_countryInfoDialog)
        m_countryInfoDialog->close();

    auto* dlg = new CountryInfoDialog(country, iso, this);
    m_countryInfoDialog = dlg;
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowModality(Qt::NonModal);

    connect(dlg, &CountryInfoDialog::centralBankRequested,
            this, [this](const QString& name, double lat, double lon) {
                centerOnLatLon(lat, lon, 13);
                setStatus("Bank centralny: " + name);
            });
    connect(dlg, &QObject::destroyed, this, [this, dlg]() {
        if (m_countryInfoDialog == dlg)
            m_countryInfoDialog = nullptr;
    });
    dlg->adjustSize();

    const int margin = 16;
    const int top = 70;
    QPoint pos = mapToGlobal(QPoint(margin, top));
    dlg->move(pos);
    dlg->show();
    dlg->raise();
}

void MapPanel::showRegionInfo(const SubdivisionInfo& region)
{
    if (m_regionInfoDialog)
        m_regionInfoDialog->close();

    auto* dlg = new RegionInfoDialog(region, this);
    m_regionInfoDialog = dlg;
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowModality(Qt::NonModal);
    connect(dlg, &QObject::destroyed, this, [this, dlg]() {
        if (m_regionInfoDialog == dlg)
            m_regionInfoDialog = nullptr;
    });

    const int margin = 16;
    const int top = 70;
    QPoint pos = mapToGlobal(QPoint(margin, top));
    dlg->move(pos);
    dlg->show();
    dlg->raise();
}

bool MapPanel::selectSubdivisionAt(double lat, double lon)
{
    if (m_selectedSubdivisions.isEmpty()) return false;

    for (const SubdivisionInfo& subdivision : m_selectedSubdivisions) {
        if (!subdivisionContainsLonLat(subdivision, lon, lat))
            continue;

        m_selectedSubdivisionCode = subdivision.code;
        showRegionInfo(subdivision);
        setStatus("Region: " + subdivision.name);
        update();
        return true;
    }

    return false;
}

void MapPanel::drawSubdivisionLayer(QPainter& painter, double cx, double cy, int tileCount)
{
    if (m_selectedSubdivisions.isEmpty()) return;

    QPainterPath borderPath;
    QPainterPath selectedRegionPath;
    for (const SubdivisionInfo& subdivision : m_selectedSubdivisions) {
        for (const QVector<QPointF>& ring : subdivision.rings) {
            if (ring.size() < 2) continue;

            QPainterPath ringPath;
            bool started = false;
            for (const QPointF& lonLat : ring) {
                double tx = lon2tx(lonLat.x(), m_zoom);
                while (tx - m_centerTX > tileCount / 2.0) tx -= tileCount;
                while (m_centerTX - tx > tileCount / 2.0) tx += tileCount;

                const double ty = lat2ty(lonLat.y(), m_zoom);
                const double sx = (tx - m_centerTX) * TILE_SIZE + cx;
                const double sy = (ty - m_centerTY) * TILE_SIZE + cy;

                if (!started) {
                    borderPath.moveTo(sx, sy);
                    ringPath.moveTo(sx, sy);
                    started = true;
                } else {
                    borderPath.lineTo(sx, sy);
                    ringPath.lineTo(sx, sy);
                }
            }

            if (started && !m_selectedSubdivisionCode.isEmpty() &&
                subdivision.code == m_selectedSubdivisionCode) {
                ringPath.closeSubpath();
                selectedRegionPath.addPath(ringPath);
            }
        }
    }

    if (!selectedRegionPath.isEmpty()) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillPath(selectedRegionPath, QColor(37, 99, 235, 70));
        painter.setPen(QPen(QColor(30, 64, 175, 235), 2.0));
        painter.drawPath(selectedRegionPath);
    }

    if (!borderPath.isEmpty()) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor(37, 99, 235, 190), 1.35));
        painter.drawPath(borderPath);
    }

    if (m_zoom < 5) return;

    painter.setFont(QFont("Segoe UI", 8, QFont::DemiBold));
    const QFontMetrics fm = painter.fontMetrics();
    const double mapH = height() - STATUS_H;

    for (const SubdivisionInfo& subdivision : m_selectedSubdivisions) {
        if (subdivision.labelLat == 0.0 && subdivision.labelLon == 0.0) continue;

        double tx = lon2tx(subdivision.labelLon, m_zoom);
        while (tx - m_centerTX > tileCount / 2.0) tx -= tileCount;
        while (m_centerTX - tx > tileCount / 2.0) tx += tileCount;

        const double ty = lat2ty(subdivision.labelLat, m_zoom);
        const double sx = (tx - m_centerTX) * TILE_SIZE + cx;
        const double sy = (ty - m_centerTY) * TILE_SIZE + cy;

        if (sx < -40 || sx > width() + 40 || sy < -20 || sy > mapH + 20)
            continue;

        QString text = subdivision.name;
        if (!subdivision.type.isEmpty() && m_zoom >= 7)
            text += " (" + subdivision.type + ")";
        text = fm.elidedText(text, Qt::ElideRight, 150);

        QRectF textBox(sx - 76, sy - 10, 152, 20);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 220));
        painter.drawRoundedRect(textBox, 5, 5);
        painter.setPen(QColor(30, 64, 175));
        painter.drawText(textBox.adjusted(6, 0, -6, 0),
                         Qt::AlignCenter,
                         text);
    }
}

void MapPanel::setSelectedCountry(const QString& country, const QString& iso, const QJsonArray& boundingBox)
{
    m_selectedCountry = country;
    m_selectedIso = iso.trimmed().toUpper();
    m_selectedBorderPolygons = CountryBorders::polygonsForIso(m_selectedIso);
    m_selectedSubdivisions = CountrySubdivisions::forIso(m_selectedIso);
    m_selectedSubdivisionCode.clear();
    updateLocalDataPanel(m_selectedIso);
    m_hasSelectedBorders = !m_selectedBorderPolygons.isEmpty();
    m_hasSelectedCountry = false;

    if (boundingBox.size() >= 4) {
        m_selectedSouth = boundingBox.at(0).toString().toDouble();
        m_selectedNorth = boundingBox.at(1).toString().toDouble();
        m_selectedWest  = boundingBox.at(2).toString().toDouble();
        m_selectedEast  = boundingBox.at(3).toString().toDouble();

        m_hasSelectedCountry =
            m_selectedSouth != m_selectedNorth &&
            m_selectedWest  != m_selectedEast;
    }

    if (m_hasSelectedBorders)
        m_hasSelectedCountry = true;

    if (m_showSubdivisionsCheck && m_showSubdivisionsCheck->isChecked()) {
        if (m_selectedSubdivisions.isEmpty())
            setStatus("Brak lokalnych regionow dla: " + m_selectedCountry);
        else
            setStatus(QString("Regiony administracyjne: %1 (%2)")
                          .arg(m_selectedCountry)
                          .arg(m_selectedSubdivisions.size()));
    }

    update();
}

void MapPanel::centerOnLatLon(double lat, double lon, int zoom)
{
    m_zoom = qBound(MIN_ZOOM, zoom, MAX_ZOOM);
    m_centerTX = lon2tx(lon, m_zoom);
    m_centerTY = lat2ty(lat, m_zoom);
    update();
}

void MapPanel::pixelToLatLon(QPointF px, double& lat, double& lon) const
{
    const double mapH = height() - STATUS_H;
    const double cx   = width() / 2.0;
    const double cy   = mapH / 2.0;
    lon = tx2lon(m_centerTX + (px.x() - cx) / TILE_SIZE, m_zoom);
    lat = ty2lat(m_centerTY + (px.y() - cy) / TILE_SIZE, m_zoom);
}

void MapPanel::setStatus(const QString& text)
{
    m_status->setText(text);
}
