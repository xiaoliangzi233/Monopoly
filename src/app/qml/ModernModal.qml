import QtQuick

Item {
    id: modal
    property string title: ""
    property string eyebrow: "盛世百业 · SHENGSHI BAIYE"
    property real panelWidth: 560
    property real panelHeight: Math.min(parent ? parent.height - 80 : 720, contentColumn.implicitHeight + 112)
    property bool dismissOnBackdrop: true
    default property alias content: contentColumn.data
    signal closed()

    anchors.fill: parent
    visible: false
    opacity: 0
    z: 1000

    function open() {
        visible = true
        opacity = 1
        panel.scale = 1
    }
    function close() {
        opacity = 0
        panel.scale = .96
        hideTimer.restart()
        closed()
    }

    Rectangle { anchors.fill: parent; color: "#B00A181B" }
    MouseArea { anchors.fill: parent; enabled: modal.dismissOnBackdrop; onClicked: modal.close() }

    Rectangle {
        id: panel
        anchors.centerIn: parent
        width: Math.min(modal.panelWidth, modal.width - 40)
        height: Math.min(modal.panelHeight, modal.height - 40)
        radius: 24
        color: "#F21B3436"
        border.width: 1
        border.color: "#BFD6A84B"
        scale: .96
        clip: true
        Behavior on scale { NumberAnimation { duration: 210; easing.type: Easing.OutBack } }

        Rectangle { anchors.fill: parent; anchors.margins: 7; radius: 18; color: "transparent"; border.color: "#365F6260" }
        Rectangle { width: 92; height: 3; radius: 2; color: "#C83A3F"; anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 24 }
        MouseArea { anchors.fill: parent }

        Column {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 28
            spacing: 15
            Text { text: modal.eyebrow; color: "#82AAA9"; font.pixelSize: 10; font.letterSpacing: 2 }
            Text { width: parent.width; text: modal.title; color: "#F8F3E8"; font.pixelSize: 26; font.weight: Font.DemiBold; wrapMode: Text.Wrap }
            Rectangle { width: parent.width; height: 1; color: "#47676A" }
        }
    }

    Timer { id: hideTimer; interval: 170; onTriggered: modal.visible = false }
    Behavior on opacity { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }
}
