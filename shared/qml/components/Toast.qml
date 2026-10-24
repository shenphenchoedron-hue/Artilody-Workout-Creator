import QtQuick

Rectangle {
    id: toast
    property string text
    function show(message) {
        text = message
        opacity = 1
        // longer messages (e.g. import results with warnings) stay a bit longer
        hideTimer.interval = Math.min(9000, 3500 + Math.max(0, message.length - 60) * 40)
        hideTimer.restart()
    }

    z: 2000
    width: Math.min(label.implicitWidth + 48, parent.width - 80)
    height: Math.max(52, label.implicitHeight + 26)
    radius: 12
    color: Theme.dark ? "#0f1846" : "#16214f"
    border.color: Qt.rgba(1, 1, 1, 0.15)
    opacity: 0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: 220 } }

    Text {
        id: label
        anchors.centerIn: parent
        width: parent.width - 32
        horizontalAlignment: Text.AlignHCenter
        text: toast.text
        color: "white"
        font.pixelSize: 16
        wrapMode: Text.WordWrap
        maximumLineCount: 3
        elide: Text.ElideRight
    }
    Timer {
        id: hideTimer
        interval: 3500
        onTriggered: toast.opacity = 0
    }
}
