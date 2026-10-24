import QtQuick
import QtQuick.Layouts
import Artilody.Backend

// Renames a workout file (the workout name and the file name).
AppDialog {
    id: dlg
    property string path
    title: "Rename workout"
    panelWidth: 560
    panelHeight: 270

    function openFor(p, name) {
        path = p
        field.text = name
        open()
        field.forceActiveFocus()
        field.selectAll()
    }
    function accept() {
        if (Library.renameFile(path, field.text))
            close()
    }

    AppTextField {
        id: field
        x: 28
        y: 88
        width: dlg.panelWidth - 56
        placeholderText: "Workout name"
        onAccepted: dlg.accept()
        Keys.onEscapePressed: dlg.close()
    }
    Text {
        x: 28
        y: 148
        width: dlg.panelWidth - 56
        text: "The file is renamed to match."
        color: Theme.textMuted
        font.pixelSize: 14
    }
    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24
        spacing: 12
        Item { Layout.fillWidth: true }
        AppButton { text: "Cancel"; kind: "secondary"; Layout.preferredWidth: 120; onClicked: dlg.close() }
        AppButton { text: "Rename"; Layout.preferredWidth: 130; enabled: field.text.trim().length > 0; onClicked: dlg.accept() }
    }
}
