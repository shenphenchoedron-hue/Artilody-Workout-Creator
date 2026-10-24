import QtQuick
import QtQuick.Controls
import Artilody.Backend

// Left column: the workout files of the Workouts folder (search, sport filter,
// context menu) – the creator's counterpart of the main app's sidebar.
Rectangle {
    id: panel
    signal newRequested(string sport)
    signal openRequested(string path)
    signal openFileRequested()
    signal importRequested()
    signal changeFolderRequested()
    signal deleteRequested(string path)
    signal renameRequested(string path)
    signal exportRequested(string path)

    color: Theme.sidebar

    readonly property string mod: Qt.platform.os === "osx" ? "⌘" : "Ctrl+"
    readonly property string modShift: Qt.platform.os === "osx" ? "⇧⌘" : "Ctrl+Shift+"

    function focusSearch() {
        search.forceActiveFocus()
        search.selectAll()
    }
    function openFileMenu(path, item, x, y) {
        fileMenu.path = path
        var p = item.mapToItem(panel, x, y)
        fileMenu.popup(panel, p.x, p.y)
    }
    // screenshot aid: context menu of the current (or first) file
    function openScreenshotMenu() {
        var idx = 0
        for (var i = 0; i < list.count; ++i) {
            var it = list.itemAtIndex(i)
            if (it && it.path === Library.currentPath) { idx = i; break }
        }
        var item = list.itemAtIndex(idx)
        if (!item)
            return null
        openFileMenu(item.path, item, item.width - 60, item.height / 2)
        return fileMenu
    }

    // --- header: app identity ----------------------------------------------------
    Row {
        x: 20
        y: 22
        spacing: 12
        Image {
            source: "qrc:/icons/app-icon-256.png"
            width: 44
            height: 44
            sourceSize: Qt.size(88, 88)
            smooth: true
            mipmap: true
            anchors.verticalCenter: parent.verticalCenter
        }
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0
            Text {
                text: "Workout Creator"
                color: Theme.text
                font.pixelSize: 21
                font.weight: Font.Bold
            }
            Text {
                text: "Artilody"
                color: Theme.textSecondary
                font.pixelSize: 14
                font.letterSpacing: 1.2
            }
        }
    }

    // --- actions ---------------------------------------------------------------------
    Row {
        id: actions
        x: 16
        y: 90
        width: parent.width - 32
        height: 50
        spacing: 8

        AppButton {
            id: newButton
            width: parent.width - 2 * (46 + parent.spacing)
            height: parent.height
            iconName: "plus"
            text: "New Workout"
            fontSize: 17
            iconSize: 20
            horizontalPadding: 14
            onClicked: newMenu.popup(newButton, 0, newButton.height + 6)

            AppMenu {
                id: newMenu
                minWidth: 260
                AppMenuItem { text: "Cycling workout"; iconName: "bike"; hint: panel.mod + "N"; onTriggered: panel.newRequested("cycling") }
                AppMenuItem { text: "Rowing workout"; iconName: "rowing"; hint: panel.modShift + "N"; onTriggered: panel.newRequested("rowing") }
            }
        }
        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            iconName: "folder"
            tooltip: "Open a workout file… (" + panel.mod + "O)"
            onClicked: panel.openFileRequested()
        }
        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            iconName: "import"
            tooltip: "Import .zwo / .awt files into the folder… (" + panel.mod + "I)"
            onClicked: panel.importRequested()
        }
    }

    AppTextField {
        id: search
        x: 16
        y: actions.y + actions.height + 14
        width: parent.width - 32
        height: 46
        font.pixelSize: 16
        iconName: "search"
        trailingIcon: text.length > 0 ? "close" : ""
        placeholderText: "Search workouts"
        text: Library.filterText
        onTextEdited: Library.filterText = text
        onTrailingClicked: { text = ""; Library.filterText = "" }
        Keys.onEscapePressed: { text = ""; Library.filterText = ""; focus = false }
    }

    SegmentedControl {
        id: sportFilter
        x: 16
        y: search.y + search.height + 12
        width: parent.width - 32
        height: 44
        fontSize: 15
        iconSize: 16
        segmentPadding: 12
        value: Library.sportFilter
        options: [
            { value: "all", label: "All" },
            { value: "cycling", label: "Cycling", icon: "bike" },
            { value: "rowing", label: "Rowing", icon: "rowing", color: Theme.rowingColor, textColor: Theme.textOnColor }
        ]
        onActivated: (v) => Library.sportFilter = v
    }

    Text {
        id: sectionLabel
        x: 22
        y: sportFilter.y + sportFilter.height + 18
        width: parent.width - 44
        text: (Library.count === Library.totalCount ? Library.totalCount : Library.count + " of " + Library.totalCount)
              + (Library.totalCount === 1 ? " WORKOUT" : " WORKOUTS")
        color: Theme.textMuted
        font.pixelSize: 12
        font.weight: Font.DemiBold
        font.letterSpacing: 1
    }

    // --- file list -----------------------------------------------------------------------
    ListView {
        id: list
        x: 10
        y: sectionLabel.y + 24
        width: parent.width - 20
        height: footer.y - y - 8
        clip: true
        spacing: 4
        model: Library
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; width: 6 }

        delegate: LibraryItem {
            width: list.width - 4
            onActivated: (p) => panel.openRequested(p)
            onMenuRequested: (p, item, mx, my) => panel.openFileMenu(p, item, mx, my)
        }
    }

    // empty states
    Column {
        visible: list.count === 0
        x: 24
        y: list.y + 30
        width: parent.width - 48
        spacing: 10
        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: Library.totalCount === 0 ? "folder" : "search"
            size: 40
            color: Theme.textMuted
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: Library.totalCount === 0 ? "No workouts yet" : "No matching workouts"
            color: Theme.text
            font.pixelSize: 17
            font.weight: Font.DemiBold
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: Library.totalCount === 0
                  ? "Save the workout on the right, or import .zwo / .awt files. They are stored as files in your Workouts folder."
                  : "Try another search or sport filter."
            color: Theme.textSecondary
            font.pixelSize: 14
            lineHeight: 1.15
        }
    }

    // --- footer: the workouts folder -------------------------------------------------------
    Rectangle {
        id: footer
        x: 10
        width: parent.width - 20
        height: 62
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        radius: 10
        color: folderMouse.containsMouse ? Theme.hover : "transparent"
        border.width: 1
        border.color: Theme.cardBorder

        Icon {
            id: folderIcon
            x: 12
            anchors.verticalCenter: parent.verticalCenter
            name: "folder"
            size: 24
            color: Theme.accentLight
        }
        Column {
            anchors.left: folderIcon.right
            anchors.leftMargin: 10
            anchors.right: folderMore.left
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Text {
                text: "Workouts folder"
                color: Theme.text
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }
            Text {
                width: parent.width
                text: Library.nativePath(Library.folder)
                color: Theme.textSecondary
                font.pixelSize: 12
                elide: Text.ElideMiddle
            }
        }
        Icon {
            id: folderMore
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            name: "chevronUp"
            size: 18
            color: Theme.textSecondary
        }
        MouseArea {
            id: folderMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: folderMenu.popup(footer, 0, -folderMenu.implicitHeight - 6)
        }
        ToolTip.visible: folderMouse.containsMouse
        ToolTip.delay: 700
        ToolTip.text: Library.nativePath(Library.folder)

        AppMenu {
            id: folderMenu
            minWidth: 280
            AppMenuItem {
                text: Qt.platform.os === "osx" ? "Show in Finder" : (Qt.platform.os === "windows" ? "Show in Explorer" : "Open folder")
                iconName: "folder"
                onTriggered: Library.openFolder()
            }
            AppMenuItem { text: "Change folder…"; iconName: "edit"; onTriggered: panel.changeFolderRequested() }
            AppMenuItem { text: "Import files…"; iconName: "import"; hint: panel.mod + "I"; onTriggered: panel.importRequested() }
            AppMenuSeparator {}
            AppMenuItem { text: "Refresh"; iconName: "refresh"; onTriggered: Library.refresh() }
        }
    }

    // --- context menu of a file -----------------------------------------------------------------
    AppMenu {
        id: fileMenu
        property string path
        readonly property bool isCurrent: path === Library.currentPath
        minWidth: 270
        AppMenuItem { text: "Open"; iconName: "edit"; enabled: !fileMenu.isCurrent; onTriggered: panel.openRequested(fileMenu.path) }
        AppMenuItem { text: "Export…"; iconName: "export"; hint: panel.mod + "E"; onTriggered: panel.exportRequested(fileMenu.path) }
        AppMenuSeparator {}
        AppMenuItem { text: "Duplicate"; iconName: "copy"; onTriggered: Library.duplicateFile(fileMenu.path) }
        AppMenuItem { text: "Rename…"; iconName: "document"; onTriggered: panel.renameRequested(fileMenu.path) }
        AppMenuItem {
            text: Qt.platform.os === "osx" ? "Show in Finder" : (Qt.platform.os === "windows" ? "Show in Explorer" : "Show in folder")
            iconName: "folder"
            onTriggered: Library.reveal(fileMenu.path)
        }
        AppMenuSeparator {}
        AppMenuItem { text: "Delete…"; iconName: "trash"; destructive: true; onTriggered: panel.deleteRequested(fileMenu.path) }
    }
}
