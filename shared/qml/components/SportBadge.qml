import QtQuick

// Small pill that tells cycling and rowing apart: sport icon + optional label.
//   SportBadge { sport: "rowing" }                 -> [oar] ROWING
//   SportBadge { sport: "cycling"; compact: true } -> [bike] (icon only)
Rectangle {
    id: badge
    property string sport: "cycling"
    property bool compact: false
    property real iconSize: 16
    property real fontSize: 12
    readonly property bool rowing: sport === "rowing"
    readonly property color sportColor: Theme.sportColor(sport)

    implicitWidth: compact ? implicitHeight : row.implicitWidth + 18
    implicitHeight: Math.max(24, iconSize + 8)
    radius: height / 2
    color: Qt.rgba(sportColor.r, sportColor.g, sportColor.b, 0.18)
    border.width: 1
    border.color: Qt.rgba(sportColor.r, sportColor.g, sportColor.b, 0.7)

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 5
        Icon {
            name: badge.rowing ? "rowing" : "bike"
            size: badge.iconSize
            color: badge.sportColor
            strokeWidth: 2
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            visible: !badge.compact
            text: badge.rowing ? "ROWING" : "CYCLING"
            color: Theme.text
            font.pixelSize: badge.fontSize
            font.weight: Font.DemiBold
            font.letterSpacing: 0.5
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
