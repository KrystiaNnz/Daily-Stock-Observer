#include "MapBridge.h"

MapBridge::MapBridge(QObject* parent) : QObject(parent) {}

void MapBridge::onCountryClicked(const QString& name, const QString& iso)
{
    emit countryClicked(name, iso);
}
