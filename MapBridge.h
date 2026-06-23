#pragma once
#include <QObject>

class MapBridge : public QObject
{
    Q_OBJECT
public:
    explicit MapBridge(QObject* parent = nullptr);

    Q_INVOKABLE void onCountryClicked(const QString& name, const QString& iso);

signals:
    void countryClicked(const QString& name, const QString& iso);
};
