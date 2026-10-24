import QtQuick
import QtQuick.Controls

// Thin divider between groups of AppMenuItems.
MenuSeparator {
    topPadding: 5
    bottomPadding: 5
    leftPadding: 10
    rightPadding: 10
    contentItem: Rectangle {
        implicitWidth: 180
        implicitHeight: 1
        color: Theme.menuBorder
    }
}
