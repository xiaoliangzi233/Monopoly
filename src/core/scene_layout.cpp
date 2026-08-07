#include "core/scene_layout.h"

#include <QLineF>

namespace neon {

QRectF SceneLayout::worldBounds()
{
    return {0, 0, 4096, 3072};
}

QStringList SceneLayout::validate(const QList<TileDefinition> &tiles)
{
    QStringList errors;
    QList<QPair<int, QRectF>> industries;
    for (const auto &tile : tiles) {
        if (!worldBounds().contains(tile.worldPosition))
            errors.append(QStringLiteral("节点 %1 超出世界边界").arg(tile.index));
        if (tile.type == TileType::Property) industries.append({tile.index, tile.industryFootprint});
        for (int neighbor : tile.neighbors) {
            if (neighbor < 0 || neighbor >= tiles.size() || !tiles[neighbor].neighbors.contains(tile.index))
                errors.append(QStringLiteral("节点 %1 的道路连接不对称").arg(tile.index));
        }
    }
    for (int i = 0; i < industries.size(); ++i) {
        for (int j = i + 1; j < industries.size(); ++j)
            if (industries[i].second.intersects(industries[j].second))
                errors.append(QStringLiteral("产业 %1 与 %2 占地重叠").arg(industries[i].first).arg(industries[j].first));
        for (const auto &tile : tiles)
            if (industries[i].second.intersects(tile.roadPolygon.boundingRect()))
                errors.append(QStringLiteral("产业 %1 侵入节点道路 %2").arg(industries[i].first).arg(tile.index));
    }
    return errors;
}

} // namespace neon
