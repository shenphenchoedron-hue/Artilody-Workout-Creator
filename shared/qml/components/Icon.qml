import QtQuick

// Vector line icon drawn from Theme.icons (24×24 viewBox).
Canvas {
    id: icon

    property string name: "dashboard"
    property color color: Theme.text
    property real size: 24
    property real strokeWidth: 1.9

    implicitWidth: size
    implicitHeight: size
    width: size
    height: size
    antialiasing: true
    renderStrategy: Canvas.Cooperative

    onNameChanged: requestPaint()
    onColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onStrokeWidthChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        var def = Theme.icons[name]
        if (!def)
            return
        var d = typeof def === "string" ? def : def.d
        var filled = typeof def === "object" && def.fill === true
        ctx.save()
        ctx.scale(width / 24, height / 24)
        ctx.lineWidth = strokeWidth
        ctx.lineCap = "round"
        ctx.lineJoin = "round"
        ctx.strokeStyle = icon.color
        ctx.fillStyle = icon.color
        ctx.beginPath()
        ctx.path = d
        if (filled)
            ctx.fill()
        else
            ctx.stroke()
        ctx.restore()
    }
}
