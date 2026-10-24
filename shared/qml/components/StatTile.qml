import QtQuick

// Small statistic tile (icon + caption + big value)
Rectangle {
    id: tile
    property string iconName
    property color iconColor: Theme.text
    property real iconSize: 26
    property string caption
    property string value
    property real valueSize: 24
    property bool large: false     // History top tiles

    radius: 12
    color: large ? Theme.card : Theme.cardStrong
    border.width: 1
    border.color: Theme.cardBorder

    Row {
        anchors.verticalCenter: parent.verticalCenter
        x: tile.large ? 26 : 16
        spacing: tile.large ? 26 : 12
        Icon {
            name: tile.iconName
            color: tile.iconColor
            size: tile.large ? 54 : tile.iconSize
            strokeWidth: tile.large ? 2.4 : 2
            anchors.verticalCenter: tile.large ? parent.verticalCenter : undefined
            y: tile.large ? 0 : 4
        }
        Column {
            id: textCol
            // room left of the icon; long values shrink instead of overflowing the tile
            readonly property real maxWidth: Math.max(20, tile.width - parent.x - parent.spacing
                                                          - (tile.large ? 54 : tile.iconSize) - 10)
            spacing: tile.large ? 2 : 6
            Text {
                width: textCol.maxWidth
                text: tile.caption
                color: Theme.textSecondary
                font.pixelSize: tile.large ? 17 : 15
                elide: Text.ElideRight
            }
            Text {
                // fixed width (not implicitWidth-based: with HorizontalFit that is a
                // binding loop); the font shrinks only when the value does not fit
                width: textCol.maxWidth
                text: tile.value
                color: Theme.text
                font.pixelSize: tile.large ? 36 : tile.valueSize
                font.weight: Font.Bold
                fontSizeMode: Text.HorizontalFit
                minimumPixelSize: 12
            }
        }
    }
}
