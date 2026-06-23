#pragma once

#include "CountrySubdivisions.h"

#include <QPointF>
#include <QPointer>
#include <QVector>
#include <QWidget>

class QLabel;
class QLineEdit;
class QJsonArray;
class QNetworkAccessManager;
class QNetworkReply;
class QPainter;
class QComboBox;
class QPushButton;
class QSpinBox;
class QCheckBox;
class CountryInfoDialog;
class RegionInfoDialog;
class TileManager;

class MapPanel : public QWidget
{
    Q_OBJECT
public:
    explicit MapPanel(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private slots:
    void onTileReady(int z, int x, int y);
    void onNominatimReply(QNetworkReply* reply);
    void onSearchRequested();
    void onSearchReply(QNetworkReply* reply);
    void zoomIn();
    void zoomOut();
    void resetView();
    void toggleFilterPanel();

private:
    void    setupControls();
    void    setupFilterPanel();
    void    setupLocalDataPanel();
    void    positionOverlays();
    void    populateLocalDataCombo();
    void    updateLocalDataPanel(const QString& iso);
    void    onLocalDataComboChanged(int index);
    void    showCountryInfo(const QString& country, const QString& iso);
    void    showRegionInfo(const SubdivisionInfo& region);
    void    setSelectedCountry(const QString& country, const QString& iso, const QJsonArray& boundingBox);
    bool    selectSubdivisionAt(double lat, double lon);
    void    drawSubdivisionLayer(QPainter& painter, double cx, double cy, int tileCount);
    void    setStatus(const QString& text);
    void    centerOnLatLon(double lat, double lon, int zoom);
    void    pixelToLatLon(QPointF px, double& lat, double& lon) const;

    static double lon2tx(double lon, int z);
    static double lat2ty(double lat, int z);
    static double tx2lon(double tx, int z);
    static double ty2lat(double ty, int z);

    TileManager*           m_tiles  = nullptr;
    QNetworkAccessManager* m_nam    = nullptr;
    QLabel*                m_status = nullptr;
    QWidget*               m_toolbar = nullptr;
    QLineEdit*             m_searchEdit = nullptr;
    QPushButton*           m_searchBtn = nullptr;
    QPushButton*           m_zoomInBtn = nullptr;
    QPushButton*           m_zoomOutBtn = nullptr;
    QPushButton*           m_resetBtn = nullptr;
    QPushButton*           m_filterBtn = nullptr;
    QWidget*               m_localDataPanel = nullptr;
    QComboBox*             m_localDataCombo = nullptr;
    QLabel*                m_localDataDetails = nullptr;
    QWidget*               m_filterPanel = nullptr;
    QSpinBox*              m_cityLimitSpin = nullptr;
    QCheckBox*             m_showCitiesCheck = nullptr;
    QCheckBox*             m_showCompaniesCheck = nullptr;
    QCheckBox*             m_showCountryInfoCheck = nullptr;
    QCheckBox*             m_showSubdivisionsCheck = nullptr;
    QCheckBox*             m_pickSubdivisionsCheck = nullptr;
    QPointer<CountryInfoDialog> m_countryInfoDialog;
    QPointer<RegionInfoDialog> m_regionInfoDialog;

    double m_centerTX = 0;   // pozycja środka widoku w układzie kafelkowym (ułamkowa)
    double m_centerTY = 0;
    int    m_zoom     = 3;

    bool   m_dragging = false;
    QPoint m_lastMouse;

    bool    m_hasSelectedCountry = false;
    bool    m_hasSelectedBorders = false;
    QString m_selectedCountry;
    QString m_selectedIso;
    QVector<QVector<QPointF>> m_selectedBorderPolygons;
    QVector<SubdivisionInfo> m_selectedSubdivisions;
    QString m_selectedSubdivisionCode;
    double  m_selectedSouth = 0.0;
    double  m_selectedNorth = 0.0;
    double  m_selectedWest  = 0.0;
    double  m_selectedEast  = 0.0;

    static constexpr int TILE_SIZE = 256;
    static constexpr int STATUS_H  = 28;
    static constexpr int MIN_ZOOM  = 2;
    static constexpr int MAX_ZOOM  = 18;
};
