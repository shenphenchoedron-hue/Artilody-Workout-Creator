import QtQuick
import QtQuick.Layouts
import Artilody.Backend

// FTP (watts preview only), theme, workouts folder.
AppDialog {
    id: dlg
    signal changeFolderRequested()
    title: "Settings"
    panelWidth: 820
    panelHeight: 830

    component Label: Text {
        color: Theme.text
        font.pixelSize: 17
        Layout.preferredWidth: 230
    }

    ColumnLayout {
        x: 28
        y: 86
        width: dlg.panelWidth - 56
        spacing: 16

        Text {
            text: "WATTS PREVIEW"
            color: Theme.textMuted
            font.pixelSize: 13
            font.weight: Font.DemiBold
            font.letterSpacing: 1
        }
        RowLayout {
            spacing: 16
            Label { text: "Cycling FTP" }
            UnitSpinBox {
                from: Settings.minFtp
                to: Settings.maxFtp
                stepSize: 5
                unit: "W"
                value: Settings.ftp
                onValueChanged: Settings.ftp = value
            }
        }
        RowLayout {
            spacing: 16
            Label { text: "Rowing FTP" }
            UnitSpinBox {
                from: Settings.minFtp
                to: Settings.maxFtp
                stepSize: 5
                unit: "W"
                value: Settings.rowingFtp
                onValueChanged: Settings.rowingFtp = value
            }
        }
        RowLayout {
            spacing: 16
            Label { text: "Default cadence" }
            UnitSpinBox {
                from: 40
                to: 140
                unit: "RPM"
                value: Settings.preferredCadence
                onValueChanged: Settings.preferredCadence = value
            }
        }
        Text {
            Layout.fillWidth: true
            text: "Only used to show watts while you edit. Workout files store % of FTP, so on the training "
                  + "computer they follow the rider's own FTP (or Rowing FTP)."
            color: Theme.textSecondary
            font.pixelSize: 14
            wrapMode: Text.WordWrap
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.cardBorder }

        RowLayout {
            spacing: 16
            Label { text: "Theme" }
            SegmentedControl {
                Layout.preferredHeight: 46
                fontSize: 16
                value: Settings.darkTheme ? "dark" : "light"
                options: [
                    { value: "dark", label: "Dark", icon: "moon" },
                    { value: "light", label: "Light", icon: "sun" }
                ]
                onActivated: (v) => Settings.darkTheme = v === "dark"
            }
        }
        RowLayout {
            spacing: 16
            Label { text: "Workouts folder" }
            Text {
                Layout.fillWidth: true
                text: Library.nativePath(Settings.workoutsFolder)
                color: Theme.textSecondary
                font.pixelSize: 15
                elide: Text.ElideMiddle
            }
        }
        RowLayout {
            spacing: 12
            Item { Layout.preferredWidth: 230 }
            AppButton {
                kind: "secondary"; bold: false; fontSize: 16; iconSize: 20; Layout.preferredHeight: 44
                iconName: "edit"; text: "Change…"
                onClicked: dlg.changeFolderRequested()
            }
            AppButton {
                kind: "secondary"; bold: false; fontSize: 16; iconSize: 20; Layout.preferredHeight: 44
                iconName: "folder"
                text: Qt.platform.os === "osx" ? "Show in Finder" : (Qt.platform.os === "windows" ? "Show in Explorer" : "Open")
                onClicked: Library.openFolder()
            }
            AppButton {
                kind: "secondary"; bold: false; fontSize: 16; iconSize: 20; Layout.preferredHeight: 44
                visible: Settings.workoutsFolder !== Settings.defaultWorkoutsFolder
                iconName: "refresh"; text: "Default"
                onClicked: Settings.workoutsFolder = Settings.defaultWorkoutsFolder
            }
        }
        RowLayout {
            spacing: 16
            Label { text: "Show Zwift .zwo files" }
            ToggleSwitch {
                checked: Settings.showZwoFiles
                onColor: Theme.accent
                onToggled: (c) => Settings.showZwoFiles = c
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.cardBorder }

        UsageNote {
            Layout.fillWidth: true
            fontSize: 14
            spacing: 12
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
            text: "Done"
            Layout.preferredWidth: 130
            onClicked: dlg.close()
        }
    }
}
