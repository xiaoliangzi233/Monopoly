import QtQuick
import QtQuick.Controls

Button {
    id: control
    property color accent: "#B83A2D"
    implicitWidth: 126
    implicitHeight: 46
    hoverEnabled: true
    font.family: "Microsoft YaHei UI"
    font.pixelSize: 15
    font.weight: Font.DemiBold
    onPressed: audio.playEffect(0)
    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? "#F7EFD8" : "#8E8877"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: 5
        color: control.down ? "#743126" : control.hovered ? "#315E54" : "#243F38"
        border.width: control.activeFocus ? 2 : 1
        border.color: control.enabled ? control.accent : "#625F53"
        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; height: 2; color: "#D0A646"; opacity: .65 }
        Behavior on color { ColorAnimation { duration: 120 } }
    }
}
