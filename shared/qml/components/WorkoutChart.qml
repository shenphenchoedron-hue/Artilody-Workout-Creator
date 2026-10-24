import QtQuick
import Artilody.Backend

// Power profile chart used on Dashboard, Workouts, Builder and History.
//  - segments:       target blocks [{start, duration, watts, zone}] drawn as zone coloured bars
//  - targetSeries:   per-second target (History) drawn as zone coloured area
//  - powerSeries:    per-second actual power (line, or area when no targets exist)
//  - hrSeries:       per-second heart rate (magenta line on the right axis)
Canvas {
    id: chart

    property var segments: []
    property var targetSeries: []
    property var powerSeries: []
    property var hrSeries: []
    property real seriesStep: 1   // seconds represented by one powerSeries/hrSeries value
    property int totalSec: 0
    property int ftp: 250
    property var zoneBounds: Profiles.zoneBounds   // personal zones (fractions of FTP)

    property bool showAxes: true
    property bool showAxisTitles: true
    property bool showHeartRate: true
    property bool showActualLine: false
    property bool compact: false
    property bool percentAxis: false
    property bool barGaps: true
    property real minPowerAxis: 300
    property real hrMin: 80
    property real hrMax: 180
    property real fontSize: compact ? 12 : 15

    property int cursorSec: -1
    property bool showCursorLabel: false
    property int cursorWatts: 0
    property int selectionStart: -1
    property int selectionEnd: -1

    signal clickedAt(int second)

    readonly property real leftPad: !showAxes ? 2 : (compact ? 38 : (showAxisTitles ? 74 : 50))
    readonly property real rightPad: !showAxes ? 2 : (showHeartRate && !compact ? (showAxisTitles ? 70 : 46) : (compact ? 10 : 22))
    readonly property real topPad: compact ? 6 : 14
    readonly property real bottomPad: !showAxes ? 2 : (compact ? 24 : (showAxisTitles ? 60 : 34))
    readonly property real plotW: Math.max(10, width - leftPad - rightPad)
    readonly property real plotH: Math.max(10, height - topPad - bottomPad)

    renderStrategy: Canvas.Cooperative
    antialiasing: true

    onSegmentsChanged: requestPaint()
    onTargetSeriesChanged: requestPaint()
    onPowerSeriesChanged: requestPaint()
    onHrSeriesChanged: requestPaint()
    onTotalSecChanged: requestPaint()
    onSelectionStartChanged: requestPaint()
    onSelectionEndChanged: requestPaint()
    onPercentAxisChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onFtpChanged: requestPaint()
    onZoneBoundsChanged: requestPaint()
    Connections {
        target: Theme
        function onDarkChanged() { chart.requestPaint() }
    }

    function zoneFor(watts) {
        if (ftp <= 0)
            return 0
        var f = watts / ftp
        for (var i = 0; i < zoneBounds.length; ++i)
            if (f < zoneBounds[i])
                return i
        return 5
    }

    function duration() {
        if (totalSec > 0)
            return totalSec
        var t = 0
        for (var i = 0; i < segments.length; ++i)
            t = Math.max(t, segments[i].start + segments[i].duration)
        t = Math.max(t, powerSeries.length * seriesStep, targetSeries.length, hrSeries.length * seriesStep)
        return Math.max(60, t)
    }

    function maxWatts() {
        var m = 0
        for (var i = 0; i < segments.length; ++i)
            m = Math.max(m, segments[i].watts)
        for (var j = 0; j < targetSeries.length; ++j)
            m = Math.max(m, targetSeries[j])
        if (powerSeries.length > 0 && (showActualLine || segments.length === 0)) {
            var step = Math.max(1, Math.floor(powerSeries.length / 400))
            for (var k = 0; k < powerSeries.length; k += step)
                m = Math.max(m, powerSeries[k])
        }
        var axis = Math.max(minPowerAxis, Math.ceil((m * 1.08) / 100) * 100)
        return axis
    }

    function colorString(c, alpha) {
        return Qt.rgba(c.r, c.g, c.b, alpha === undefined ? c.a : alpha)
    }

    // Bucketed series for drawing (average, ignoring zeros when requested)
    function bucket(series, buckets, ignoreZero, step) {
        var out = []
        var n = series.length
        if (n === 0)
            return out
        var per = Math.max(1, n / buckets)
        for (var b = 0; b * per < n; ++b) {
            var from = Math.floor(b * per), to = Math.min(n, Math.floor((b + 1) * per))
            var sum = 0, cnt = 0
            for (var i = from; i < Math.max(to, from + 1); ++i) {
                var v = series[i]
                if (ignoreZero && v <= 0)
                    continue
                sum += v
                ++cnt
            }
            out.push({ t: (from + to) / 2 * (step || 1), v: cnt > 0 ? sum / cnt : -1 })
        }
        return out
    }

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        var W = width, H = height
        var L = leftPad, T = topPad, PW = plotW, PH = plotH
        var dur = duration()
        var maxW = maxWatts()
        var xOf = function(sec) { return L + sec / dur * PW }
        var yOf = function(w) { return T + PH - Math.min(1, w / maxW) * PH }
        var yHr = function(h) { return T + PH - (Math.max(hrMin, Math.min(hrMax, h)) - hrMin) / (hrMax - hrMin) * PH }
        var textColor = colorString(Theme.textSecondary)
        var fontPx = fontSize

        // --- grid & axes --------------------------------------------------
        var yStep = maxW > 600 ? 200 : 100
        var minutes = dur / 60
        var xStepMin = minutes > 90 ? 20 : minutes > 40 ? 10 : minutes > 15 ? 5 : minutes > 6 ? 2 : 1
        if (showAxes) {
            ctx.font = fontPx + "px sans-serif"
            ctx.strokeStyle = colorString(Theme.grid)
            ctx.lineWidth = 1
            ctx.setLineDash([4, 4])
            ctx.beginPath()
            for (var yw = yStep; yw <= maxW; yw += yStep) {
                var gy = Math.round(yOf(yw)) + 0.5
                ctx.moveTo(L, gy)
                ctx.lineTo(L + PW, gy)
            }
            for (var xm = xStepMin; xm < minutes; xm += xStepMin) {
                var gx = Math.round(xOf(xm * 60)) + 0.5
                ctx.moveTo(gx, T)
                ctx.lineTo(gx, T + PH)
            }
            ctx.stroke()
            ctx.setLineDash([])

            // axis lines
            ctx.strokeStyle = colorString(Theme.textMuted)
            ctx.beginPath()
            ctx.moveTo(L + 0.5, T)
            ctx.lineTo(L + 0.5, T + PH + 0.5)
            ctx.lineTo(L + PW, T + PH + 0.5)
            if (showHeartRate && !compact) {
                ctx.moveTo(L + PW + 0.5, T)
                ctx.lineTo(L + PW + 0.5, T + PH)
            }
            ctx.stroke()

            // y labels
            ctx.fillStyle = textColor
            ctx.textAlign = "right"
            ctx.textBaseline = "middle"
            for (var yl = 0; yl <= maxW; yl += yStep) {
                var label = percentAxis && ftp > 0 ? Math.round(yl / ftp * 100) + "%" : "" + yl
                ctx.beginPath()
                ctx.moveTo(L - 5, yOf(yl))
                ctx.lineTo(L, yOf(yl))
                ctx.stroke()
                ctx.fillText(label, L - 9, yOf(yl))
            }
            // x labels
            ctx.textAlign = "center"
            ctx.textBaseline = "top"
            for (var xl = 0; xl <= minutes + 0.01; xl += xStepMin)
                ctx.fillText("" + xl, xOf(xl * 60), T + PH + (compact ? 6 : 10))

            // heart-rate axis
            if (showHeartRate && !compact) {
                ctx.fillStyle = colorString(Theme.heartRate)
                ctx.textAlign = "left"
                ctx.textBaseline = "middle"
                for (var hl = hrMin; hl <= hrMax; hl += 20) {
                    ctx.fillText("" + hl, L + PW + 10, yHr(hl))
                }
            }

            if (showAxisTitles && !compact) {
                ctx.fillStyle = textColor
                ctx.textAlign = "center"
                ctx.textBaseline = "alphabetic"
                ctx.font = (fontPx + 3) + "px sans-serif"
                ctx.fillText("Time (min)", L + PW / 2, H - 6)
                ctx.save()
                ctx.translate(18, T + PH / 2)
                ctx.rotate(-Math.PI / 2)
                ctx.textBaseline = "middle"
                ctx.fillText(percentAxis ? "Power (% FTP)" : "Power (W)", 0, 0)
                ctx.restore()
                if (showHeartRate) {
                    ctx.save()
                    ctx.fillStyle = colorString(Theme.heartRate)
                    ctx.translate(W - 14, T + PH / 2)
                    ctx.rotate(-Math.PI / 2)
                    ctx.textBaseline = "middle"
                    ctx.fillText("Heart Rate (BPM)", 0, 0)
                    ctx.restore()
                }
            }
        }

        // --- target bars ---------------------------------------------------
        var gap = barGaps ? (compact ? 1 : 2) : 0
        for (var i = 0; i < segments.length; ++i) {
            var s = segments[i]
            var x0 = xOf(s.start), x1 = xOf(s.start + s.duration)
            var y0 = yOf(s.watts)
            var bw = Math.max(1, x1 - x0 - gap)
            ctx.fillStyle = Theme.zoneColor(s.zone)
            ctx.fillRect(x0 + gap / 2, y0, bw, T + PH - y0)
        }

        // --- target area (History) ----------------------------------------
        if (targetSeries.length > 0) {
            var runStart = 0
            for (var t = 1; t <= targetSeries.length; ++t) {
                if (t === targetSeries.length || targetSeries[t] !== targetSeries[runStart]) {
                    var tw = targetSeries[runStart]
                    if (tw > 0) {
                        ctx.fillStyle = Theme.zoneColor(zoneFor(tw))
                        var ax0 = xOf(runStart), ax1 = xOf(t)
                        ctx.fillRect(ax0, yOf(tw), Math.max(1, ax1 - ax0), T + PH - yOf(tw))
                    }
                    runStart = t
                }
            }
        }

        // --- actual power --------------------------------------------------
        var hasTargets = segments.length > 0 || targetSeries.some(function(v) { return v > 0 })
        if (powerSeries.length > 1) {
            var pb = bucket(powerSeries, Math.max(50, PW / (compact ? 3 : 1.5)), false, seriesStep)
            if (!hasTargets) {
                // free ride: zone coloured area of the actual power
                for (var a = 0; a < pb.length; ++a) {
                    var nextT = a + 1 < pb.length ? pb[a + 1].t : pb[a].t + seriesStep
                    var px0 = xOf(pb[a].t), px1 = xOf(nextT)
                    ctx.fillStyle = colorString(Theme.zoneColor(zoneFor(pb[a].v)), 0.85)
                    ctx.fillRect(px0, yOf(pb[a].v), Math.max(1, px1 - px0 + 0.5), T + PH - yOf(pb[a].v))
                }
            }
            if (showActualLine || !hasTargets) {
                ctx.strokeStyle = hasTargets ? colorString(Theme.actualPower) : "rgba(255,255,255,0.85)"
                ctx.lineWidth = compact ? 1 : 1.6
                ctx.beginPath()
                for (var p = 0; p < pb.length; ++p) {
                    if (p === 0)
                        ctx.moveTo(xOf(pb[p].t), yOf(pb[p].v))
                    else
                        ctx.lineTo(xOf(pb[p].t), yOf(pb[p].v))
                }
                ctx.stroke()
            }
        }

        // --- selection (Builder) -------------------------------------------
        if (selectionStart >= 0 && selectionEnd > selectionStart) {
            var sx0 = xOf(selectionStart), sx1 = xOf(selectionEnd)
            var selTop = T + PH
            for (var si = 0; si < segments.length; ++si) {
                var sg = segments[si]
                if (sg.start >= selectionStart && sg.start < selectionEnd)
                    selTop = Math.min(selTop, yOf(sg.watts))
            }
            ctx.strokeStyle = "white"
            ctx.lineWidth = 2.5
            ctx.strokeRect(sx0 + 1, selTop, sx1 - sx0 - 2, T + PH - selTop)
            ctx.fillStyle = Theme.accent
            ctx.strokeStyle = "white"
            ctx.lineWidth = 2
            var handles = [sx0 + 1, sx1 - 1]
            for (var hh = 0; hh < 2; ++hh) {
                ctx.beginPath()
                ctx.arc(handles[hh], selTop, 7, 0, Math.PI * 2)
                ctx.fill()
                ctx.stroke()
            }
        }

        // --- heart rate ----------------------------------------------------
        if (showHeartRate && hrSeries.length > 1) {
            var hb = bucket(hrSeries, Math.max(40, PW / 4), true, seriesStep)
            // light smoothing
            ctx.strokeStyle = colorString(Theme.heartRate)
            ctx.lineWidth = compact ? 1.5 : 2.6
            ctx.lineJoin = "round"
            ctx.beginPath()
            var started = false
            for (var h = 0; h < hb.length; ++h) {
                if (hb[h].v <= 0) {
                    started = false
                    continue
                }
                var sum = 0, cnt = 0
                for (var k = Math.max(0, h - 2); k <= Math.min(hb.length - 1, h + 2); ++k)
                    if (hb[k].v > 0) { sum += hb[k].v; ++cnt }
                var hy = yHr(sum / cnt)
                if (!started) {
                    ctx.moveTo(xOf(hb[h].t), hy)
                    started = true
                } else {
                    ctx.lineTo(xOf(hb[h].t), hy)
                }
            }
            ctx.stroke()
        }

        chart.paintedDuration = dur
        chart.paintedMaxWatts = maxW
    }

    // --- cursor (plain items: moving it does not repaint the canvas) -------
    property real paintedDuration: 60
    property real paintedMaxWatts: 300
    readonly property bool cursorVisible: cursorSec >= 0 && cursorSec <= paintedDuration
    readonly property real cursorX: leftPad + Math.min(cursorSec, paintedDuration) / paintedDuration * plotW
    readonly property real cursorY: topPad + plotH - Math.min(1, Math.max(0, cursorWatts) / paintedMaxWatts) * plotH

    Rectangle {
        visible: chart.cursorVisible
        x: Math.round(chart.cursorX) - 1
        y: chart.topPad
        width: 1.5
        height: chart.plotH
        color: "white"
        opacity: 0.8
    }
    Rectangle {
        visible: chart.cursorVisible && chart.showCursorLabel
        width: 14; height: 14; radius: 7
        x: chart.cursorX - 7
        y: chart.cursorY - 7
        color: "white"
        border.width: 3
        border.color: Theme.danger
    }
    Item {
        visible: chart.cursorVisible && chart.showCursorLabel
        width: bubbleText.implicitWidth + 22
        height: 39
        x: Math.max(chart.leftPad, Math.min(chart.leftPad + chart.plotW - width, chart.cursorX - width / 2))
        y: Math.max(0, chart.cursorY - 52)
        Rectangle {
            width: 12; height: 12
            rotation: 45
            color: Theme.danger
            x: chart.cursorX - parent.x - 6
            y: 25
        }
        Rectangle {
            width: parent.width
            height: 32
            radius: 7
            color: Theme.danger
            Text {
                id: bubbleText
                anchors.centerIn: parent
                text: chart.cursorWatts + " W"
                color: "white"
                font.pixelSize: chart.fontSize + 2
                font.weight: Font.Bold
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: (mouse) => {
            var sec = (mouse.x - chart.leftPad) / chart.plotW * chart.duration()
            if (sec >= 0 && sec <= chart.duration())
                chart.clickedAt(Math.floor(sec))
        }
    }
}
