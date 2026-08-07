#pragma once

#include "core/game_types.h"

#include <QRectF>

namespace neon {

class SceneLayout final {
public:
    static constexpr qreal PropertySetback = 1.45;
    static constexpr qreal PropertyFootprint = 0.72;
    static constexpr qreal RoadFootprint = 0.90;
    static constexpr qreal SkylineFootprint = 0.78;

    static QPointF propertyWorldAnchor(const QPoint &roadGrid);
    static QPointF propertyScreenOffset(const QPoint &roadGrid, qreal halfTileWidth, qreal halfTileHeight);
    static QList<QPointF> skylineWorldAnchors();
    static QStringList validate(const QList<TileDefinition> &tiles);
};

} // namespace neon
