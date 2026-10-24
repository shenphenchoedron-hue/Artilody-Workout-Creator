import QtQuick
import QtQuick.Controls
import Artilody.Backend

// One workout file in the library list: name, sport, duration, category and
// a mini power profile. Click opens it in the Builder, right-click / ⋯ shows
// the file menu.
Rectangle {
    id: item
    signal activated(string path)
    signal menuRequested(string path, Item item, real x, real y)

    readonly property string path: model.path
    readonly property bool current: model.path === Library.currentPath
    readonly property bool rowing: model.sport === "rowing"
    readonly property var bars: model.bars
    readonly property bool hovered: hover.hovered

    HoverHandler { id: hover }

    height: 84
    radius: 10
    color: current ? Theme.navSelected : (hovered ? Theme.hover : "transparent")
    border.width: current ? 1 : 0
    border.color: Qt.rgba(0.3, 0.64, 1, 0.45)
    Behavior on color { ColorAnimation { duration: 120 } }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor
        onClicked: (m) => {
            if (m.button === Qt.RightButton)
                item.menuRequested(item.path, item, m.x, m.y)
            else if (!item.current)
                item.activated(item.path)
        }
    }

    // sport icon
    Rectangle {
        id: sportDot
        x: 10
        y: 12
        width: 36
        height: 36
        radius: 18
        readonly property color tint: Theme.sportColor(model.sport)
        color: Qt.rgba(tint.r, tint.g, tint.b, 0.18)
        border.width: 1
        border.color: Qt.rgba(tint.r, tint.g, tint.b, 0.7)
        Icon {
            anchors.centerIn: parent
            name: !model.valid ? "info" : (item.rowing ? "rowing" : "bike")
            size: 19
            strokeWidth: 2
            color: model.valid ? sportDot.tint : Theme.danger
        }
    }

    Text {
        id: title
        x: 56
        y: 10
        width: parent.width - x - (moreButton.visible ? 40 : 12)
        text: model.name
        color: item.current ? Theme.accentText : Theme.text
        font.pixelSize: 16
        font.weight: Font.DemiBold
        elide: Text.ElideRight
    }
    Row {
        x: 56
        y: 33
        spacing: 6
        Text {
            text: !model.valid ? "Cannot read file"
                  : model.durationText + "  ·  " + model.category
                    + (model.workoutCount > 1 ? "  ·  " + model.workoutCount + " workouts" : "")
            color: model.valid ? Theme.textSecondary : Theme.danger
            font.pixelSize: 13
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle {
            visible: model.format === "zwo"
            width: zwoLabel.implicitWidth + 10
            height: 17
            radius: 4
            color: "transparent"
            border.width: 1
            border.color: Theme.textMuted
            anchors.verticalCenter: parent.verticalCenter
            Text {
                id: zwoLabel
                anchors.centerIn: parent
                text: "ZWO"
                color: Theme.textSecondary
                font.pixelSize: 10
                font.weight: Font.DemiBold
                font.letterSpacing: 0.5
            }
        }
        Rectangle {
            visible: item.current && Builder.dirty
            width: 8; height: 8; radius: 4
            color: Theme.warning
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // mini power profile
    Item {
        id: mini
        x: 56
        y: 56
        width: parent.width - x - 12
        height: 18
        Repeater {
            model: item.bars
            Rectangle {
                x: modelData.s * mini.width
                width: Math.max(1, modelData.w * mini.width - 0.5)
                height: Math.max(2, modelData.h * mini.height)
                anchors.bottom: parent.bottom
                color: Theme.zoneColor(modelData.z)
                opacity: item.current || item.hovered ? 1 : 0.8
            }
        }
    }

    IconButton {
        id: moreButton
        visible: item.hovered || item.current
        anchors.right: parent.right
        anchors.rightMargin: 6
        y: 8
        flat: true
        iconName: "dots"
        iconSize: 18
        tooltip: "More"
        onClicked: item.menuRequested(item.path, moreButton, moreButton.width / 2, moreButton.height)
    }
}
