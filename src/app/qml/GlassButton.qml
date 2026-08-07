import QtQuick
import QtQuick.Controls

Button {
    id: control
    property color accent: "#52F7FF"
    implicitWidth: 126
    implicitHeight: 46
    hoverEnabled: true
    font.pixelSize: 15
    font.weight: Font.DemiBold
    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? "#F1FCFF" : "#718097"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 13
        color: control.down ? Qt.rgba(control.accent.r, control.accent.g, control.accent.b, 0.30)
                            : control.hovered ? "#263858" : "#17243D"
        border.width: control.activeFocus ? 2 : 1
        border.color: control.enabled ? Qt.rgba(control.accent.r, control.accent.g, control.accent.b, 0.72) : "#344158"
        Behavior on color { ColorAnimation { duration: 120 } }
    }
}
