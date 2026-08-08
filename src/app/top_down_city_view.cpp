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
        staticWorld = new QSGNode;
        dynamicWorld = new QSGNode;
        appendChildNode(transform);
        transform->appendChildNode(staticWorld);
        transform->appendChildNode(dynamicWorld);
    }
    QSGTransformNode *transform = nullptr;
    QSGNode *staticWorld = nullptr;
    QSGNode *dynamicWorld = nullptr;
    quint64 sequence = std::numeric_limits<quint64>::max();
    quint64 visualRevision = std::numeric_limits<quint64>::max();
    quint64 staticFingerprint = std::numeric_limits<quint64>::max();
    QUuid matchId;
    QUuid dynamicMatchId;
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
    static const QList<QColor> colors = {QColor("#1E7376"), QColor("#3D6684"), QColor("#A23C50"), QColor("#3E8C78"),
        QColor("#4D8060"), QColor("#B0783E"), QColor("#28758A"), QColor("#785C8C")};
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

void renderIndustry(QSGNode *root, const TileDefinition &tile, int level, const QColor &owner,
                    bool mortgaged, bool active)
{
    const QRectF lot = tile.industryFootprint;
    const int visualLevel = qBound(1, level, 3);
    const int seed = tile.index * 37 + tile.district * 19;
    const QColor accent = districtColor(tile.district);
    QColor glass = accent.darker(142); glass.setAlpha(245);
    QColor glassLight = accent.lighter(168); glassLight.setAlpha(215);
    QColor metal("#A8B9B6");
    QColor warm("#E1B75D");
    if (active) { glass.setAlpha(150); glassLight.setAlpha(145); }

    addRect(root, lot, QColor("#31474A"));
    addRect(root, lot.adjusted(5, 5, -5, -5), QColor("#D6D9D0"));
    addRect(root, {lot.left() + 8, lot.bottom() - 28, lot.width() - 16, 20}, QColor("#667C75"));
    addLine(root, {lot.left() + 18, lot.bottom() - 18}, {lot.right() - 18, lot.bottom() - 18}, 3, warm);

    auto tower = [&](QRectF body, int bays, bool terrace) {
        addRect(root, body.translated(8, 8), QColor(15, 31, 34, 90));
        addRect(root, body, glass);
        addRect(root, {body.left(), body.top(), body.width(), 7}, metal);
        if (terrace) {
            addRect(root, {body.left() - 7, body.top() + body.height() * .46,
                           body.width() + 14, 8}, QColor("#D6DAD2"));
            addRect(root, {body.left() - 3, body.top() + body.height() * .46 - 5,
                           body.width() + 6, 3}, accent.lighter(148));
        }
        for (int bay = 1; bay < bays; ++bay)
            addRect(root, {body.left() + bay * body.width() / bays - 1, body.top() + 9,
                           2, body.height() - 16}, glassLight);
        for (qreal y = body.top() + 18; y < body.bottom() - 8; y += 20)
            addRect(root, {body.left() + 5, y, body.width() - 10, 2}, QColor("#D6B85F"));
    };

    switch (tile.district) {
    case 0: { // 智造湾：锯齿厂房、机械核心与连廊
        tower({lot.left() + 22, lot.top() + 42, lot.width() - 44, 64.0 + visualLevel * 12.0}, 6, false);
        for (int i = 0; i < 5; ++i)
            addPolygon(root, {{lot.left() + 30 + i * 45.0, lot.top() + 42},
                              {lot.left() + 48 + i * 45.0, lot.top() + 24 - (seed % 9)},
                              {lot.left() + 70 + i * 45.0, lot.top() + 42}}, metal);
        addPolygon(root, ellipse(lot.center() + QPointF(0, 12), 20 + visualLevel * 4, 20, 18), accent.lighter(145));
        break;
    }
    case 1: { // 云创中心：双塔、空中连廊、数据灯带
        tower({lot.left() + 33, lot.top() + 22, 70, 88.0 + visualLevel * 13.0}, 3, true);
        tower({lot.right() - 108, lot.top() + 38 - (seed % 8), 75, 76.0 + visualLevel * 15.0}, 3, true);
        addRect(root, {lot.center().x() - 45, lot.top() + 70, 90, 15}, glassLight);
        addRect(root, {lot.center().x() - 41, lot.top() + 74, 82, 4}, warm);
        break;
    }
    case 2: { // 潮流艺仓：错位盒子、展厅天窗、朱红框架
        tower({lot.left() + 24, lot.top() + 54, 128, 65}, 4, false);
        tower({lot.left() + 116, lot.top() + 25, 138, 68.0 + visualLevel * 10.0}, 5, false);
        addPolygon(root, {{lot.left() + 38, lot.top() + 54}, {lot.left() + 94, lot.top() + 20},
                          {lot.left() + 146, lot.top() + 54}}, QColor("#C23C44"));
        addRect(root, {lot.left() + 12, lot.top() + 44, 7, 88}, QColor("#C23C44"));
        break;
    }
    case 3: { // 健康新城：弧形体量、空中花园
        addPolygon(root, ellipse(lot.center() - QPointF(0, 12), 102, 58 + visualLevel * 7, 28), glass);
        addPolygon(root, ellipse(lot.center() - QPointF(0, 17), 78, 39 + visualLevel * 4, 28), QColor("#E5E7DF"));
        addPolygon(root, ellipse(lot.center() - QPointF(0, 19), 56, 25, 24), QColor("#5A9C78"));
        addRect(root, {lot.center().x() - 11, lot.center().y() + 13, 22, 45}, glassLight);
        break;
    }
    case 4: { // 低碳绿谷：退台、光伏屋顶、垂直绿化
        tower({lot.left() + 31, lot.top() + 58, lot.width() - 62, 69}, 5, true);
        if (visualLevel >= 2) tower({lot.left() + 73, lot.top() + 30, lot.width() - 146, 47}, 3, true);
        for (int i = 0; i < 4; ++i)
            addRect(root, {lot.left() + 42 + i * 51.0, lot.top() + 63, 34, 18}, QColor("#245B70"));
        addRect(root, {lot.left() + 22, lot.top() + 91, 12, 38}, QColor("#4F8A62"));
        addRect(root, {lot.right() - 34, lot.top() + 74, 12, 55}, QColor("#4F8A62"));
        break;
    }
    case 5: { // 都会商圈：高低塔楼、裙房、金色冠部
        tower({lot.left() + 25, lot.top() + 54, lot.width() - 50, 73}, 7, false);
        tower({lot.left() + 68, lot.top() + 15, 67, 92.0 + visualLevel * 11.0}, 3, true);
        tower({lot.right() - 121, lot.top() + 32, 64, 76.0 + visualLevel * 8.0}, 3, false);
        addPolygon(root, {{lot.left() + 67, lot.top() + 15}, {lot.left() + 101, lot.top() - 4},
                          {lot.left() + 136, lot.top() + 15}}, warm);
        break;
    }
    case 6: { // 滨水港区：仓储模块、起重臂、物流连廊
        tower({lot.left() + 20, lot.top() + 60, 168, 66}, 6, false);
        tower({lot.right() - 91, lot.top() + 36, 63, 90}, 3, false);
        addLine(root, {lot.left() + 40, lot.top() + 57}, {lot.left() + 40, lot.top() + 15}, 7, QColor("#E0B354"));
        addLine(root, {lot.left() + 40, lot.top() + 16}, {lot.left() + 126, lot.top() + 16}, 6, QColor("#E0B354"));
        addLine(root, {lot.left() + 123, lot.top() + 16}, {lot.left() + 150, lot.top() + 45}, 5, QColor("#E0B354"));
        break;
    }
    default: { // 城市乐活区：剧场弧顶、开放露台、灯光舞台
        addPolygon(root, {{lot.left() + 25, lot.bottom() - 28}, {lot.left() + 52, lot.top() + 42},
                          {lot.center().x(), lot.top() + 16 - visualLevel * 5},
                          {lot.right() - 52, lot.top() + 42}, {lot.right() - 25, lot.bottom() - 28}}, glass);
        addPolygon(root, ellipse(lot.center() + QPointF(0, 25), 76, 38, 24), QColor("#D9DCD4"));
        addPolygon(root, ellipse(lot.center() + QPointF(0, 25), 55, 25, 24), QColor("#8D3E66"));
        for (int i = -2; i <= 2; ++i) addLine(root, lot.center() + QPointF(i * 24, 3), lot.center() + QPointF(i * 32, -38), 3, warm);
        break;
    }
    }

    // 每块产业的参数种子改变入口与绿化位置，相邻模型不会形成机械复制。
    const qreal entranceX = lot.left() + 38 + (seed % 5) * 42.0;
    addRect(root, {entranceX, lot.bottom() - 42, 27, 32}, QColor("#9C343B"));
    renderTree(root, lot.topLeft() + QPointF(20 + seed % 31, 29), 13 + seed % 5);
    renderTree(root, lot.bottomRight() - QPointF(25 + seed % 27, 24), 12 + (seed / 3) % 5);
    addRect(root, {lot.left() + 10, lot.top() + 10, 38, 7}, owner.isValid() ? owner : accent);
    if (mortgaged) addRect(root, lot.adjusted(11, 11, -11, -11), QColor(36, 42, 44, 150));
}

void renderLandmark(QSGNode *root, const QPointF &p, int landmark)
{
    const QColor glass("#2C6673"), metal("#CFD8D3"), red("#B73840"), gold("#D6AE56");
    switch (landmark) {
    case 0: // 上海中心式螺旋塔
        for (int i = 0; i < 7; ++i) {
            const qreal w = 76 - i * 7.0;
            addPolygon(root, {{p.x() - w / 2 + i * 3, p.y() + 46 - i * 16.0},
                              {p.x() + w / 2, p.y() + 46 - i * 16.0},
                              {p.x() + w / 2 - 8, p.y() + 31 - i * 16.0},
                              {p.x() - w / 2, p.y() + 31 - i * 16.0}}, i % 2 ? glass.lighter(120) : glass);
        }
        addLine(root, p + QPointF(11, -64), p + QPointF(18, -91), 4, gold); break;
    case 1: // 东方明珠式球塔
        addLine(root, p + QPointF(0, 48), p + QPointF(0, -79), 10, metal);
        addPolygon(root, ellipse(p + QPointF(0, 5), 31, 25), red);
        addPolygon(root, ellipse(p + QPointF(0, -46), 18, 15), red.lighter(116));
        addLine(root, p + QPointF(0, -60), p + QPointF(0, -91), 4, gold); break;
    case 2: // 广州塔式网格腰身
        addPolygon(root, {{p.x() - 32, p.y() + 49}, {p.x() - 12, p.y() - 66},
                          {p.x() + 12, p.y() - 66}, {p.x() + 32, p.y() + 49}}, QColor("#B9CBC6"));
        for (int i = 0; i < 6; ++i) addLine(root, p + QPointF(-27 + i * 10, 43), p + QPointF(8 - i * 3, -61), 2, red);
        addLine(root, p + QPointF(0, -66), p + QPointF(0, -91), 4, gold); break;
    case 3: // 国家体育场式编织结构
        addPolygon(root, ellipse(p, 55, 38), QColor("#BFCAC6"));
        addPolygon(root, ellipse(p, 39, 24), QColor("#294A50"));
        for (int i = -4; i <= 4; ++i) addLine(root, p + QPointF(-48, i * 7), p + QPointF(48, -i * 7), 3, red); break;
    case 4: // 港珠澳大桥式桥塔
        addLine(root, p - QPointF(82, 0), p + QPointF(82, 0), 10, metal);
        for (int side : {-1, 1}) {
            addLine(root, p + QPointF(side * 34, 22), p + QPointF(side * 34, -48), 8, red);
            addLine(root, p + QPointF(side * 34, -42), p + QPointF(side * 78, 0), 3, gold);
        }
        break;
    case 5: // 天坛祈年殿的现代化圆形展馆意象
        addPolygon(root, ellipse(p + QPointF(0, 22), 58, 24), QColor("#D5D8D0"));
        addPolygon(root, ellipse(p, 47, 31), QColor("#235C68"));
        addPolygon(root, ellipse(p - QPointF(0, 18), 34, 21), QColor("#2F7180"));
        addPolygon(root, ellipse(p - QPointF(0, 34), 21, 13), gold); break;
    case 6: // 长城关城式城市门廊
        addRect(root, {p.x() - 58, p.y() - 33, 116, 68}, QColor("#8A8173"));
        addRect(root, {p.x() - 18, p.y() - 4, 36, 39}, QColor("#243B3D"));
        for (int i = 0; i < 6; ++i) addRect(root, {p.x() - 58 + i * 23.0, p.y() - 45, 15, 14}, red);
        addLine(root, p - QPointF(88, 23), p - QPointF(58, 4), 16, QColor("#8A8173"));
        addLine(root, p + QPointF(58, 4), p + QPointF(88, 23), 16, QColor("#8A8173")); break;
    default: // 江南园林月门与现代景观廊架
        addPolygon(root, ellipse(p, 51, 51), QColor("#ECE9DF"));
        addPolygon(root, ellipse(p, 31, 34), QColor("#294B4D"));
        addLine(root, p - QPointF(76, 35), p + QPointF(76, 35), 8, red);
        renderTree(root, p - QPointF(62, 9), 20); renderTree(root, p + QPointF(63, 12), 18); break;
    }
}

void renderSpecial(QSGNode *root, const TileDefinition &tile)
{
    const QPointF p = tile.worldPosition;
    if (tile.styleId.startsWith(QStringLiteral("landmark-"))) {
        renderLandmark(root, p, tile.styleId.sliced(9).toInt());
        return;
    }
    switch (tile.type) {
    case TileType::Start:
        addPolygon(root, ellipse(p, 63, 48), QColor("#E4E3DB"));
        addPolygon(root, ellipse(p, 47, 34), QColor("#1F5A60"));
        addPolygon(root, ellipse(p, 23, 17), QColor("#B93B42"));
        addLine(root, p - QPointF(70, 0), p + QPointF(70, 0), 5, QColor("#D6AE56"));
        break;
    case TileType::Transit:
        addRect(root, {p.x() - 58, p.y() - 31, 116, 62}, QColor("#296270"));
        addPolygon(root, {{p.x() - 70, p.y() - 31}, {p.x(), p.y() - 54}, {p.x() + 70, p.y() - 31}}, QColor("#D9DCD4"));
        addLine(root, p - QPointF(82, 41), p + QPointF(82, 41), 7, QColor("#D6AE56"));
        break;
    case TileType::Guild:
        addPolygon(root, {{p.x() - 48, p.y() + 35}, {p.x() - 32, p.y() - 35},
                          {p.x() + 32, p.y() - 35}, {p.x() + 48, p.y() + 35}}, QColor("#315F68"));
        addPolygon(root, ellipse(p, 16, 16), QColor("#D6AE56"));
        break;
    case TileType::Civic:
        addPolygon(root, ellipse(p, 52, 36), QColor("#4F8E73"));
        addPolygon(root, ellipse(p, 29, 20), QColor("#87B6A0"));
        addPolygon(root, ellipse(p, 11, 9), QColor("#E3E3D9"));
        break;
    case TileType::Commission:
        addRect(root, {p.x() - 52, p.y() - 31, 104, 62}, QColor("#315C70"));
        addRect(root, {p.x() - 44, p.y() - 23, 88, 9}, QColor("#B93B42"));
        addRect(root, {p.x() - 5, p.y() - 10, 10, 36}, QColor("#D8B65E"));
        break;
    case TileType::Tax:
        addRect(root, {p.x() - 44, p.y() - 32, 88, 64}, QColor("#D8D8D0"));
        addPolygon(root, {{p.x() - 52, p.y() - 32}, {p.x(), p.y() - 55}, {p.x() + 52, p.y() - 32}}, QColor("#374F55"));
        break;
    case TileType::Festival:
        addLine(root, p - QPointF(68, 14), p + QPointF(68, 14), 5, QColor("#B93B42"));
        for (int i = -2; i <= 2; ++i) {
            addLine(root, p + QPointF(i * 26, 23), p + QPointF(i * 26, -24), 3, QColor("#C5CDD0"));
            addPolygon(root, ellipse(p + QPointF(i * 26, -27), 9, 9), i % 2 ? QColor("#D8B45C") : QColor("#B93B42"));
        }
        break;
    case TileType::Event:
        addRect(root, {p.x() - 42, p.y() - 28, 84, 56}, districtColor(tile.district));
        addRect(root, {p.x() - 34, p.y() - 20, 68, 8}, QColor("#D8B45C"));
        renderTree(root, p - QPointF(54, 2), 19);
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

void rebuildStaticWorld(QSGNode *root, const GameState &state, int lod)
{
    addRect(root, SceneLayout::worldBounds(), QColor("#9EAFA4"));
    addRect(root, {0, 0, 4096, 170}, QColor("#174954"));
    addRect(root, {0, 2870, 4096, 202}, QColor("#174954"));
    addRect(root, {1960, 0, 176, 3072}, QColor("#327487"));
    for (int wave = 0; wave < 18; ++wave) {
        const qreal x = 70.0 + wave * 236.0;
        addLine(root, {x, 83}, {x + 88, 83}, 3, QColor("#75AEB0"));
        addLine(root, {x + 28, 2985}, {x + 116, 2985}, 3, QColor("#75AEB0"));
    }
    for (int bridge = 0; bridge < 3; ++bridge) {
        const qreal y = 560.0 + bridge * 890.0;
        addRect(root, {1944, y - 72, 208, 144}, QColor("#273E42"));
        addRect(root, {1952, y - 63, 192, 126}, QColor("#B9C6C1"));
        for (int plank = 0; plank < 6; ++plank)
            addRect(root, {1960 + plank * 30.0, y - 58, 3, 116}, QColor("#B33C43"));
    }
    for (int district = 0; district < 8; ++district) {
        QColor wash = districtColor(district).lighter(165); wash.setAlpha(70);
        addRect(root, {180, 165 + district * 350.0, 3735, 270}, wash);
    }
    for (const auto &tile : state.tiles) {
        for (int neighbor : tile.neighbors) if (neighbor > tile.index) {
            const auto *other = state.tileAt(neighbor);
            if (!other) continue;
            addLine(root, tile.worldPosition, other->worldPosition, 86, QColor("#3A4D4D"));
            addLine(root, tile.worldPosition, other->worldPosition, 64, QColor("#6E7C79"));
            if (lod >= 1) addLine(root, tile.worldPosition, other->worldPosition, 3, QColor("#D2B15E"));
            if (lod >= 2) {
                const QLineF segment(tile.worldPosition, other->worldPosition);
                for (qreal at = 115; at < segment.length() - 70; at += 135) {
                    const QPointF p = segment.pointAt(at / segment.length());
                    addPolygon(root, ellipse(p, 7, 5, 12), QColor("#DCE0D9"));
                }
            }
        }
    }
    for (const auto &tile : state.tiles) {
        addPolygon(root, ellipse(tile.worldPosition, 58, 42), QColor("#314648"));
        addPolygon(root, ellipse(tile.worldPosition, 48, 33), QColor("#AAB7B1"));
        addPolygon(root, ellipse(tile.worldPosition, 33, 21), QColor("#E1E4DC"));
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
            addLine(root, tile.worldPosition + QPointF(90, 48), tile.worldPosition + QPointF(90, 10), 4, QColor("#40585A"));
            addPolygon(root, ellipse(tile.worldPosition + QPointF(90, 7), 10, 8), QColor("#E2BA61"));
        }
    }
}

void rebuildDynamicWorld(QSGNode *root, const GameState &state, const QUuid &animatingPlayerId,
                         const QPointF &actorFrom, const QPointF &actorTo, qreal actorProgress)
{
    for (int option = 0; option < state.routeOptions.size(); ++option) {
        const auto &route = state.routeOptions.at(option);
        QColor choice = option == 0 ? QColor("#C03A42") : QColor("#D5AD55"); choice.setAlpha(155);
        QPointF previous;
        bool hasPrevious = false;
        for (int node : route) {
            const auto *tile = state.tileAt(node);
            if (!tile) continue;
            if (hasPrevious) addLine(root, previous, tile->worldPosition, 16, choice);
            addPolygon(root, ellipse(tile->worldPosition, 66, 45), choice);
            previous = tile->worldPosition;
            hasPrevious = true;
        }
    }
    if (state.phase == GamePhase::Moving && !state.pendingMovePath.isEmpty()) {
        QColor progress("#E0B559"); progress.setAlpha(185);
        for (int i = state.pendingMoveIndex; i < state.pendingMovePath.size(); ++i) {
            const auto *tile = state.tileAt(state.pendingMovePath.at(i));
            if (tile) addPolygon(root, ellipse(tile->worldPosition, 61, 41), progress);
        }
    }
    for (int i = 0; i < state.players.size(); ++i) {
        const auto &player = state.players.at(i);
        const auto *tile = state.tileAt(player.position);
        if (!tile || player.bankrupt) continue;
        const QPointF offset((i % 3 - 1) * 27, (i / 3) * 28 - 14);
        const QPointF base = player.id == animatingPlayerId
            ? actorFrom + (actorTo - actorFrom) * actorProgress : tile->worldPosition;
        renderPlayer(root, base + offset, player.color, i == state.currentPlayer, i);
    }
}

quint64 cityStaticFingerprint(const GameState &state)
{
    quint64 value = 1469598103934665603ULL;
    for (const auto &property : state.properties) {
        value ^= quint64(property.tileIndex + 1) * 1099511628211ULL;
        value ^= quint64(property.level + 1) << ((property.tileIndex % 7) + 3);
        value ^= property.mortgaged ? 0x9e3779b97f4a7c15ULL : 0;
        value ^= qHash(property.ownerId);
        value *= 1099511628211ULL;
    }
    return value;
}

} // namespace

TopDownCityView::TopDownCityView(QQuickItem *parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setAntialiasing(true);
    m_focusAnimation = new QPropertyAnimation(this, "cameraCenter", this);
    m_focusAnimation->setDuration(450);
    m_focusAnimation->setEasingCurve(QEasingCurve::InOutCubic);
    m_actorAnimation = new QVariantAnimation(this);
    m_actorAnimation->setDuration(260);
    m_actorAnimation->setStartValue(0.0);
    m_actorAnimation->setEndValue(1.0);
    m_actorAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    connect(m_actorAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_actorProgress = value.toReal();
        ++m_visualRevision;
        update();
    });
}

QObject *TopDownCityView::viewModel() const { return m_viewModel; }

void TopDownCityView::setViewModel(QObject *object)
{
    auto *viewModel = qobject_cast<GameViewModel *>(object);
    if (m_viewModel == viewModel) return;
    if (m_viewModel) disconnect(m_viewModel, nullptr, this, nullptr);
    m_viewModel = viewModel;
    if (m_viewModel) {
        connect(m_viewModel, &GameViewModel::stateChanged, this, &TopDownCityView::onViewModelStateChanged);
        for (const auto &player : m_viewModel->state().players) m_lastPlayerPositions.insert(player.id, player.position);
    }
    emit viewModelChanged();
    update();
}

void TopDownCityView::onViewModelStateChanged()
{
    if (!m_viewModel) return;
    const auto &state = m_viewModel->state();
    for (const auto &player : state.players) {
        const int previous = m_lastPlayerPositions.value(player.id, player.position);
        if (previous != player.position) {
            const auto *from = state.tileAt(previous);
            const auto *to = state.tileAt(player.position);
            if (from && to) {
                m_animatingPlayerId = player.id;
                m_actorFrom = from->worldPosition;
                m_actorTo = to->worldPosition;
                m_actorProgress = 0.0;
                m_actorAnimation->stop();
                m_actorAnimation->start();
            }
        }
        m_lastPlayerPositions.insert(player.id, player.position);
    }
    ++m_visualRevision;
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
    const quint64 staticFingerprint = cityStaticFingerprint(state);
    if (root->staticFingerprint != staticFingerprint || root->matchId != state.matchId || root->lod != lod) {
        root->transform->removeChildNode(root->staticWorld);
        delete root->staticWorld;
        root->staticWorld = new QSGNode;
        root->transform->prependChildNode(root->staticWorld);
        rebuildStaticWorld(root->staticWorld, state, lod);
        root->staticFingerprint = staticFingerprint;
        root->matchId = state.matchId;
        root->lod = lod;
    }
    if (root->sequence != state.sequence || root->dynamicMatchId != state.matchId
        || root->visualRevision != m_visualRevision) {
        root->transform->removeChildNode(root->dynamicWorld);
        delete root->dynamicWorld;
        root->dynamicWorld = new QSGNode;
        root->transform->appendChildNode(root->dynamicWorld);
        rebuildDynamicWorld(root->dynamicWorld, state, m_animatingPlayerId,
                            m_actorFrom, m_actorTo, m_actorProgress);
        root->sequence = state.sequence;
        root->visualRevision = m_visualRevision;
        root->dynamicMatchId = state.matchId;
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
