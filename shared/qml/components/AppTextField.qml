import QtQuick
import QtQuick.Controls

TextField {
    id: field
    property string iconName
    property string trailingIcon
    signal trailingClicked()

    implicitHeight: 50
    font.pixelSize: 18
    color: Theme.text
    placeholderTextColor: Theme.textMuted
    selectByMouse: true
    leftPadding: iconName.length > 0 ? 56 : 18
    rightPadding: trailingIcon.length > 0 ? 50 : 14
    verticalAlignment: TextInput.AlignVCenter

    background: Rectangle {
        radius: 10
        color: Theme.field
        border.width: 1
        border.color: field.activeFocus ? Theme.accentLight : Theme.fieldBorder
    }

    Icon {
        visible: field.iconName.length > 0
        name: field.iconName
        size: 24
        color: Theme.textSecondary
        x: 18
        anchors.verticalCenter: parent.verticalCenter
    }
    Icon {
        visible: field.trailingIcon.length > 0
        name: field.trailingIcon
        size: 22
        color: Theme.textSecondary
        anchors.right: parent.right
        anchors.rightMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        MouseArea {
            anchors.fill: parent
            anchors.margins: -6
            cursorShape: Qt.PointingHandCursor
            onClicked: field.trailingClicked()
        }
    }
}
