import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NeonTycoon 1.0

ApplicationWindow {
    id: root
    width: 1440; height: 900
    minimumWidth: 1100; minimumHeight: 680
    visible: false
    color: "#09191D"
    title: "盛世百业"

    property string page: "menu"
    property bool suppressAutoReleaseNotes: false
    property int hoveredTile: -1
    property point hoverPoint: Qt.point(0, 0)
    property bool drawerOpen: false
    property int drawerPage: 0
    property bool rosterOpen: true
    property bool mapOpen: false

    function openDrawer(index) { drawerPage = index; drawerOpen = true; mapOpen = false }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#143D42" }
            GradientStop { position: .52; color: "#0B272D" }
            GradientStop { position: 1; color: "#07171B" }
        }
    }

    Item {
        id: menuPage
        anchors.fill: parent
        visible: root.page === "menu"

        // 程序化新中式都市天际线：玻璃塔楼、金属窗格与暖色城市灯带。
        Repeater {
            model: 14
            Item {
                required property int index
                width: 92 + (index * 37) % 126
                height: 190 + (index * 71) % 360
                x: index * (root.width + 120) / 13 - 70
                anchors.bottom: parent.bottom
                opacity: .52
                Rectangle { anchors.fill: parent; color: index % 3 === 0 ? "#173D47" : index % 3 === 1 ? "#1D4B4C" : "#243B48"; border.color: "#487276" }
                Rectangle { width: parent.width - 20; height: 5; x: 10; y: 22; color: index % 4 === 0 ? "#C03B42" : "#CDAA58" }
                Repeater {
                    model: 6
                    Rectangle { required property int index; width: parent.width - 30; height: 2; x: 15; y: 54 + index * 42; color: "#9CC3BC"; opacity: .42 }
                }
                Rectangle { visible: index % 4 === 0; width: 8; height: 70; anchors.horizontalCenter: parent.horizontalCenter; y: -70; color: "#C6A352" }
            }
        }
        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 130; color: "#07181C"; opacity: .88 }
        Repeater {
            model: 24
            Rectangle { required property int index; width: 3; height: 52 + index % 4 * 12; x: index * root.width / 23; anchors.bottom: parent.bottom; color: index % 3 ? "#BFA454" : "#B8383F"; opacity: .54 }
        }

        ColumnLayout {
            anchors.left: parent.left
            anchors.leftMargin: Math.max(64, root.width * .075)
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -22
            width: 520; spacing: 13
            Text { text: "城市产业主理人"; color: "#79A9A8"; font.pixelSize: 13; font.letterSpacing: 4 }
            Text { text: "盛世百业"; color: "#F7F1E4"; font.family: "Microsoft YaHei UI"; font.pixelSize: 67; font.weight: Font.Bold; font.letterSpacing: 5 }
            Rectangle { Layout.preferredWidth: 360; height: 4; radius: 2; color: "#C33B42" }
            Text { text: "经营产业 · 塑造品牌 · 共建未来城市"; color: "#C2D4CF"; font.pixelSize: 17; font.letterSpacing: 2 }
            Item { height: 22 }
            ModernButton { Layout.preferredWidth: 286; text: "单人游戏"; iconText: "▸"; accent: "#D6AF57"; onClicked: newGameModal.open() }
            ModernButton { Layout.preferredWidth: 286; text: "继续游戏"; iconText: "↻"; onClicked: { if (game.loadGame()) root.page = "game" } }
            ModernButton { Layout.preferredWidth: 286; text: "联机游戏"; iconText: "◎"; accent: "#4E9D9C"; onClicked: networkModal.open() }
            ModernButton { Layout.preferredWidth: 286; text: "退出游戏"; iconText: "×"; accent: "#A84A4E"; onClicked: Qt.quit() }
        }

        Rectangle {
            id: versionCard
            anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 28
            width: 320; height: 76; radius: 16
            color: versionMouse.containsMouse ? "#F0254A4D" : "#E51B363A"
            border.color: releaseNotes.unread ? "#E0B654" : "#527476"
            RowLayout {
                anchors.fill: parent; anchors.margins: 13
                Rectangle { width: 42; height: 42; radius: 12; color: "#A9363C"; Text { anchors.centerIn: parent; text: "更"; color: "#FFF5DD"; font.pixelSize: 18; font.bold: true } }
                ColumnLayout { Layout.fillWidth: true; spacing: 1
                    Text { text: "更新内容 · v" + releaseNotes.version; color: "#F3EDE1"; font.pixelSize: 14; font.bold: true }
                    Text { text: releaseNotes.title; color: "#9DB9B6"; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true }
                }
                Rectangle { visible: releaseNotes.unread; width: 30; height: 22; radius: 11; color: "#D9AE50"; Text { anchors.centerIn: parent; text: "新"; color: "#173638"; font.bold: true } }
            }
            MouseArea { id: versionMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: releaseNotesModal.open() }
            Behavior on color { ColorAnimation { duration: 130 } }
        }
    }

    Item {
        id: gamePage
        anchors.fill: parent
        visible: root.page === "game"
        clip: true

        TopDownCityView {
            id: board
            anchors.fill: parent
            viewModel: game
            focus: true
            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_W || event.key === Qt.Key_Up) board.panBy(0, 55)
                else if (event.key === Qt.Key_S || event.key === Qt.Key_Down) board.panBy(0, -55)
                else if (event.key === Qt.Key_A || event.key === Qt.Key_Left) board.panBy(55, 0)
                else if (event.key === Qt.Key_D || event.key === Qt.Key_Right) board.panBy(-55, 0)
            }
            MouseArea {
                anchors.fill: parent; hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                property real lastX; property real lastY; property bool dragged: false
                onPressed: function(mouse) { lastX = mouse.x; lastY = mouse.y; dragged = false; board.forceActiveFocus() }
                onPositionChanged: function(mouse) {
                    if (pressed) {
                        var dx = mouse.x - lastX; var dy = mouse.y - lastY
                        if (Math.abs(dx) + Math.abs(dy) > 1) { board.panBy(dx, dy); dragged = true }
                        lastX = mouse.x; lastY = mouse.y
                    }
                    root.hoveredTile = board.tileAt(mouse.x, mouse.y)
                    root.hoverPoint = Qt.point(mouse.x, mouse.y)
                }
                onWheel: function(wheel) { board.zoomAt(wheel.x, wheel.y, wheel.angleDelta.y); wheel.accepted = true }
                onExited: root.hoveredTile = -1
            }
            Connections { target: game; function onCameraFocusRequested(tileIndex) { board.focusTile(tileIndex) } }
        }

        Rectangle {
            anchors.top: parent.top; anchors.horizontalCenter: parent.horizontalCenter; anchors.topMargin: 14
            width: Math.min(780, parent.width * .58); height: 72; radius: 18
            color: "#E51A3438"; border.color: "#60858A"
            RowLayout {
                anchors.fill: parent; anchors.margins: 12; spacing: 13
                Rectangle { width: 45; height: 45; radius: 13; color: game.currentPlayerColor; border.color: "#D8B45C"; border.width: 2
                    Text { anchors.centerIn: parent; text: game.currentPlayerName.substring(0, 1); color: "white"; font.pixelSize: 21; font.bold: true }
                }
                ColumnLayout { Layout.fillWidth: true; spacing: 1
                    Text { text: game.currentPlayerName + " · " + game.tileName; color: "#F6F0E5"; font.pixelSize: 17; font.bold: true }
                    Text { text: game.statusText + "　 第 " + game.currentRound + " / " + game.maxRounds + " 回合"; color: "#9DB8B5"; font.pixelSize: 12 }
                }
                Text { text: "资产  " + game.currentCash.toLocaleString(); color: "#E2BE68"; font.pixelSize: 16; font.bold: true }
                Rectangle { width: 1; Layout.fillHeight: true; color: "#557173" }
                Text { text: "繁荣 " + game.currentProsperity; color: "#DDE9E4"; font.pixelSize: 15 }
            }
        }

        Column {
            anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 14; anchors.topMargin: 18; spacing: 7
            ModernButton { width: 138; compact: true; text: root.rosterOpen ? "收起玩家列表" : "玩家列表"; iconText: "◉"; onClicked: root.rosterOpen = !root.rosterOpen }
            ModernButton { width: 138; compact: true; text: "城市地图"; iconText: "◇"; accent: "#D6AF57"; onClicked: { root.mapOpen = !root.mapOpen; root.drawerOpen = false } }
            ModernButton { width: 138; compact: true; text: "我的产业"; iconText: "▦"; onClicked: root.openDrawer(0) }
            ModernButton { width: 138; compact: true; text: "策略与交易"; iconText: "⇄"; onClicked: root.openDrawer(1) }
            ModernButton { width: 138; compact: true; text: "对局记录"; iconText: "≡"; onClicked: root.openDrawer(2) }
            ModernButton { width: 138; compact: true; text: "游戏设置"; iconText: "⚙"; onClicked: root.openDrawer(3) }
            ModernButton { width: 138; compact: true; text: "返回主菜单"; iconText: "←"; accent: "#A84A4E"; onClicked: root.page = "menu" }
        }

        Rectangle {
            visible: root.rosterOpen
            anchors.left: parent.left; anchors.top: parent.top; anchors.leftMargin: 164; anchors.topMargin: 104
            width: 228; height: Math.min(playerListContent.implicitHeight + 18, parent.height * .56)
            radius: 16; color: "#DD173236"; border.color: "#537577"
            Column { id: playerListContent; anchors.fill: parent; anchors.margins: 9; spacing: 6
                Repeater { model: game.players
                    Rectangle {
                        required property var modelData
                        width: playerListContent.width; height: 55; radius: 10
                        color: modelData.bankrupt ? "#4A4D4C" : "#203F42"; border.color: modelData.color; opacity: modelData.bankrupt ? .55 : .96
                        RowLayout { anchors.fill: parent; anchors.margins: 8
                            Rectangle { width: 7; Layout.fillHeight: true; radius: 3; color: modelData.color }
                            ColumnLayout { Layout.fillWidth: true; spacing: 0
                                Text { text: modelData.name + (modelData.ai ? " · AI" : ""); color: "#F0EEE7"; font.pixelSize: 13; font.bold: true }
                                Text { text: "资产 " + modelData.cash.toLocaleString() + " · 繁荣 " + modelData.score; color: "#9FB8B5"; font.pixelSize: 10 }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 14
            width: 238; height: 52; radius: 14; color: "#E0173236"; border.color: "#4F7072"
            RowLayout { anchors.fill: parent; anchors.margins: 10
                Rectangle { width: 9; height: 9; radius: 5; color: game.networkStatus.indexOf("错误") >= 0 ? "#CF4A50" : "#65B69B" }
                Text { Layout.fillWidth: true; text: game.networkStatus; color: "#B8CCCA"; elide: Text.ElideRight; font.pixelSize: 11 }
            }
        }

        Rectangle {
            id: actionBar
            anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottomMargin: 14
            width: Math.min(parent.width - 510, actionRow.implicitWidth + 30); height: 72; radius: 19
            color: "#EC173337"; border.color: "#5D7E80"
            Row { id: actionRow; anchors.centerIn: parent; spacing: 8
                ModernButton { visible: game.canRoll && !game.presentationBusy; text: "投掷骰子"; accent: "#D8B258"; onClicked: game.rollDice() }
                ModernButton { visible: game.canReroll && !game.presentationBusy; text: "重新投掷"; onClicked: game.rerollDice() }
                ModernButton { visible: game.canUseSkill && !game.presentationBusy; text: "使用技能"; accent: "#B93C43"; onClicked: game.useSkill() }
                ModernButton { visible: game.canUseCard && !game.presentationBusy; text: "使用策略卡"; onClicked: game.useFirstCard() }
                ModernButton { visible: game.canBuy && !game.presentationBusy; text: "购买产业"; accent: "#D8B258"; onClicked: game.buyCurrentProperty() }
                ModernButton { visible: game.canUpgrade && !game.presentationBusy; text: "升级产业"; onClicked: game.upgradeCurrentProperty() }
                ModernButton { visible: game.canMortgage && !game.presentationBusy; text: "抵押 / 赎回"; onClicked: game.mortgageCurrentProperty() }
                ModernButton { visible: game.canContribute && !game.presentationBusy; text: "投资公共项目"; onClicked: game.contributeCivic() }
                ModernButton { visible: game.canEndTurn && !game.presentationBusy; text: "结束回合"; accent: "#A84A4E"; onClicked: game.endTurn() }
            }
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: actionBar.top; anchors.bottomMargin: 10
            width: Math.min(470, thinkingLabel.implicitWidth + 54); height: 38; radius: 19
            color: "#DD10272B"; border.color: "#466A6C"
            Rectangle { width: 7; height: 7; radius: 4; color: "#E0B456"; anchors.left: parent.left; anchors.leftMargin: 17; anchors.verticalCenter: parent.verticalCenter
                SequentialAnimation on opacity { loops: Animation.Infinite; NumberAnimation { to: .25; duration: 500 } NumberAnimation { to: 1; duration: 500 } }
            }
            Text { id: thinkingLabel; anchors.centerIn: parent; text: game.thinkingText; color: "#CFE0DC"; font.pixelSize: 12 }
        }

        ModernButton { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 16; width: 148; text: "回到当前玩家"; iconText: "◎"; onClicked: board.focusCurrentPlayer() }

        Rectangle {
            visible: root.hoveredTile >= 0 && !root.drawerOpen && !root.mapOpen
            x: Math.min(root.width - width - 18, Math.max(18, root.hoverPoint.x + 18))
            y: Math.min(root.height - height - 96, Math.max(92, root.hoverPoint.y + 16))
            width: 278; height: hoverText.implicitHeight + 30; radius: 14; color: "#F01B3639"; border.color: "#6B8B8C"
            Text { id: hoverText; anchors.fill: parent; anchors.margins: 15; text: game.tileDescription(root.hoveredTile); color: "#F2EFE8"; font.pixelSize: 13; wrapMode: Text.WordWrap }
        }

        Rectangle { anchors.fill: parent; visible: root.drawerOpen || root.mapOpen; color: "#78071317"; MouseArea { anchors.fill: parent; onClicked: { root.drawerOpen = false; root.mapOpen = false } } }

        Rectangle {
            id: drawer
            width: Math.min(430, root.width * .36); height: root.height - 24; y: 12
            x: root.drawerOpen ? root.width - width - 12 : root.width + 30
            radius: 22; color: "#F31A3438"; border.color: "#648184"
            Behavior on x { NumberAnimation { duration: 230; easing.type: Easing.OutCubic } }
            MouseArea { anchors.fill: parent }
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 22; spacing: 14
                RowLayout { Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: ["我的产业", "策略与交易", "对局记录", "游戏设置"][root.drawerPage]; color: "#F5F0E7"; font.pixelSize: 24; font.bold: true }
                    ModernButton { compact: true; text: "关闭"; onClicked: root.drawerOpen = false }
                }
                ListView {
                    visible: root.drawerPage === 0
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8; model: game.players
                    delegate: Rectangle { required property var modelData; width: ListView.view.width; height: 94; radius: 12; color: "#203F42"; border.color: modelData.color
                        Column { anchors.fill: parent; anchors.margins: 12; spacing: 5
                            Text { text: modelData.name + "　总繁荣 " + modelData.score; color: "#F1EEE7"; font.bold: true }
                            Text { text: "品牌 " + modelData.reputation + "　创新 " + modelData.culture + "　宜居 " + modelData.livelihood; color: "#A8C0BD"; font.pixelSize: 12 }
                            Rectangle { width: parent.width; height: 6; radius: 3; color: "#142B2E"; Rectangle { width: parent.width * Math.min(1, (modelData.reputation + modelData.culture + modelData.livelihood) / 300); height: parent.height; radius: 3; color: "#C13B42" } }
                        }
                    }
                }
                ColumnLayout {
                    visible: root.drawerPage === 1; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
                    Text { text: "当前策略卡：" + game.firstCardName; color: "#F0E9DA"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    Text { text: "每回合可向一名玩家提出一次资金交易，房主会校验双方资产。"; color: "#A9BFBC"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    Text { text: "交易对象"; color: "#D9E4E0" }
                    SegmentedChoice { id: tradeTarget; Layout.fillWidth: true; options: game.players.map(function(p, i) { return i }); labels: game.players.map(function(p) { return p.name }); suffix: ""; value: Math.min(1, game.players.length - 1) }
                    Text { text: "选择：" + (game.players[tradeTarget.value] ? game.players[tradeTarget.value].name : "—"); color: "#E0B95F" }
                    ModernStepper { id: offeredCash; Layout.fillWidth: true; label: "我方提供资金"; from: 0; to: 20000; stepSize: 100; value: 500 }
                    ModernStepper { id: requestedCash; Layout.fillWidth: true; label: "请求对方资金"; from: 0; to: 20000; stepSize: 100; value: 800 }
                    ModernButton { Layout.fillWidth: true; text: "发起交易"; onClicked: game.proposeSimpleTrade(tradeTarget.value, offeredCash.value, requestedCash.value) }
                    Item { Layout.fillHeight: true }
                }
                ListView { visible: root.drawerPage === 2; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8; model: game.eventLog
                    delegate: Text { required property string modelData; width: ListView.view.width; text: modelData; color: "#B8CDCA"; font.pixelSize: 12; wrapMode: Text.Wrap }
                }
                ColumnLayout {
                    visible: root.drawerPage === 3; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
                    Text { text: "地图缩放  " + board.zoom.toFixed(2) + "×"; color: "#DDE8E4" }
                    RowLayout { Layout.fillWidth: true; ModernButton { compact: true; text: "−"; onClicked: board.zoom = Math.max(.55, board.zoom - .1) } Item { Layout.fillWidth: true } ModernButton { compact: true; text: "+"; onClicked: board.zoom = Math.min(2, board.zoom + .1) } }
                    Text { text: "音乐音量  " + Math.round(audio.musicVolume * 100) + "%"; color: "#DDE8E4" }
                    RowLayout { Layout.fillWidth: true; ModernButton { compact: true; text: "−"; onClicked: audio.musicVolume = Math.max(0, audio.musicVolume - .1) } Item { Layout.fillWidth: true } ModernButton { compact: true; text: "+"; onClicked: audio.musicVolume = Math.min(1, audio.musicVolume + .1) } }
                    Text { text: "音效音量  " + Math.round(audio.effectsVolume * 100) + "%"; color: "#DDE8E4" }
                    RowLayout { Layout.fillWidth: true; ModernButton { compact: true; text: "−"; onClicked: audio.effectsVolume = Math.max(0, audio.effectsVolume - .1) } Item { Layout.fillWidth: true } ModernButton { compact: true; text: "+"; onClicked: audio.effectsVolume = Math.min(1, audio.effectsVolume + .1) } }
                    ModernButton { Layout.fillWidth: true; text: audio.muted ? "恢复声音" : "全部静音"; onClicked: audio.muted = !audio.muted }
                    ModernButton { Layout.fillWidth: true; text: "保存当前对局"; onClicked: game.saveGame() }
                    ModernButton { Layout.fillWidth: true; text: "读取快速存档"; onClicked: game.loadGame() }
                    Item { Layout.fillHeight: true }
                    Text { text: "盛世百业 v0.2.0\n正交俯视 · 现代国潮都市"; color: "#759795"; font.pixelSize: 11 }
                }
            }
        }

        Rectangle {
            id: mapPanel
            visible: root.mapOpen; anchors.centerIn: parent
            width: Math.min(800, parent.width * .72); height: Math.min(580, parent.height * .74)
            radius: 22; color: "#F21A3438"; border.color: "#D3AC58"
            MouseArea { anchors.fill: parent }
            ColumnLayout { anchors.fill: parent; anchors.margins: 18
                RowLayout { Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: "城市地图"; color: "#F5EFE5"; font.pixelSize: 25; font.bold: true }
                    Text { text: "点击节点临时查看"; color: "#A7C0BD"; font.pixelSize: 12 }
                    ModernButton { compact: true; text: "关闭"; onClicked: root.mapOpen = false }
                }
                TopDownCityView { id: overview; Layout.fillWidth: true; Layout.fillHeight: true; viewModel: game; overviewMode: true
                    MouseArea { anchors.fill: parent; onClicked: function(mouse) { var t = overview.tileAt(mouse.x, mouse.y); if (t >= 0) { board.focusTile(t); root.mapOpen = false } } }
                }
            }
        }

        ModernModal {
            id: routeModal
            visible: game.routeSelectionVisible
            opacity: visible ? 1 : 0
            title: "选择前进路线"
            dismissOnBackdrop: false
            panelWidth: 480
            panelHeight: routeChoices.implicitHeight + 130
            Column { id: routeChoices; width: parent.width; spacing: 9
                Text { width: parent.width; text: "选择目的地后，角色将沿高亮路径逐格移动。"; color: "#A9C1BE"; wrapMode: Text.Wrap }
                Repeater { model: game.routeOptions
                    ModernButton { required property var modelData; width: routeChoices.width; text: modelData.name + " · " + modelData.steps + " 步"; onClicked: game.chooseRoute(modelData.option) }
                }
            }
        }

        Item {
            id: diceOverlay
            anchors.fill: parent; visible: game.diceAnimating; z: 900
            Rectangle { anchors.fill: parent; color: "#45071317" }
            Rectangle {
                id: dice
                width: 132; height: 132; radius: 25
                anchors.horizontalCenter: parent.horizontalCenter; y: parent.height * .34
                color: "#F4F0E6"; border.width: 5; border.color: "#D0A748"
                transform: Rotation {
                    id: diceRotation
                    origin.x: dice.width / 2
                    origin.y: dice.height / 2
                    axis.x: .7
                    axis.y: 1
                    axis.z: .4
                    angle: 0
                }
                Repeater {
                    model: 9
                    Rectangle {
                        required property int index
                        property int row: Math.floor(index / 3); property int column: index % 3
                        property bool dotVisible: {
                            var v = game.diceValue
                            if (v === 1) return index === 4
                            if (v === 2) return index === 0 || index === 8
                            if (v === 3) return index === 0 || index === 4 || index === 8
                            if (v === 4) return index === 0 || index === 2 || index === 6 || index === 8
                            if (v === 5) return index === 0 || index === 2 || index === 4 || index === 6 || index === 8
                            return index !== 1 && index !== 4 && index !== 7
                        }
                        visible: dotVisible; width: 19; height: 19; radius: 10; color: index === 4 ? "#C13B42" : "#173B3F"
                        x: 24 + column * 34; y: 24 + row * 34
                    }
                }
            }
            Text { anchors.top: dice.bottom; anchors.topMargin: 24; anchors.horizontalCenter: parent.horizontalCenter; text: game.currentPlayerName + " 正在投掷骰子"; color: "#F6F0E6"; font.pixelSize: 18; font.bold: true }
            SequentialAnimation {
                id: diceAnimation
                ParallelAnimation { NumberAnimation { target: diceRotation; property: "angle"; from: 0; to: 610; duration: 620; easing.type: Easing.OutCubic } NumberAnimation { target: dice; property: "y"; from: root.height * .48; to: root.height * .22; duration: 300; easing.type: Easing.OutQuad } }
                NumberAnimation { target: dice; property: "y"; to: root.height * .34; duration: 220; easing.type: Easing.OutBounce }
            }
            onVisibleChanged: if (visible) { diceRotation.angle = 0; diceAnimation.restart() }
        }

        ModernModal { id: auctionModal; visible: game.canBidAuction; opacity: visible ? 1 : 0; dismissOnBackdrop: false; title: "公开竞价 · " + game.auctionTileName; panelWidth: 470; panelHeight: 300
            Text { width: parent.width; text: "当前出价 " + game.auctionBid.toLocaleString() + "　剩余 " + game.auctionSeconds + " 秒"; color: "#E9C975"; font.pixelSize: 17 }
            Row { width: parent.width; spacing: 10; ModernButton { width: (parent.width - 10) / 2; text: "加价 200"; onClicked: game.bidAuction(game.auctionBid + 200) } ModernButton { width: (parent.width - 10) / 2; text: "放弃竞价"; accent: "#A84A4E"; onClicked: game.passAuction() } }
        }
    }

    ModernModal {
        id: newGameModal
        title: "创建单人游戏"
        panelWidth: 590; panelHeight: 650
        ModernStepper { id: totalPlayers; width: parent.width; label: "总玩家数"; from: 2; to: 6; value: 4; onValueChanged: aiPlayers.value = Math.min(aiPlayers.value, value - 1) }
        ModernStepper { id: aiPlayers; width: parent.width; label: "AI 玩家数"; from: 0; to: totalPlayers.value - 1; value: 3 }
        Text { text: "对局长度"; color: "#DDE7E3"; font.pixelSize: 14 }
        SegmentedChoice { id: rounds; width: parent.width; value: 120 }
        Text { text: "选择你的主理人"; color: "#DDE7E3"; font.pixelSize: 14 }
        Item { width: parent.width; height: 116
            Row { anchors.fill: parent; spacing: 7
                Repeater { model: game.characters
                    Rectangle { required property var modelData; width: (parent.width - 35) / 6; height: 108; radius: 12; color: characterChoice.value === modelData.index ? "#A9363C" : "#173437"; border.color: characterChoice.value === modelData.index ? "#E0B658" : "#48696B"
                        Text { anchors.centerIn: parent; width: parent.width - 8; text: modelData.name; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap; color: "#F4EFE5"; font.pixelSize: 13; font.bold: true }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: characterChoice.value = modelData.index }
                    }
                }
            }
        }
        QtObject { id: characterChoice; property int value: 0 }
        Row { width: parent.width; spacing: 10
            ModernButton { width: (parent.width - 10) / 2; text: "开始游戏"; accent: "#D6AF57"; onClicked: { game.newGame(totalPlayers.value, aiPlayers.value, rounds.value, characterChoice.value); newGameModal.close(); root.page = "game" } }
            ModernButton { width: (parent.width - 10) / 2; text: "取消"; accent: "#A84A4E"; onClicked: newGameModal.close() }
        }
    }

    ModernModal {
        id: networkModal
        title: "联机游戏"
        panelWidth: 620; panelHeight: 700
        Text { width: parent.width; text: "同一局域网或 Radmin 网络中的玩家可输入房主 IPv4 地址连接。"; color: "#ABC1BE"; wrapMode: Text.Wrap }
        ModernStepper { id: hostTotal; width: parent.width; label: "房间总人数"; from: 2; to: 6; value: 4; onValueChanged: hostAi.value = Math.min(hostAi.value, value - 2) }
        ModernStepper { id: hostAi; width: parent.width; label: "AI 玩家数"; from: 0; to: Math.max(0, hostTotal.value - 2); value: 2 }
        Text { text: "房间对局长度"; color: "#DDE7E3" }
        SegmentedChoice { id: hostRounds; width: parent.width; value: 120 }
        Text { text: "选择房主角色"; color: "#DDE7E3" }
        SegmentedChoice { id: hostCharacter; width: parent.width; options: [0, 1, 2, 3, 4, 5]; labels: game.characters.map(function(c) { return c.name }); suffix: ""; value: 0 }
        Rectangle { width: parent.width; height: 52; radius: 11; color: "#122B2E"; border.color: addressInput.activeFocus ? "#D6AF57" : "#47686A"
            TextInput { id: addressInput; anchors.fill: parent; anchors.margins: 14; text: "127.0.0.1"; color: "#F4F0E7"; selectionColor: "#A9363C"; font.pixelSize: 15; verticalAlignment: TextInput.AlignVCenter }
        }
        Row { width: parent.width; spacing: 10
            ModernButton { width: (parent.width - 10) / 2; text: "创建房间"; accent: "#D6AF57"; onClicked: { game.hostGame(hostTotal.value, hostAi.value, hostRounds.value, hostCharacter.value, 29450); networkModal.close(); root.page = "game" } }
            ModernButton { width: (parent.width - 10) / 2; text: "加入房间"; accent: "#4E9D9C"; onClicked: { game.joinGame(addressInput.text, 29450); networkModal.close(); root.page = "game" } }
        }
        ModernButton { width: parent.width; text: "取消"; accent: "#A84A4E"; onClicked: networkModal.close() }
    }

    ModernModal {
        id: releaseNotesModal
        title: releaseNotes.title
        eyebrow: "更新内容 · V" + releaseNotes.version + " · " + releaseNotes.releaseDate
        panelWidth: 720; panelHeight: Math.min(720, root.height - 60)
        onClosed: releaseNotes.markRead()
        Flickable {
            width: parent.width; height: releaseNotesModal.panelHeight - 190
            contentWidth: width; contentHeight: notesColumn.implicitHeight; clip: true
            boundsBehavior: Flickable.StopAtBounds
            Column {
                id: notesColumn; width: parent.width; spacing: 15
                Repeater { model: releaseNotes.sections
                    Rectangle { required property var modelData; width: notesColumn.width; height: sectionColumn.implicitHeight + 26; radius: 14; color: "#173437"; border.color: "#496A6C"
                        Column { id: sectionColumn; anchors.fill: parent; anchors.margins: 13; spacing: 8
                            Text { text: modelData.title; color: "#E5BC62"; font.pixelSize: 17; font.bold: true }
                            Repeater { model: modelData.items
                                Text { required property string modelData; width: sectionColumn.width; text: "•  " + modelData; color: "#D0DEDA"; font.pixelSize: 13; wrapMode: Text.Wrap }
                            }
                        }
                    }
                }
            }
        }
        ModernButton { width: parent.width; text: "我知道了"; accent: "#D6AF57"; onClicked: releaseNotesModal.close() }
    }

    Connections { target: game; function onToastRequested(message) { toastText.text = message; toast.visible = true; toastTimer.restart() } }
    Rectangle { id: toast; visible: false; z: 2000; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 96; width: Math.min(620, toastText.implicitWidth + 48); height: toastText.implicitHeight + 26; radius: 14; color: "#F0244144"; border.color: "#D3AC58"
        Text { id: toastText; anchors.centerIn: parent; color: "#F7F2E8"; font.pixelSize: 13 }
        Timer { id: toastTimer; interval: 3200; onTriggered: toast.visible = false }
    }

    Component.onCompleted: if (releaseNotes.unread && !suppressAutoReleaseNotes) Qt.callLater(function() { releaseNotesModal.open() })
}
