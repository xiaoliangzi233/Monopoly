#pragma once

#include "core/game_types.h"

#include <QRectF>

namespace neon {

class SceneLayout final {
public:
    static QRectF worldBounds();
    static QStringList validate(const QList<TileDefinition> &tiles);
};

} // namespace neon
