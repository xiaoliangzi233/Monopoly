import QtQuick

Rectangle {
    id: control
    property var options: [90, 120, 150]
    property var labels: []
    property string suffix: " 回合"
    property int value: 120
    implicitWidth: 420
    implicitHeight: 52
    radius: 12
    color: "#122B2E"
    border.color: "#426365"

    Row {
        anchors.fill: parent; anchors.margins: 4; spacing: 4
        Repeater {
            model: control.options
            Rectangle {
                required property var modelData
                width: (control.width - 16) / control.options.length
                height: parent.height
                radius: 9
                color: control.value === modelData ? "#A9363C" : choiceMouse.containsMouse ? "#245053" : "transparent"
                border.color: control.value === modelData ? "#E1B658" : "transparent"
                Text { anchors.centerIn: parent; text: control.labels.length > index ? control.labels[index] : modelData + control.suffix; color: "#F5F0E5"; font.pixelSize: 14; font.bold: control.value === modelData }
                MouseArea { id: choiceMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: control.value = modelData }
                Behavior on color { ColorAnimation { duration: 120 } }
            }
        }
    }
}
