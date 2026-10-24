import QtQuick

// Rounded translucent panel used for every section in the mockups.
Rectangle {
    id: card
    property string title: ""
    property alias headerRight: headerRightSlot.data
    property int padding: 18
    property real titleSize: 19
    readonly property real contentTop: title.length > 0 ? padding + titleLabel.height + 12 : padding

    radius: Theme.radius
    color: Theme.card
    border.color: Theme.cardBorder
    border.width: 1

    Text {
        id: titleLabel
        visible: card.title.length > 0
        text: card.title
        color: Theme.text
        font.pixelSize: card.titleSize
        font.weight: Font.Bold
        x: card.padding
        y: card.padding
    }

    Item {
        id: headerRightSlot
        anchors.right: parent.right
        anchors.rightMargin: card.padding
        anchors.verticalCenter: titleLabel.verticalCenter
        width: childrenRect.width
        height: childrenRect.height
    }
}
