import QtQuick
import Artilody.Backend

// Small zone-coloured profile of the blocks a Builder dialog is about to add.
//   blocks: [{ sec, percent, endPercent (optional, ramps) }] – power in % of FTP
Rectangle {
    id: preview
    property var blocks: []
    property var zoneBounds: Profiles.zoneBounds   // personal zones (fractions of FTP)

    radius: 10
    color: Theme.field
    border.width: 1
    border.color: Theme.fieldBorder

    function zoneFor(percent) {
        var f = percent / 100
        for (var i = 0; i < zoneBounds.length; ++i)
            if (f < zoneBounds[i])
                return i
        return 5
    }

    // ramps are drawn as thin slices; labels only on whole blocks
    readonly property var bars: {
        var out = []
        var start = 0
        for (var i = 0; i < blocks.length; ++i) {
            var b = blocks[i]
            var end = b.endPercent !== undefined ? b.endPercent : b.percent
            if (end === b.percent) {
                out.push({ start: start, sec: b.sec, percent: b.percent, label: b.percent + "%" })
            } else {
                var slices = 24
                for (var k = 0; k < slices; ++k) {
                    var p = b.percent + (end - b.percent) * (k + 0.5) / slices
                    out.push({ start: start + b.sec * k / slices, sec: b.sec / slices, percent: p, label: "" })
                }
            }
            start += b.sec
        }
        return out
    }
    readonly property real totalSec: {
        var t = 0
        for (var i = 0; i < blocks.length; ++i)
            t += blocks[i].sec
        return Math.max(1, t)
    }
    readonly property real maxPercent: {
        var m = 100
        for (var i = 0; i < bars.length; ++i)
            m = Math.max(m, bars[i].percent)
        return m * 1.12
    }

    Item {
        id: area
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 26
        anchors.bottomMargin: 10

        // 100 % FTP reference line
        Rectangle {
            width: parent.width
            height: 1
            y: parent.height * (1 - 100 / preview.maxPercent)
            color: Theme.textMuted
            opacity: 0.6
        }
        Text {
            anchors.right: parent.right
            y: parent.height * (1 - 100 / preview.maxPercent) - height - 1
            text: "100%"
            color: Theme.textMuted
            font.pixelSize: 11
        }

        Repeater {
            model: preview.bars
            Rectangle {
                readonly property real h: Math.max(3, area.height * modelData.percent / preview.maxPercent)
                x: area.width * modelData.start / preview.totalSec
                width: Math.max(1, area.width * modelData.sec / preview.totalSec - (modelData.label.length > 0 ? 2 : 0))
                height: h
                y: area.height - h
                radius: modelData.label.length > 0 ? 2 : 0
                color: Theme.zoneColor(preview.zoneFor(modelData.percent))
                Text {
                    visible: modelData.label.length > 0 && parent.width >= implicitWidth + 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.top
                    anchors.bottomMargin: 2
                    text: modelData.label
                    color: Theme.textSecondary
                    font.pixelSize: 12
                }
            }
        }
    }
}
