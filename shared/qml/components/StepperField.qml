import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Builder "Interval Settings" row: icon + label | value field | − | +
RowLayout {
    id: row
    property string iconName
    property color iconColor: Theme.text
    property string label
    property string sublabel
    property string value
    property string unit
    property bool enabled: true
    signal decrement()
    signal increment()
    signal edited(string text)

    spacing: 10
    opacity: enabled ? 1 : 0.45

    Icon {
        name: row.iconName
        color: row.iconColor
        size: 24
        Layout.alignment: Qt.AlignVCenter
    }
    Column {
        Layout.preferredWidth: 86
        Layout.alignment: Qt.AlignVCenter
        Text {
            text: row.label
            color: Theme.textSecondary
            font.pixelSize: 16
            width: parent.width
            wrapMode: Text.WordWrap
        }
        Text {
            visible: row.sublabel.length > 0
            text: row.sublabel
            color: Theme.textMuted
            font.pixelSize: 11
        }
    }
    TextField {
        id: input
        Layout.fillWidth: true
        Layout.minimumWidth: 92
        Layout.preferredHeight: 50
        enabled: row.enabled
        text: row.value
        font.pixelSize: 21
        font.weight: Font.DemiBold
        color: Theme.text
        selectByMouse: true
        leftPadding: 12
        rightPadding: row.unit.length > 0 ? unitLabel.implicitWidth + 16 : 10
        verticalAlignment: TextInput.AlignVCenter
        onEditingFinished: row.edited(text)
        background: Rectangle {
            radius: 9
            color: Theme.field
            border.width: 1
            border.color: input.activeFocus ? Theme.accentLight : Theme.fieldBorder
        }
        Text {
            id: unitLabel
            visible: row.unit.length > 0
            text: row.unit
            color: Theme.textSecondary
            font.pixelSize: 12
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    Rectangle {
        Layout.preferredWidth: 84
        Layout.preferredHeight: 50
        radius: 9
        color: Theme.field
        border.width: 1
        border.color: Theme.fieldBorder
        Row {
            anchors.fill: parent
            Repeater {
                model: ["minus", "plus"]
                Item {
                    width: 42
                    height: 50
                    Icon {
                        anchors.centerIn: parent
                        name: modelData
                        size: 20
                    }
                    Rectangle {
                        visible: index === 1
                        width: 1
                        height: parent.height - 12
                        y: 6
                        color: Theme.fieldBorder
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: row.enabled
                        cursorShape: Qt.PointingHandCursor
                        onClicked: index === 0 ? row.decrement() : row.increment()
                        onPressAndHold: repeat.start()
                        onReleased: repeat.stop()
                        onCanceled: repeat.stop()
                        Timer {
                            id: repeat
                            interval: 90
                            repeat: true
                            onTriggered: index === 0 ? row.decrement() : row.increment()
                        }
                    }
                }
            }
        }
    }
}
