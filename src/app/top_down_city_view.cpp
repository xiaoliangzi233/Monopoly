#include "app/top_down_city_view.h"

#include "app/game_view_model.h"
#include "core/scene_layout.h"

#include <QLineF>
#include <QMatrix4x4>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGTransformNode>
#include <algorithm>
#include <cmath>
#include <limits>

namespace neon {
namespace {

class CityRootNode final : public QSGNode {
public:
    CityRootNode()
    {
        transform = new QSGTransformNode;
        world = new QSGNode;
        appendChildNode(transform);
        transform->appendChildNode(world);
    }
    QSGTransformNode *transform = nullptr;
    QSGNode *world = nullptr;
    quint64 sequence = std::numeric_limits<quint64>::max();
    QUuid matchId;
    int lod = -1;
};

QSGGeometryNode *polygonNode(const QList<QPointF> &points, QColor color)
{
    if (points.size() < 3) return nullptr;
    const int triangleVertices = (points.size() - 2) * 3;
    auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), triangleVertices);
    geometry->setDrawingMode(QSGGeometry::DrawTriangles);
    auto *vertices = geometry->vertexDataAsPoint2D();
    int vertex = 0;
    for (int i = 1; i < points.size() - 1; ++i) {
        vertices[vertex++].set(float(points[0].x()), float(points[0].y()));
        vertices[vertex++].set(float(points[i].x()), float(points[i].y()));
        vertices[vertex++].set(float(points[i + 1].x()), float(points[i + 1].y()));
    }
    auto *material = new QSGFlatColorMaterial;
    material->setColor(color);
    auto *node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

void addPolygon(QSGNode *root, const QList<QPointF> &points, const QColor &color)
{
    if (auto *node = polygonNode(points, color)) root->appendChildNode(node);
}

void addRect(QSGNode *root, const QRectF &rect, const QColor &color)
{
    addPolygon(root, {{rect.left(), rect.top()}, {rect.right(), rect.top()},
                      {rect.right(), rect.bottom()}, {rect.left(), rect.bottom()}}, color);
}

QList<QPointF> ellipse(const QPointF &center, qreal rx, qreal ry, int segments = 22)
{
    QList<QPointF> points;
    constexpr qreal tau = 6.283185307179586;
    for (int i = 0; i < segments; ++i) {
        const qreal angle = tau * i / segments;
        points.append(center + QPointF(std::cos(angle) * rx, std::sin(angle) * ry));
    }
    return points;
}

void addLine(QSGNode *root, const QPointF &a, const QPointF &b, qreal width, const QColor &color)
{
    const QLineF line(a, b);
    if (line.length() < .1) return;
    const QPointF normal(-line.dy() / line.length() * width * .5,
                         line.dx() / line.length() * width * .5);
    addPolygon(root, {a + normal, b + normal, b - normal, a - normal}, color);
}

QColor districtColor(int district)
{
    static const QList<QColor> colors = {QColor("#476B58"), QColor("#8B4F5D"), QColor("#5D7850"), QColor("#496A75"),
        QColor("#315F72"), QColor("#8A633D"), QColor("#607A86"), QColor("#755271")};
    return colors[qBound(0, district, colors.size() - 1)];
}

void renderTree(QSGNode *root, const QPointF &p, qreal size)
{
    QColor shadow("#1B2921"); shadow.setAlpha(95);
    addPolygon(root, ellipse(p + QPointF(7, 7), size, size * .72), shadow);
    addRect(root, {p.x() - 3, p.y() - 4, 6, 15}, QColor("#73513A"));
    addPolygon(root, ellipse(p, size, size * .78), QColor("#315F43"));
    addPolygon(root, ellipse(p - QPointF(size * .28, size * .18), size * .62, size * .48), QColor("#4E8054"));
}

void renderLantern(QSGNode *root, const QPointF &p)
{
    addRect(root, {p.x() - 2, p.y() - 17, 4, 20}, QColor("#574235"));
    QColor glow("#EFCB72"); glow.setAlpha(75);
    addPolygon(root, ellipse(p - QPointF(0, 17), 11, 11), glow);
    addRect(root, {p.x() - 6, p.y() - 23, 12, 13}, QColor("#B83A2D"));
    addRect(root, {p.x() - 1, p.y() - 10, 2, 7}, QColor("#D7A53D"));
}

void renderRoof(QSGNode *root, const QRectF &rect, const QColor &base, qreal opacity, bool vertical)
{
    QColor shadow("#243027"); shadow.setAlphaF(.38 * opacity);
    addRect(root, rect.translated(8, 9), shadow);
    QColor eave = base.darker(135); eave.setAlphaF(opacity);
    QColor tile = base; tile.setAlphaF(opacity);
    QColor ridge = base.lighter(128); ridge.setAlphaF(opacity);
    addRect(root, rect, eave);
    const QRectF inner = rect.adjusted(7, 7, -7, -7);
    addRect(root, inner, tile);
    if (vertical) {
        addRect(root, {inner.center().x() - 3, inner.top(), 6, inner.height()}, ridge);
        addPolygon(root, {{rect.left() - 7, rect.top() + 4}, {rect.left(), rect.top()},
                          {rect.left(), rect.bottom()}, {rect.left() - 7, rect.bottom() - 4}}, eave);
        addPolygon(root, {{rect.right() + 7, rect.top() + 4}, {rect.right(), rect.top()},
                          {rect.right(), rect.bottom()}, {rect.right() + 7, rect.bottom() - 4}}, eave);
        for (qreal x = inner.left() + 10; x < inner.right(); x += 12)
            addRect(root, {x, inner.top() + 3, 2, inner.height() - 6}, base.lighter(118));
    } else {
        addRect(root, {inner.left(), inner.center().y() - 3, inner.width(), 6}, ridge);
        addPolygon(root, {{rect.left() + 4, rect.top() - 7}, {rect.left(), rect.top()},
                          {rect.right(), rect.top()}, {rect.right() - 4, rect.top() - 7}}, eave);
        addPolygon(root, {{rect.left() + 4, rect.bottom() + 7}, {rect.left(), rect.bottom()},
                          {rect.right(), rect.bottom()}, {rect.right() - 4, rect.bottom() + 7}}, eave);
        for (qreal y = inner.top() + 9; y < inner.bottom(); y += 11)
            addRect(root, {inner.left() + 3, y, inner.width() - 6, 2}, base.lighter(118));
    }
}

void renderCourtyard(QSGNode *root, const QRectF &lot, const QColor &accent)
{
    const QColor stone("#D8C9A6");
    const QColor wall("#EEE2C4");
    const QColor wallCap("#5A6259");
    addRect(root, lot, QColor("#6A563D"));
    addRect(root, lot.adjusted(5, 5, -5, -5), stone);
    addRect(root, {lot.left() + 7, lot.top() + 7, lot.width() - 14, 9}, wall);
    addRect(root, {lot.left() + 7, lot.bottom() - 16, lot.width() - 14, 9}, wall);
    addRect(root, {lot.left() + 7, lot.top() + 7, 9, lot.height() - 14}, wall);
    addRect(root, {lot.right() - 16, lot.top() + 7, 9, lot.height() - 14}, wall);
    addRect(root, {lot.left() + 7, lot.top() + 5, lot.width() - 14, 4}, wallCap);
    addRect(root, {lot.left() + 7, lot.bottom() - 9, lot.width() - 14, 4}, wallCap);
    addRect(root, {lot.center().x() - 33, lot.bottom() - 19, 66, 16}, stone);
    addRect(root, {lot.center().x() - 24, lot.bottom() - 23, 48, 6}, accent);
    for (qreal x = lot.left() + 28; x < lot.right() - 20; x += 34)
        addLine(root, {x, lot.top() + 22}, {x + 18, lot.top() + 40}, 2, QColor("#B7A37C"));
}

void renderMarketStall(QSGNode *root, const QPointF &p, const QColor &accent)
{
    addRect(root, {p.x() - 25, p.y() - 13, 50, 28}, QColor("#8C653E"));
    addRect(root, {p.x() - 29, p.y() - 19, 58, 12}, accent);
    for (int i = -2; i <= 2; ++i)
        addRect(root, {p.x() + i * 11 - 3, p.y() - 19, 6, 12}, QColor("#E8D7AA"));
    addRect(root, {p.x() - 20, p.y() + 2, 40, 7}, QColor("#C3924F"));
}

void renderIndustry(QSGNode *root, const TileDefinition &tile, int level, const QColor &owner,
                    bool mortgaged, bool active)
{
    const QRectF lot = tile.industryFootprint;
    renderCourtyard(root, lot, districtColor(tile.district));
    if (mortgaged)
        addRect(root, lot.adjusted(16, 16, -16, -16), QColor(120, 118, 110, 105));
    if (level <= 0) {
        renderTree(root, lot.center() - QPointF(58, 2), 22);
        renderMarketStall(root, lot.center() + QPointF(36, 4), districtColor(tile.district).lighter(120));
        addPolygon(root, ellipse(lot.center() + QPointF(-3, 36), 24, 11), QColor("#A4B88D"));
        return;
    }
    const qreal opacity = active ? .55 : 1.0;
    QColor roof = districtColor(tile.district).lighter(115);
    renderRoof(root, {lot.left() + 27, lot.top() + 27, lot.width() - 54, 64}, roof, opacity, false);
    addRect(root, {lot.center().x() - 21, lot.bottom() - 50, 42, 45}, QColor("#B9925D"));
    addRect(root, {lot.center().x() - 8, lot.bottom() - 29, 16, 24}, QColor("#713A2E"));
    if (level >= 2) {
        renderRoof(root, {lot.left() + 12, lot.top() + 50, 55, lot.height() - 65}, roof.darker(108), opacity, true);
        renderRoof(root, {lot.right() - 67, lot.top() + 50, 55, lot.height() - 65}, roof.darker(108), opacity, true);
    }
    if (level >= 3) {
        renderRoof(root, {lot.center().x() - 42, lot.top() + 4, 84, 42}, roof.lighter(112), opacity, false);
        addPolygon(root, ellipse(lot.center() + QPointF(0, 11), 21, 13), QColor("#4A705D"));
    }
    if (tile.district == 1 || tile.district == 5)
        renderMarketStall(root, lot.topRight() + QPointF(-54, 39), QColor("#A93A31"));
    if (tile.district == 2 || tile.district == 7)
        renderTree(root, lot.topLeft() + QPointF(34, 42), 16);
    QColor ownership = owner; ownership.setAlpha(220);
    addRect(root, {lot.left() + 8, lot.top() + 8, 34, 8}, ownership);
    renderLantern(root, lot.bottomRight() - QPointF(22, 7));
}

void renderSpecial(QSGNode *root, const TileDefinition &tile)
{
    const QPointF p = tile.worldPosition;
    switch (tile.type) {
    case TileType::Start:
        addRect(root, {p.x() - 58, p.y() - 35, 116, 70}, QColor("#CDBB91"));
        addRect(root, {p.x() - 49, p.y() - 29, 98, 58}, QColor("#7E302B"));
        addRect(root, {p.x() - 15, p.y() - 23, 30, 52}, QColor("#E6D6AF"));
        break;
    case TileType::Transit:
        renderRoof(root, {p.x() - 52, p.y() - 30, 104, 60}, QColor("#426A6C"), 1, false);
        addLine(root, p - QPointF(80, 0), p + QPointF(80, 0), 8, QColor("#D1B56B"));
        break;
    case TileType::Guild:
        renderRoof(root, {p.x() - 45, p.y() - 32, 90, 64}, QColor("#9D4B3D"), 1, false);
        addPolygon(root, ellipse(p, 13, 13), QColor("#D5A63B"));
        break;
    case TileType::Civic:
        addPolygon(root, ellipse(p, 44, 32), QColor("#6C9A80"));
        addPolygon(root, ellipse(p, 25, 18), QColor("#92B9A0"));
        addPolygon(root, ellipse(p, 10, 8), QColor("#D9E1C7"));
        break;
    case TileType::Commission:
        renderRoof(root, {p.x() - 48, p.y() - 29, 96, 58}, QColor("#506C78"), 1, false);
        addRect(root, {p.x() - 4, p.y() - 18, 8, 36}, QColor("#DCCB9B"));
        break;
    case TileType::Tax:
        addRect(root, {p.x() - 38, p.y() - 29, 76, 58}, QColor("#C5B28B"));
        renderRoof(root, {p.x() - 45, p.y() - 35, 90, 30}, QColor("#5B665D"), 1, false);
        break;
    case TileType::Festival:
        for (int i = -2; i <= 2; ++i) renderLantern(root, p + QPointF(i * 23, 8));
        addLine(root, p - QPointF(60, 16), p + QPointF(60, 16), 4, QColor("#8A3C31"));
        break;
    case TileType::Event:
        renderTree(root, p - QPointF(17, 4), 24);
        addRect(root, {p.x() + 3, p.y() - 18, 45, 36}, districtColor(tile.district).lighter(132));
        addRect(root, {p.x() + 8, p.y() - 12, 35, 7}, QColor("#E9D39C"));
        break;
    default: break;
    }
}

void renderPlayer(QSGNode *root, const QPointF &p, const QColor &color, bool active, int index)
{
    if (active) {
        QColor halo("#D8A937"); halo.setAlpha(150);
        addPolygon(root, ellipse(p + QPointF(0, 8), 28, 19), halo);
    }
    QColor shadow("#263027"); shadow.setAlpha(120);
    addPolygon(root, ellipse(p + QPointF(6, 9), 15, 10), shadow);
    addPolygon(root, ellipse(p, 13, 13), color);
    addRect(root, {p.x() - 9, p.y() - 1, 18, 25}, color.darker(118));
    addPolygon(root, ellipse(p - QPointF(0, 8), 14, 6), QColor("#2C2925"));
    addRect(root, {p.x() - 2, p.y() - 22, 4, 14}, QColor("#2C2925"));
    addRect(root, {p.x() - 10, p.y() + 20 + index * 2, 20, 4}, color.lighter(150));
}

void rebuildWorld(QSGNode *root, const GameState &state, int lod)
{
    addRect(root, SceneLayout::worldBounds(), QColor("#B9C6A2"));
    addRect(root, {0, 0, 4096, 170}, QColor("#315D63"));
    addRect(root, {0, 2870, 4096, 202}, QColor("#315D63"));
    addRect(root, {1960, 0, 176, 3072}, QColor("#5E8790"));
    for (int wave = 0; wave < 18; ++wave) {
        const qreal x = 70.0 + wave * 236.0;
        addLine(root, {x, 83}, {x + 88, 83}, 3, QColor("#8DB2AA"));
        addLine(root, {x + 28, 2985}, {x + 116, 2985}, 3, QColor("#8DB2AA"));
    }
    for (int bridge = 0; bridge < 3; ++bridge) {
        const qreal y = 560.0 + bridge * 890.0;
        addRect(root, {1944, y - 72, 208, 144}, QColor("#73624D"));
        addRect(root, {1952, y - 63, 192, 126}, QColor("#D1BF91"));
        for (int plank = 0; plank < 6; ++plank)
            addRect(root, {1960 + plank * 30.0, y - 58, 3, 116}, QColor("#9E875E"));
    }
    for (int district = 0; district < 8; ++district) {
        QColor wash = districtColor(district).lighter(165); wash.setAlpha(70);
        addRect(root, {180, 165 + district * 350.0, 3735, 270}, wash);
    }
    for (const auto &tile : state.tiles) {
        for (int neighbor : tile.neighbors) if (neighbor > tile.index) {
            const auto *other = state.tileAt(neighbor);
            if (!other) continue;
            addLine(root, tile.worldPosition, other->worldPosition, 86, QColor("#6A6558"));
            addLine(root, tile.worldPosition, other->worldPosition, 64, QColor("#D3C49D"));
            if (lod >= 1) addLine(root, tile.worldPosition, other->worldPosition, 3, QColor("#A58C58"));
            if (lod >= 2) {
                const QLineF segment(tile.worldPosition, other->worldPosition);
                for (qreal at = 115; at < segment.length() - 70; at += 135) {
                    const QPointF p = segment.pointAt(at / segment.length());
                    addPolygon(root, ellipse(p, 7, 5, 12), QColor("#B39A68"));
                }
            }
        }
    }
    for (const auto &tile : state.tiles) {
        addPolygon(root, ellipse(tile.worldPosition, 58, 42), QColor("#695F50"));
        addPolygon(root, ellipse(tile.worldPosition, 48, 33), QColor("#D8CBA8"));
        addPolygon(root, ellipse(tile.worldPosition, 33, 21), QColor("#E8DDBD"));
        if (tile.type == TileType::Property) {
            const auto *property = state.propertyAt(tile.index);
            QColor owner("#8A7959");
            int level = 0;
            bool mortgaged = false;
            if (property) {
                level = property->ownerId.isNull() ? 0 : qMax(1, property->level);
                mortgaged = property->mortgaged;
                const auto it = std::find_if(state.players.cbegin(), state.players.cend(),
                    [&](const auto &player) { return player.id == property->ownerId; });
                if (it != state.players.cend()) owner = it->color;
            }
            const bool active = state.activePlayer() && state.activePlayer()->position == tile.index;
            renderIndustry(root, tile, level, owner, mortgaged, active);
        } else renderSpecial(root, tile);
        if (lod >= 2 && tile.index % 3 == 0) {
            renderTree(root, tile.worldPosition + QPointF(-105, -66), 17);
            renderLantern(root, tile.worldPosition + QPointF(90, 43));
        }
    }
    for (int option = 0; option < state.routeOptions.size(); ++option) {
        const auto &route = state.routeOptions.at(option);
        const auto *tile = route.isEmpty() ? nullptr : state.tileAt(route.last());
        if (!tile) continue;
        QColor choice = option == 0 ? QColor("#B83A2D") : QColor("#D5A63B"); choice.setAlpha(155);
        addPolygon(root, ellipse(tile->worldPosition, 72, 52), choice);
    }
    for (int i = 0; i < state.players.size(); ++i) {
        const auto &player = state.players.at(i);
        const auto *tile = state.tileAt(player.position);
        if (!tile || player.bankrupt) continue;
        const QPointF offset((i % 3 - 1) * 27, (i / 3) * 28 - 14);
        renderPlayer(root, tile->worldPosition + offset, player.color, i == state.currentPlayer, i);
    }
}

} // namespace

TopDownCityView::TopDownCityView(QQuickItem *parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setAntialiasing(true);
    m_focusAnimation = new QPropertyAnimation(this, "cameraCenter", this);
    m_focusAnimation->setDuration(450);
    m_focusAnimation->setEasingCurve(QEasingCurve::InOutCubic);
}

QObject *TopDownCityView::viewModel() const { return m_viewModel; }

void TopDownCityView::setViewModel(QObject *object)
{
    auto *viewModel = qobject_cast<GameViewModel *>(object);
    if (m_viewModel == viewModel) return;
    if (m_viewModel) disconnect(m_viewModel, nullptr, this, nullptr);
    m_viewModel = viewModel;
    if (m_viewModel) connect(m_viewModel, &GameViewModel::stateChanged, this, &TopDownCityView::update);
    emit viewModelChanged();
    update();
}

void TopDownCityView::setZoom(qreal value)
{
    value = qBound(.55, value, 2.0);
    if (qFuzzyCompare(m_zoom, value)) return;
    m_zoom = value;
    clampCamera();
    emit cameraChanged();
    update();
}

void TopDownCityView::setCameraCenter(const QPointF &center)
{
    if (m_overviewMode) return;
    m_cameraCenter = center;
    clampCamera();
    emit cameraChanged();
    update();
}

void TopDownCityView::setOverviewMode(bool overview)
{
    if (m_overviewMode == overview) return;
    m_overviewMode = overview;
    emit overviewModeChanged();
    update();
}

qreal TopDownCityView::effectiveZoom() const
{
    if (!m_overviewMode) return m_zoom;
    return qMax(.01, qMin(width() / SceneLayout::worldBounds().width(),
                          height() / SceneLayout::worldBounds().height()) * .94);
}

QPointF TopDownCityView::effectiveCenter() const
{
    return m_overviewMode ? SceneLayout::worldBounds().center() : m_cameraCenter;
}

QPointF TopDownCityView::screenToWorld(const QPointF &screen) const
{
    const qreal scale = effectiveZoom();
    return effectiveCenter() + (screen - QPointF(width() * .5, height() * .5)) / scale;
}

void TopDownCityView::clampCamera()
{
    const QRectF bounds = SceneLayout::worldBounds();
    const qreal halfW = width() / qMax(.01, m_zoom) * .5;
    const qreal halfH = height() / qMax(.01, m_zoom) * .5;
    m_cameraCenter.setX(halfW * 2 >= bounds.width() ? bounds.center().x()
        : qBound(bounds.left() + halfW, m_cameraCenter.x(), bounds.right() - halfW));
    m_cameraCenter.setY(halfH * 2 >= bounds.height() ? bounds.center().y()
        : qBound(bounds.top() + halfH, m_cameraCenter.y(), bounds.bottom() - halfH));
}

int TopDownCityView::tileAt(qreal x, qreal y) const
{
    if (!m_viewModel) return -1;
    const QPointF world = screenToWorld({x, y});
    int closest = -1;
    qreal distance = 95.0;
    for (const auto &tile : m_viewModel->state().tiles) {
        const qreal candidate = QLineF(world, tile.worldPosition).length();
        if (candidate < distance) { distance = candidate; closest = tile.index; }
    }
    return closest;
}

void TopDownCityView::panBy(qreal dx, qreal dy)
{
    if (!m_overviewMode) {
        m_focusAnimation->stop();
        setCameraCenter(m_cameraCenter - QPointF(dx, dy) / m_zoom);
    }
}

void TopDownCityView::zoomAt(qreal x, qreal y, qreal delta)
{
    if (m_overviewMode) return;
    const QPointF before = screenToWorld({x, y});
    m_zoom = qBound(.55, m_zoom * std::pow(1.0015, delta), 2.0);
    m_cameraCenter += before - screenToWorld({x, y});
    clampCamera();
    emit cameraChanged();
    update();
}

void TopDownCityView::focusTile(int index)
{
    if (!m_viewModel || m_overviewMode) return;
    if (const auto *tile = m_viewModel->state().tileAt(index)) {
        m_focusAnimation->stop();
        m_focusAnimation->setStartValue(m_cameraCenter);
        m_focusAnimation->setEndValue(tile->worldPosition);
        m_focusAnimation->start();
    }
}

void TopDownCityView::focusCurrentPlayer()
{
    if (m_viewModel && m_viewModel->state().activePlayer()) focusTile(m_viewModel->state().activePlayer()->position);
}

void TopDownCityView::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    clampCamera();
    update();
}

QSGNode *TopDownCityView::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    auto *root = static_cast<CityRootNode *>(oldNode);
    if (!root) root = new CityRootNode;
    if (!m_viewModel) return root;
    const auto &state = m_viewModel->state();
    const int lod = effectiveZoom() < .72 ? 0 : (effectiveZoom() < 1.35 ? 1 : 2);
    if (root->sequence != state.sequence || root->matchId != state.matchId || root->lod != lod) {
        root->transform->removeChildNode(root->world);
        delete root->world;
        root->world = new QSGNode;
        root->transform->appendChildNode(root->world);
        rebuildWorld(root->world, state, lod);
        root->sequence = state.sequence;
        root->matchId = state.matchId;
        root->lod = lod;
    }
    const qreal scale = effectiveZoom();
    const QPointF center = effectiveCenter();
    QMatrix4x4 matrix;
    matrix.translate(float(width() * .5), float(height() * .5));
    matrix.scale(float(scale), float(scale));
    matrix.translate(float(-center.x()), float(-center.y()));
    root->transform->setMatrix(matrix);
    return root;
}

} // namespace neon
