#include "core/scene_layout.h"

namespace neon {
namespace {

QRectF footprint(const QPointF &center, qreal size)
{
    return {center.x() - size * 0.5, center.y() - size * 0.5, size, size};
}

} // namespace

QPointF SceneLayout::propertyWorldAnchor(const QPoint &grid)
{
    if (grid.y() == 0) return QPointF(grid) + QPointF(0, -PropertySetback);
    if (grid.x() == 14) return QPointF(grid) + QPointF(PropertySetback, 0);
    if (grid.y() == 10) return QPointF(grid) + QPointF(0, PropertySetback);
    return QPointF(grid) + QPointF(-PropertySetback, 0);
}

QPointF SceneLayout::propertyScreenOffset(const QPoint &grid, qreal halfW, qreal halfH)
{
    if (grid.y() == 0) return {halfW * PropertySetback, -halfH * PropertySetback};
    if (grid.x() == 14) return {halfW * PropertySetback, halfH * PropertySetback};
    if (grid.y() == 10) return {-halfW * PropertySetback, halfH * PropertySetback};
    return {-halfW * PropertySetback, -halfH * PropertySetback};
}

QList<QPointF> SceneLayout::skylineWorldAnchors()
{
    return {{4, 3}, {6, 3}, {8, 3}, {10, 3}, {4, 5}, {6, 5},
            {8, 5}, {10, 5}, {4, 7}, {6, 7}, {8, 7}, {10, 7}};
}

QStringList SceneLayout::validate(const QList<TileDefinition> &tiles)
{
    QStringList errors;
    QList<QRectF> roads;
    QList<QPair<int, QRectF>> lots;
    for (const auto &tile : tiles) {
        roads.append(footprint(tile.gridPosition, RoadFootprint));
        if (tile.type == TileType::Property)
            lots.append({tile.index, footprint(propertyWorldAnchor(tile.gridPosition), PropertyFootprint)});
    }
    for (int i = 0; i < lots.size(); ++i) {
        for (int j = i + 1; j < lots.size(); ++j) {
            if (lots.at(i).second.intersects(lots.at(j).second))
                errors.append(QStringLiteral("Property lots %1 and %2 overlap").arg(lots.at(i).first).arg(lots.at(j).first));
        }
        for (int road = 0; road < roads.size(); ++road) {
            if (lots.at(i).second.intersects(roads.at(road)))
                errors.append(QStringLiteral("Property lot %1 intrudes into road %2").arg(lots.at(i).first).arg(road));
        }
    }
    const auto skyline = skylineWorldAnchors();
    for (int i = 0; i < skyline.size(); ++i) {
        const QRectF building = footprint(skyline.at(i), SkylineFootprint);
        for (int j = i + 1; j < skyline.size(); ++j) {
            if (building.intersects(footprint(skyline.at(j), SkylineFootprint)))
                errors.append(QStringLiteral("Skyline buildings %1 and %2 overlap").arg(i).arg(j));
        }
        for (int road = 0; road < roads.size(); ++road) {
            if (building.intersects(roads.at(road)))
                errors.append(QStringLiteral("Skyline building %1 intrudes into road %2").arg(i).arg(road));
        }
    }
    return errors;
}

} // namespace neon
