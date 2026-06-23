#pragma once

#include <QWidget>
#include "PeersFetcher.h"

class QLabel;
class QTableWidget;
class QSplitter;
class WorldHeatmapWidget;

class PeersPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PeersPanel(QWidget* parent = nullptr);

public slots:
    void loadPeers(const QString& ticker);
    void clear();

private slots:
    void onPeersReady(const PeersData& data);
    void onFetchError(const QString& msg);
    void onCountryClicked(const QString& code);

private:
    void setupUi();
    void setLoading(const QString& ticker);
    void populate(const PeersData& data);
    void populateTable(const PeersData& data);

    QLabel*            m_titleLabel   = nullptr;
    QLabel*            m_subLabel     = nullptr;
    QLabel*            m_statusLabel  = nullptr;
    QSplitter*         m_splitter     = nullptr;
    WorldHeatmapWidget* m_heatmap     = nullptr;
    QTableWidget*      m_table        = nullptr;

    PeersFetcher       m_fetcher;
    QString            m_currentTicker;
};
