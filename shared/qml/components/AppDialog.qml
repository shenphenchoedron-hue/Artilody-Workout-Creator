import QtQuick

// Modal dialog drawn inside the (scaled) root item so it scales with the UI.
Item {
    id: dialog
    property string title
    property real panelWidth: 540
    property real panelHeight: 400
    property bool closable: true
    default property alias content: body.data
    signal closed()

    anchors.fill: parent
    visible: false
    z: 1000

    function open() {
        visible = true
        panel.scale = 0.96
        panel.opacity = 0
        openAnim.start()
        forceActiveFocus()
    }
    function close() {
        visible = false
        closed()
    }

    Keys.onEscapePressed: if (closable) close()

    Rectangle {
        anchors.fill: parent
        color: Theme.dim
        MouseArea {
            anchors.fill: parent
            onClicked: if (dialog.closable) dialog.close()
            onWheel: (wheel) => wheel.accepted = true
        }
    }

    Rectangle {
        id: panel
        width: dialog.panelWidth
        height: dialog.panelHeight
        anchors.centerIn: parent
        radius: 16
        color: Theme.dark ? "#25378a" : "#ffffff"
        border.color: Theme.cardBorder
        border.width: 1

        MouseArea { anchors.fill: parent }   // swallow clicks

        Text {
            visible: dialog.title.length > 0
            x: 28
            y: 24
            text: dialog.title
            color: Theme.text
            font.pixelSize: 30
            font.weight: Font.Bold
        }
        IconButton {
            visible: dialog.closable
            anchors.right: parent.right
            anchors.rightMargin: 18
            y: 18
            flat: true
            iconName: "close"
            iconSize: 24
            onClicked: dialog.close()
        }
        Item {
            id: body
            anchors.fill: parent
        }
    }

    ParallelAnimation {
        id: openAnim
        NumberAnimation { target: panel; property: "scale"; to: 1; duration: 160; easing.type: Easing.OutCubic }
        NumberAnimation { target: panel; property: "opacity"; to: 1; duration: 160 }
    }
}
