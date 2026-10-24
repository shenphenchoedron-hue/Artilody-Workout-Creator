import QtQuick

// "How to use the files" – shown in the export, help and settings dialogs.
Column {
    id: note
    property real fontSize: 15
    spacing: 14

    readonly property var steps: [
        { icon: "bike", tint: Theme.accentLight, title: "Artilody Watts Trainer",
          text: "Copy the .awt (or .zwo) file to the training computer – e.g. with a USB stick or a cloud folder – "
                + "then open Workouts → Import and choose the file. Watts follow that computer's FTP." },
        { icon: "export", tint: Theme.orange, title: "Zwift",
          text: "Copy the .zwo file into Documents/Zwift/Workouts/<your Zwift ID>/ and restart Zwift. "
                + "It shows up under Workouts → Custom Workouts. Zwift has no rowing: rowing workouts are exported as bike workouts." }
    ]

    Repeater {
        model: note.steps
        Row {
            width: note.width
            spacing: 14
            Rectangle {
                width: 38
                height: 38
                radius: 10
                color: Qt.rgba(modelData.tint.r, modelData.tint.g, modelData.tint.b, 0.16)
                Icon {
                    anchors.centerIn: parent
                    name: modelData.icon
                    size: 22
                    strokeWidth: 2
                    color: modelData.tint
                }
            }
            Column {
                width: parent.width - 52
                spacing: 3
                Text {
                    text: modelData.title
                    color: Theme.text
                    font.pixelSize: note.fontSize + 1
                    font.weight: Font.DemiBold
                }
                Text {
                    width: parent.width
                    text: modelData.text
                    color: Theme.textSecondary
                    font.pixelSize: note.fontSize
                    wrapMode: Text.WordWrap
                    lineHeight: 1.12
                }
            }
        }
    }
}
