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
    color: "#172821"
    title: "盛世百业"

    property string page: "menu"
    property int hoveredTile: -1
    property point hoverPoint: Qt.point(0, 0)
    property bool drawerOpen: false
    property int drawerPage: 0
    property bool mapOpen: false
    property bool rosterOpen: true

    function openDrawer(page) {
        drawerPage = page
        drawerOpen = true
        mapOpen = false
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#28483D" }
            GradientStop { position: .52; color: "#172C26" }
            GradientStop { position: 1; color: "#101D1A" }
        }
    }

    Item {
        id: menuPage
        anchors.fill: parent
        visible: root.page === "menu"

        Repeater {
            model: 10
            Item {
                required property int index
                width: 250 + (index % 3) * 60
                height: 180 + (index % 4) * 35
                x: index * (root.width + 180) / 9 - 130
                y: root.height * .52 + (index % 2) * 70
                opacity: .56
                Rectangle { width: parent.width; height: parent.height * .34; y: parent.height * .28; color: index % 2 ? "#35574C" : "#2C4A42" }
                Rectangle { width: parent.width + 30; height: 24; x: -15; y: parent.height * .22; color: index % 2 ? "#81443F" : "#456B5D"; rotation: index % 2 ? -2 : 2 }
                Rectangle { width: 7; height: parent.height * .62; x: 28; y: parent.height * .34; color: "#765A3D" }
                Rectangle { width: 7; height: parent.height * .62; x: parent.width - 35; y: parent.height * .34; color: "#765A3D" }
            }
        }

        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: parent.height * .22; color: "#173C3C"; opacity: .74 }
        Repeater {
            model: 16
            Rectangle {
                required property int index
                width: 3
                height: 70 + (index * 31) % 100
                x: index * root.width / 15
                anchors.bottom: parent.bottom
                color: "#D8B651"
                opacity: .28
                Rectangle { width: 18; height: 13; radius: 6; x: -8; color: "#B83A2D" }
            }
        }

        ColumnLayout {
            anchors.left: parent.left
            anchors.leftMargin: Math.max(72, root.width * .09)
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -40
            width: 470
            spacing: 14
            Text { text: "盛世百业"; color: "#F1E4BD"; font.family: "STKaiti"; font.pixelSize: 76; font.weight: Font.Bold; font.letterSpacing: 8 }
            Rectangle { Layout.preferredWidth: 320; height: 3; color: "#D2A641" }
            Text { text: "架空盛世 · 百工商旅 · 山河共兴"; color: "#C8D1B0"; font.family: "Microsoft YaHei UI"; font.pixelSize: 18; font.letterSpacing: 3 }
            Item { height: 26 }
            GlassButton { Layout.preferredWidth: 260; text: "开设新局"; accent: "#D3A743"; onClicked: newGameDialog.open() }
            GlassButton { Layout.preferredWidth: 260; text: "恢复经营"; onClicked: { if (game.loadGame()) root.page = "game" } }
            GlassButton { Layout.preferredWidth: 260; text: "联机雅集"; onClicked: networkDialog.open() }
            GlassButton { Layout.preferredWidth: 260; text: "退出游戏"; accent: "#8E5846"; onClicked: Qt.quit() }
        }

        HudPanel {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 28
            width: 330
            height: 76
            RowLayout {
                anchors.fill: parent; anchors.margins: 13
                Rectangle { width: 42; height: 42; radius: 21; color: "#B83A2D"; Text { anchors.centerIn: parent; text: "印"; color: "#F6E3B7"; font.family: "STKaiti"; font.pixelSize: 22 } }
                ColumnLayout { Layout.fillWidth: true; spacing: 1
                    Text { text: "v0.2.0 国潮重制"; color: "#F0E6C9"; font.pixelSize: 14; font.bold: true }
                    Text { text: "程序化绘制 · 房主权威联机"; color: "#9DB3A4"; font.pixelSize: 11 }
                }
            }
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

            MouseArea {
                id: boardMouse
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                property real lastX
                property real lastY
                property bool dragged: false
                onPressed: function(mouse) { lastX = mouse.x; lastY = mouse.y; dragged = false }
                onPositionChanged: function(mouse) {
                    if (pressed) {
                        var dx = mouse.x - lastX
                        var dy = mouse.y - lastY
                        if (Math.abs(dx) + Math.abs(dy) > 1) { board.panBy(dx, dy); dragged = true }
                        lastX = mouse.x; lastY = mouse.y
                    }
                    root.hoveredTile = board.tileAt(mouse.x, mouse.y)
                    root.hoverPoint = Qt.point(mouse.x, mouse.y)
                }
                onWheel: function(wheel) { board.zoomAt(wheel.x, wheel.y, wheel.angleDelta.y); wheel.accepted = true }
                onExited: root.hoveredTile = -1
            }
        }

        HudPanel {
            id: turnHud
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 14
            width: Math.min(760, parent.width * .56)
            height: 72
            RowLayout {
                anchors.fill: parent; anchors.margins: 12; spacing: 13
                Rectangle { width: 44; height: 44; radius: 22; color: game.currentPlayerColor; border.color: "#D8B04B"; border.width: 2
                    Text { anchors.centerIn: parent; text: game.currentPlayerName.substring(0, 1); color: "#F7E9C4"; font.family: "STKaiti"; font.pixelSize: 24; font.bold: true }
                }
                ColumnLayout { Layout.fillWidth: true; spacing: 1
                    Text { text: game.currentPlayerName + " · " + game.tileName; color: "#F3E8C9"; font.pixelSize: 18; font.bold: true }
                    Text { text: game.statusText + "　 第 " + game.currentRound + " / " + game.maxRounds + " 轮"; color: "#AFC1AE"; font.pixelSize: 12 }
                }
                Text { text: "银 " + game.currentCash.toLocaleString(); color: "#E2BA55"; font.pixelSize: 17; font.bold: true }
                Rectangle { width: 1; Layout.fillHeight: true; color: "#776B4C" }
                Text { text: "繁荣 " + game.currentProsperity; color: "#F0D68E"; font.pixelSize: 16 }
            }
        }

        Column {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: 14
            anchors.topMargin: 18
            spacing: 7
            GlassButton { width: 50; height: 42; text: root.rosterOpen ? "收" : "众"; onClicked: root.rosterOpen = !root.rosterOpen }
            GlassButton { width: 50; height: 42; text: "图"; accent: "#D3A743"; onClicked: { root.mapOpen = !root.mapOpen; root.drawerOpen = false } }
            GlassButton { width: 50; height: 42; text: "业"; onClicked: root.openDrawer(0) }
            GlassButton { width: 50; height: 42; text: "策"; onClicked: root.openDrawer(1) }
            GlassButton { width: 50; height: 42; text: "记"; onClicked: root.openDrawer(2) }
            GlassButton { width: 50; height: 42; text: "设"; onClicked: root.openDrawer(3) }
            GlassButton { width: 50; height: 42; text: "返"; accent: "#8F5A48"; onClicked: root.page = "menu" }
        }

        HudPanel {
            visible: root.rosterOpen
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 76
            anchors.topMargin: 104
            width: 220
            height: Math.min(playerColumn.implicitHeight + 20, parent.height * .57)
            Column {
                id: playerColumn
                anchors.fill: parent; anchors.margins: 10; spacing: 6
                Repeater {
                    model: game.players
                    Rectangle {
                        required property var modelData
                        width: playerColumn.width
                        height: 54
                        radius: 4
                        color: modelData.bankrupt ? "#594D46" : "#344E42"
                        border.color: modelData.color
                        opacity: modelData.bankrupt ? .55 : .94
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 8
                            Rectangle { width: 7; Layout.fillHeight: true; color: modelData.color }
                            ColumnLayout { Layout.fillWidth: true; spacing: 0
                                Text { text: modelData.name + (modelData.ai ? " · AI" : ""); color: "#F1E6C8"; font.pixelSize: 13; font.bold: true }
                                Text { text: "银 " + modelData.cash.toLocaleString() + " · 繁荣 " + modelData.score; color: "#B6C3AD"; font.pixelSize: 10 }
                            }
                        }
                    }
                }
            }
        }

        HudPanel {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: 14
            width: 228
            height: 58
            RowLayout { anchors.fill: parent; anchors.margins: 10
                Rectangle { width: 10; height: 10; radius: 5; color: game.networkStatus.indexOf("错误") >= 0 ? "#B83A2D" : "#6F9D69" }
                Text { Layout.fillWidth: true; text: game.networkStatus; color: "#C7D1B9"; elide: Text.ElideRight; font.pixelSize: 11 }
            }
        }

        HudPanel {
            id: actionBar
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 14
            width: Math.min(parent.width - 500, actionRow.implicitWidth + 26)
            height: 68
            Row {
                id: actionRow
                anchors.centerIn: parent; spacing: 8
                GlassButton { visible: game.canRoll; text: "投掷行筹"; accent: "#D3A743"; onClicked: game.rollDice() }
                GlassButton { visible: game.canReroll; text: "重新投掷"; onClicked: game.rerollDice() }
                GlassButton { visible: game.canUseSkill; text: "施展才略"; accent: "#B83A2D"; onClicked: game.useSkill() }
                GlassButton { visible: game.canUseCard; text: game.firstCardName; onClicked: game.useFirstCard() }
                GlassButton { visible: game.canBuy; text: "置办产业"; accent: "#D3A743"; onClicked: game.buyCurrentProperty() }
                GlassButton { visible: game.canUpgrade; text: "扩建产业"; onClicked: game.upgradeCurrentProperty() }
                GlassButton { visible: game.canMortgage; text: "典当 / 赎回"; onClicked: game.mortgageCurrentProperty() }
                GlassButton { visible: game.canContribute; text: "兴办民生"; onClicked: game.contributeCivic() }
                GlassButton { visible: game.canEndTurn; text: "结束回合"; accent: "#8F5A48"; onClicked: game.endTurn() }
                BusyIndicator { visible: game.aiThinking; running: visible; width: 38; height: 38 }
            }
        }

        GlassButton {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 16
            width: 128
            text: "回到当前人物"
            onClicked: board.focusCurrentPlayer()
        }

        HudPanel {
            visible: root.hoveredTile >= 0 && !root.drawerOpen && !root.mapOpen
            x: Math.min(root.width - width - 18, Math.max(18, root.hoverPoint.x + 18))
            y: Math.min(root.height - height - 92, Math.max(92, root.hoverPoint.y + 16))
            width: 260
            height: hoverText.implicitHeight + 28
            Text { id: hoverText; anchors.fill: parent; anchors.margins: 14; text: game.tileDescription(root.hoveredTile); color: "#F4E9CC"; font.pixelSize: 13; wrapMode: Text.WordWrap }
        }

        Rectangle {
            anchors.fill: parent
            color: "#77131D19"
            visible: root.drawerOpen || root.mapOpen
            MouseArea { anchors.fill: parent; onClicked: { root.drawerOpen = false; root.mapOpen = false } }
        }

        HudPanel {
            id: drawer
            width: Math.min(410, root.width * .34)
            height: root.height - 24
            y: 12
            x: root.drawerOpen ? root.width - width - 12 : root.width + 24
            Behavior on x { NumberAnimation { duration: 230; easing.type: Easing.OutCubic } }
            MouseArea { anchors.fill: parent }
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 20; spacing: 13
                RowLayout { Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: ["百业总览", "筹策与交易", "城市记事", "游玩设置"][root.drawerPage]; color: "#F1E3C0"; font.family: "STKaiti"; font.pixelSize: 25; font.bold: true }
                    GlassButton { width: 44; text: "合"; onClicked: root.drawerOpen = false }
                }
                ListView {
                    visible: root.drawerPage === 0
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8; model: game.players
                    delegate: Rectangle {
                        required property var modelData
                        width: ListView.view.width; height: 92; radius: 5; color: "#314B40"; border.color: modelData.color
                        Column { anchors.fill: parent; anchors.margins: 11; spacing: 4
                            Text { text: modelData.name + "　繁荣 " + modelData.score; color: "#F0E4C6"; font.bold: true }
                            Text { text: "商誉 " + modelData.reputation + "　文脉 " + modelData.culture + "　民生 " + modelData.livelihood; color: "#B8C7B2"; font.pixelSize: 12 }
                            ProgressBar { width: parent.width; from: 0; to: 300; value: modelData.reputation + modelData.culture + modelData.livelihood }
                        }
                    }
                }
                ColumnLayout {
                    visible: root.drawerPage === 1
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
                    Text { text: "当前筹策：" + game.firstCardName; color: "#EADCB9"; wrapMode: Text.Wrap }
                    Text { text: "每回合可向一名玩家提出一次银两交易。房主会再次校验双方资产。"; color: "#AFC0AE"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    ComboBox { id: tradeTarget; Layout.fillWidth: true; model: game.players; textRole: "name" }
                    SpinBox { id: offeredCash; Layout.fillWidth: true; from: 0; to: 20000; stepSize: 100; value: 500 }
                    SpinBox { id: requestedCash; Layout.fillWidth: true; from: 0; to: 20000; stepSize: 100; value: 800 }
                    GlassButton { Layout.fillWidth: true; text: "提出交易"; onClicked: game.proposeSimpleTrade(tradeTarget.currentIndex, offeredCash.value, requestedCash.value) }
                    Item { Layout.fillHeight: true }
                }
                ListView {
                    visible: root.drawerPage === 2
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8; model: game.eventLog
                    delegate: Text { required property string modelData; width: ListView.view.width; text: modelData; color: "#B9C8B5"; font.pixelSize: 12; wrapMode: Text.Wrap }
                }
                ColumnLayout {
                    visible: root.drawerPage === 3
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
                    Text { text: "地图缩放"; color: "#E5D7B5" }
                    Slider { from: .55; to: 2; value: board.zoom; Layout.fillWidth: true; onMoved: board.zoom = value }
                    Text { text: "音乐"; color: "#E5D7B5" }
                    Slider { from: 0; to: 1; value: audio.musicVolume; Layout.fillWidth: true; onMoved: audio.musicVolume = value }
                    Text { text: "音效"; color: "#E5D7B5" }
                    Slider { from: 0; to: 1; value: audio.effectsVolume; Layout.fillWidth: true; onMoved: audio.effectsVolume = value }
                    CheckBox { text: "全部静音"; checked: audio.muted; onToggled: audio.muted = checked }
                    GlassButton { Layout.fillWidth: true; text: "保存当前对局"; onClicked: game.saveGame() }
                    GlassButton { Layout.fillWidth: true; text: "恢复快速存档"; onClicked: game.loadGame() }
                    GlassButton { Layout.fillWidth: true; text: "新建本地对局"; onClicked: newGameDialog.open() }
                    Item { Layout.fillHeight: true }
                    Text { text: "盛世百业 v0.2.0\n正交俯视 · 程序化国潮场景"; color: "#829789"; font.pixelSize: 11 }
                }
            }
        }

        HudPanel {
            id: mapPanel
            visible: root.mapOpen
            anchors.centerIn: parent
            width: Math.min(780, parent.width * .72)
            height: Math.min(570, parent.height * .72)
            MouseArea { anchors.fill: parent }
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 18
                RowLayout { Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: "山河舆图"; color: "#F1E2BB"; font.family: "STKaiti"; font.pixelSize: 27; font.bold: true }
                    Text { text: "点击节点临时查看"; color: "#A9B9A7"; font.pixelSize: 12 }
                    GlassButton { width: 48; text: "合"; onClicked: root.mapOpen = false }
                }
                TopDownCityView {
                    id: overview
                    Layout.fillWidth: true; Layout.fillHeight: true
                    viewModel: game
                    overviewMode: true
                    MouseArea { anchors.fill: parent; onClicked: function(mouse) { var t = overview.tileAt(mouse.x, mouse.y); if (t >= 0) { board.focusTile(t); root.mapOpen = false } } }
                }
            }
        }

        HudPanel {
            visible: game.routeOptions.length > 1
            anchors.centerIn: parent
            width: 420
            height: routeColumn.implicitHeight + 42
            Column {
                id: routeColumn
                anchors.fill: parent; anchors.margins: 20; spacing: 9
                Text { text: "择路而行"; color: "#F0DFB8"; font.family: "STKaiti"; font.pixelSize: 25; anchors.horizontalCenter: parent.horizontalCenter }
                Repeater { model: game.routeOptions
                    GlassButton { required property var modelData; width: routeColumn.width; text: (modelData.option + 1) + " · " + modelData.name + "（" + modelData.steps + "步）"; onClicked: game.chooseRoute(modelData.option) }
                }
                GlassButton { visible: game.canReroll; width: routeColumn.width; text: "消耗一次重掷"; accent: "#8F5A48"; onClicked: game.rerollDice() }
            }
        }

        HudPanel {
            visible: game.auctionSeconds > 0
            anchors.centerIn: parent
            width: 430
            height: 245
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 22; spacing: 10
                Text { text: "百业竞价"; color: "#F0DFB8"; font.family: "STKaiti"; font.pixelSize: 27; Layout.alignment: Qt.AlignHCenter }
                Text { text: game.auctionTileName; color: "#D7C8A5"; font.pixelSize: 16; Layout.alignment: Qt.AlignHCenter }
                Text { text: "当前 " + game.auctionBid.toLocaleString() + " 两　余 " + game.auctionSeconds + " 秒"; color: "#E1B74E"; font.pixelSize: 18; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                RowLayout { Layout.alignment: Qt.AlignHCenter
                    GlassButton { text: "+100出价"; enabled: game.canBidAuction; onClicked: game.bidAuction(game.auctionBid + 100) }
                    GlassButton { text: "+500出价"; enabled: game.canBidAuction; onClicked: game.bidAuction(game.auctionBid + 500) }
                    GlassButton { text: "退出竞价"; accent: "#8F5A48"; onClicked: game.passAuction() }
                }
            }
        }

        HudPanel {
            visible: game.tradePending
            anchors.centerIn: parent
            width: 390
            height: 190
            ColumnLayout { anchors.fill: parent; anchors.margins: 22
                Text { text: "商议交易"; color: "#F0DFB8"; font.family: "STKaiti"; font.pixelSize: 26; Layout.alignment: Qt.AlignHCenter }
                Text { text: "对方提出一项经房主校验的交易，请选择接受或婉拒。"; color: "#C6D0BA"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                RowLayout { Layout.alignment: Qt.AlignHCenter
                    GlassButton { text: "接受"; accent: "#D3A743"; onClicked: game.respondTrade(true) }
                    GlassButton { text: "婉拒"; onClicked: game.respondTrade(false) }
                }
            }
        }
    }

    Dialog {
        id: newGameDialog
        modal: true
        anchors.centerIn: parent
        title: "开设盛世新局"
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: { game.newGame(playerCount.value, aiCount.value, roundBox.currentValue); root.page = "game"; root.drawerOpen = false }
        ColumnLayout {
            width: 350
            Label { text: "总玩家数" }
            SpinBox { id: playerCount; from: 2; to: 6; value: 4; Layout.fillWidth: true }
            Label { text: "AI玩家数" }
            SpinBox { id: aiCount; from: 0; to: Math.max(0, playerCount.value - 1); value: 3; Layout.fillWidth: true }
            Label { text: "对局轮数" }
            ComboBox { id: roundBox; model: [24, 32, 40]; currentIndex: 1; Layout.fillWidth: true }
        }
    }

    Dialog {
        id: networkDialog
        modal: true
        anchors.centerIn: parent
        title: "局域网 / Radmin 联机"
        standardButtons: Dialog.Close
        ColumnLayout {
            width: 410; spacing: 11
            Label { text: "房主开启29450端口；其他玩家填写房主的局域网或Radmin IPv4。v0.1.0客户端不能加入新版房间。"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            RowLayout {
                Label { text: "总人数" }
                SpinBox { id: onlinePlayers; from: 2; to: 6; value: 4 }
                Label { text: "AI" }
                SpinBox { id: onlineAi; from: 0; to: Math.max(0, onlinePlayers.value - 2); value: 2 }
            }
            GlassButton { Layout.fillWidth: true; text: "创建房主权威房间"; onClicked: { game.hostGame(onlinePlayers.value, onlineAi.value, 29450); root.page = "game"; networkDialog.close() } }
            TextField { id: hostAddress; Layout.fillWidth: true; placeholderText: "例如 26.12.34.56"; text: "127.0.0.1" }
            GlassButton { Layout.fillWidth: true; text: "连接房间"; onClicked: { game.joinGame(hostAddress.text, 29450); root.page = "game"; networkDialog.close() } }
        }
    }

    Popup {
        id: toast
        x: (root.width - width) / 2
        y: root.height - height - 100
        width: Math.min(540, toastLabel.implicitWidth + 44)
        height: 54
        modal: false
        closePolicy: Popup.NoAutoClose
        background: Rectangle { radius: 5; color: "#F02A4037"; border.color: "#C5A24B" }
        contentItem: Label { id: toastLabel; color: "#F1E6C8"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        Timer { id: toastTimer; interval: 3200; onTriggered: toast.close() }
    }

    Connections {
        target: game
        function onToastRequested(message) { toastLabel.text = message; toast.open(); toastTimer.restart() }
        function onCameraFocusRequested(tileIndex) { if (root.page === "game") board.focusTile(tileIndex) }
        function onStateChanged() {
            audio.district = Math.max(0, Math.min(7, Math.floor(game.currentPosition / 8)))
            audio.contextMode = game.auctionSeconds > 0 ? 1 : 0
        }
    }

    Shortcut { sequence: "Space"; enabled: root.page === "game" && game.canRoll; onActivated: game.rollDice() }
    Shortcut { sequence: "Ctrl+S"; enabled: root.page === "game"; onActivated: game.saveGame() }
    Shortcut { sequence: "F"; enabled: root.page === "game"; onActivated: board.focusCurrentPlayer() }
    Shortcut { sequence: "Left"; enabled: root.page === "game"; onActivated: board.panBy(80, 0) }
    Shortcut { sequence: "Right"; enabled: root.page === "game"; onActivated: board.panBy(-80, 0) }
    Shortcut { sequence: "Up"; enabled: root.page === "game"; onActivated: board.panBy(0, 80) }
    Shortcut { sequence: "Down"; enabled: root.page === "game"; onActivated: board.panBy(0, -80) }
}
