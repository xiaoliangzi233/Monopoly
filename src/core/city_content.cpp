#include "core/city_content.h"

namespace neon {
namespace {

QList<QPoint> perimeterPositions()
{
    QList<QPoint> positions;
    positions.reserve(48);
    for (int x = 0; x <= 14; ++x) positions.append({x, 0});
    for (int y = 1; y <= 10; ++y) positions.append({14, y});
    for (int x = 13; x >= 0; --x) positions.append({x, 10});
    for (int y = 9; y >= 1; --y) positions.append({0, y});
    return positions;
}

template <typename Generator>
QStringList generatedList(int count, Generator generator)
{
    QStringList result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) result.append(generator(i));
    return result;
}

} // namespace

QList<TileDefinition> CityContent::createNeonCityMap()
{
    const auto positions = perimeterPositions();
    const QStringList districtNames = {
        QStringLiteral("星港"), QStringLiteral("流光"), QStringLiteral("云芯"), QStringLiteral("幻彩"),
        QStringLiteral("天际"), QStringLiteral("脉冲"), QStringLiteral("晶塔"), QStringLiteral("极光")
    };
    const QList<int> propertySlots = {
        1, 2, 4, 6, 7, 9, 11, 12, 14, 16, 17, 19,
        21, 22, 24, 26, 27, 29, 31, 32, 34, 36, 37, 39
    };

    QList<TileDefinition> tiles;
    tiles.reserve(48);
    int propertyNumber = 0;
    for (int i = 0; i < positions.size(); ++i) {
        TileDefinition tile;
        tile.index = i;
        tile.gridPosition = positions.at(i);
        if (i == 0) {
            tile.type = TileType::Start;
            tile.name = QStringLiteral("霓虹广场");
        } else if (propertySlots.contains(i)) {
            tile.type = TileType::Property;
            tile.district = propertyNumber / 3;
            tile.price = 1800 + tile.district * 280 + (propertyNumber % 3) * 160;
            tile.baseRent = tile.price / 10;
            tile.name = districtNames.at(tile.district) + QStringLiteral("·")
                + QString::number(propertyNumber % 3 + 1);
            ++propertyNumber;
        } else {
            static const QList<TileType> cycle = {
                TileType::Event, TileType::Transit, TileType::Mission, TileType::Shop,
                TileType::Event, TileType::Service, TileType::Tax
            };
            tile.type = cycle.at(i % cycle.size());
            switch (tile.type) {
            case TileType::Event: tile.name = QStringLiteral("城市奇遇"); break;
            case TileType::Transit: tile.name = QStringLiteral("磁悬浮站"); break;
            case TileType::Mission: tile.name = QStringLiteral("委托终端"); break;
            case TileType::Shop: tile.name = QStringLiteral("策略商店"); break;
            case TileType::Service: tile.name = QStringLiteral("城市服务"); break;
            case TileType::Tax: tile.name = QStringLiteral("能源税站"); break;
            default: break;
            }
        }
        tiles.append(tile);
    }
    return tiles;
}

QStringList CityContent::characterNames()
{
    return {QStringLiteral("凌光"), QStringLiteral("米娅"), QStringLiteral("零号"),
            QStringLiteral("阿洛"), QStringLiteral("月见"), QStringLiteral("欧姆")};
}

QStringList CityContent::cityPulseEvents()
{
    return {
        QStringLiteral("霓虹节：娱乐街区租金提升"), QStringLiteral("绿色通勤：交通移动免费"),
        QStringLiteral("芯片短缺：升级费用提高"), QStringLiteral("创投热潮：空地购买返现"),
        QStringLiteral("极光之夜：技能能量恢复"), QStringLiteral("暴雨预警：支路暂时关闭"),
        QStringLiteral("夜市开放：商店卡牌折扣"), QStringLiteral("城市更新：低级建筑获得补助")
    };
}

QStringList CityContent::strategyCards()
{
    static const QStringList verbs = {QStringLiteral("跃迁"), QStringLiteral("护盾"), QStringLiteral("议价"),
        QStringLiteral("巡游"), QStringLiteral("改造"), QStringLiteral("反转")};
    return generatedList(36, [&](int i) {
        return verbs.at(i % verbs.size()) + QStringLiteral("协议 ") + QString::number(i + 1);
    });
}

QStringList CityContent::personalEvents()
{
    return generatedList(30, [](int i) { return QStringLiteral("霓城奇遇 ") + QString::number(i + 1); });
}

QStringList CityContent::missions()
{
    return generatedList(18, [](int i) { return QStringLiteral("城市委托 ") + QString::number(i + 1); });
}

} // namespace neon
