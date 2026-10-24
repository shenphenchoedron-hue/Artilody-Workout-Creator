import QtQuick

Column {
    property string title
    property string subtitle
    spacing: 2

    Text {
        text: parent.title
        color: Theme.text
        font.pixelSize: 50
        font.weight: Font.Black
    }
    Text {
        visible: text.length > 0
        text: parent.subtitle
        color: Theme.textSecondary
        font.pixelSize: 23
    }
}
