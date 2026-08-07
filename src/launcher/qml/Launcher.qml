import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 760
    height: 470
    minimumWidth: 700
    minimumHeight: 430
    visible: true
    title: "霓城大亨 · 启动器"
    color: "#070B16"

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#14264B" }
            GradientStop { position: 0.55; color: "#0A1226" }
            GradientStop { position: 1; color: "#050712" }
        }
    }
    Canvas {
        anchors.fill: parent
        opacity: 0.42
        onPaint: {
            var c = getContext("2d")
            c.clearRect(0, 0, width, height)
            for (var i = 0; i < 16; ++i) {
                var x = i * width / 15
                var h = 45 + ((i * 53) % 170)
                c.fillStyle = i % 2 ? "#143456" : "#2A1746"
                c.fillRect(x - 12, height - h, 24, h)
                c.fillStyle = i % 3 ? "#3DDDE8" : "#F052B0"
                c.fillRect(x - 9, height - h + 15, 18, 2)
            }
        }
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 44
        spacing: 13
        Item { Layout.fillHeight: true }
        Label { text: "霓城大亨"; color: "#F2FCFF"; font.pixelSize: 46; font.weight: Font.Bold }
        Label { text: "NEON TYCOON"; color: "#5DF3FF"; font.pixelSize: 15; font.letterSpacing: 5 }
        Item { height: 20 }
        Label { text: updater.status; color: "#AFC5DE"; font.pixelSize: 14 }
        ProgressBar { visible: updater.busy; value: updater.progress; indeterminate: updater.progress <= 0; Layout.fillWidth: true }
        RowLayout {
            spacing: 10
            Button { text: "检查更新"; enabled: !updater.busy; onClicked: updater.checkForUpdates(channel.currentText === "测试版" ? "beta" : "stable") }
            ComboBox { id: channel; model: ["稳定版", "测试版"] }
            Item { Layout.fillWidth: true }
            Button { visible: updater.updateAvailable && !updater.packageReady; text: "下载 " + updater.latestVersion; enabled: !updater.busy; onClicked: updater.downloadUpdate() }
            Button { visible: updater.packageReady; text: "安装已验证更新"; onClicked: updater.installVerifiedUpdate() }
            Button { text: "进入霓城"; highlighted: true; enabled: !updater.busy; onClicked: updater.launchGame() }
        }
        Item { Layout.fillHeight: true }
        Label { text: "双源校验 · Ed25519 签名 · 失败不覆盖当前版本"; color: "#607997"; font.pixelSize: 11 }
    }
}
