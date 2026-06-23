#pragma once

#include <QObject>
#include <QPixmap>
#include <QMap>
#include <QSet>

class QNetworkAccessManager;

class TileManager : public QObject
{
    Q_OBJECT
public:
    explicit TileManager(QObject* parent = nullptr);

    // Zwraca tile z cache lub QPixmap{} jeśli jeszcze nie gotowy (i startuje pobieranie)
    QPixmap getTile(int z, int x, int y);

signals:
    void tileReady(int z, int x, int y);

private:
    void      startDownload(int z, int x, int y);
    QString   diskPath(int z, int x, int y) const;
    QString   key(int z, int x, int y) const;

    QNetworkAccessManager* m_nam;
    QMap<QString, QPixmap> m_mem;       // cache pamięci
    QSet<QString>          m_inflight;  // aktualnie pobierane
};
