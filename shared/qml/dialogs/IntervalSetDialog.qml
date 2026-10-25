import QtQuick
import QtQuick.Layouts
import Artilody.Backend

// Builder: add an interval set – repeats × (work + recovery), e.g.
// 5 × 30 s @ 120 % followed by 1 min recovery @ 55 % FTP. Power is % of FTP,
// so the workout scales with every rider's FTP.
AppDialog {
    id: dlg
    signal message(string text)

    title: "Add Intervals"
    panelWidth: 720
    panelHeight: 800

    property int repeats: 5
    property int workSec: 30
    property int workPercent: 120
    property int restSec: 60
    property int restPercent: 55
    property bool recoveryAfterLast: true

    readonly property string ftpLabel: Builder.sport === "rowing" ? "Rowing FTP" : "FTP"
    readonly property int blockCount: repeats * 2 - (recoveryAfterLast ? 0 : 1)
    readonly property int totalSec: repeats * workSec + (repeats - (recoveryAfterLast ? 0 : 1)) * restSec

    function watts(percent) {
        return Math.round(percent * Builder.ftp / 100)
    }
    function clampPercent(v) {
        return Math.max(0, Math.min(400, Math.round(v)))
    }
    function setDuration(which, text) {
        var s = Theme.parseDuration(text)
        if (s < 5) {
            message("Use mm:ss, e.g. 0:30 or 1:00")
            s = which === "work" ? workSec : restSec
        }
        s = Math.min(6 * 3600, s)
        if (which === "work") { workSec = s; workSecChanged() } else { restSec = s; restSecChanged() }
    }

    readonly property var previewBlocks: {
        var out = []
        for (var r = 0; r < repeats; ++r) {
            out.push({ sec: workSec, percent: workPercent })
            if (r < repeats - 1 || recoveryAfterLast)
                out.push({ sec: restSec, percent: restPercent })
        }
        return out
    }

    function add() {
        Builder.addIntervalSet(repeats, workSec, workPercent, restSec, restPercent, recoveryAfterLast)
        close()
        message("Added " + repeats + " × " + Theme.clock(workSec) + " @ " + workPercent + "% – the set is selected, so you can move it together")
    }

    // --- values -------------------------------------------------------------------
    ColumnLayout {
        x: 28
        y: 84
        width: dlg.panelWidth - 56
        spacing: 12

        StepperField {
            Layout.fillWidth: true
            iconName: "repeat"
            iconColor: Theme.accentLight
            label: "Repeats"
            unit: "×"
            value: dlg.repeats
            onDecrement: dlg.repeats = Math.max(1, dlg.repeats - 1)
            onIncrement: dlg.repeats = Math.min(50, dlg.repeats + 1)
            onEdited: (text) => { var v = parseInt(text); if (!isNaN(v)) dlg.repeats = Math.max(1, Math.min(50, v)); else dlg.repeatsChanged() }
        }

        Text { text: "Interval"; color: Theme.text; font.pixelSize: 17; font.weight: Font.DemiBold; Layout.topMargin: 6 }
        StepperField {
            Layout.fillWidth: true
            iconName: "clock"
            label: "Time"
            unit: "mm:ss"
            value: Theme.clock(dlg.workSec)
            onDecrement: dlg.workSec = Theme.stepDuration(dlg.workSec, -1)
            onIncrement: dlg.workSec = Theme.stepDuration(dlg.workSec, 1)
            onEdited: (text) => dlg.setDuration("work", text)
        }
        StepperField {
            Layout.fillWidth: true
            iconName: "bolt"
            iconColor: Theme.warning
            label: "Power"
            sublabel: "≈ " + dlg.watts(dlg.workPercent) + " W"
            unit: "% " + dlg.ftpLabel
            value: dlg.workPercent
            onDecrement: dlg.workPercent = dlg.clampPercent(dlg.workPercent - 1)
            onIncrement: dlg.workPercent = dlg.clampPercent(dlg.workPercent + 1)
            onEdited: (text) => { var v = parseInt(text); if (!isNaN(v)) dlg.workPercent = dlg.clampPercent(v); else dlg.workPercentChanged() }
        }

        Text { text: "Recovery"; color: Theme.text; font.pixelSize: 17; font.weight: Font.DemiBold; Layout.topMargin: 6 }
        StepperField {
            Layout.fillWidth: true
            iconName: "clock"
            label: "Time"
            unit: "mm:ss"
            value: Theme.clock(dlg.restSec)
            onDecrement: dlg.restSec = Theme.stepDuration(dlg.restSec, -1)
            onIncrement: dlg.restSec = Theme.stepDuration(dlg.restSec, 1)
            onEdited: (text) => dlg.setDuration("rest", text)
        }
        StepperField {
            Layout.fillWidth: true
            iconName: "bolt"
            iconColor: Theme.success
            label: "Power"
            sublabel: "≈ " + dlg.watts(dlg.restPercent) + " W"
            unit: "% " + dlg.ftpLabel
            value: dlg.restPercent
            onDecrement: dlg.restPercent = dlg.clampPercent(dlg.restPercent - 1)
            onIncrement: dlg.restPercent = dlg.clampPercent(dlg.restPercent + 1)
            onEdited: (text) => { var v = parseInt(text); if (!isNaN(v)) dlg.restPercent = dlg.clampPercent(v); else dlg.restPercentChanged() }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Text {
                Layout.fillWidth: true
                text: "Recovery after the last interval"
                color: Theme.textSecondary
                font.pixelSize: 16
            }
            ToggleSwitch {
                checked: dlg.recoveryAfterLast
                onToggled: (on) => dlg.recoveryAfterLast = on
            }
        }
    }

    // --- preview --------------------------------------------------------------------
    Text {
        x: 28
        y: 528
        width: dlg.panelWidth - 56
        text: dlg.repeats + " × " + Theme.clock(dlg.workSec) + " @ " + dlg.workPercent + "%  +  "
              + Theme.clock(dlg.restSec) + " @ " + dlg.restPercent + "% " + dlg.ftpLabel
              + "  ·  total " + Theme.clock(dlg.totalSec) + "  ·  " + dlg.blockCount + " blocks"
        color: Theme.textSecondary
        font.pixelSize: 15
        elide: Text.ElideRight
    }
    BlockPreview {
        x: 28
        y: 558
        width: dlg.panelWidth - 56
        height: 130
        blocks: dlg.previewBlocks
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
            text: "Add " + dlg.repeats + " intervals"
            Layout.preferredWidth: 220
            onClicked: dlg.add()
        }
    }
}
