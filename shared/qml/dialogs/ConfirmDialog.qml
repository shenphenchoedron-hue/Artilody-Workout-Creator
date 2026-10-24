import QtQuick
import QtQuick.Layouts

AppDialog {
    id: dlg
    property string message
    property string acceptText: "OK"
    property string secondaryText: ""
    property string acceptKind: "primary"
    signal accepted()
    signal secondary()

    panelWidth: 520
    panelHeight: 250

    Text {
        x: 28
        y: 80
        width: dlg.panelWidth - 56
        text: dlg.message
        color: Theme.textSecondary
        font.pixelSize: 17
        wrapMode: Text.WordWrap
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 26
        spacing: 12
        AppButton {
            text: "Cancel"
            kind: "secondary"
            Layout.preferredWidth: 120
            onClicked: dlg.close()
        }
        Item { Layout.fillWidth: true }
        AppButton {
            visible: dlg.secondaryText.length > 0
            text: dlg.secondaryText
            kind: "secondary"
            onClicked: {
                dlg.close()
                dlg.secondary()
            }
        }
        AppButton {
            text: dlg.acceptText
            kind: dlg.acceptKind
            onClicked: {
                dlg.close()
                dlg.accepted()
            }
        }
    }
}
