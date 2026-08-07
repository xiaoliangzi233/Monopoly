#include "core/ai_player.h"
#include "core/city_content.h"
#include "core/save_manager.h"
#include "core/scene_layout.h"
#include "network/game_client.h"
#include "network/host_server.h"
#include "network/protocol.h"

#include <QFile>
#include <QQueue>
#include <QSet>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>
#include <algorithm>

using namespace neon;

class CoreTests final : public QObject {
    Q_OBJECT
private slots:
    void mapHasExpectedNationalContent();
    void mapGraphIsConnectedAndSymmetric();
    void sceneLayoutReservesRoadsAndIndustries();
    void rejectsDuplicateCommands();
    void routeChoiceIsHostAuthoritative();
    void canBuyUpgradeAndPawnIndustry();
    void auctionAndTradeValidateAssets();
    void characterCardsAndProsperityChangeState();
    void saveV2RoundTripPreservesHash();
    void protocolRejectsOversizedFrame();
    void hostAuthorizesSeatAndSynchronizesSnapshot();
    void aiSimulationAlwaysTerminates();
};

void CoreTests::mapHasExpectedNationalContent()
{
    const auto tiles = CityContent::createProsperousCityMap();
    QCOMPARE(tiles.size(), 64);
    QCOMPARE(std::count_if(tiles.cbegin(), tiles.cend(), [](const auto &t) { return t.type == TileType::Property; }), 32);
    QCOMPARE(CityContent::districtNames().size(), 8);
    QCOMPARE(CityContent::characterNames(), QStringList({QStringLiteral("鲁班"), QStringLiteral("黄道婆"),
        QStringLiteral("李时珍"), QStringLiteral("沈括"), QStringLiteral("李清照"), QStringLiteral("郑和")}));
    QCOMPARE(CityContent::strategyCards().size(), 48);
    QCOMPARE(CityContent::personalEvents().size(), 36);
    QCOMPARE(CityContent::missions().size(), 24);
    QCOMPARE(CityContent::cityPulseEvents().size(), 12);
    QCOMPARE(CityContent::contentHash().size(), 64);
}

void CoreTests::mapGraphIsConnectedAndSymmetric()
{
    const auto tiles = CityContent::createProsperousCityMap();
    QSet<int> visited{0};
    QQueue<int> pending;
    pending.enqueue(0);
    while (!pending.isEmpty()) {
        const int current = pending.dequeue();
        for (int neighbor : tiles[current].neighbors) {
            QVERIFY(neighbor >= 0 && neighbor < tiles.size());
            QVERIFY(tiles[neighbor].neighbors.contains(current));
            if (!visited.contains(neighbor)) { visited.insert(neighbor); pending.enqueue(neighbor); }
        }
    }
    QCOMPARE(visited.size(), 64);
    QSet<QString> anchors;
    for (const auto &tile : tiles) {
        const QString key = QStringLiteral("%1,%2").arg(tile.worldPosition.x()).arg(tile.worldPosition.y());
        QVERIFY2(!anchors.contains(key), "Two nodes share the same world anchor");
        anchors.insert(key);
    }
}

void CoreTests::sceneLayoutReservesRoadsAndIndustries()
{
    const auto errors = SceneLayout::validate(CityContent::createProsperousCityMap());
    QVERIFY2(errors.isEmpty(), qPrintable(errors.join(QLatin1Char('\n'))));
}

void CoreTests::rejectsDuplicateCommands()
{
    GameEngine engine;
    QVERIFY(engine.createGame({QStringLiteral("A"), QStringLiteral("B")}, 0, 24, 42));
    const auto player = engine.state().activePlayer()->id;
    const GameCommand roll{engine.state().matchId, player, 1, CommandType::Roll, {}};
    QVERIFY(engine.apply(roll).accepted);
    const auto duplicate = engine.apply(roll);
    QVERIFY(!duplicate.accepted);
    QVERIFY(duplicate.error.contains(QStringLiteral("重复")));
}

void CoreTests::routeChoiceIsHostAuthoritative()
{
    GameEngine engine;
    QVERIFY(engine.createGame({QStringLiteral("A"), QStringLiteral("B")}, 0, 24, 7));
    GameState state = engine.state();
    state.players[0].position = 3;
    state.players[0].previousPosition = 2;
    state.phase = GamePhase::AwaitingRoll;
    engine.restore(state);
    const auto player = engine.state().activePlayer()->id;
    QVERIFY(engine.apply({engine.state().matchId, player, 1, CommandType::Roll, {}}).accepted);
    if (engine.state().phase == GamePhase::AwaitingRoute) {
        QVERIFY(engine.state().routeOptions.size() >= 2);
        const auto chosen = engine.state().routeOptions.last();
        QVERIFY(engine.apply({engine.state().matchId, player, 2, CommandType::ChooseRoute,
                              {{QStringLiteral("option"), engine.state().routeOptions.size() - 1}}}).accepted);
        QCOMPARE(engine.state().players[0].position, chosen.last());
    }
    QCOMPARE(engine.state().phase, GamePhase::AwaitingDecision);
}

void CoreTests::canBuyUpgradeAndPawnIndustry()
{
    GameEngine engine;
    QVERIFY(engine.createGame({QStringLiteral("A"), QStringLiteral("B")}, 0, 24, 99));
    GameState state = engine.state();
    state.players[0].position = 1;
    state.phase = GamePhase::AwaitingDecision;
    engine.restore(state);
    const auto player = engine.state().players.first().id;
    QVERIFY(engine.apply({engine.state().matchId, player, 1, CommandType::BuyProperty, {}}).accepted);
    QCOMPARE(engine.state().propertyAt(1)->ownerId, player);
    QVERIFY(engine.apply({engine.state().matchId, player, 2, CommandType::UpgradeProperty,
                          {{QStringLiteral("tile"), 1}}}).accepted);
    QCOMPARE(engine.state().propertyAt(1)->level, 1);
    const int beforePawn = engine.state().players[0].cash;
    QVERIFY(engine.apply({engine.state().matchId, player, 3, CommandType::MortgageProperty,
                          {{QStringLiteral("tile"), 1}}}).accepted);
    QVERIFY(engine.state().propertyAt(1)->mortgaged);
    QVERIFY(engine.state().players[0].cash > beforePawn);
}

void CoreTests::auctionAndTradeValidateAssets()
{
    GameEngine engine;
    QVERIFY(engine.createGame({QStringLiteral("A"), QStringLiteral("B")}, 0, 24, 123));
    GameState state = engine.state();
    state.players[0].position = 1;
    state.phase = GamePhase::AwaitingDecision;
    engine.restore(state);
    const auto a = engine.state().players[0].id;
    const auto b = engine.state().players[1].id;
    QVERIFY(engine.apply({engine.state().matchId, a, 1, CommandType::EndTurn, {}}).accepted);
    QCOMPARE(engine.state().phase, GamePhase::Auction);
    const int bid = engine.state().auctionHighBid + 100;
    QVERIFY(engine.apply({engine.state().matchId, b, 2, CommandType::PlaceBid,
                          {{QStringLiteral("amount"), bid}}}).accepted);
    QVERIFY(engine.apply({engine.state().matchId, a, 3, CommandType::PassAuction, {}}).accepted);
    QCOMPARE(engine.state().propertyAt(1)->ownerId, b);

    state = engine.state();
    state.currentPlayer = 0;
    state.phase = GamePhase::AwaitingDecision;
    engine.restore(state);
    const int aBefore = engine.state().players[0].cash;
    const int bBefore = engine.state().players[1].cash;
    QVERIFY(engine.apply({engine.state().matchId, a, 4, CommandType::ProposeTrade,
        {{QStringLiteral("recipient"), b.toString()}, {QStringLiteral("offeredCash"), 500},
         {QStringLiteral("requestedCash"), 300}}}).accepted);
    QVERIFY(engine.apply({engine.state().matchId, b, 5, CommandType::RespondTrade,
                          {{QStringLiteral("accept"), true}}}).accepted);
    QCOMPARE(engine.state().players[0].cash, aBefore - 200);
    QCOMPARE(engine.state().players[1].cash, bBefore + 200);
}

void CoreTests::characterCardsAndProsperityChangeState()
{
    GameEngine engine;
    QVERIFY(engine.createGame(CityContent::characterNames(), 0, 24, 2026));
    GameState state = engine.state();
    state.players[0].strategyCards = {2};
    engine.restore(state);
    const auto player = engine.state().players[0].id;
    QVERIFY(engine.apply({engine.state().matchId, player, 1, CommandType::UseSkill, {}}).accepted);
    QCOMPARE(engine.state().players[0].upgradeDiscountPercent, 50);
    const int cash = engine.state().players[0].cash;
    QVERIFY(engine.apply({engine.state().matchId, player, 2, CommandType::UseCard,
                          {{QStringLiteral("slot"), 0}}}).accepted);
    QCOMPARE(engine.state().players[0].cash, cash + 800);
    QVERIFY(engine.finalScore(player) >= 100);
}

void CoreTests::saveV2RoundTripPreservesHash()
{
    GameEngine engine;
    QVERIFY(engine.createGame({QStringLiteral("A"), QStringLiteral("B")}, 1, 32, 1234));
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("roundtrip.ntsave"));
    QString error;
    QVERIFY2(SaveManager::saveAtomic(engine.state(), path, &error), qPrintable(error));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readLine(), QByteArray("SHENGSHIBAIYE-SAVE-V2\n"));
    file.close();
    const auto restored = SaveManager::load(path, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(restored.stableHash(), engine.state().stableHash());
}

void CoreTests::protocolRejectsOversizedFrame()
{
    QCOMPARE(net::ProtocolVersion, quint32(2));
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
    QVERIFY(engine.createGame({QStringLiteral("房主"), QStringLiteral("来客")}, 0, 24, 29450));
    net::HostServer host(&engine);
    QString error;
    QVERIFY2(host.listen(0, &error), qPrintable(error));
    net::GameClient client;
    QSignalSpy snapshotSpy(&client, &net::GameClient::snapshotReceived);
    QSignalSpy rejectionSpy(&client, &net::GameClient::commandRejected);
    client.connectToHost(QStringLiteral("127.0.0.1"), host.port());
    QTRY_VERIFY_WITH_TIMEOUT(snapshotSpy.count() >= 1, 3000);
    const QUuid local = engine.state().players[0].id;
    const QUuid remote = engine.state().players[1].id;
    QCOMPARE(client.assignedPlayerId(), remote);
    client.sendCommand({engine.state().matchId, local, 100, CommandType::Roll, {}});
    QTRY_VERIFY_WITH_TIMEOUT(rejectionSpy.count() >= 1, 3000);
    client.sendCommand({engine.state().matchId, remote, 101, CommandType::Roll, {}});
    QTRY_VERIFY_WITH_TIMEOUT(rejectionSpy.count() >= 2, 3000);
    QCOMPARE(client.state().contentHash, engine.state().contentHash);
}

void CoreTests::aiSimulationAlwaysTerminates()
{
    bool ok = false;
    const int requested = qEnvironmentVariableIntValue("NEON_SIMULATION_GAMES", &ok);
    const int games = ok ? qBound(1, requested, 10000) : 100;
    AiPlayer ai(AiPlayer::Difficulty::Standard);
    const QList<int> rounds{24, 32, 40};
    for (int gameIndex = 0; gameIndex < games; ++gameIndex) {
        GameEngine engine;
        QVERIFY(engine.createGame({QStringLiteral("鲁班"), QStringLiteral("黄道婆"),
            QStringLiteral("李时珍"), QStringLiteral("沈括")}, 4, rounds[gameIndex % rounds.size()], quint64(gameIndex + 1)));
        quint64 commandId = 0;
        int actions = 0;
        while (engine.state().phase != GamePhase::Finished && actions < 12000) {
            const auto command = ai.chooseCommand(engine.state(), ++commandId);
            QVERIFY2(!command.playerId.isNull(), "AI failed to find a legal acting player");
            const auto result = engine.apply(command);
            QVERIFY2(result.accepted, qPrintable(result.error));
            ++actions;
        }
        QVERIFY2(engine.state().phase == GamePhase::Finished, "AI simulation exceeded action safety limit");
        for (const auto &player : engine.state().players) {
            QVERIFY(player.cash >= 0);
            QVERIFY(player.reputation >= 0 && player.reputation <= 100);
            QVERIFY(player.culture >= 0 && player.culture <= 100);
            QVERIFY(player.livelihood >= 0 && player.livelihood <= 100);
        }
    }
}

QTEST_GUILESS_MAIN(CoreTests)
#include "core_tests.moc"
