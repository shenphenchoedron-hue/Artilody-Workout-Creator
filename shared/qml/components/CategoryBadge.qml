import QtQuick

Rectangle {
    property string category
    property string label: category === "Custom" ? "CUSTOM" : category.toUpperCase()

    implicitWidth: badgeText.implicitWidth + 30
    implicitHeight: 28
    radius: height / 2
    color: Theme.categoryColor(category)

    Text {
        id: badgeText
        anchors.centerIn: parent
        text: parent.label
        color: "white"
        font.pixelSize: 13
        font.weight: Font.DemiBold
        font.letterSpacing: 0.5
    }
}
