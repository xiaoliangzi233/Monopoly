#include "core/city_content.h"

#include <QCryptographicHash>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>
#include <algorithm>

namespace neon {
namespace {

constexpr qreal WorldLeft = 400.0;
constexpr qreal WorldTop = 300.0;
constexpr qreal ColumnSpacing = 470.0;
constexpr qreal RowSpacing = 350.0;

int nodeAt(int x, int y)
{
    return y * 8 + ((y % 2) == 0 ? x : 7 - x);
}

void connect(QList<TileDefinition> &tiles, int left, int right)
{
    if (!tiles[left].neighbors.contains(right)) tiles[left].neighbors.append(right);
    if (!tiles[right].neighbors.contains(left)) tiles[right].neighbors.append(left);
}

template <typename Generator>
QStringList generatedList(int count, Generator generator)
{
    QStringList result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) result.append(generator(i));
    return result;
}

const QList<QStringList> &industryNames()
{
    static const QList<QStringList> names = {
        {QStringLiteral("木作坊"), QStringLiteral("营造局"), QStringLiteral("铜器铺"), QStringLiteral("造纸坊")},
        {QStringLiteral("织机坊"), QStringLiteral("染彩堂"), QStringLiteral("绫罗庄"), QStringLiteral("锦绣阁")},
        {QStringLiteral("本草铺"), QStringLiteral("仁心堂"), QStringLiteral("香药局"), QStringLiteral("百草园")},
        {QStringLiteral("书肆"), QStringLiteral("刻印坊"), QStringLiteral("砚墨斋"), QStringLiteral("藏书楼")},
        {QStringLiteral("平码头"), QStringLiteral("通远仓"), QStringLiteral("车马行"), QStringLiteral("市舶栈")},
        {QStringLiteral("茶寮"), QStringLiteral("酒肆"), QStringLiteral("食珍楼"), QStringLiteral("香茗馆")},
        {QStringLiteral("陶坊"), QStringLiteral("青瓷窑"), QStringLiteral("釉彩铺"), QStringLiteral("官瓷行")},
        {QStringLiteral("梨园"), QStringLiteral("灯彩坊"), QStringLiteral("花木园"), QStringLiteral("雅乐台")}
    };
    return names;
}

} // namespace

QList<TileDefinition> CityContent::createProsperousCityMap()
{
    const auto districts = districtNames();
    const auto industries = industryNames();
    QList<TileDefinition> tiles(64);

    for (int y = 0; y < 8; ++y) {
        int industry = 0;
        for (int x = 0; x < 8; ++x) {
            const int index = nodeAt(x, y);
            auto &tile = tiles[index];
            tile.index = index;
            tile.worldPosition = {WorldLeft + x * ColumnSpacing, WorldTop + y * RowSpacing};
            tile.district = y;
            tile.styleId = QStringLiteral("district-%1").arg(y);
            const QPointF p = tile.worldPosition;
            tile.roadPolygon = {{p.x() - 82, p.y() - 48}, {p.x() + 82, p.y() - 48},
                                {p.x() + 82, p.y() + 48}, {p.x() - 82, p.y() + 48}};

            if (index == 0) {
                tile.type = TileType::Start;
                tile.name = QStringLiteral("承平门");
            } else if (x == 1 || x == 2 || x == 5 || x == 6) {
                tile.type = TileType::Property;
                tile.name = districts[y] + QStringLiteral("·") + industries[y][industry++];
                tile.price = 2200 + y * 260 + (industry - 1) * 180;
                tile.baseRent = tile.price / 11;
                tile.industryFootprint = {p.x() - 145, p.y() + 68, 290, 155};
            }
        }
    }

    QList<int> special;
    for (const auto &tile : std::as_const(tiles))
        if (tile.index != 0 && tile.type != TileType::Property) special.append(tile.index);
    std::sort(special.begin(), special.end());
    int cursor = 0;
    auto assign = [&](int count, TileType type, const QString &name) {
        for (int i = 0; i < count; ++i) {
            auto &tile = tiles[special[cursor++]];
            tile.type = type;
            tile.name = name + (count > 1 ? QStringLiteral(" %1").arg(i + 1) : QString());
        }
    };
    assign(8, TileType::Event, QStringLiteral("市井奇遇"));
    assign(6, TileType::Transit, QStringLiteral("驿站漕运"));
    assign(4, TileType::Guild, QStringLiteral("百业商会"));
    assign(4, TileType::Civic, QStringLiteral("民生工程"));
    assign(4, TileType::Commission, QStringLiteral("文会委托"));
    assign(3, TileType::Tax, QStringLiteral("市舶税课"));
    assign(2, TileType::Festival, QStringLiteral("盛世节庆"));

    for (int i = 0; i < tiles.size() - 1; ++i) connect(tiles, i, i + 1);
    connect(tiles, 63, 0);
    for (int y = 0; y < 7; ++y) {
        const int x = (y % 2 == 0) ? 3 : 4;
        connect(tiles, nodeAt(x, y), nodeAt(x, y + 1));
    }
    for (auto &tile : tiles) std::sort(tile.neighbors.begin(), tile.neighbors.end());
    return tiles;
}

QByteArray CityContent::contentHash()
{
    QCborArray values;
    for (const auto &tile : createProsperousCityMap()) {
        QCborArray neighbors;
        for (int neighbor : tile.neighbors) neighbors.append(neighbor);
        values.append(QCborMap{{QStringLiteral("id"), tile.index}, {QStringLiteral("name"), tile.name},
            {QStringLiteral("type"), int(tile.type)}, {QStringLiteral("district"), tile.district},
            {QStringLiteral("x"), tile.worldPosition.x()}, {QStringLiteral("y"), tile.worldPosition.y()},
            {QStringLiteral("neighbors"), neighbors}, {QStringLiteral("price"), tile.price}});
    }
    return QCryptographicHash::hash(QCborValue(values).toCbor(), QCryptographicHash::Sha256).toHex();
}

QStringList CityContent::districtNames()
{
    return {QStringLiteral("天工坊"), QStringLiteral("锦绣坊"), QStringLiteral("百草巷"), QStringLiteral("文墨里"),
            QStringLiteral("通津埠"), QStringLiteral("茗香市"), QStringLiteral("瓷窑街"), QStringLiteral("乐游园")};
}

QStringList CityContent::characterNames()
{
    return {QStringLiteral("鲁班"), QStringLiteral("黄道婆"), QStringLiteral("李时珍"),
            QStringLiteral("沈括"), QStringLiteral("李清照"), QStringLiteral("郑和")};
}

QStringList CityContent::characterBiographies()
{
    return {
        QStringLiteral("春秋时期工匠祖师。本作以其营造与巧思为技能灵感，剧情为架空演绎。"),
        QStringLiteral("元代棉纺织革新者。本作以其纺织技艺与惠民贡献为技能灵感。"),
        QStringLiteral("明代医药学家。本作以本草研究与济世精神为技能灵感。"),
        QStringLiteral("北宋科学家、政治家。本作以格物、测算与《梦溪笔谈》为技能灵感。"),
        QStringLiteral("宋代词人、金石学者。本作以文学与金石收藏为技能灵感。"),
        QStringLiteral("明代航海家。本作以远航、交流与海上商路为技能灵感。")
    };
}

QStringList CityContent::cityPulseEvents()
{
    return {QStringLiteral("上元灯会"), QStringLiteral("春耕劝业"), QStringLiteral("运河开漕"),
        QStringLiteral("百工竞巧"), QStringLiteral("端阳竞渡"), QStringLiteral("中秋雅集"),
        QStringLiteral("秋市互贸"), QStringLiteral("瑞雪丰年"), QStringLiteral("万国来朝"),
        QStringLiteral("书院会讲"), QStringLiteral("杏林义诊"), QStringLiteral("窑火新成")};
}

QStringList CityContent::strategyCards()
{
    static const QStringList schools = {QStringLiteral("行商"), QStringLiteral("工造"),
        QStringLiteral("交游"), QStringLiteral("应变")};
    static const QStringList actions = {QStringLiteral("借东风"), QStringLiteral("巧周转"),
        QStringLiteral("访名师"), QStringLiteral("结善缘"), QStringLiteral("修栈道"), QStringLiteral("开新局"),
        QStringLiteral("护商契"), QStringLiteral("添薪火"), QStringLiteral("通关津"), QStringLiteral("聚百工"),
        QStringLiteral("观天时"), QStringLiteral("济邻里")};
    return generatedList(48, [&](int i) { return schools[i / 12] + QStringLiteral("·") + actions[i % 12]; });
}

QStringList CityContent::personalEvents()
{
    static const QStringList seeds = {QStringLiteral("雨后春笋"), QStringLiteral("失而复得"),
        QStringLiteral("远客来访"), QStringLiteral("匠心偶得"), QStringLiteral("邻里相助"), QStringLiteral("货通南北")};
    return generatedList(36, [&](int i) { return seeds[i % seeds.size()] + QStringLiteral("·第%1则").arg(i + 1); });
}

QStringList CityContent::missions()
{
    static const QStringList goals = {QStringLiteral("修桥铺路"), QStringLiteral("赈济邻里"),
        QStringLiteral("搜集金石"), QStringLiteral("百工兴业"), QStringLiteral("通达商路"), QStringLiteral("雅集传文")};
    return generatedList(24, [&](int i) { return goals[i % goals.size()] + QStringLiteral("·%1").arg(i + 1); });
}

} // namespace neon
