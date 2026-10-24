import QtQuick

// Pill-style segmented selector.
//   SegmentedControl {
//       options: [{ value: "all", label: "All" },
//                 { value: "rowing", label: "Rowing", icon: "rowing", color: Theme.rowingColor, textColor: Theme.textOnColor }]
//       value: Workouts.sportFilter
//       onActivated: (v) => Workouts.sportFilter = v
//   }
// Keyboard: Tab to focus, Left / Right to step through the options.
Rectangle {
    id: seg
    property var options: []
    property string value
    property real fontSize: 16
    property real segmentPadding: 16
    property real iconSize: 18
    signal activated(string value)

    function indexOfValue(v) {
        for (var i = 0; i < options.length; ++i)
            if (options[i].value === v)
                return i
        return -1
    }
    function step(delta) {
        if (options.length === 0)
            return
        var i = indexOfValue(value)
        var n = i < 0 ? (delta > 0 ? 0 : options.length - 1) : Math.max(0, Math.min(options.length - 1, i + delta))
        if (n !== i)
            activated(options[n].value)
    }

    activeFocusOnTab: true
    Keys.onLeftPressed: step(-1)
    Keys.onRightPressed: step(1)
    Accessible.role: Accessible.PageTabList
    Accessible.name: { var i = indexOfValue(value); return i >= 0 ? options[i].label : "" }

    implicitWidth: segRow.implicitWidth + 8
    implicitHeight: 50
    radius: 10
    color: Theme.card
    border.color: activeFocus ? Theme.accentLight : Theme.cardBorder
    border.width: activeFocus ? 2 : 1

    Row {
        id: segRow
        anchors.centerIn: parent
        spacing: 0
        Repeater {
            model: seg.options
            Rectangle {
                readonly property bool active: seg.value === modelData.value
                readonly property color tint: modelData.color !== undefined ? modelData.color : Theme.accent
                readonly property color fg: modelData.textColor !== undefined ? modelData.textColor : "white"
                width: segContent.implicitWidth + 2 * seg.segmentPadding
                height: seg.height - 8
                radius: 8
                Accessible.role: Accessible.PageTab
                Accessible.name: modelData.label
                Accessible.selectable: true
                Accessible.selected: active
                Accessible.onPressAction: seg.activated(modelData.value)
                color: active ? tint : (segMouse.containsMouse ? Theme.hover : "transparent")
                Row {
                    id: segContent
                    anchors.centerIn: parent
                    spacing: 7
                    Icon {
                        visible: modelData.icon !== undefined
                        name: modelData.icon !== undefined ? modelData.icon : ""
                        size: seg.iconSize
                        strokeWidth: 2
                        color: parent.parent.active ? parent.parent.fg : Theme.text
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: modelData.label
                        color: parent.parent.active ? parent.parent.fg : Theme.text
                        font.pixelSize: seg.fontSize
                        font.weight: parent.parent.active ? Font.DemiBold : Font.Normal
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                MouseArea {
                    id: segMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: seg.activated(modelData.value)
                }
            }
        }
    }
}
