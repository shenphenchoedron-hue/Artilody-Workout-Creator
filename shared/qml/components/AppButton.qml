import QtQuick

// Button in the mockup styles: "primary" (blue), "danger" (red),
// "secondary" (outlined) and "dashed" (Add Interval).
Rectangle {
    id: button
    property string text
    property string iconName
    property string kind: "primary"
    property real fontSize: 18
    property real iconSize: 22
    property bool enabled: true
    property bool bold: true
    property real horizontalPadding: 22
    property real iconSpacing: 12
    signal clicked()

    implicitWidth: content.implicitWidth + 2 * horizontalPadding
    implicitHeight: 54
    radius: 10
    opacity: enabled ? 1 : 0.45

    readonly property color baseColor: kind === "primary" ? Theme.accent
                                       : kind === "danger" ? Theme.danger
                                       : kind === "success" ? "#22b84a"
                                       : "transparent"
    color: mouse.pressed && enabled ? Qt.darker(baseColor === "transparent" ? Theme.hover : baseColor, 1.15)
         : (mouse.containsMouse && enabled && kind !== "primary" && kind !== "danger" && kind !== "success")
           ? Theme.hover : (kind === "secondary" ? Theme.cardStrong : baseColor)
    border.width: kind === "secondary" || kind === "dashed" ? 1 : 0
    border.color: kind === "dashed" ? Qt.rgba(0.6, 0.7, 1, 0.45) : Theme.fieldBorder

    Row {
        id: content
        anchors.centerIn: parent
        spacing: button.iconSpacing
        Icon {
            visible: button.iconName.length > 0
            name: button.iconName
            size: button.iconSize
            color: button.kind === "dashed" ? Theme.accentLight
                 : (button.kind === "secondary" ? Theme.text : "white")
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            visible: button.text.length > 0
            text: button.text
            font.pixelSize: button.fontSize
            font.weight: button.bold ? Font.DemiBold : Font.Normal
            color: button.kind === "dashed" ? Theme.accentLight
                 : (button.kind === "secondary" ? Theme.text : "white")
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // dashed outline
    Canvas {
        anchors.fill: parent
        visible: button.kind === "dashed"
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = "rgba(140,170,255,0.55)"
            ctx.lineWidth = 1.5
            ctx.setLineDash([6, 5])
            var r = button.radius
            ctx.beginPath()
            ctx.roundedRect(1, 1, width - 2, height - 2, r, r)
            ctx.stroke()
        }
        onWidthChanged: requestPaint()
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: button.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: if (button.enabled) button.clicked()
    }
}
