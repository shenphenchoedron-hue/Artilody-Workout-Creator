import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import Artilody.Backend

// Artilody Workout Creator: file library (left) + the Workout Builder page
// shared with Artilody Watts Trainer (right).
ApplicationWindow {
    id: window

    // Design size; smaller windows scale the whole UI uniformly (as the main app).
    readonly property real designWidth: 1700
    readonly property real designHeight: 992
    readonly property real uiScale: Math.min(1, Math.min(width / designWidth, height / designHeight))

    width: Math.min(designWidth, Screen.desktopAvailableWidth)
    height: Math.min(designHeight, Screen.desktopAvailableHeight)
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    title: (Builder.dirty ? "• " : "") + (Builder.name.length > 0 ? Builder.name : "Untitled")
           + (Library.currentFileName.length > 0 ? " — " + Library.currentFileName : "")
           + " — Artilody Workout Creator"
    color: Theme.bgTop

    Binding { target: Theme; property: "dark"; value: Settings.darkTheme }

    readonly property bool anyDialogOpen: exportDialog.visible || settingsDialog.visible || unsavedDialog.visible
                                          || deleteDialog.visible || renameDialog.visible || helpDialog.visible

    function showToast(text) {
        if (text && text.length > 0)
            toast.show(text)
    }

    // --- unsaved changes ------------------------------------------------------
    property var pendingAction: null
    property bool forceClose: false

    // Runs action now, or after asking what to do with unsaved changes.
    function guard(action) {
        if (!Builder.dirty) {
            action()
            return
        }
        pendingAction = action
        unsavedDialog.open()
    }

    function saveCurrent() {
        var id = Builder.save()
        if (id >= 0) {
            showToast("Saved \"" + Builder.name + "\" to " + Library.currentFileName)
            return true
        }
        showToast(Builder.lastError)
        return false
    }

    function newWorkout(sport) { guard(function() { Library.newWorkout(sport) }) }
    function openPath(path) { guard(function() { Library.openFile(path) }) }
    function requestOpen(path) { openPath(path) }
    function openFileDialog() { guard(function() { openDialog.open() }) }
    function saveAsDialog() {
        saveDialog.currentFolder = Library.folderUrl()
        saveDialog.selectedFile = Library.suggestedSaveUrl("awt")
        saveDialog.open()
    }
    function exportFile(kind) {
        exportFileDialog.kind = kind
        exportFileDialog.selectedFile = Library.suggestedExportUrl(kind)
        exportFileDialog.open()
    }
    function changeFolder() {
        folderDialog.currentFolder = Library.folderUrl()
        folderDialog.open()
    }
    function confirmDelete(path) {
        deleteDialog.path = path
        deleteDialog.message = "Move \"" + Library.nameOf(path) + "\" to the trash?"
                               + (path === Library.currentPath && Builder.dirty ? " Your unsaved changes are lost." : "")
        deleteDialog.open()
    }
    function rename(path) {
        renameDialog.openFor(path, Library.nameOf(path))
    }

    onClosing: (close) => {
        if (Builder.dirty && !forceClose) {
            close.accepted = false
            guard(function() { forceClose = true; window.close() })
        }
    }

    Connections {
        target: Library
        function onMessage(text) { window.showToast(text) }
    }

    // --- keyboard shortcuts ------------------------------------------------------
    Shortcut { sequences: [StandardKey.New]; enabled: !window.anyDialogOpen; onActivated: window.newWorkout("cycling") }
    Shortcut { sequence: "Ctrl+Shift+N"; enabled: !window.anyDialogOpen; onActivated: window.newWorkout("rowing") }
    Shortcut { sequences: [StandardKey.Open]; enabled: !window.anyDialogOpen; onActivated: window.openFileDialog() }
    Shortcut { sequences: [StandardKey.Save]; enabled: !window.anyDialogOpen; onActivated: window.saveCurrent() }
    Shortcut { sequence: "Ctrl+Shift+S"; enabled: !window.anyDialogOpen; onActivated: window.saveAsDialog() }
    Shortcut { sequence: "Ctrl+E"; enabled: !window.anyDialogOpen; onActivated: exportDialog.open() }
    Shortcut { sequence: "Ctrl+I"; enabled: !window.anyDialogOpen; onActivated: importDialog.open() }
    Shortcut { sequence: "Ctrl+F"; enabled: !window.anyDialogOpen; onActivated: library.focusSearch() }
    Shortcut { sequence: "Ctrl+,"; enabled: !window.anyDialogOpen; onActivated: settingsDialog.open() }

    // --- screenshot mode (WATT_SCREENSHOT_DIR, see main.cpp) ------------------------
    property var screenshotMenu: null
    function prepareScreenshot(name) {
        if (screenshotMenu)
            screenshotMenu.close()
        screenshotMenu = null
        exportDialog.close()
        settingsDialog.close()
        Settings.darkTheme = name !== "light"
        var target = name === "rowing" ? "Rowing Pyramid 1-2-3-2-1" : "Sweet Spot 3 × 12"
        var path = Library.findByName(target)
        if (path.length > 0 && Library.currentPath !== path)
            Library.openFile(path)
        Builder.selectedIndex = name === "rowing" ? 3 : 1
        switch (name) {
        case "export": exportDialog.open(); break
        case "settings": settingsDialog.open(); break
        case "menu-file": screenshotMenu = library.openScreenshotMenu(); break
        case "menu-block": screenshotMenu = builderPage.openScreenshotMenu(); break
        case "menu-filemenu": screenshotMenu = topBar.openScreenshotMenu(); break
        }
        if (screenshotMenu)
            screenshotMenu.currentIndex = 0
    }

    // --- UI ---------------------------------------------------------------------
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.bgTop }
            GradientStop { position: 1; color: Theme.bgBottom }
        }
    }

    Item {
        id: root
        width: window.width / window.uiScale
        height: window.height / window.uiScale
        scale: window.uiScale
        transformOrigin: Item.TopLeft

        LibraryPanel {
            id: library
            width: 312
            height: parent.height
            onNewRequested: (sport) => window.newWorkout(sport)
            onOpenRequested: (path) => window.openPath(path)
            onOpenFileRequested: window.openFileDialog()
            onImportRequested: importDialog.open()
            onChangeFolderRequested: window.changeFolder()
            onDeleteRequested: (path) => window.confirmDelete(path)
            onRenameRequested: (path) => window.rename(path)
            onExportRequested: (path) => window.guard(function() {
                if (Library.currentPath !== path)
                    Library.openFile(path)
                exportDialog.open()
            })
        }

        Item {
            id: content
            anchors.left: library.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            BuilderPage {
                id: builderPage
                anchors.fill: parent
                showExport: true
                pageSubtitle: {
                    var sport = Builder.sport === "rowing" ? "Rowing workout" : "Cycling workout"
                    if (Library.currentPath.length === 0)
                        return sport + "  ·  not saved yet"
                    var f = Library.currentFileName
                    if (f.length > 44)
                        f = f.substring(0, 41) + "…"
                    return sport + "  ·  " + f + (Builder.dirty ? "  ·  edited" : "")
                }
                onMessage: (text) => window.showToast(text)
                onExportRequested: exportDialog.open()
            }

            CreatorTopBar {
                id: topBar
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.rightMargin: 24
                anchors.topMargin: 2
                onNewRequested: (sport) => window.newWorkout(sport)
                onOpenRequested: window.openFileDialog()
                onSaveRequested: window.saveCurrent()
                onSaveAsRequested: window.saveAsDialog()
                onExportRequested: exportDialog.open()
                onImportRequested: importDialog.open()
                onSettingsRequested: settingsDialog.open()
                onHelpRequested: helpDialog.open()
            }
        }

        // --- dialogs (inside the scaled root) -----------------------------------
        ExportDialog {
            id: exportDialog
            onExportRequested: (kind) => window.exportFile(kind)
            onHelpRequested: { exportDialog.close(); helpDialog.open() }
        }
        CreatorSettingsDialog {
            id: settingsDialog
            onChangeFolderRequested: window.changeFolder()
        }
        HelpDialog { id: helpDialog }
        RenameDialog { id: renameDialog }

        ConfirmDialog {
            id: unsavedDialog
            title: "Unsaved changes"
            message: "Save the changes to \"" + Builder.name + "\" first? Otherwise they are lost."
            acceptText: "Save"
            secondaryText: "Don't Save"
            onAccepted: {
                var action = window.pendingAction
                window.pendingAction = null
                if (window.saveCurrent() && action)
                    action()
            }
            onSecondary: {
                var action = window.pendingAction
                window.pendingAction = null
                if (action)
                    action()
            }
        }
        ConfirmDialog {
            id: deleteDialog
            property string path
            title: "Delete workout?"
            acceptText: "Move to Trash"
            acceptKind: "danger"
            onAccepted: Library.removeFile(path)
        }

        Toast {
            id: toast
            anchors.horizontalCenter: content.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40
        }
    }

    // --- native file dialogs ---------------------------------------------------------
    FileDialog {
        id: openDialog
        title: "Open workout"
        fileMode: FileDialog.OpenFile
        currentFolder: Library.folderUrl()
        nameFilters: ["Workout files (*.awt *.zwo)", "Artilody workouts (*.awt)", "Zwift workouts (*.zwo)", "All files (*)"]
        onAccepted: Library.openFile(selectedFile)
    }
    FileDialog {
        id: importDialog
        title: "Import workouts into your Workouts folder"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Workout files (*.awt *.zwo *.json *.xml)", "All files (*)"]
        onAccepted: Library.importFiles(selectedFiles)
    }
    FileDialog {
        id: saveDialog
        title: "Save workout as"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "awt"
        nameFilters: ["Artilody workout (*.awt)"]
        onAccepted: if (Library.saveAs(selectedFile)) window.showToast("Saved as " + Library.currentFileName)
                    else window.showToast(Builder.lastError.length > 0 ? Builder.lastError : Library.lastError)
    }
    FileDialog {
        id: exportFileDialog
        property string kind: "zwo"
        title: kind === "zwo" ? "Export for Zwift (.zwo)" : "Export Artilody workout (.awt)"
        fileMode: FileDialog.SaveFile
        defaultSuffix: kind
        nameFilters: kind === "zwo" ? ["Zwift workout (*.zwo)"] : ["Artilody workout (*.awt)"]
        onAccepted: {
            if (kind === "zwo")
                Library.exportZwo(selectedFile)
            else
                Library.exportAwt(selectedFile)
            exportDialog.close()
        }
    }
    FolderDialog {
        id: folderDialog
        title: "Choose your Workouts folder"
        onAccepted: Settings.workoutsFolder = Library.toLocalPath(selectedFolder)
    }
}
