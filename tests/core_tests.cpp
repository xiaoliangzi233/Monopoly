#include "core/ai_player.h"
#include "core/city_content.h"
#include "core/save_manager.h"
#include "core/scene_layout.h"
#include "network/game_client.h"
#include "network/host_server.h"
#include "network/protocol.h"

#include <QSet>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>
#include <algorithm>

using namespace neon;

class CoreTests final : public QObject {
    Q_OBJECT
private slots:
    void mapHasExpectedContent();
    void mapAnchorsNeverOverlap();
    void sceneLayoutReservesRoadsAndLots();
    void rejectsDuplicateCommands();
    void canBuyAndUpgradeProperty();
    void characterCardsAndMortgageChangeAuthoritativeState();
    void saveRoundTripPreservesHash();
    void protocolRejectsOversizedFrame();
    void hostAuthorizesSeatAndSynchronizesSnapshot();
    void aiSimulationAlwaysTerminates();
};

void CoreTests::mapHasExpectedContent()
{
    const auto tiles = CityContent::createNeonCityMap();
    QCOMPARE(tiles.size(), 48);
    QCOMPARE(std::count_if(tiles.cbegin(), tiles.cend(), [](const auto &tile) { return tile.type == TileType::Property; }), 24);
    QCOMPARE(CityContent::characterNames().size(), 6);
    QCOMPARE(CityContent::strategyCards().size(), 36);
    QCOMPARE(CityContent::personalEvents().size(), 30);
    QCOMPARE(CityContent::missions().size(), 18);
    QCOMPARE(CityContent::cityPulseEvents().size(), 8);
}

void CoreTests::mapAnchorsNeverOverlap()
{
    const auto tiles = CityContent::createNeonCityMap();
    QSet<QPoint> occupied;
    for (const auto &tile : tiles) {
        QVERIFY2(!occupied.contains(tile.gridPosition), "Two scene models share the same world anchor");
        occupied.insert(tile.gridPosition);
        QVERIFY(tile.gridPosition.x() >= 0 && tile.gridPosition.x() <= 14);
        QVERIFY(tile.gridPosition.y() >= 0 && tile.gridPosition.y() <= 10);
    }
}

void CoreTests::sceneLayoutReservesRoadsAndLots()
{
    const auto errors = SceneLayout::validate(CityContent::createNeonCityMap());
    QVERIFY2(errors.isEmpty(), qPrintable(errors.join(QStringLiteral("\n"))));
}

void CoreTests::rejectsDuplicateCommands()
{
    GameEngine engine;
    QVERIFY(engine.createGame({QStringLiteral("A"), QStringLiteral("B")}, 0, 16, 42));
    const auto *player = engine.state().activePlayer();
    QVERIFY(player);
    const GameCommand roll{engine.state().matchId, player->id, 1, CommandType::Roll, {}};
    QVERIFY(engine.apply(roll).accepted);
    const auto duplicate = engine.apply(roll);
    QVERIFY(!duplicate.accepted);
    QVERIFY(duplicate.error.contains(QStringLiteral("Duplicate")));
}

void CoreTests::canBuyAndUpgradeProperty()
{
    GameEngine engine;
    QVERIFY(engine.createGame({QStringLiteral("A"), QStringLiteral("B")}, 0, 16, 99));
    GameState state = engine.state();
    state.players[0].position = 1;
    state.phase = GamePhase::AwaitingDecision;
    engine.restore(state);
    const auto playerId = engine.state().players.first().id;
    QVERIFY(engine.apply({engine.state().matchId, playerId, 1, CommandType::BuyProperty, {}}).accepted);
    const auto *property = engine.state().propertyAt(1);
    QVERIFY(property);
    QCOMPARE(property->ownerId, playerId);
    QVERIFY(engine.apply({engine.state().matchId, playerId, 2, CommandType::UpgradeProperty,
                          {{QStringLiteral("tile"), 1}}}).accepted);
    QCOMPARE(engine.state().propertyAt(1)->level, 1);
}

void CoreTests::characterCardsAndMortgageChangeAuthoritativeState()
{
    GameEngine engine;
    QVERIFY(engine.createGame({QStringLiteral("A"), QStringLiteral("B")}, 0, 16, 2026));
    GameState state = engine.state();
    state.players[0].strategyCards = {2};
    engine.restore(state);
    const auto playerId = engine.state().players.first().id;
    const int initialCash = engine.state().players.first().cash;
    QVERIFY(engine.apply({engine.state().matchId, playerId, 1, CommandType::UseSkill, {}}).accepted);
    QCOMPARE(engine.state().players.first().skillCooldown, 3);
    QVERIFY(engine.apply({engine.state().matchId, playerId, 2, CommandType::UseCard,
                          {{QStringLiteral("slot"), 0}}}).accepted);
    QCOMPARE(engine.state().players.first().cash, initialCash + 700);

    state = engine.state();
    state.players[0].position = 1;
    state.phase = GamePhase::AwaitingDecision;
    engine.restore(state);
    QVERIFY(engine.apply({engine.state().matchId, playerId, 3, CommandType::BuyProperty, {}}).accepted);
    const int afterPurchase = engine.state().players.first().cash;
    QVERIFY(engine.apply({engine.state().matchId, playerId, 4, CommandType::MortgageProperty,
                          {{QStringLiteral("tile"), 1}}}).accepted);
    QVERIFY(engine.state().propertyAt(1)->mortgaged);
    QVERIFY(engine.state().players.first().cash > afterPurchase);
}

void CoreTests::saveRoundTripPreservesHash()
{
    GameEngine engine;
    QVERIFY(engine.createGame({QStringLiteral("A"), QStringLiteral("B")}, 1, 24, 1234));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("roundtrip.ntsave"));
    QString error;
    QVERIFY2(SaveManager::saveAtomic(engine.state(), path, &error), qPrintable(error));
    const auto restored = SaveManager::load(path, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(restored.stableHash(), engine.state().stableHash());
}

void CoreTests::protocolRejectsOversizedFrame()
{
    QByteArray buffer(sizeof(quint32), Qt::Uninitialized);
    qToBigEndian<quint32>(net::MaximumFrameSize + 1, reinterpret_cast<uchar *>(buffer.data()));
    QByteArray frame;
    QString error;
    QVERIFY(!net::takeFrame(buffer, &frame, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(buffer.isEmpty());
}

void CoreTests::hostAuthorizesSeatAndSynchronizesSnapshot()
{
    GameEngine engine;
    QVERIFY(engine.createGame({QStringLiteral("房主"), QStringLiteral("客户")}, 0, 16, 29450));

    net::HostServer host(&engine);
    QString error;
    QVERIFY2(host.listen(0, &error), qPrintable(error));

    net::GameClient client;
    QSignalSpy snapshotSpy(&client, &net::GameClient::snapshotReceived);
    QSignalSpy rejectionSpy(&client, &net::GameClient::commandRejected);
    client.connectToHost(QStringLiteral("127.0.0.1"), host.port());
    QTRY_VERIFY_WITH_TIMEOUT(snapshotSpy.count() >= 1, 3000);

    const QUuid hostPlayer = engine.state().players.at(0).id;
    const QUuid remotePlayer = engine.state().players.at(1).id;
    QCOMPARE(client.assignedPlayerId(), remotePlayer);
    QVERIFY(host.hasConnectionForPlayer(remotePlayer));

    QVERIFY(engine.apply({engine.state().matchId, hostPlayer, 1, CommandType::Roll, {}}).accepted);
    QVERIFY(engine.apply({engine.state().matchId, hostPlayer, 2, CommandType::EndTurn, {}}).accepted);
    host.publishState();
    QTRY_COMPARE_WITH_TIMEOUT(client.state().currentPlayer, 1, 3000);

    client.sendCommand({engine.state().matchId, hostPlayer, 100, CommandType::Roll, {}});
    QTRY_VERIFY_WITH_TIMEOUT(rejectionSpy.count() >= 1, 3000);
    QCOMPARE(engine.state().phase, GamePhase::AwaitingRoll);

    client.sendCommand({engine.state().matchId, remotePlayer, 101, CommandType::Roll, {}});
    QTRY_COMPARE_WITH_TIMEOUT(engine.state().phase, GamePhase::AwaitingDecision, 3000);
    QTRY_COMPARE_WITH_TIMEOUT(client.state().sequence, engine.state().sequence, 3000);
    QCOMPARE(client.state().stableHash(), engine.state().stableHash());
}

void CoreTests::aiSimulationAlwaysTerminates()
{
    bool ok = false;
    const int requested = qEnvironmentVariableIntValue("NEON_SIMULATION_GAMES", &ok);
    const int games = ok ? qBound(1, requested, 10000) : 100;
    AiPlayer ai(AiPlayer::Difficulty::Standard);
    for (int gameIndex = 0; gameIndex < games; ++gameIndex) {
        GameEngine engine;
        QVERIFY(engine.createGame({QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C"), QStringLiteral("D")}, 4, 16, quint64(gameIndex + 1)));
        quint64 commandId = 0;
        int actions = 0;
        while (engine.state().phase != GamePhase::Finished && actions < 4000) {
            const auto result = engine.apply(ai.chooseCommand(engine.state(), ++commandId));
            QVERIFY2(result.accepted, qPrintable(result.error));
            ++actions;
        }
        QVERIFY2(engine.state().phase == GamePhase::Finished, "AI simulation exceeded action safety limit");
        for (const auto &player : engine.state().players) QVERIFY(player.cash >= 0);
    }
}

QTEST_GUILESS_MAIN(CoreTests)
#include "core_tests.moc"
