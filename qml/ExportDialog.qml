import QtQuick
import QtQuick.Layouts
import Artilody.Backend

// Export the workout in the Builder: Zwift (.zwo) or Artilody (.awt).
AppDialog {
    id: dlg
    signal exportRequested(string kind)
    signal exportAllRequested(string kind)     // every workout in the list, one file each: zwo | awt | zwift
    signal helpRequested()

    title: "Export Workout"
    panelWidth: 900
    panelHeight: 840

    readonly property bool rowing: Builder.sport === "rowing"
    readonly property real cardWidth: (panelWidth - 56 - 18) / 2

    // the workout
    Row {
        x: 28
        y: 80
        spacing: 12
        SportBadge {
            sport: Builder.sport
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: Builder.name
            color: Theme.text
            font.pixelSize: 20
            font.weight: Font.DemiBold
            elide: Text.ElideRight
            width: Math.min(implicitWidth, dlg.panelWidth - 420)
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: Math.round(Builder.totalDurationSec / 60) + " min  ·  " + Builder.blockCount + " blocks  ·  "
                  + Builder.category
            color: Theme.textSecondary
            font.pixelSize: 16
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    component OptionCard: Rectangle {
        id: card
        property string iconName
        property color tint
        property string heading
        property string format
        property string body
        property string warning
        default property alias actions: actionColumn.data
        width: dlg.cardWidth
        height: 300
        radius: 14
        color: Theme.card
        border.width: 1
        border.color: Theme.cardBorder

        Rectangle {
            x: 20
            y: 20
            width: 52
            height: 52
            radius: 14
            color: Qt.rgba(card.tint.r, card.tint.g, card.tint.b, 0.18)
            Icon {
                anchors.centerIn: parent
                name: card.iconName
                size: 28
                strokeWidth: 2
                color: card.tint
            }
        }
        Column {
            x: 86
            y: 20
            width: parent.width - 106
            spacing: 2
            Text { text: card.heading; color: Theme.text; font.pixelSize: 21; font.weight: Font.Bold }
            Text { text: card.format; color: Theme.textSecondary; font.pixelSize: 15 }
        }
        Column {
            x: 20
            y: 90
            width: parent.width - 40
            spacing: 12
            Text {
                width: parent.width
                text: card.body
                color: Theme.textSecondary
                font.pixelSize: 15
                wrapMode: Text.WordWrap
                lineHeight: 1.12
            }
            Row {
                visible: card.warning.length > 0
                width: parent.width
                spacing: 8
                Icon { name: "info"; size: 18; strokeWidth: 2; color: Theme.warningText }
                Text {
                    width: parent.width - 26
                    text: card.warning
                    color: Theme.warningText
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                }
            }
        }
        Column {
            id: actionColumn
            x: 20
            width: parent.width - 40
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            spacing: 10
        }
    }

    Row {
        x: 28
        y: 128
        spacing: 18

        OptionCard {
            iconName: "bike"
            tint: Theme.orange
            heading: "Zwift"
            format: ".zwo workout file"
            body: "For Zwift, and for other training apps that can import Zwift workout files. "
                  + "Power, cadence and ramps are kept."
            warning: dlg.rowing ? "This is a rowing workout. Zwift has no rowing, so it is written as a bike workout." : ""
            AppButton {
                width: parent.width
                height: 50
                iconName: "export"
                text: "Export .zwo…"
                onClicked: dlg.exportRequested("zwo")
            }
            AppButton {
                visible: Library.zwiftFolders.length > 0
                width: parent.width
                height: 46
                kind: "secondary"
                bold: false
                fontSize: 16
                iconName: "download"
                text: "Copy into Zwift's workouts folder"
                onClicked: {
                    if (Library.exportToZwift(Library.zwiftFolders[0]))
                        dlg.close()
                }
            }
        }
        OptionCard {
            iconName: "document"
            tint: Theme.accentLight
            heading: "Artilody Watts Trainer"
            format: ".awt workout file"
            body: "Exact copy for your training computer – keeps the sport (cycling / rowing), block names, "
                  + "cadence or stroke rate and RPE. Save it anywhere, e.g. on a USB stick."
            AppButton {
                width: parent.width
                height: 50
                iconName: "export"
                text: "Export .awt…"
                onClicked: dlg.exportRequested("awt")
            }
        }
    }

    // --- all workouts at once (e.g. a whole training series) ---------------------
    Rectangle {
        id: allCard
        x: 28
        y: 446
        width: dlg.panelWidth - 56
        height: 124
        radius: 14
        color: Theme.card
        border.width: 1
        border.color: Theme.cardBorder

        Rectangle {
            x: 20
            y: 20
            width: 52
            height: 52
            radius: 14
            color: Qt.rgba(Theme.success.r, Theme.success.g, Theme.success.b, 0.18)
            Icon {
                anchors.centerIn: parent
                name: "share"
                size: 28
                strokeWidth: 2
                color: Theme.success
            }
        }
        Column {
            x: 86
            y: 18
            width: parent.width - 106
            spacing: 2
            Text {
                text: "Export all workouts  (" + Library.exportAllCount + ")"
                color: Theme.text
                font.pixelSize: 21
                font.weight: Font.Bold
            }
            Text {
                width: parent.width
                text: Library.filtered
                      ? "The " + Library.exportAllCount + " workouts shown in the library (search / filter is on) – one file each, into a folder you choose."
                      : "Every workout in your Workouts folder – one file each, into a folder you choose. Saved changes only."
                color: Theme.textSecondary
                font.pixelSize: 15
                wrapMode: Text.WordWrap
            }
        }
        Row {
            x: 86
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 16
            spacing: 10
            AppButton {
                width: 190
                height: 44
                iconName: "export"
                text: "All as .zwo…"
                enabled: Library.exportAllCount > 0
                onClicked: dlg.exportAllRequested("zwo")
            }
            AppButton {
                width: 190
                height: 44
                kind: "secondary"
                iconName: "export"
                text: "All as .awt…"
                enabled: Library.exportAllCount > 0
                onClicked: dlg.exportAllRequested("awt")
            }
            AppButton {
                visible: Library.zwiftFolders.length > 0
                width: 270
                height: 44
                kind: "secondary"
                bold: false
                fontSize: 16
                iconName: "download"
                text: "Copy all into Zwift's folder"
                enabled: Library.exportAllCount > 0
                onClicked: dlg.exportAllRequested("zwift")
            }
        }
    }

    Rectangle {
        x: 28
        y: 590
        width: dlg.panelWidth - 56
        height: 1
        color: Theme.cardBorder
    }
    UsageNote {
        x: 28
        y: 610
        width: dlg.panelWidth - 56
        fontSize: 14
        spacing: 12
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24
        spacing: 12
        Text {
            Layout.fillWidth: true
            text: "Tip: files in your Workouts folder (" + Library.nativePath(Library.folder) + ") can be copied directly, too."
            color: Theme.textMuted
            font.pixelSize: 13
            elide: Text.ElideMiddle
        }
        AppButton {
            kind: "secondary"
            text: "Close"
            Layout.preferredWidth: 130
            onClicked: dlg.close()
        }
    }
}
