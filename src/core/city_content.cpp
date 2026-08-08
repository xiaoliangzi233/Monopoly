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
        {QStringLiteral("机器人实验室"), QStringLiteral("精密制造中心"), QStringLiteral("新能源车间"), QStringLiteral("空天创客基地")},
        {QStringLiteral("人工智能中心"), QStringLiteral("云计算园区"), QStringLiteral("软件创新工场"), QStringLiteral("数字安全中心")},
        {QStringLiteral("国潮设计中心"), QStringLiteral("动画制作基地"), QStringLiteral("新消费体验店"), QStringLiteral("文化创意园")},
        {QStringLiteral("智慧医疗中心"), QStringLiteral("生物科技园"), QStringLiteral("城市运动中心"), QStringLiteral("健康管理社区")},
        {QStringLiteral("光伏科技园"), QStringLiteral("风能研发中心"), QStringLiteral("循环经济基地"), QStringLiteral("都市农业工场")},
        {QStringLiteral("金融科技中心"), QStringLiteral("智慧零售中心"), QStringLiteral("国际会展中心"), QStringLiteral("城市商业综合体")},
        {QStringLiteral("智慧港口"), QStringLiteral("冷链物流中心"), QStringLiteral("跨境电商基地"), QStringLiteral("综合交通枢纽")},
        {QStringLiteral("数字艺术馆"), QStringLiteral("影视制作中心"), QStringLiteral("城市露营公园"), QStringLiteral("现场音乐空间")}
    };
    return names;
}

} // namespace

QList<TileDefinition> CityContent::createProsperousCityMap()
{
    const auto districts = districtNames();
    const auto industries = industryNames();
    const auto landmarks = landmarkNames();
    QList<TileDefinition> tiles(64);

    for (int y = 0; y < 8; ++y) {
        int industry = 0;
        for (int x = 0; x < 8; ++x) {
            const int index = nodeAt(x, y);
            auto &tile = tiles[index];
            tile.index = index;
            tile.worldPosition = {WorldLeft + x * ColumnSpacing, WorldTop + y * RowSpacing};
            tile.district = y;
            tile.styleId = QStringLiteral("modern-%1-node-%2").arg(y).arg(index);
            const QPointF p = tile.worldPosition;
            tile.roadPolygon = {{p.x() - 82, p.y() - 48}, {p.x() + 82, p.y() - 48},
                                {p.x() + 82, p.y() + 48}, {p.x() - 82, p.y() + 48}};

            if (index == 0) {
                tile.type = TileType::Start;
                tile.name = QStringLiteral("城市创想中心");
                tile.styleId = QStringLiteral("modern-start");
            } else if (x == 1 || x == 2 || x == 5 || x == 6) {
                tile.type = TileType::Property;
                tile.name = districts[y] + QStringLiteral(" · ") + industries[y][industry];
                tile.styleId = QStringLiteral("industry-%1-%2-%3").arg(y).arg(industry).arg(index);
                tile.price = 4200 + y * 420 + industry * 360;
                tile.baseRent = tile.price / 12;
                tile.industryFootprint = {p.x() - 145, p.y() + 68, 290, 155};
                ++industry;
            }
        }
    }

    QList<int> special;
    for (const auto &tile : std::as_const(tiles))
        if (tile.index != 0 && tile.type != TileType::Property) special.append(tile.index);
    std::sort(special.begin(), special.end());
    int cursor = 0;
    for (int i = 0; i < 8; ++i) {
        auto &tile = tiles[special[cursor++]];
        tile.type = TileType::Event;
        tile.name = QStringLiteral("城市机遇 · ") + landmarks[i];
        tile.styleId = QStringLiteral("landmark-%1").arg(i);
    }
    auto assign = [&](int count, TileType type, const QString &name, const QString &style) {
        for (int i = 0; i < count; ++i) {
            auto &tile = tiles[special[cursor++]];
            tile.type = type;
            tile.name = name + QStringLiteral(" %1").arg(i + 1);
            tile.styleId = QStringLiteral("%1-%2").arg(style).arg(i);
        }
    };
    assign(6, TileType::Transit, QStringLiteral("城市交通枢纽"), QStringLiteral("transit"));
    assign(4, TileType::Guild, QStringLiteral("产业协作中心"), QStringLiteral("guild"));
    assign(4, TileType::Civic, QStringLiteral("公共城市项目"), QStringLiteral("civic"));
    assign(4, TileType::Commission, QStringLiteral("创新项目委托"), QStringLiteral("commission"));
    assign(3, TileType::Tax, QStringLiteral("城市运营费用"), QStringLiteral("tax"));
    assign(2, TileType::Festival, QStringLiteral("城市潮流节"), QStringLiteral("festival"));

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
            {QStringLiteral("style"), tile.styleId}, {QStringLiteral("type"), int(tile.type)},
            {QStringLiteral("district"), tile.district}, {QStringLiteral("x"), tile.worldPosition.x()},
            {QStringLiteral("y"), tile.worldPosition.y()}, {QStringLiteral("neighbors"), neighbors},
            {QStringLiteral("price"), tile.price}, {QStringLiteral("rent"), tile.baseRent}});
    }
    for (const auto &name : characterNames()) values.append(name);
    for (const auto &name : strategyCards()) values.append(name);
    for (const auto &name : personalEvents()) values.append(name);
    for (const auto &name : missions()) values.append(name);
    for (const auto &name : cityPulseEvents()) values.append(name);
    return QCryptographicHash::hash(QCborValue(values).toCbor(), QCryptographicHash::Sha256).toHex();
}

QStringList CityContent::districtNames()
{
    return {QStringLiteral("智造湾"), QStringLiteral("云创中心"), QStringLiteral("潮流艺仓"), QStringLiteral("健康新城"),
            QStringLiteral("低碳绿谷"), QStringLiteral("都会商圈"), QStringLiteral("滨水港区"), QStringLiteral("城市乐活区")};
}

QStringList CityContent::characterNames()
{
    return {QStringLiteral("程砚"), QStringLiteral("唐织"), QStringLiteral("夏岚"),
            QStringLiteral("周衡"), QStringLiteral("许知意"), QStringLiteral("陆远航")};
}

QStringList CityContent::characterBiographies()
{
    return {
        QStringLiteral("城市建筑师，擅长以更低成本完成高品质城市更新。"),
        QStringLiteral("国潮品牌设计师，善于把完整街区转化为有影响力的城市品牌。"),
        QStringLiteral("健康科技主理人，重视风险管理与所有人的宜居体验。"),
        QStringLiteral("智能工程师，利用数据模型重新评估路线与行动方案。"),
        QStringLiteral("文化内容主理人，以创意策划推动城市创新与公共表达。"),
        QStringLiteral("供应链主理人，连接交通枢纽、城市产业和跨区域贸易。")
    };
}

QStringList CityContent::cityPulseEvents()
{
    return {QStringLiteral("国际消费季"), QStringLiteral("人工智能应用周"), QStringLiteral("城市更新计划"),
        QStringLiteral("绿色能源行动"), QStringLiteral("青年创意节"), QStringLiteral("智慧交通升级"),
        QStringLiteral("全民运动月"), QStringLiteral("数字艺术双年展"), QStringLiteral("产业协同峰会"),
        QStringLiteral("夜间经济计划"), QStringLiteral("公共空间焕新"), QStringLiteral("低碳生活倡议")};
}

QStringList CityContent::strategyCards()
{
    static const QStringList schools = {QStringLiteral("商业策略"), QStringLiteral("科技创新"),
        QStringLiteral("品牌传播"), QStringLiteral("城市协作")};
    static const QStringList actions = {QStringLiteral("精准投放"), QStringLiteral("灵活周转"),
        QStringLiteral("人才引进"), QStringLiteral("联合发布"), QStringLiteral("交通优化"), QStringLiteral("产品迭代"),
        QStringLiteral("风险保障"), QStringLiteral("能源补给"), QStringLiteral("快速通行"), QStringLiteral("产业联盟"),
        QStringLiteral("趋势研判"), QStringLiteral("社区共建")};
    return generatedList(48, [&](int i) { return schools[i / 12] + QStringLiteral(" · ") + actions[i % 12]; });
}

QStringList CityContent::personalEvents()
{
    static const QStringList seeds = {QStringLiteral("新品意外走红"), QStringLiteral("系统临时故障"),
        QStringLiteral("跨界合作邀约"), QStringLiteral("技术方案突破"), QStringLiteral("社区主动支持"),
        QStringLiteral("供应链价格波动")};
    return generatedList(36, [&](int i) { return seeds[i % seeds.size()] + QStringLiteral(" · 案例%1").arg(i + 1); });
}

QStringList CityContent::missions()
{
    static const QStringList goals = {QStringLiteral("建设公共空间"), QStringLiteral("提升社区宜居"),
        QStringLiteral("完成创新研发"), QStringLiteral("打造产业集群"), QStringLiteral("连接交通枢纽"),
        QStringLiteral("推出城市文化项目")};
    return generatedList(24, [&](int i) { return goals[i % goals.size()] + QStringLiteral(" · 项目%1").arg(i + 1); });
}

QStringList CityContent::landmarkNames()
{
    return {QStringLiteral("上海中心"), QStringLiteral("东方明珠"), QStringLiteral("广州塔"),
        QStringLiteral("国家体育场"), QStringLiteral("港珠澳大桥"), QStringLiteral("天坛祈年殿"),
        QStringLiteral("长城关城"), QStringLiteral("江南园林月门")};
}

} // namespace neon
