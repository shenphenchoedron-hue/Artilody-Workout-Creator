import QtQuick

// Pill toggle (green ERG switch / blue preference switches in the mockups).
Rectangle {
    id: toggle
    property bool checked: false
    property color onColor: Theme.success
    property bool enabled: true
    signal toggled(bool checked)

    implicitWidth: 56
    implicitHeight: 32
    radius: height / 2
    opacity: enabled ? 1 : 0.45
    color: checked ? onColor : Theme.fieldBorder
    Behavior on color { ColorAnimation { duration: 140 } }

    Rectangle {
        width: parent.height - 6
        height: width
        radius: width / 2
        y: 3
        x: toggle.checked ? toggle.width - width - 3 : 3
        color: "white"
        Behavior on x { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: toggle.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: {
            if (!toggle.enabled)
                return
            toggle.checked = !toggle.checked
            toggle.toggled(toggle.checked)
        }
    }
}
