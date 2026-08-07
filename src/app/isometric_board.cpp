#include "app/isometric_board.h"

#include "app/game_view_model.h"
#include "core/scene_layout.h"

#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <algorithm>
#include <cmath>

namespace neon {
namespace {

QSGGeometryNode *polygonNode(const QList<QPointF> &points, QColor color)
{
    if (points.size() < 3) return nullptr;
    auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), points.size());
    geometry->setDrawingMode(QSGGeometry::DrawTriangleFan);
    auto *vertices = geometry->vertexDataAsPoint2D();
    for (int i = 0; i < points.size(); ++i) vertices[i].set(float(points.at(i).x()), float(points.at(i).y()));
    auto *material = new QSGFlatColorMaterial;
    material->setColor(color);
    auto *node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

void addPolygon(QSGNode *root, std::initializer_list<QPointF> points, const QColor &color)
{
    if (auto *node = polygonNode(QList<QPointF>(points), color)) root->appendChildNode(node);
}

void addPolygon(QSGNode *root, const QList<QPointF> &points, const QColor &color)
{
    if (auto *node = polygonNode(points, color)) root->appendChildNode(node);
}

QList<QPointF> ellipse(const QPointF &center, qreal rx, qreal ry, int segments = 18)
{
    QList<QPointF> points;
    points.reserve(segments);
    constexpr qreal Tau = 6.283185307179586;
    for (int i = 0; i < segments; ++i) {
        const qreal angle = Tau * i / segments;
        points.append(center + QPointF(std::cos(angle) * rx, std::sin(angle) * ry));
    }
    return points;
}

QColor districtColor(int district)
{
    static const QList<QColor> colors = {QColor("#23D5FF"), QColor("#FF4FA3"), QColor("#FFD45A"), QColor("#76F28D"),
        QColor("#9A7DFF"), QColor("#FF7A54"), QColor("#32E6C4"), QColor("#E76DFF")};
    return colors.at(qBound(0, district, colors.size() - 1));
}

void renderIsoBox(QSGNode *root, const QPointF &anchor, qreal width, qreal depth, qreal height,
                  const QColor &base, qreal opacity = 1.0)
{
    QColor top = base.lighter(145); top.setAlphaF(opacity);
    QColor left = base.darker(118); left.setAlphaF(opacity);
    QColor right = base.darker(165); right.setAlphaF(opacity);
    const QPointF topCenter(anchor.x(), anchor.y() - height);
    const QPointF l(topCenter.x() - width, topCenter.y());
    const QPointF r(topCenter.x() + width, topCenter.y());
    const QPointF u(topCenter.x(), topCenter.y() - depth);
    const QPointF d(topCenter.x(), topCenter.y() + depth);
    const QPointF bl(anchor.x() - width, anchor.y());
    const QPointF br(anchor.x() + width, anchor.y());
    const QPointF bd(anchor.x(), anchor.y() + depth);
    addPolygon(root, {l, d, bd, bl}, left);
    addPolygon(root, {d, r, br, bd}, right);
    addPolygon(root, {u, r, d, l}, top);
}

void renderBuilding(QSGNode *root, const QPointF &anchor, int district, int level, bool faded)
{
    const qreal opacity = faded ? 0.25 : 1.0;
    const QColor accent = districtColor(district);
    const qreal width = 18 + level * 3;
    const qreal depth = 9 + level * 2;
    const qreal height = 28 + level * 18 + (district % 3) * 5;

    QColor shadow("#050712"); shadow.setAlpha(105);
    addPolygon(root, ellipse(anchor + QPointF(6, 5), width * 1.35, depth * 0.9), shadow);
    renderIsoBox(root, anchor, width + 4, depth + 2, 6, QColor("#202945"), opacity);
    renderIsoBox(root, anchor - QPointF(0, 5), width, depth, height, accent.darker(155), opacity);

    QColor window("#BFFAFF"); window.setAlphaF(opacity * 0.92);
    const int rows = 2 + level;
    for (int row = 0; row < rows; ++row) {
        const qreal y = anchor.y() - 15 - row * 11;
        addPolygon(root, {{anchor.x() - width + 4, y - 3}, {anchor.x() - 3, y + depth - 5},
                          {anchor.x() - 3, y + depth}, {anchor.x() - width + 4, y + 2}}, window);
        addPolygon(root, {{anchor.x() + 4, y + depth - 4}, {anchor.x() + width - 4, y - 3},
                          {anchor.x() + width - 4, y + 2}, {anchor.x() + 4, y + depth + 1}}, window.darker(112));
    }
    QColor neon = accent.lighter(165); neon.setAlphaF(opacity);
    addPolygon(root, {{anchor.x() - width - 2, anchor.y() - height + 4},
                      {anchor.x() - width + 2, anchor.y() - height + 2},
                      {anchor.x() - width + 2, anchor.y() - 12},
                      {anchor.x() - width - 2, anchor.y() - 10}}, neon);

    if (level >= 2) {
        renderIsoBox(root, QPointF(anchor.x(), anchor.y() - height - depth + 2), width * 0.42, depth * 0.45,
                     7 + level * 2, accent.lighter(120), opacity);
    }
    if (level >= 3) {
        QColor antenna = neon;
        addPolygon(root, {{anchor.x() - 1.5, anchor.y() - height - 28}, {anchor.x() + 1.5, anchor.y() - height - 28},
                          {anchor.x() + 1.5, anchor.y() - height - 4}, {anchor.x() - 1.5, anchor.y() - height - 4}}, antenna);
        addPolygon(root, ellipse(QPointF(anchor.x(), anchor.y() - height - 29), 5, 2.5), antenna);
    }
}

void renderTree(QSGNode *root, const QPointF &anchor)
{
    addPolygon(root, {{anchor.x() - 2, anchor.y()}, {anchor.x() + 2, anchor.y()},
                      {anchor.x() + 2, anchor.y() - 15}, {anchor.x() - 2, anchor.y() - 15}}, QColor("#714B49"));
    addPolygon(root, ellipse(anchor - QPointF(0, 18), 10, 7), QColor("#20C99A"));
    addPolygon(root, ellipse(anchor - QPointF(4, 23), 6, 5), QColor("#52F7B5"));
}

void renderLamp(QSGNode *root, const QPointF &anchor)
{
    addPolygon(root, {{anchor.x() - 1, anchor.y()}, {anchor.x() + 1, anchor.y()},
                      {anchor.x() + 1, anchor.y() - 25}, {anchor.x() - 1, anchor.y() - 25}}, QColor("#647491"));
    addPolygon(root, ellipse(anchor - QPointF(0, 27), 6, 3), QColor("#A9FFFF"));
    QColor glow("#61F7FF"); glow.setAlpha(65);
    addPolygon(root, ellipse(anchor - QPointF(0, 27), 11, 6), glow);
}

void renderPawn(QSGNode *root, const QPointF &anchor, const QColor &color, bool active, bool overlay)
{
    if (active) {
        QColor halo = color; halo.setAlpha(overlay ? 185 : 85);
        addPolygon(root, ellipse(anchor + QPointF(0, 2), overlay ? 17 : 14, overlay ? 7 : 5), halo);
    }
    QColor shadow("#03040A"); shadow.setAlpha(130);
    addPolygon(root, ellipse(anchor + QPointF(3, 3), 9, 4), shadow);
    addPolygon(root, {{anchor.x() - 8, anchor.y()}, {anchor.x() + 8, anchor.y()},
                      {anchor.x() + 5, anchor.y() - 21}, {anchor.x() - 5, anchor.y() - 21}}, color.darker(125));
    addPolygon(root, ellipse(anchor - QPointF(0, 25), 8, 8), color.lighter(135));
    addPolygon(root, {{anchor.x() - 7, anchor.y() - 20}, {anchor.x() + 7, anchor.y() - 20},
                      {anchor.x() + 3, anchor.y() - 16}, {anchor.x() - 3, anchor.y() - 16}}, QColor("#EFFCFF"));
    if (overlay) {
        QColor marker = color.lighter(180);
        addPolygon(root, {{anchor.x(), anchor.y() - 43}, {anchor.x() - 6, anchor.y() - 52},
                          {anchor.x() + 6, anchor.y() - 52}}, marker);
    }
}

} // namespace

IsometricBoard::IsometricBoard(QQuickItem *parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setAntialiasing(true);
}

QObject *IsometricBoard::viewModel() const
{
    return m_viewModel;
}

void IsometricBoard::setViewModel(QObject *viewModelObject)
{
    auto *viewModel = qobject_cast<GameViewModel *>(viewModelObject);
    if (m_viewModel == viewModel) return;
    if (m_viewModel) disconnect(m_viewModel, nullptr, this, nullptr);
    m_viewModel = viewModel;
    if (m_viewModel) connect(m_viewModel, &GameViewModel::stateChanged, this, &IsometricBoard::update);
    emit viewModelChanged();
    update();
}

void IsometricBoard::setSceneScale(qreal scale)
{
    scale = qBound(0.65, scale, 1.55);
    if (qFuzzyCompare(m_sceneScale, scale)) return;
    m_sceneScale = scale;
    emit sceneScaleChanged();
    update();
}

QPointF IsometricBoard::project(const QPoint &grid) const
{
    constexpr qreal tileWidth = 68.0;
    constexpr qreal tileHeight = 34.0;
    return {width() * 0.5 + (grid.x() - grid.y()) * tileWidth * 0.5 * m_sceneScale,
            height() * 0.16 + (grid.x() + grid.y()) * tileHeight * 0.5 * m_sceneScale};
}

int IsometricBoard::tileAt(qreal x, qreal y) const
{
    if (!m_viewModel) return -1;
    int closest = -1;
    qreal distance = 1e9;
    for (const auto &tile : m_viewModel->state().tiles) {
        const QPointF point = project(tile.gridPosition);
        const qreal dx = (x - point.x()) / (34 * m_sceneScale);
        const qreal dy = (y - point.y()) / (17 * m_sceneScale);
        const qreal d = std::abs(dx) + std::abs(dy);
        if (d < distance) { distance = d; closest = tile.index; }
    }
    return distance <= 1.15 ? closest : -1;
}

QSGNode *IsometricBoard::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    delete oldNode;
    auto *root = new QSGNode;
    if (!m_viewModel || m_viewModel->state().tiles.isEmpty()) return root;

    const auto &state = m_viewModel->state();
    QColor cityGlow("#112247"); cityGlow.setAlpha(145);
    addPolygon(root, ellipse(QPointF(width() * 0.5, height() * 0.48), width() * 0.38, height() * 0.27, 32), cityGlow);

    const qreal halfW = 34 * m_sceneScale;
    const qreal halfH = 17 * m_sceneScale;
    const auto propertyLotOffset = [halfW, halfH](const QPoint &grid) {
        return SceneLayout::propertyScreenOffset(grid, halfW, halfH);
    };
    for (const auto &tile : state.tiles) {
        const QPointF p = project(tile.gridPosition);
        QColor road = tile.type == TileType::Property ? QColor("#28334E") : QColor("#202A43");
        addPolygon(root, {{p.x(), p.y() - halfH}, {p.x() + halfW, p.y()},
                          {p.x(), p.y() + halfH}, {p.x() - halfW, p.y()}}, QColor("#0A1021"));
        addPolygon(root, {{p.x(), p.y() - halfH + 3}, {p.x() + halfW - 6, p.y()},
                          {p.x(), p.y() + halfH - 3}, {p.x() - halfW + 6, p.y()}}, road);
        QColor lane = tile.type == TileType::Property ? districtColor(tile.district) : QColor("#6984A9");
        lane.setAlpha(165);
        addPolygon(root, {{p.x() - 8, p.y() - 1}, {p.x(), p.y() - 5}, {p.x() + 8, p.y() - 1},
                          {p.x(), p.y() + 3}}, lane);
        if (tile.type == TileType::Property) {
            const QPointF lot = p + propertyLotOffset(tile.gridPosition);
            QColor connector = districtColor(tile.district).darker(210); connector.setAlpha(210);
            addPolygon(root, {{p.x() - 7, p.y()}, {p.x() + 7, p.y()},
                              {lot.x() + 10, lot.y()}, {lot.x() - 10, lot.y()}}, connector);
            addPolygon(root, {{lot.x(), lot.y() - halfH * 0.72}, {lot.x() + halfW * 0.72, lot.y()},
                              {lot.x(), lot.y() + halfH * 0.72}, {lot.x() - halfW * 0.72, lot.y()}},
                       QColor("#18213A"));
        }
    }

    struct Object { qreal depth; int kind; int index; QPointF point; };
    QList<Object> objects;
    for (const auto &tile : state.tiles) {
        QPointF point = project(tile.gridPosition);
        if (tile.type == TileType::Property) point += propertyLotOffset(tile.gridPosition);
        objects.append({point.y(), 0, tile.index, point});
    }
    const auto skyline = SceneLayout::skylineWorldAnchors();
    for (int i = 0; i < skyline.size(); ++i) {
        const QPoint grid(qRound(skyline.at(i).x()), qRound(skyline.at(i).y()));
        objects.append({project(grid).y() + 0.1, 2, i, project(grid)});
    }
    for (int i = 0; i < state.players.size(); ++i) {
        const auto &player = state.players.at(i);
        if (const auto *tile = state.tileAt(player.position))
            objects.append({project(tile->gridPosition).y() + 0.35 + i * 0.01, 1, i,
                            project(tile->gridPosition) + QPointF((i % 3 - 1) * 10, (i / 3) * 7)});
    }
    std::sort(objects.begin(), objects.end(), [](const Object &a, const Object &b) {
        return a.depth == b.depth ? a.kind < b.kind : a.depth < b.depth;
    });

    const int activePosition = state.activePlayer() ? state.activePlayer()->position : -1;
    for (const auto &object : objects) {
        if (object.kind == 2) {
            renderBuilding(root, object.point + QPointF(0, 4), object.index % 8, 1 + object.index % 3, false);
            continue;
        }
        if (object.kind == 1) {
            const auto &player = state.players.at(object.index);
            renderPawn(root, object.point - QPointF(0, 4), player.color, object.index == state.currentPlayer, false);
            continue;
        }
        const auto *tile = state.tileAt(object.index);
        if (!tile) continue;
        if (tile->type == TileType::Property) {
            const auto *property = state.propertyAt(tile->index);
            const int level = property && !property->ownerId.isNull() ? qMax(1, property->level) : 0;
            renderBuilding(root, object.point - QPointF(0, 10), tile->district, level,
                           tile->index == activePosition);
            if (level == 0) renderTree(root, object.point + QPointF(18, 1));
        } else if (tile->type == TileType::Transit) {
            renderIsoBox(root, object.point - QPointF(0, 8), 13, 7, 18, QColor("#5369A6"));
            QColor rail("#55F5FF");
            addPolygon(root, {{object.point.x() - 18, object.point.y() - 30}, {object.point.x() + 18, object.point.y() - 30},
                              {object.point.x() + 16, object.point.y() - 26}, {object.point.x() - 16, object.point.y() - 26}}, rail);
        } else if (tile->type == TileType::Shop) {
            renderIsoBox(root, object.point - QPointF(0, 7), 14, 8, 21, QColor("#C13D91"));
            addPolygon(root, ellipse(object.point - QPointF(0, 31), 10, 4), QColor("#FF8CD1"));
        } else if (tile->type == TileType::Service) {
            renderTree(root, object.point + QPointF(8, 1));
            renderLamp(root, object.point - QPointF(10, 0));
        } else if (tile->type == TileType::Start) {
            addPolygon(root, ellipse(object.point - QPointF(0, 5), 18, 8), QColor("#1AD6D0"));
            addPolygon(root, ellipse(object.point - QPointF(0, 10), 10, 5), QColor("#B7FFFF"));
        } else {
            renderLamp(root, object.point);
        }
    }

    if (const auto *active = state.activePlayer()) {
        if (const auto *tile = state.tileAt(active->position)) {
            const QPointF p = project(tile->gridPosition) + QPointF((state.currentPlayer % 3 - 1) * 10,
                                                                    (state.currentPlayer / 3) * 7 - 4);
            renderPawn(root, p, active->color, true, true);
        }
    }
    return root;
}

} // namespace neon
