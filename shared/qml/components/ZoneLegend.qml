import QtQuick

// "RPE zones: ■1 ■2–3 ■4–6 ■7–8 ■9 ■10 | ━ Heart Rate"
Row {
    property bool showHeartRate: true
    spacing: 14

    Text {
        text: "RPE zones:"
        color: Theme.textSecondary
        font.pixelSize: 15
        anchors.verticalCenter: parent.verticalCenter
    }
    Repeater {
        model: 6
        Row {
            spacing: 7
            anchors.verticalCenter: parent.verticalCenter
            Rectangle {
                width: 18; height: 18; radius: 4
                color: Theme.zoneColor(index)
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: Theme.rpeLabels[index]
                color: Theme.textSecondary
                font.pixelSize: 15
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
    Rectangle {
        visible: parent.showHeartRate
        width: 1; height: 20
        color: Theme.textMuted
        anchors.verticalCenter: parent.verticalCenter
    }
    Row {
        visible: parent.showHeartRate
        spacing: 7
        anchors.verticalCenter: parent.verticalCenter
        Rectangle {
            width: 28; height: 3; radius: 1.5
            color: Theme.heartRate
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: "Heart Rate"
            color: Theme.textSecondary
            font.pixelSize: 15
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
