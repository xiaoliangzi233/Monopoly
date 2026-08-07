#pragma once

#include <QColor>
#include <QMetaType>
#include <QPoint>
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
    Shop,
    Service,
    Tax,
    Mission
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
    UseSkill
};
Q_ENUM_NS(CommandType)

enum class GamePhase {
    Waiting,
    AwaitingRoll,
    Moving,
    AwaitingDecision,
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
    QPoint gridPosition;
    TileType type = TileType::Event;
    QString name;
    int district = -1;
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
    int cash = 18000;
    int energy = 2;
    int movementBonus = 0;
    int shieldCharges = 0;
    int skillCooldown = 0;
    QList<int> strategyCards;
    int missionScore = 0;
    bool bankrupt = false;
    bool aiControlled = false;
};

struct CommandResult {
    bool accepted = false;
    QString error;
    QList<GameEvent> events;
};

} // namespace neon

Q_DECLARE_METATYPE(neon::GamePhase)
Q_DECLARE_METATYPE(neon::TileType)
