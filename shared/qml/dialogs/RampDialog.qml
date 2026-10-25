import QtQuick
import QtQuick.Layouts
import Artilody.Backend

// Builder: add a ramp – a staircase of steps (rising or falling) or one smooth
// ramp. Power is % of FTP, so the workout scales with every rider's FTP. Each
// step becomes its own block, so its time and % FTP can be changed afterwards.
AppDialog {
    id: dlg
    signal message(string text)

    title: "Add Ramp"
    panelWidth: 720
    panelHeight: 742

    property string shape: "steps"     // steps | smooth
    property string direction: "up"    // up | down
    property int steps: 5
    property int stepSec: 120
    property int rampSec: 600
    property int startPercent: 50
    property int endPercent: 100

    readonly property bool stepMode: shape === "steps"
    readonly property string ftpLabel: Builder.sport === "rowing" ? "Rowing FTP" : "FTP"

    function percentOfStep(k) {
        if (steps <= 1)
            return startPercent
        return Math.round(startPercent + (endPercent - startPercent) * k / (steps - 1))
    }
    function watts(percent) {
        return Math.round(percent * Builder.ftp / 100)
    }
    function setDirection(d) {
        direction = d
        var lo = Math.min(startPercent, endPercent)
        var hi = Math.max(startPercent, endPercent)
        startPercent = d === "down" ? hi : lo
        endPercent = d === "down" ? lo : hi
    }
    // keeps the Up / Down switch in line with typed values
    function syncDirection() {
        if (startPercent !== endPercent)
            direction = endPercent > startPercent ? "up" : "down"
    }
    function setPercent(which, v) {
        v = Math.max(0, Math.min(400, Math.round(v)))
        if (which === "start")
            startPercent = v
        else
            endPercent = v
        syncDirection()
    }

    readonly property var previewBlocks: {
        if (!stepMode)
            return [{ sec: rampSec, percent: startPercent, endPercent: endPercent }]
        var out = []
        for (var k = 0; k < steps; ++k)
            out.push({ sec: stepSec, percent: percentOfStep(k) })
        return out
    }
    readonly property int totalSec: stepMode ? steps * stepSec : rampSec
    readonly property string stepList: {
        if (!stepMode)
            return ""
        var parts = []
        for (var k = 0; k < steps; ++k)
            parts.push(percentOfStep(k) + "%")
        return parts.join("  ›  ")
    }

    function add() {
        if (stepMode)
            Builder.addSteps(steps, stepSec, startPercent, endPercent)
        else
            Builder.addRamp(rampSec, startPercent, endPercent)
        close()
        message(stepMode ? "Added " + steps + " steps (" + Theme.clock(totalSec) + ") – they are selected, so you can move them together"
                         : "Added a ramp " + startPercent + "–" + endPercent + "% " + ftpLabel)
    }

    // --- shape / direction ----------------------------------------------------
    Row {
        x: 28
        y: 84
        spacing: 16
        SegmentedControl {
            height: 46
            fontSize: 16
            value: dlg.shape
            options: [
                { value: "steps", label: "Steps" },
                { value: "smooth", label: "Smooth ramp" }
            ]
            onActivated: (v) => dlg.shape = v
        }
        SegmentedControl {
            height: 46
            fontSize: 16
            value: dlg.direction
            options: [
                { value: "up", label: "Rising", icon: "trendUp" },
                { value: "down", label: "Falling" }
            ]
            onActivated: (v) => dlg.setDirection(v)
        }
    }

    // --- values -------------------------------------------------------------------
    ColumnLayout {
        x: 28
        y: 150
        width: dlg.panelWidth - 56
        spacing: 12

        StepperField {
            Layout.fillWidth: true
            visible: dlg.stepMode
            iconName: "dashboard"
            iconColor: Theme.accentLight
            label: "Steps"
            unit: "steps"
            value: dlg.steps
            onDecrement: dlg.steps = Math.max(1, dlg.steps - 1)
            onIncrement: dlg.steps = Math.min(50, dlg.steps + 1)
            onEdited: (text) => { var v = parseInt(text); if (!isNaN(v)) dlg.steps = Math.max(1, Math.min(50, v)); else dlg.stepsChanged() }
        }
        StepperField {
            Layout.fillWidth: true
            iconName: "clock"
            label: dlg.stepMode ? "Time per step" : "Duration"
            unit: "mm:ss"
            value: Theme.clock(dlg.stepMode ? dlg.stepSec : dlg.rampSec)
            onDecrement: dlg.stepMode ? dlg.stepSec = Theme.stepDuration(dlg.stepSec, -1) : dlg.rampSec = Theme.stepDuration(dlg.rampSec, -1)
            onIncrement: dlg.stepMode ? dlg.stepSec = Theme.stepDuration(dlg.stepSec, 1) : dlg.rampSec = Theme.stepDuration(dlg.rampSec, 1)
            onEdited: (text) => {
                var s = Theme.parseDuration(text)
                if (s < 5) {
                    dlg.message("Use mm:ss, e.g. 2:00 or 0:30")
                    s = dlg.stepMode ? dlg.stepSec : dlg.rampSec
                }
                s = Math.min(6 * 3600, s)
                if (dlg.stepMode) { dlg.stepSec = s; dlg.stepSecChanged() } else { dlg.rampSec = s; dlg.rampSecChanged() }
            }
        }
        StepperField {
            Layout.fillWidth: true
            iconName: "bolt"
            iconColor: Theme.warning
            label: dlg.stepMode ? "First step" : "Start power"
            sublabel: "≈ " + dlg.watts(dlg.startPercent) + " W"
            unit: "% " + dlg.ftpLabel
            value: dlg.startPercent
            onDecrement: dlg.setPercent("start", dlg.startPercent - 1)
            onIncrement: dlg.setPercent("start", dlg.startPercent + 1)
            onEdited: (text) => { var v = parseInt(text); if (!isNaN(v)) dlg.setPercent("start", v); else dlg.startPercentChanged() }
        }
        StepperField {
            Layout.fillWidth: true
            iconName: "trendUp"
            iconColor: Theme.warning
            label: dlg.stepMode ? "Last step" : "End power"
            sublabel: "≈ " + dlg.watts(dlg.endPercent) + " W"
            unit: "% " + dlg.ftpLabel
            value: dlg.endPercent
            onDecrement: dlg.setPercent("end", dlg.endPercent - 1)
            onIncrement: dlg.setPercent("end", dlg.endPercent + 1)
            onEdited: (text) => { var v = parseInt(text); if (!isNaN(v)) dlg.setPercent("end", v); else dlg.endPercentChanged() }
        }
    }

    // --- preview --------------------------------------------------------------------
    Text {
        x: 28
        y: 418
        width: dlg.panelWidth - 56
        text: (dlg.stepMode ? dlg.steps + " × " + Theme.clock(dlg.stepSec) + "  ·  " : "")
              + "total " + Theme.clock(dlg.totalSec)
              + (dlg.stepMode ? "  ·  " + dlg.stepList : "  ·  " + dlg.startPercent + "% → " + dlg.endPercent + "% " + dlg.ftpLabel)
        color: Theme.textSecondary
        font.pixelSize: 15
        elide: Text.ElideRight
    }
    BlockPreview {
        x: 28
        y: 448
        width: dlg.panelWidth - 56
        height: 150
        blocks: dlg.previewBlocks
    }
    Text {
        x: 28
        y: 608
        width: dlg.panelWidth - 56
        text: dlg.stepMode ? "Every step becomes its own block: click a step afterwards to change its time or % " + dlg.ftpLabel + "."
                           : "Power changes smoothly from start to end over the whole duration."
        color: Theme.textMuted
        font.pixelSize: 13
        wrapMode: Text.WordWrap
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24
        spacing: 12
        Item { Layout.fillWidth: true }
        AppButton {
            kind: "secondary"
            text: "Cancel"
            Layout.preferredWidth: 130
            onClicked: dlg.close()
        }
        AppButton {
            iconName: "plus"
            text: dlg.stepMode ? "Add " + dlg.steps + " steps" : "Add ramp"
            Layout.preferredWidth: 200
            onClicked: dlg.add()
        }
    }
}
