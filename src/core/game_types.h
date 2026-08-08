#pragma once

#include <QColor>
#include <QMetaType>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QUuid>
#include <QVariantMap>

namespace neon {
Q_NAMESPACE

enum class TileType {
    Start,
    Property,
    Event,
    Transit,
    Guild,
    Civic,
    Tax,
    Commission,
    Festival
};
Q_ENUM_NS(TileType)

enum class CommandType {
    Roll,
    BuyProperty,
    UpgradeProperty,
    MortgageProperty,
    EndTurn,
    UseReroll,
    ChooseRoute,
    UseCard,
    UseSkill,
    PlaceBid,
    PassAuction,
    ProposeTrade,
    RespondTrade,
    SellProperty,
    ContributeCivic
};
Q_ENUM_NS(CommandType)

enum class GamePhase {
    Waiting,
    AwaitingRoll,
    AwaitingRoute,
    Moving,
    AwaitingDecision,
    Auction,
    Trade,
    ForcedSettlement,
    Finished
};
Q_ENUM_NS(GamePhase)

struct GameCommand {
    QUuid matchId;
    QUuid playerId;
    quint64 commandId = 0;
    CommandType type = CommandType::Roll;
    QVariantMap arguments;
};

struct GameEvent {
    quint64 sequence = 0;
    QString type;
    QUuid playerId;
    QVariantMap data;
};

struct TileDefinition {
    int index = 0;
    QPointF worldPosition;
    QList<int> neighbors;
    QPolygonF roadPolygon;
    QRectF industryFootprint;
    TileType type = TileType::Event;
    QString name;
    int district = -1;
    QString styleId;
    int price = 0;
    int baseRent = 0;
};

struct PropertyState {
    int tileIndex = -1;
    QUuid ownerId;
    int level = 0;
    bool mortgaged = false;
};

struct PlayerState {
    QUuid id;
    QString name;
    QColor color;
    int characterIndex = 0;
    int position = 0;
    int previousPosition = -1;
    int cash = 24000;
    int reputation = 0;
    int culture = 0;
    int livelihood = 0;
    int energy = 3;
    int rerolls = 1;
    int movementBonus = 0;
    int shieldCharges = 0;
    int skillCooldown = 0;
    int upgradeDiscountPercent = 0;
    int boostedIndustryTile = -1;
    int boostedCollections = 0;
    QList<int> strategyCards;
    QList<int> commissions;
    QStringList statusEffects;
    bool bankrupt = false;
    bool aiControlled = false;
    bool tradedThisTurn = false;
};

struct TradeOfferState {
    bool active = false;
    QUuid proposerId;
    QUuid recipientId;
    int offeredCash = 0;
    int requestedCash = 0;
    int offeredTile = -1;
    int requestedTile = -1;
    int offeredCardSlot = -1;
    int requestedCardSlot = -1;
    qint64 deadlineMs = 0;
};

struct CommandResult {
    bool accepted = false;
    QString error;
    QList<GameEvent> events;
};

} // namespace neon

Q_DECLARE_METATYPE(neon::GamePhase)
Q_DECLARE_METATYPE(neon::TileType)
