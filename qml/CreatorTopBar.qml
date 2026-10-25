import QtQuick
import QtQuick.Controls
import Artilody.Backend

// Top-right bar (same place and look as the main app's TopBar): file menu,
// theme toggle, help and settings.
Item {
    id: bar
    signal newRequested(string sport)
    signal openRequested()
    signal saveRequested()
    signal saveAsRequested()
    signal exportRequested()
    signal exportAllRequested()
    signal importRequested()
    signal settingsRequested()
    signal helpRequested()

    readonly property string mod: Qt.platform.os === "osx" ? "⌘" : "Ctrl+"
    readonly property string modShift: Qt.platform.os === "osx" ? "⇧⌘" : "Ctrl+Shift+"

    width: row.width
    height: 60

    function openScreenshotMenu() {
        fileMenu.popup(fileButton, 0, fileButton.height + 4)
        return fileMenu
    }

    Row {
        id: row
        anchors.verticalCenter: parent.verticalCenter
        spacing: 18

        Rectangle {
            id: fileButton
            anchors.verticalCenter: parent.verticalCenter
            width: fileRow.implicitWidth + 24
            height: 42
            radius: 10
            color: fileMouse.containsMouse || fileMenu.visible ? Theme.hover : "transparent"
            Row {
                id: fileRow
                anchors.centerIn: parent
                spacing: 8
                Icon { name: "document"; size: 22; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "File"; color: Theme.text; font.pixelSize: 18; anchors.verticalCenter: parent.verticalCenter }
                Icon { name: "chevronDown"; size: 18; color: Theme.textSecondary; anchors.verticalCenter: parent.verticalCenter }
            }
            MouseArea {
                id: fileMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: bar.openScreenshotMenu()
            }
            AppMenu {
                id: fileMenu
                minWidth: 290
                AppMenuItem { text: "New cycling workout"; iconName: "bike"; hint: bar.mod + "N"; onTriggered: bar.newRequested("cycling") }
                AppMenuItem { text: "New rowing workout"; iconName: "rowing"; hint: bar.modShift + "N"; onTriggered: bar.newRequested("rowing") }
                AppMenuItem { text: "Open…"; iconName: "folder"; hint: bar.mod + "O"; onTriggered: bar.openRequested() }
                AppMenuSeparator {}
                AppMenuItem { text: "Save"; iconName: "save"; hint: bar.mod + "S"; onTriggered: bar.saveRequested() }
                AppMenuItem { text: "Save As…"; iconName: "copy"; hint: bar.modShift + "S"; onTriggered: bar.saveAsRequested() }
                AppMenuSeparator {}
                AppMenuItem { text: "Import .zwo / .awt…"; iconName: "import"; hint: bar.mod + "I"; onTriggered: bar.importRequested() }
                AppMenuItem { text: "Export…"; iconName: "export"; hint: bar.mod + "E"; onTriggered: bar.exportRequested() }
                AppMenuItem { text: "Export all workouts…"; iconName: "share"; hint: bar.modShift + "E"; enabled: Library.exportAllCount > 0; onTriggered: bar.exportAllRequested() }
                AppMenuItem {
                    text: Qt.platform.os === "osx" ? "Show in Finder" : (Qt.platform.os === "windows" ? "Show in Explorer" : "Show in folder")
                    iconName: "search"
                    enabled: Library.currentPath.length > 0
                    onTriggered: Library.reveal(Library.currentPath)
                }
            }
        }
        Rectangle {
            width: 2; height: 26; radius: 1
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.textMuted
        }
        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            iconName: Theme.dark ? "sun" : "moon"
            iconSize: 26
            flat: true
            tooltip: Theme.dark ? "Light mode" : "Dark mode"
            onClicked: Settings.darkTheme = !Settings.darkTheme
        }
        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            iconName: "info"
            iconSize: 26
            flat: true
            tooltip: "How to use your workout files"
            onClicked: bar.helpRequested()
        }
        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            iconName: "settings"
            iconSize: 26
            flat: true
            tooltip: "Settings (FTP, theme, folder)"
            onClicked: bar.settingsRequested()
        }
    }
}
