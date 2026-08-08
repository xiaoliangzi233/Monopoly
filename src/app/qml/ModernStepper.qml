import QtQuick

Rectangle {
    id: control
    property string label: ""
    property int value: 2
    property int from: 0
    property int to: 6
    property int stepSize: 1
    implicitWidth: 420
    implicitHeight: 64
    radius: 12
    color: "#142C2F"
    border.color: "#416264"

    Text { anchors.left: parent.left; anchors.leftMargin: 16; anchors.verticalCenter: parent.verticalCenter; text: control.label; color: "#D9E3DF"; font.pixelSize: 14 }
    Row {
        anchors.right: parent.right; anchors.rightMargin: 10; anchors.verticalCenter: parent.verticalCenter; spacing: 8
        ModernButton { compact: true; text: "−"; enabled: control.value > control.from; onClicked: control.value = Math.max(control.from, control.value - control.stepSize) }
        Text { width: 42; height: 42; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; text: control.value; color: "#F5D990"; font.pixelSize: 20; font.bold: true }
        ModernButton { compact: true; text: "+"; enabled: control.value < control.to; onClicked: control.value = Math.min(control.to, control.value + control.stepSize) }
    }
}
