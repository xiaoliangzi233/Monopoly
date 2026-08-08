import QtQuick

Rectangle {
    id: control
    property string text: ""
    property string iconText: ""
    property color accent: "#D6A84B"
    property bool compact: false
    signal clicked()

    implicitWidth: compact ? 48 : Math.max(132, label.implicitWidth + 42)
    implicitHeight: compact ? 42 : 46
    radius: 10
    color: !enabled ? "#18302F" : mouse.pressed ? "#9C2F32" : mouse.containsMouse ? "#244E4F" : "#163B3D"
    border.width: mouse.containsMouse ? 2 : 1
    border.color: enabled ? accent : "#536566"
    opacity: enabled ? 1 : .5

    Rectangle {
        width: 3; height: parent.height - 16; radius: 2
        anchors.left: parent.left; anchors.leftMargin: 7; anchors.verticalCenter: parent.verticalCenter
        color: control.accent; opacity: control.enabled ? .9 : .3
    }
    Text {
        id: label
        anchors.centerIn: parent
        text: (control.iconText.length ? control.iconText + "  " : "") + control.text
        color: "#F5F2E8"
        font.family: "Microsoft YaHei UI"
        font.pixelSize: control.compact ? 13 : 15
        font.weight: Font.DemiBold
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        enabled: control.enabled
        onClicked: control.clicked()
    }
    Behavior on color { ColorAnimation { duration: 120 } }
}
