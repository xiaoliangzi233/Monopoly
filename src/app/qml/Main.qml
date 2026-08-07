import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NeonTycoon 1.0

ApplicationWindow {
    id: root
    width: 1440
    height: 900
    minimumWidth: 1100
    minimumHeight: 680
    visible: false
    color: "#070B16"
    title: "霓城大亨 · Neon Tycoon"

    property int hoveredTile: -1
    property point hoverPoint: Qt.point(0, 0)
    property bool drawerOpen: false
    property int drawerPage: 0

    function openDrawer(page) {
        drawerPage = page
        drawerOpen = true
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#101C38" }
            GradientStop { position: 0.42; color: "#080F20" }
            GradientStop { position: 1; color: "#050812" }
        }
    }

    Repeater {
        model: 22
        Rectangle {
            required property int index
            width: index % 3 === 0 ? 3 : 1
            height: parent.height * (0.15 + ((index * 37) % 60) / 100)
            x: (index * root.width / 21) - width / 2
            anchors.bottom: parent.bottom
            color: index % 2 ? "#0C1B32" : "#101F3A"
            opacity: 0.5
            Rectangle {
                width: parent.width + 1
                height: 2
                y: parent.height * 0.18
                color: index % 3 ? "#2A8FA8" : "#9B3B8B"
                opacity: 0.35
            }
        }
    }

    Item {
        id: sceneViewport
        anchors.fill: parent
        anchors.margins: 6
        clip: true

        IsometricBoard {
            id: board
            anchors.fill: parent
            viewModel: game
            sceneScale: zoomSlider.value * Math.min(1.42, Math.max(0.86, root.width / 1440))

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
                onPositionChanged: function(mouse) {
                    root.hoveredTile = board.tileAt(mouse.x, mouse.y)
                    root.hoverPoint = Qt.point(mouse.x, mouse.y)
                }
                onExited: root.hoveredTile = -1
            }
        }
    }

    HudPanel {
        id: turnHud
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 18
        width: Math.min(650, parent.width * 0.48)
        height: 66
        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 15
            Rectangle {
                Layout.preferredWidth: 9
                Layout.fillHeight: true
                radius: 5
                color: game.currentPlayerColor
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1
                Text { text: game.currentPlayerName; color: "#F5FDFF"; font.pixelSize: 19; font.weight: Font.Bold }
                Text { text: game.statusText + "  ·  " + game.tileName; color: "#91A9C8"; font.pixelSize: 12 }
            }
            Text { text: "第 " + game.currentRound + " / " + game.maxRounds + " 轮"; color: "#BFEFFF"; font.pixelSize: 14 }
            Rectangle { width: 1; Layout.fillHeight: true; color: "#35506C" }
            Text { text: "⚡" + game.currentEnergy + "  ◈ " + game.currentCash.toLocaleString(); color: "#FFD66B"; font.pixelSize: 17; font.weight: Font.DemiBold }
        }
    }

    Column {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 18
        spacing: 8
        GlassButton { width: 54; text: "排行"; onClicked: root.openDrawer(0) }
        GlassButton { width: 54; text: "记录"; onClicked: root.openDrawer(1) }
        GlassButton { width: 54; text: "联机"; onClicked: networkDialog.open() }
        GlassButton { width: 54; text: "设置"; onClicked: root.openDrawer(2) }
    }

    HudPanel {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 18
        width: 210
        height: 52
        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            Text { text: game.networkStatus; color: "#9CB4CF"; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
            Rectangle { width: 8; height: 8; radius: 4; color: game.networkStatus.indexOf("错误") >= 0 ? "#FF6179" : "#58EFA5" }
        }
    }

    HudPanel {
        id: actionBar
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 18
        width: actionRow.implicitWidth + 28
        height: 70
        Row {
            id: actionRow
            anchors.centerIn: parent
            spacing: 10
            GlassButton { visible: game.canRoll; text: "掷骰  ◈"; accent: "#52F7FF"; onClicked: game.rollDice() }
            GlassButton { visible: game.canUseSkill; text: "角色技能"; accent: "#FF5CA8"; onClicked: game.useSkill() }
            GlassButton { visible: game.canUseCard; text: game.firstCardName; accent: "#32E6C4"; onClicked: game.useFirstCard() }
            GlassButton { visible: game.canBuy; text: "购置地产"; accent: "#FFD166"; onClicked: game.buyCurrentProperty() }
            GlassButton { visible: game.canUpgrade; text: "升级建筑"; accent: "#76F28D"; onClicked: game.upgradeCurrentProperty() }
            GlassButton { visible: game.canMortgage; text: "抵押 / 赎回"; accent: "#FF875E"; onClicked: game.mortgageCurrentProperty() }
            GlassButton { visible: game.canEndTurn; text: "结束回合"; accent: "#AA8BFF"; onClicked: game.endTurn() }
            BusyIndicator { visible: game.aiThinking; running: visible; width: 40; height: 40 }
            Text { visible: game.aiThinking; anchors.verticalCenter: parent.verticalCenter; text: "城市代理正在规划…"; color: "#BBD0E8" }
        }
    }

    HudPanel {
        visible: root.hoveredTile >= 0 && !root.drawerOpen
        x: Math.min(root.width - width - 20, Math.max(20, root.hoverPoint.x + 18))
        y: Math.min(root.height - height - 90, Math.max(92, root.hoverPoint.y + 16))
        width: 230
        height: hoverText.implicitHeight + 28
        Text {
            id: hoverText
            anchors.fill: parent
            anchors.margins: 14
            text: game.tileDescription(root.hoveredTile)
            color: "#E9FAFF"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
        }
    }

    Rectangle {
        id: drawerShade
        anchors.fill: parent
        color: "#70020812"
        visible: root.drawerOpen
        MouseArea { anchors.fill: parent; onClicked: root.drawerOpen = false }
    }

    HudPanel {
        id: drawer
        width: Math.min(380, root.width * 0.32)
        y: 12
        height: root.height - 24
        x: root.drawerOpen ? root.width - width - 12 : root.width + 20
        Behavior on x { NumberAnimation { duration: 210; easing.type: Easing.OutCubic } }

        MouseArea { anchors.fill: parent }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 14
            RowLayout {
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: root.drawerPage === 0 ? "城市排行" : root.drawerPage === 1 ? "事件记录" : "显示与存档"
                    color: "#F1FBFF"; font.pixelSize: 23; font.weight: Font.Bold
                }
                GlassButton { width: 48; text: "×"; onClicked: root.drawerOpen = false }
            }
            ListView {
                visible: root.drawerPage === 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8
                model: game.players
                delegate: Rectangle {
                    required property var modelData
                    width: ListView.view.width
                    height: 64
                    radius: 12
                    color: "#182641"
                    border.color: modelData.color
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 12
                        Rectangle { width: 9; Layout.fillHeight: true; radius: 4; color: modelData.color }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 1
                            Text { text: modelData.name + (modelData.ai ? "  · AI" : ""); color: "#EAF9FF"; font.pixelSize: 15 }
                            Text { text: "现金 " + modelData.cash.toLocaleString() + "  ·  估值 " + modelData.score.toLocaleString(); color: "#8FAAC8"; font.pixelSize: 12 }
                        }
                    }
                }
            }
            ListView {
                visible: root.drawerPage === 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 7
                model: game.eventLog
                delegate: Text { required property string modelData; width: ListView.view.width; text: modelData; color: "#AFC5DB"; font.pixelSize: 13; wrapMode: Text.Wrap }
            }
            ColumnLayout {
                visible: root.drawerPage === 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 13
                Text { text: "地图缩放"; color: "#BFD3E7" }
                Slider { id: zoomSlider; from: 0.7; to: 1.45; value: 1.0; Layout.fillWidth: true }
                GlassButton { Layout.fillWidth: true; text: "保存当前对局"; onClicked: game.saveGame() }
                GlassButton { Layout.fillWidth: true; text: "恢复快速存档"; onClicked: game.loadGame() }
                GlassButton { Layout.fillWidth: true; text: "开始新的本地对局"; onClicked: newGameDialog.open() }
                Item { Layout.fillHeight: true }
                Text { text: "所有场景模型均由程序化几何生成\n版本 0.1.0"; color: "#687F9C"; font.pixelSize: 11 }
            }
        }
    }

    Dialog {
        id: newGameDialog
        modal: true
        anchors.centerIn: parent
        title: "新建本地对局"
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: { game.newGame(playerCount.value, aiCount.value, roundBox.currentValue); root.drawerOpen = false }
        ColumnLayout {
            width: 330
            Label { text: "总玩家数" }
            SpinBox { id: playerCount; from: 2; to: 6; value: 4; Layout.fillWidth: true }
            Label { text: "AI玩家数" }
            SpinBox { id: aiCount; from: 0; to: Math.max(0, playerCount.value - 1); value: 3; Layout.fillWidth: true }
            Label { text: "回合数" }
            ComboBox { id: roundBox; model: [16, 24, 32]; currentIndex: 1; Layout.fillWidth: true }
        }
    }

    Dialog {
        id: networkDialog
        modal: true
        anchors.centerIn: parent
        title: "局域网 / Radmin 联机"
        standardButtons: Dialog.Close
        ColumnLayout {
            width: 390
            spacing: 12
            Label { text: "房主可直接开启端口 29450；其他玩家填写房主的局域网或 Radmin IPv4。"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            RowLayout {
                Layout.fillWidth: true
                Label { text: "总人数" }
                SpinBox { id: onlinePlayers; from: 2; to: 6; value: 4 }
                Label { text: "AI" }
                SpinBox { id: onlineAi; from: 0; to: Math.max(0, onlinePlayers.value - 2); value: 2 }
            }
            GlassButton { Layout.fillWidth: true; text: "创建房主权威房间"; onClicked: { game.hostGame(onlinePlayers.value, onlineAi.value, 29450); networkDialog.close() } }
            TextField { id: hostAddress; Layout.fillWidth: true; placeholderText: "例如 26.12.34.56"; text: "127.0.0.1" }
            GlassButton { Layout.fillWidth: true; text: "连接房间"; onClicked: { game.joinGame(hostAddress.text, 29450); networkDialog.close() } }
        }
    }

    Popup {
        id: toast
        x: (root.width - width) / 2
        y: root.height - height - 105
        width: Math.min(460, toastLabel.implicitWidth + 42)
        height: 50
        modal: false
        closePolicy: Popup.NoAutoClose
        background: Rectangle { radius: 14; color: "#EA16243C"; border.color: "#5F7FA4" }
        contentItem: Label { id: toastLabel; color: "#EAF9FF"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        Timer { id: toastTimer; interval: 2600; onTriggered: toast.close() }
    }

    Connections {
        target: game
        function onToastRequested(message) {
            toastLabel.text = message
            toast.open()
            toastTimer.restart()
        }
    }

    Shortcut { sequence: "Ctrl+S"; onActivated: game.saveGame() }
    Shortcut { sequence: "Space"; enabled: game.canRoll; onActivated: game.rollDice() }
}
