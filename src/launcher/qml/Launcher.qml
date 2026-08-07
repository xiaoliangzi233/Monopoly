import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 820
    height: 500
    minimumWidth: 760
    minimumHeight: 460
    visible: true
    title: "盛世百业 · 启动器"
    color: "#172A24"

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#29483D" }
            GradientStop { position: .58; color: "#182E28" }
            GradientStop { position: 1; color: "#111F1C" }
        }
    }
    Repeater {
        model: 7
        Item {
            required property int index
            width: 180; height: 160; x: index * 135 - 35
            anchors.bottom: parent.bottom
            opacity: .38
            Rectangle { width: parent.width; height: 58; y: 45; color: index % 2 ? "#4C6558" : "#405B50" }
            Rectangle { width: parent.width + 24; height: 18; x: -12; y: 35; color: index % 2 ? "#8C4A3F" : "#526D5E"; rotation: index % 2 ? -2 : 2 }
            Rectangle { width: 6; height: 80; x: 18; y: 80; color: "#725A3E" }
            Rectangle { width: 6; height: 80; x: parent.width - 24; y: 80; color: "#725A3E" }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 44
        spacing: 40
        ColumnLayout {
            Layout.preferredWidth: 350
            Layout.fillHeight: true
            spacing: 9
            Item { Layout.fillHeight: true }
            Text { text: "盛世百业"; color: "#F1E3BB"; font.family: "STKaiti"; font.pixelSize: 54; font.bold: true; font.letterSpacing: 5 }
            Rectangle { Layout.preferredWidth: 280; height: 3; color: "#D3A743" }
            Text { text: "百工商旅 · 山河共兴"; color: "#B9C7AF"; font.pixelSize: 15; font.letterSpacing: 3 }
            Item { Layout.fillHeight: true }
            Text { text: "双源校验 · Ed25519签名 · 损坏包拒绝"; color: "#82998B"; font.pixelSize: 10 }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 330
            color: "#E5233B33"
            border.color: "#A6A9834C"
            radius: 7
            Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; height: 2; color: "#D3A743" }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 13
                Text { text: "行前整备"; color: "#F0E2BD"; font.family: "STKaiti"; font.pixelSize: 25; font.bold: true }
                Text { text: updater.status; color: "#B9C8B6"; font.pixelSize: 13; wrapMode: Text.Wrap; Layout.fillWidth: true }
                ProgressBar { visible: updater.busy; value: updater.progress; indeterminate: updater.progress <= 0; Layout.fillWidth: true }
                RowLayout {
                    Layout.fillWidth: true
                    Button { text: "检查更新"; enabled: !updater.busy; onClicked: updater.checkForUpdates(channel.currentIndex === 1 ? "beta" : "stable") }
                    ComboBox { id: channel; model: ["稳定版", "测试版"] }
                }
                Item { Layout.fillHeight: true }
                Button { visible: updater.updateAvailable && !updater.packageReady; text: "下载 " + updater.latestVersion; enabled: !updater.busy; Layout.fillWidth: true; onClicked: updater.downloadUpdate() }
                Button { visible: updater.packageReady; text: "安装已验证更新"; Layout.fillWidth: true; onClicked: updater.installVerifiedUpdate() }
                Button { text: "进入盛世"; highlighted: true; enabled: !updater.busy; Layout.fillWidth: true; onClicked: updater.launchGame() }
            }
        }
    }
}
