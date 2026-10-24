import QtQuick
import QtQuick.Controls

// Profile dialog field: value | unit | ▲▼
Rectangle {
    id: box
    property real value: 0
    property real from: 0
    property real to: 1000
    property real stepSize: 1
    property int decimals: 0
    property string unit

    implicitWidth: 204
    implicitHeight: 46
    radius: 9
    color: Theme.field
    border.width: 1
    border.color: input.activeFocus ? Theme.accentLight : Theme.fieldBorder

    function clampValue(v) {
        return Math.max(from, Math.min(to, v))
    }
    function step(dir) {
        value = clampValue(Math.round((value + dir * stepSize) * Math.pow(10, decimals)) / Math.pow(10, decimals))
    }

    TextInput {
        id: input
        x: 16
        width: parent.width * 0.48
        anchors.verticalCenter: parent.verticalCenter
        text: box.value.toFixed(box.decimals)
        color: Theme.text
        font.pixelSize: 18
        selectByMouse: true
        validator: DoubleValidator { bottom: box.from; top: box.to; decimals: box.decimals }
        onEditingFinished: {
            var v = parseFloat(text.replace(",", "."))
            if (!isNaN(v))
                box.value = box.clampValue(v)
            text = Qt.binding(function() { return box.value.toFixed(box.decimals) })
        }
        Keys.onUpPressed: box.step(1)
        Keys.onDownPressed: box.step(-1)
    }
    Rectangle {
        x: parent.width * 0.5
        width: 1
        height: parent.height
        color: Theme.fieldBorder
    }
    Text {
        text: box.unit
        color: Theme.textSecondary
        font.pixelSize: 15
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: arrows.left
        anchors.rightMargin: 14
    }
    Column {
        id: arrows
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        Repeater {
            model: ["chevronUp", "chevronDown"]
            Icon {
                name: modelData
                size: 15
                strokeWidth: 2.4
                color: Theme.textSecondary
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: box.step(index === 0 ? 1 : -1)
                    onPressAndHold: rep.start()
                    onReleased: rep.stop()
                    Timer {
                        id: rep
                        interval: 80
                        repeat: true
                        onTriggered: box.step(index === 0 ? 1 : -1)
                    }
                }
            }
        }
    }
    WheelHandler {
        onWheel: (event) => box.step(event.angleDelta.y > 0 ? 1 : -1)
    }
}
