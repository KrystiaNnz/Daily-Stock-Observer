#include "TileManager.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

TileManager::TileManager(QObject* parent) : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
}

QString TileManager::key(int z, int x, int y) const
{
    return QStringLiteral("%1/%2/%3").arg(z).arg(x).arg(y);
}

QString TileManager::diskPath(int z, int x, int y) const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                  + QStringLiteral("/osm/%1/%2").arg(z).arg(x);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/%1.png").arg(y);
}

QPixmap TileManager::getTile(int z, int x, int y)
{
    const QString k = key(z, x, y);

    if (m_mem.contains(k))
        return m_mem[k];

    // Sprawdź cache dyskowy
    const QString path = diskPath(z, x, y);
    if (QFile::exists(path)) {
        QPixmap pm;
        if (pm.load(path)) {
            m_mem[k] = pm;
            return pm;
        }
    }

    // Pobierz z sieci (jeśli nie jest już w toku)
    if (!m_inflight.contains(k))
        startDownload(z, x, y);

    return {};
}

void TileManager::startDownload(int z, int x, int y)
{
    const QString k = key(z, x, y);
    m_inflight.insert(k);

    const char* subs[] = {"a", "b", "c", "d"};
    QString url = QStringLiteral("https://%1.basemaps.cartocdn.com/rastertiles/voyager/%2/%3/%4.png")
                  .arg(subs[(x + y) % 4]).arg(z).arg(x).arg(y);

    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  "DailyStockObserver/1.0 (Qt6; educational use)");

    const int cz = z, cx = x, cy = y;
    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, cz, cx, cy]() {
        reply->deleteLater();
        const QString k = key(cz, cx, cy);
        m_inflight.remove(k);

        if (reply->error() != QNetworkReply::NoError) return;

        QPixmap pm;
        if (!pm.loadFromData(reply->readAll())) return;

        pm.save(diskPath(cz, cx, cy));
        m_mem[k] = pm;
        emit tileReady(cz, cx, cy);
    });
}
