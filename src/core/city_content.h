#pragma once

#include "core/game_types.h"

#include <QList>
#include <QStringList>

namespace neon {

class CityContent final {
public:
    static QList<TileDefinition> createNeonCityMap();
    static QStringList characterNames();
    static QStringList cityPulseEvents();
    static QStringList strategyCards();
    static QStringList personalEvents();
    static QStringList missions();
};

} // namespace neon
