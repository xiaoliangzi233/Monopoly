import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 900; height: 560
    minimumWidth: 780; minimumHeight: 500
    visible: true
    title: "盛世百业 · 启动器"
    color: "#09191D"
    property string channel: "stable"

    component LauncherButton: Rectangle {
        id: button
        property string text: ""
        property color accent: "#D6AE56"
        property bool buttonEnabled: true
        signal clicked()
        implicitHeight: 48; radius: 11
        color: !buttonEnabled ? "#173034" : buttonMouse.pressed ? "#A8343B" : buttonMouse.containsMouse ? "#285054" : "#183B3F"
        border.color: buttonEnabled ? accent : "#526466"; opacity: buttonEnabled ? 1 : .5
        Rectangle { width: 3; height: parent.height - 14; radius: 2; anchors.left: parent.left; anchors.leftMargin: 7; anchors.verticalCenter: parent.verticalCenter; color: button.accent }
        Text { anchors.centerIn: parent; text: button.text; color: "#F5F1E8"; font.pixelSize: 14; font.bold: true }
        MouseArea { id: buttonMouse; anchors.fill: parent; enabled: button.buttonEnabled; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: button.clicked() }
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient { GradientStop { position: 0; color: "#123A40" } GradientStop { position: .55; color: "#0A252B" } GradientStop { position: 1; color: "#07171B" } }
    }
    Repeater {
        model: 10
        Item {
            required property int index
            width: 80 + index % 3 * 36; height: 160 + index % 4 * 54
            x: index * root.width / 9 - 35; anchors.bottom: parent.bottom; opacity: .4
            Rectangle { anchors.fill: parent; color: index % 2 ? "#214951" : "#193C47"; border.color: "#477175" }
            Rectangle { width: parent.width - 16; height: 4; x: 8; y: 25; color: index % 3 ? "#C6A652" : "#B83A42" }
            Repeater { model: 4; Rectangle { required property int index; width: parent.width - 20; height: 2; x: 10; y: 55 + index * 34; color: "#87B4B0"; opacity: .5 } }
        }
    }

    RowLayout {
        anchors.fill: parent; anchors.margins: 46; spacing: 42
        ColumnLayout {
            Layout.preferredWidth: 350; Layout.fillHeight: true; spacing: 10
            Item { Layout.fillHeight: true }
            Text { text: "城市产业主理人"; color: "#78AAA8"; font.pixelSize: 12; font.letterSpacing: 3 }
            Text { text: "盛世百业"; color: "#F7F1E7"; font.pixelSize: 51; font.bold: true; font.letterSpacing: 4 }
            Rectangle { Layout.preferredWidth: 275; height: 4; radius: 2; color: "#C13A42" }
            Text { text: "检查更新 · 校验签名 · 安全启动"; color: "#B8CECA"; font.pixelSize: 14; font.letterSpacing: 2 }
            Item { Layout.fillHeight: true }
            Text { text: "GitHub / Gitee 双源 · SHA-256 · Ed25519"; color: "#779795"; font.pixelSize: 10 }
        }

        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 390
            radius: 22; color: "#F01A3438"; border.color: "#A8D2AC58"
            Rectangle { anchors.fill: parent; anchors.margins: 7; radius: 17; color: "transparent"; border.color: "#46676A" }
            Rectangle { width: 90; height: 3; radius: 2; anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 23; color: "#C13A42" }
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 27; spacing: 13
                Text { text: "游戏与更新"; color: "#F4EEE4"; font.pixelSize: 25; font.bold: true }
                Text { text: updater.status; color: "#B4CBC7"; font.pixelSize: 13; wrapMode: Text.Wrap; Layout.fillWidth: true }
                Rectangle {
                    visible: updater.busy; Layout.fillWidth: true; height: 8; radius: 4; color: "#10282C"
                    Rectangle { width: parent.width * Math.max(.08, updater.progress); height: parent.height; radius: 4; color: "#D7AF56" }
                }
                Text { text: "更新频道"; color: "#D8E2DF"; font.pixelSize: 12 }
                Row {
                    Layout.fillWidth: true; spacing: 8
                    Repeater {
                        model: [{ key: "stable", label: "稳定版" }, { key: "beta", label: "测试版" }]
                        Rectangle {
                            required property var modelData
                            width: 118; height: 40; radius: 10
                            color: root.channel === modelData.key ? "#A9363D" : channelMouse.containsMouse ? "#285054" : "#153438"
                            border.color: root.channel === modelData.key ? "#D9B159" : "#4D6D70"
                            Text { anchors.centerIn: parent; text: modelData.label; color: "#F4F0E8"; font.bold: root.channel === modelData.key }
                            MouseArea { id: channelMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.channel = modelData.key }
                        }
                    }
                }
                LauncherButton { Layout.fillWidth: true; text: "检查更新"; buttonEnabled: !updater.busy; onClicked: updater.checkForUpdates(root.channel) }
                Item { Layout.fillHeight: true }
                LauncherButton { visible: updater.updateAvailable && !updater.packageReady; Layout.fillWidth: true; text: "下载更新 " + updater.latestVersion; buttonEnabled: !updater.busy; onClicked: updater.downloadUpdate() }
                LauncherButton { visible: updater.packageReady; Layout.fillWidth: true; text: "安装已验证更新"; accent: "#C13A42"; onClicked: updater.installVerifiedUpdate() }
                LauncherButton { Layout.fillWidth: true; text: "启动游戏"; accent: "#D6AE56"; buttonEnabled: !updater.busy; onClicked: updater.launchGame() }
            }
        }
    }
}
