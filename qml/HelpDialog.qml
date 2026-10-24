import QtQuick
import QtQuick.Layouts
import Artilody.Backend

// How the Workout Creator's files get to the training computer / Zwift.
AppDialog {
    id: dlg
    title: "Using your workouts"
    panelWidth: 820
    panelHeight: 600

    Column {
        x: 28
        y: 84
        width: dlg.panelWidth - 56
        spacing: 22

        Text {
            width: parent.width
            text: "Every workout is a file in your Workouts folder. Workouts are stored as % of FTP, "
                  + "so they scale to the FTP of whoever rides them – the FTP here only previews the watts."
            color: Theme.textSecondary
            font.pixelSize: 16
            wrapMode: Text.WordWrap
            lineHeight: 1.12
        }
        UsageNote {
            width: parent.width
            fontSize: 15
        }
        Row {
            width: parent.width
            spacing: 14
            Rectangle {
                width: 38; height: 38; radius: 10
                color: Qt.rgba(0.24, 0.86, 0.36, 0.16)
                Icon { anchors.centerIn: parent; name: "import"; size: 22; strokeWidth: 2; color: Theme.success }
            }
            Column {
                width: parent.width - 52
                spacing: 3
                Text { text: "Bring workouts in"; color: Theme.text; font.pixelSize: 16; font.weight: Font.DemiBold }
                Text {
                    width: parent.width
                    text: "Import .zwo files from Zwift or .awt files exported by Artilody Watts Trainer "
                          + "(Workouts → Export) to edit them here. Files with several workouts are split into one file each."
                    color: Theme.textSecondary
                    font.pixelSize: 15
                    wrapMode: Text.WordWrap
                    lineHeight: 1.12
                }
            }
        }
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24
        Text {
            Layout.fillWidth: true
            text: "Artilody Workout Creator " + appVersion + "  ·  GPL-3.0"
            color: Theme.textMuted
            font.pixelSize: 13
        }
        AppButton {
            text: "Close"
            Layout.preferredWidth: 130
            onClicked: dlg.close()
        }
    }
}
