import QtQuick
import QtQuick.Controls

// Themed menu entry for AppMenu: leading icon (Theme.icons name), optional hint
// (shortcut text) on the right, destructive (red) flag, check mark for checkable
// items and a chevron for sub-menus.
MenuItem {
    id: item

    property string iconName
    property color iconColor: "transparent"   // default: follows the text colour
    property string hint
    property bool destructive: false

    readonly property color textColor: destructive ? Theme.danger : Theme.text
    readonly property bool hasLeading: iconName.length > 0 || checkable
    readonly property real arrowWidth: subMenu ? 18 + spacing : 0

    implicitHeight: visible ? Theme.menuItemHeight : 0
    implicitWidth: leftPadding + rightPadding
                   + (hasLeading ? 22 + spacing : 0)
                   + label.implicitWidth
                   + (hint.length > 0 ? 28 + hintLabel.implicitWidth : 0)
                   + arrowWidth
    leftPadding: 12
    rightPadding: 14
    spacing: 12
    font.pixelSize: Theme.menuFontSize
    font.weight: Font.Medium

    indicator: null   // the check mark is drawn in the leading icon slot

    arrow: Icon {
        visible: item.subMenu
        x: item.width - width - item.rightPadding
        anchors.verticalCenter: parent.verticalCenter
        name: "chevronRight"
        size: 18
        color: Theme.textSecondary
    }

    contentItem: Item {
        opacity: item.enabled ? 1 : 0.38
        Icon {
            id: leading
            visible: item.hasLeading && name.length > 0
            anchors.verticalCenter: parent.verticalCenter
            name: item.checkable ? (item.checked ? "check" : "") : item.iconName
            size: 22
            strokeWidth: item.checkable ? 2.4 : 1.9
            color: item.checkable ? Theme.accentLight
                 : item.iconColor.a > 0 ? item.iconColor
                 : item.destructive ? Theme.danger
                 : item.highlighted ? Theme.text : Theme.textSecondary
        }
        Text {
            id: label
            anchors.left: parent.left
            anchors.leftMargin: item.hasLeading ? 22 + item.spacing : 0
            anchors.right: hintLabel.visible ? hintLabel.left : parent.right
            anchors.rightMargin: (hintLabel.visible ? 16 : 0) + item.arrowWidth
            anchors.verticalCenter: parent.verticalCenter
            text: item.text
            font.pixelSize: item.font.pixelSize
            font.weight: item.checked ? Font.DemiBold : item.font.weight
            color: item.textColor
            elide: Text.ElideRight
        }
        Text {
            id: hintLabel
            visible: item.hint.length > 0
            anchors.right: parent.right
            anchors.rightMargin: item.arrowWidth
            anchors.verticalCenter: parent.verticalCenter
            text: item.hint
            font.pixelSize: item.font.pixelSize - 2
            color: Theme.textMuted
        }
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: Theme.menuItemHeight
        radius: 8
        visible: item.enabled && (item.highlighted || item.down)
        color: item.destructive
               ? Qt.rgba(0.96, 0.32, 0.23, item.down ? 0.32 : 0.18)
               : Qt.rgba(0.13, 0.47, 1.0, item.down ? 0.45 : (Theme.dark ? 0.30 : 0.16))
    }
}
