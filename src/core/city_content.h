#pragma once

#include "core/game_types.h"

#include <QList>
#include <QStringList>

namespace neon {

class CityContent final {
public:
    static constexpr quint32 ContentVersion = 2;
    static QList<TileDefinition> createProsperousCityMap();
    static QByteArray contentHash();
    static QStringList characterNames();
    static QStringList characterBiographies();
    static QStringList districtNames();
    static QStringList cityPulseEvents();
    static QStringList strategyCards();
    static QStringList personalEvents();
    static QStringList missions();
};

} // namespace neon
