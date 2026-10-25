import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Artilody.Backend

Item {
    id: page
    signal message(string text)

    readonly property var sel: Builder.selected
    readonly property int selIndex: Builder.selectedIndex
    readonly property bool hasSel: selIndex >= 0 && sel.index !== undefined
    readonly property bool showEndPower: hasSel && (sel.isRamp || sel.type === "ramp" || sel.type === "warmup" || sel.type === "cooldown")
    // All power targets are % of FTP (that is what workouts store, so they
    // scale with every rider's FTP); watts are only shown as a hint.
    property bool percentView: true

    // multi-selection (Ctrl/⌘-click, Shift-click, Ctrl/⌘+A)
    readonly property var selRows: Builder.selection
    readonly property int selCount: Builder.selectionCount
    readonly property bool multi: selCount > 1
    readonly property int selFirst: selCount > 0 ? selRows[0] : -1
    readonly property int selLast: selCount > 0 ? selRows[selCount - 1] : -1
    readonly property bool selContiguous: selCount > 0 && selLast - selFirst + 1 === selCount
    readonly property int selDuration: Builder.selectionDurationSec
    readonly property bool rowing: Builder.sport === "rowing"
    readonly property string ftpLabel: rowing ? "Rowing FTP" : "FTP"

    // Hooks for the standalone "Artilody Workout Creator", which shares this
    // page (the defaults are the Watts Trainer behaviour).
    property string pageTitle: "Workout Builder"
    property string pageSubtitle: page.rowing ? "Design your own rowing session" : "Design your own training session"
    property bool showExport: false     // adds an "Export" button next to Save
    signal exportRequested()

    PageHeader {
        x: Theme.pageMargin + 8
        y: 30
        width: parent.width - 2 * Theme.pageMargin - 8
        title: page.pageTitle
        subtitle: page.pageSubtitle
    }

    // FTP the watts are based on (rowing workouts use the rowing FTP)
    Row {
        anchors.right: parent.right
        anchors.rightMargin: Theme.pageMargin
        y: 90
        spacing: 8
        Icon {
            name: page.rowing ? "rowing" : "bike"
            size: 22
            strokeWidth: 2
            color: Theme.sportColor(Builder.sport)
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: "Power in % of " + page.ftpLabel + "  ·  your " + page.ftpLabel + ": " + Builder.ftp + " W"
            color: Theme.textSecondary
            font.pixelSize: 17
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // --- name / actions ----------------------------------------------------
    RowLayout {
        x: Theme.pageMargin
        y: 144
        width: parent.width - 2 * Theme.pageMargin
        height: 56
        spacing: 14

        AppTextField {
            id: nameField
            Layout.fillWidth: true
            Layout.fillHeight: true
            font.pixelSize: 22
            text: Builder.name
            trailingIcon: "edit"
            placeholderText: "Workout name"
            onTextEdited: Builder.name = text
            // show the start of a long name (not its end) when a workout is loaded
            onTextChanged: if (!activeFocus) cursorPosition = 0
            onTrailingClicked: { nameField.forceActiveFocus(); nameField.selectAll() }
        }
        SegmentedControl {
            Layout.fillHeight: true
            fontSize: 16
            segmentPadding: 14
            value: Builder.sport
            options: [
                { value: "cycling", label: "Cycling", icon: "bike" },
                { value: "rowing", label: "Rowing", icon: "rowing", color: Theme.rowingColor, textColor: Theme.textOnColor }
            ]
            onActivated: (v) => Builder.sport = v
        }
        AppComboBox {
            Layout.preferredWidth: 180
            Layout.fillHeight: true
            model: Builder.categories
            currentIndex: Math.max(0, Builder.categories.indexOf(Builder.category))
            onActivated: (index) => Builder.category = Builder.categories[index]
        }
        Item { Layout.preferredWidth: 20 }
        AppButton {
            Layout.preferredWidth: page.showExport ? 146 : 166
            Layout.fillHeight: true
            kind: "secondary"
            iconName: "play"
            text: "Preview"
            bold: false
            onClicked: previewDialog.open()
        }
        AppButton {
            visible: page.showExport
            Layout.preferredWidth: 146
            Layout.fillHeight: true
            kind: "secondary"
            iconName: "export"
            text: "Export"
            bold: false
            onClicked: page.exportRequested()
        }
        AppButton {
            Layout.preferredWidth: 216
            Layout.fillHeight: true
            iconName: "save"
            text: "Save Workout"
            onClicked: page.save()
        }
    }

    // --- reorder / delete helpers --------------------------------------------
    readonly property string altKey: Qt.platform.os === "osx" ? "⌥" : "Alt+"

    function moveTo(from, to) {
        var n = Builder.blockCount
        if (from < 0 || from >= n)
            return
        to = Math.max(0, Math.min(n - 1, to))
        if (to === from)
            return
        Builder.moveBlock(from, to)
        Builder.selectedIndex = to
        blockList.positionViewAtIndex(to, ListView.Contain)
    }
    function deleteBlock(index) {
        if (index < 0 || index >= Builder.blockCount)
            return
        Builder.removeBlock(index)
    }
    // true when index belongs to a selection of several blocks: moving,
    // duplicating and deleting it then acts on the whole group
    function inGroup(index) {
        return page.multi && page.selRows.indexOf(index) >= 0
    }
    function canMoveEarlier(index) {
        return inGroup(index) ? !(page.selContiguous && page.selFirst === 0) : index > 0
    }
    function canMoveLater(index) {
        return inGroup(index) ? !(page.selContiguous && page.selLast === Builder.blockCount - 1)
                              : index >= 0 && index < Builder.blockCount - 1
    }
    function shift(index, delta) {
        if (inGroup(index)) {
            Builder.shiftSelection(delta)
            blockList.positionViewAtIndex(delta < 0 ? page.selFirst : page.selLast, ListView.Contain)
        } else {
            page.moveTo(index, index + delta)
        }
    }
    function moveToStart(index) {
        if (inGroup(index)) {
            Builder.moveSelection(0)
            blockList.positionViewAtIndex(0, ListView.Contain)
        } else {
            page.moveTo(index, 0)
        }
    }
    function moveToEnd(index) {
        if (inGroup(index)) {
            Builder.moveSelection(Builder.blockCount)
            blockList.positionViewAtIndex(Builder.blockCount - 1, ListView.Contain)
        } else {
            page.moveTo(index, Builder.blockCount - 1)
        }
    }
    function duplicate(index) {
        if (inGroup(index))
            Builder.duplicateSelection()
        else
            Builder.duplicateBlock(index)
    }
    function requestDelete(index) {
        if (inGroup(index))
            deleteGroupDialog.open()
        else
            page.deleteBlock(index)
    }
    // click on a block card: plain = select only it, Ctrl/⌘ = toggle, Shift = range
    function clickBlock(index, modifiers) {
        if (modifiers & Qt.ShiftModifier)
            Builder.selectRange(index)
        else if (modifiers & (Qt.ControlModifier | Qt.MetaModifier))
            Builder.toggleSelected(index)
        else
            Builder.selectedIndex = index
    }
    // Applies an edit to the selected block, or to every selected block.
    // fn(index, block) gets the block's current values (see Builder.block()).
    // All edits are one batch, so the workout is recomputed once, not per block.
    function forSelected(fn) {
        var rows = page.multi ? page.selRows.slice() : [page.selIndex]
        Builder.beginBatch()
        try {
            for (var i = 0; i < rows.length; ++i)
                fn(rows[i], Builder.block(rows[i]))
        } finally {
            Builder.endBatch()
        }
    }
    function openBlockMenu(index) {
        // right-click outside the selection selects that block alone
        if (page.selRows.indexOf(index) < 0)
            Builder.selectedIndex = index
        blockMenu.blockIndex = index
        blockMenu.popup()
    }
    // screenshot aid (WATT_SCREENSHOT_MENU=builder): menu of the selected block
    function openScreenshotMenu() {
        var i = Math.max(0, page.selIndex)
        var item = blockList.itemAtIndex(i)
        var p = item ? item.mapToItem(page, item.width * 0.6, 60) : Qt.point(page.width / 3, page.height / 2)
        blockMenu.blockIndex = i
        blockMenu.popup(p.x, p.y)
        return blockMenu
    }

    // true while a text field has the keyboard focus (Alt+arrows edit text there)
    readonly property bool textFocus: {
        var f = page.Window.activeFocusItem
        return f !== null && f !== undefined && f.cursorPosition !== undefined
    }
    Shortcut {
        sequences: ["Alt+Left", "Alt+Up"]
        enabled: page.visible && page.hasSel && !page.textFocus
        onActivated: page.shift(page.selIndex, -1)
    }
    Shortcut {
        sequences: ["Alt+Right", "Alt+Down"]
        enabled: page.visible && page.hasSel && !page.textFocus
        onActivated: page.shift(page.selIndex, 1)
    }
    Shortcut {
        sequences: [StandardKey.SelectAll]
        enabled: page.visible && !page.textFocus && Builder.blockCount > 0
        onActivated: Builder.selectAll()
    }

    AppMenu {
        id: blockMenu
        property int blockIndex: -1
        minWidth: 250
        readonly property bool group: page.inGroup(blockIndex)
        readonly property string what: group ? page.selCount + " blocks" : ""
        AppMenuItem {
            text: blockMenu.group ? "Move " + blockMenu.what + " earlier" : "Move earlier"
            iconName: "chevronLeft"
            hint: page.altKey + "←"
            enabled: page.canMoveEarlier(blockMenu.blockIndex)
            onTriggered: page.shift(blockMenu.blockIndex, -1)
        }
        AppMenuItem {
            text: blockMenu.group ? "Move " + blockMenu.what + " later" : "Move later"
            iconName: "chevronRight"
            hint: page.altKey + "→"
            enabled: page.canMoveLater(blockMenu.blockIndex)
            onTriggered: page.shift(blockMenu.blockIndex, 1)
        }
        AppMenuItem {
            text: "Move to start"
            iconName: "skipBack"
            enabled: page.canMoveEarlier(blockMenu.blockIndex)
            onTriggered: page.moveToStart(blockMenu.blockIndex)
        }
        AppMenuItem {
            text: "Move to end"
            iconName: "skipForward"
            enabled: page.canMoveLater(blockMenu.blockIndex)
            onTriggered: page.moveToEnd(blockMenu.blockIndex)
        }
        AppMenuSeparator {}
        AppMenuItem {
            text: blockMenu.group ? "Duplicate " + blockMenu.what : "Duplicate"
            iconName: "copy"
            onTriggered: page.duplicate(blockMenu.blockIndex)
        }
        AppMenuItem {
            text: "Select all blocks"
            iconName: "check"
            hint: (Qt.platform.os === "osx" ? "⌘" : "Ctrl+") + "A"
            enabled: page.selCount < Builder.blockCount
            onTriggered: Builder.selectAll()
        }
        AppMenuSeparator {}
        AppMenuItem {
            text: blockMenu.group ? "Delete " + blockMenu.what : "Delete interval"
            iconName: "trash"
            destructive: true
            onTriggered: page.requestDelete(blockMenu.blockIndex)
        }
    }

    function save() {
        var id = Builder.save()
        if (id >= 0)
            page.message("Workout \"" + Builder.name + "\" saved")
        else
            page.message(Builder.lastError)
    }

    // --- timeline card -------------------------------------------------------
    Card {
        id: timelineCard
        x: Theme.pageMargin
        y: 218
        width: settingsCard.x - x - Theme.spacing
        height: addBlockCard.y - y - Theme.spacing
        title: "Workout Timeline"
        titleSize: 22

        Row {
            x: 18
            y: 54
            spacing: 8
            Icon { name: "clock"; size: 20; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: Math.round(Builder.totalDurationSec / 60) + " min   •   " + Builder.blockCount + " blocks"
                color: Theme.textSecondary
                font.pixelSize: 17
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 18
            y: 14
            spacing: 14
            Text {
                text: "View:"
                color: Theme.textSecondary
                font.pixelSize: 16
                anchors.verticalCenter: parent.verticalCenter
            }
            AppComboBox {
                width: 206
                height: 46
                model: ["Power (Watts)", "% " + page.ftpLabel]
                currentIndex: page.percentView ? 1 : 0
                onActivated: (index) => page.percentView = index === 1
            }
        }

        WorkoutChart {
            id: builderChart
            x: 4
            y: 88
            width: parent.width - 20
            height: 238
            segments: Builder.segments
            totalSec: Builder.totalDurationSec
            showHeartRate: false
            percentAxis: page.percentView
            ftp: Builder.ftp
            barGaps: false
            // a contiguous multi-selection is highlighted as one range
            readonly property int selA: page.multi && page.selContiguous ? page.selFirst : page.selIndex
            readonly property int selB: page.multi && page.selContiguous ? page.selLast : page.selIndex
            selectionStart: page.hasSel ? Builder.blockStartSec(selA, Builder.totalDurationSec) : -1
            selectionEnd: page.hasSel ? Builder.blockStartSec(selB, Builder.totalDurationSec)
                                        + (Builder.block(selB, Builder.totalDurationSec).duration || 0) : -1
            onClickedAt: (second) => Builder.selectedIndex = Builder.blockAtSecond(second)
        }

        // how to reorder / edit / delete (the grip alone was easy to miss)
        Row {
            id: reorderHint
            x: 18
            y: 328
            width: parent.width - 36
            height: 20
            spacing: 6
            clip: true
            readonly property color tint: Theme.textSecondary
            readonly property int fontSize: 14
            Icon { name: "grip"; size: 18; color: reorderHint.tint; anchors.verticalCenter: parent.verticalCenter }
            Text { text: "Drag the grip or use"; color: reorderHint.tint; font.pixelSize: reorderHint.fontSize; anchors.verticalCenter: parent.verticalCenter }
            Icon { name: "chevronLeft"; size: 16; strokeWidth: 2.2; color: reorderHint.tint; anchors.verticalCenter: parent.verticalCenter }
            Icon { name: "chevronRight"; size: 16; strokeWidth: 2.2; color: reorderHint.tint; anchors.verticalCenter: parent.verticalCenter }
            Text { text: "to reorder   ·"; color: reorderHint.tint; font.pixelSize: reorderHint.fontSize; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: (Qt.platform.os === "osx" ? "⌘" : "Ctrl+") + "click / Shift+click selects several blocks to move or edit together   ·   "
                      + page.altKey + "←/→ moves the selection   ·   right-click for more"
                color: reorderHint.tint
                font.pixelSize: reorderHint.fontSize
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // block cards: drag the grip strip (or use the arrows / right-click menu /
        // Alt+←/→) to reorder, trash to delete
        ListView {
            id: blockList
            x: 10
            y: 354
            width: parent.width - 20
            height: 166
            orientation: ListView.Horizontal
            spacing: 8
            leftMargin: 6       // room for the drop indicator before the first card
            rightMargin: 6
            clip: true
            model: Builder.intervals
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded; height: 6 }
            // while dragging keep every card alive: the dragged card owns the
            // mouse grab and must not be destroyed when auto-scroll moves it out of view
            cacheBuffer: dragFrom >= 0 ? Math.max(320, count * (cardWidth + spacing)) : 320

            readonly property int cardWidth: 150
            property int dragFrom: -1       // index of the dragged card, -1 = no drag
            property int dropIndex: -1
            property real dragX: 0          // pointer x in content coordinates
            property real dragViewX: 0      // pointer x in view coordinates
            property string dragName
            property int dragZone: 0

            // Card under content x. Uses the cards' real positions: after moves
            // and inserts ListView may shift all cards (originX), so x can't be
            // computed as index × card width.
            function cardAt(contentPos) {
                var y = cardWidth / 4
                var i = indexAt(contentPos, y)
                if (i < 0)
                    i = indexAt(contentPos - spacing, y)   // in the gap between two cards
                if (i < 0)
                    i = indexAt(contentPos + spacing, y)
                if (i < 0) {                                // before the first / after the last card
                    var last = itemAtIndex(count - 1)
                    i = last && contentPos >= last.x ? count - 1 : 0
                }
                return i
            }
            function updateDrop(viewX) {
                dragViewX = viewX
                dragX = contentItem.mapFromItem(blockList, viewX, 0).x
                dropIndex = Math.max(0, Math.min(count - 1, cardAt(dragX)))
            }
            // auto-scroll while the pointer is near an edge, faster closer to it
            Timer {
                interval: 16
                repeat: true
                running: blockList.dragFrom >= 0
                         && (blockList.dragViewX < 60 || blockList.dragViewX > blockList.width - 60)
                onTriggered: {
                    var v = blockList.dragViewX
                    var edge = v < 60 ? 60 - v : v - (blockList.width - 60)
                    var step = Math.min(40, 4 + edge / 2) * (v < 60 ? -1 : 1)
                    var minX = blockList.originX - blockList.leftMargin
                    var maxX = blockList.originX + blockList.contentWidth + blockList.rightMargin - blockList.width
                    if (maxX <= minX)
                        return
                    blockList.contentX = Math.max(minX, Math.min(maxX, blockList.contentX + step))
                    blockList.updateDrop(v)
                }
            }
            property bool dragGroup: false   // dragging a multi-selection
            function endDrag(commit) {
                if (commit && dragFrom >= 0 && dropIndex >= 0 && dropIndex !== dragFrom) {
                    if (dragGroup) {
                        // drop next to an unselected card: the group goes before it
                        // (dragging left) or after it (dragging right)
                        if (page.selRows.indexOf(dropIndex) < 0) {
                            var before = 0
                            for (var i = 0; i < page.selCount; ++i)
                                if (page.selRows[i] < dropIndex)
                                    ++before
                            var dest = dropIndex - before + (dropIndex > dragFrom ? 1 : 0)
                            Builder.moveSelection(dest)
                            positionViewAtIndex(Math.min(Builder.blockCount - 1, dest), ListView.Contain)
                        }
                    } else {
                        page.moveTo(dragFrom, dropIndex)
                    }
                }
                dragFrom = -1
                dropIndex = -1
                dragGroup = false
            }

            delegate: Item {
                id: blockDelegate
                width: blockList.cardWidth
                height: 160
                readonly property bool isPrimary: index === Builder.selectedIndex
                readonly property bool isSel: page.selRows.indexOf(index) >= 0
                readonly property bool dragged: blockList.dragFrom === index
                                                || (blockList.dragGroup && blockList.dragFrom >= 0 && isSel)

                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    color: blockDelegate.isSel ? Theme.cardStrong : Theme.card
                    border.width: blockDelegate.dragged ? 2 : blockDelegate.isPrimary ? 3 : blockDelegate.isSel ? 2 : 1
                    border.color: blockDelegate.dragged ? Theme.accentLight
                                : blockDelegate.isSel ? Theme.accent : Theme.cardBorder
                    opacity: blockDelegate.dragged ? 0.45 : 1

                    MouseArea {
                        id: cardMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton)
                                page.openBlockMenu(index)
                            else
                                page.clickBlock(index, mouse.modifiers)
                        }
                    }

                    // grip strip (drag to reorder); the zone colour bar sits inside it
                    Rectangle {
                        id: gripStrip
                        x: 4
                        y: 4
                        width: parent.width - 8
                        height: 26
                        radius: 7
                        // faint at rest so it reads as a handle, stronger on hover
                        color: gripMouse.containsMouse || gripMouse.pressed ? Theme.hover
                             : (Theme.dark ? Qt.rgba(1, 1, 1, 0.03) : Qt.rgba(0.1, 0.2, 0.6, 0.03))
                        border.width: 1
                        border.color: gripMouse.containsMouse || gripMouse.pressed ? Theme.accentLight : Theme.cardBorder

                        Icon {
                            x: 2
                            anchors.verticalCenter: parent.verticalCenter
                            name: "grip"
                            size: 20
                            color: gripMouse.containsMouse || gripMouse.pressed ? Theme.text : Theme.textSecondary
                        }
                        Rectangle {
                            x: 26
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - 32
                            height: 10
                            radius: 5
                            color: Theme.zoneColor(zone)
                        }

                        MouseArea {
                            id: gripMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            preventStealing: true
                            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                            property real pressX: 0
                            onPressed: (mouse) => {
                                pressX = mouse.x
                                // grabbing a card of a multi-selection drags the whole group
                                if (mouse.modifiers & (Qt.ShiftModifier | Qt.ControlModifier | Qt.MetaModifier))
                                    page.clickBlock(index, mouse.modifiers)
                                else if (!page.inGroup(index))
                                    Builder.selectedIndex = index
                            }
                            onPositionChanged: (mouse) => {
                                if (!pressed)
                                    return
                                if (blockList.dragFrom < 0) {
                                    if (Math.abs(mouse.x - pressX) < 5)
                                        return
                                    blockList.dragGroup = page.inGroup(index)
                                    blockList.dragName = blockList.dragGroup ? page.selCount + " blocks" : name
                                    blockList.dragZone = zone
                                    blockList.dragFrom = index
                                }
                                blockList.updateDrop(mapToItem(blockList, mouse.x, mouse.y).x)
                            }
                            onReleased: blockList.endDrag(true)
                            onCanceled: blockList.endDrag(false)
                        }
                        ToolTip.visible: gripMouse.containsMouse && !gripMouse.pressed
                        ToolTip.delay: 500
                        ToolTip.text: page.inGroup(index) ? "Drag to move the " + page.selCount + " selected blocks" : "Drag to reorder"
                    }

                    Text {
                        x: 12
                        y: 36
                        width: parent.width - 20
                        text: name
                        color: Theme.text
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        elide: Text.ElideRight
                    }
                    Column {
                        x: 12
                        y: 62
                        spacing: 6
                        Row {
                            spacing: 8
                            Icon { name: "clock"; size: 16; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: durationText; color: Theme.text; font.pixelSize: 15 }
                        }
                        Row {
                            spacing: 8
                            Icon { name: "bolt"; size: 16; color: zone === 3 ? Theme.warning : Theme.text; anchors.verticalCenter: parent.verticalCenter }
                            Text {
                                text: isRamp ? percent + "–" + endPercent + "%" : percent + "%"
                                color: Theme.text
                                font.pixelSize: 15
                            }
                        }
                    }
                    // move earlier / later
                    Row {
                        x: 5
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 6
                        spacing: 2
                        IconButton {
                            flat: true; iconName: "chevronLeft"; iconSize: 18
                            enabled: page.canMoveEarlier(index)
                            tooltip: page.inGroup(index) ? "Move the " + page.selCount + " selected blocks earlier" : "Move earlier (left)"
                            onClicked: page.shift(index, -1)
                        }
                        IconButton {
                            flat: true; iconName: "chevronRight"; iconSize: 18
                            enabled: page.canMoveLater(index)
                            tooltip: page.inGroup(index) ? "Move the " + page.selCount + " selected blocks later" : "Move later (right)"
                            onClicked: page.shift(index, 1)
                        }
                    }
                    Row {
                        anchors.right: parent.right
                        anchors.rightMargin: 5
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 6
                        spacing: 2
                        IconButton {
                            flat: true; iconName: "copy"; iconSize: 18
                            tooltip: page.inGroup(index) ? "Duplicate the " + page.selCount + " selected blocks" : "Duplicate interval"
                            onClicked: page.duplicate(index)
                        }
                        IconButton {
                            flat: true; iconName: "trash"; iconSize: 18; iconColor: Theme.danger
                            tooltip: page.inGroup(index) ? "Delete the " + page.selCount + " selected blocks" : "Delete interval"
                            onClicked: page.requestDelete(index)
                        }
                    }
                }

                // drop indicator: where the dragged card will land
                Rectangle {
                    visible: blockList.dragFrom >= 0 && blockList.dropIndex === index && !blockDelegate.dragged
                             && !(blockList.dragGroup && blockDelegate.isSel)
                    width: 4
                    height: parent.height
                    radius: 2
                    x: blockList.dragFrom > index ? -6 : parent.width + 2
                    color: Theme.accentLight
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: -1
                        width: 12; height: 12; radius: 6
                        color: Theme.accentLight
                    }
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: -1
                        width: 12; height: 12; radius: 6
                        color: Theme.accentLight
                    }
                }
            }

            // card that follows the pointer while dragging
            Rectangle {
                visible: blockList.dragFrom >= 0
                z: 10
                x: blockList.dragX - width / 2
                y: 14
                width: blockList.cardWidth
                height: 44
                radius: 10
                color: Theme.cardStrong
                border.width: 2
                border.color: Theme.accent
                opacity: 0.95
                Icon {
                    x: 6
                    anchors.verticalCenter: parent.verticalCenter
                    name: "grip"
                    size: 20
                    color: Theme.text
                }
                Rectangle {
                    x: 30
                    y: 8
                    width: parent.width - 40
                    height: 6
                    radius: 3
                    color: Theme.zoneColor(blockList.dragZone)
                }
                Text {
                    x: 30
                    y: 18
                    width: parent.width - 38
                    text: blockList.dragName
                    color: Theme.text
                    font.pixelSize: 15
                    font.weight: Font.Bold
                    elide: Text.ElideRight
                }
            }
        }

        AppButton {
            x: 18
            width: parent.width - 36
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 14
            height: 50
            kind: "dashed"
            iconName: "plus"
            text: "Add Interval"
            onClicked: Builder.addInterval()
        }
    }

    // --- add block --------------------------------------------------------------
    Card {
        id: addBlockCard
        x: Theme.pageMargin
        width: timelineCard.width
        height: 150
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.pageMargin
        title: "Add Block"
        titleSize: 22

        RowLayout {
            x: 18
            y: 58
            width: parent.width - 36
            height: 78
            spacing: 12
            Repeater {
                model: [
                    { type: "warmup", label: "Warm Up", bars: [0.4, 0.6, 0.8], color: "#3d86fb" },
                    { type: "steady", label: "Steady", bars: [0.7, 0.7, 0.7, 0.7], color: "#25d3e3" },
                    { type: "intervals", label: "Intervals", bars: [0.5, 0.8, 0.6, 1.0], color: "#ffd21f" },
                    { type: "recovery", label: "Recovery", bars: [0.6, 0.9, 0.6], color: "#45d659" },
                    { type: "cooldown", label: "Cool Down", bars: [0.9, 0.6, 0.4], color: "#3d86fb" },
                    { type: "ramp", label: "Ramp / Steps", bars: [0.4, 0.6, 0.8, 1.0], color: "#ff3b30" }
                ]
                Rectangle {
                    id: addTile
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 10
                    color: blockMouse.containsMouse ? Theme.cardStrong : Theme.card
                    border.color: Theme.cardBorder
                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: 12
                        height: 26
                        spacing: 3
                        Repeater {
                            model: modelData.bars
                            Rectangle {
                                width: 8
                                height: 26 * modelData
                                anchors.bottom: parent.bottom
                                radius: 1
                                color: addTile.blockColor
                            }
                        }
                    }
                    readonly property color blockColor: modelData.color
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 10
                        text: modelData.label
                        color: Theme.text
                        font.pixelSize: 16
                    }
                    MouseArea {
                        id: blockMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (modelData.type === "intervals")
                                intervalSetDialog.open()
                            else if (modelData.type === "ramp")
                                rampDialog.open()
                            else
                                Builder.addBlock(modelData.type)
                        }
                    }
                }
            }
        }
    }

    // --- interval settings ------------------------------------------------------
    Card {
        id: settingsCard
        anchors.right: parent.right
        anchors.rightMargin: Theme.pageMargin
        y: 218
        width: 372
        height: parent.height - y - Theme.pageMargin
        title: "Interval Settings"
        titleSize: 22

        Item {
            x: 20
            y: 58
            width: parent.width - 40
            height: parent.height - 58 - 20
            visible: page.hasSel

            Rectangle {
                width: 8
                height: 52
                radius: 4
                color: Theme.zoneColor(page.sel.zone || 0)
            }
            TextField {
                id: blockName
                x: 22
                y: -6
                width: parent.width - 22
                height: 38
                // several blocks: the name field shows the selection (names stay per block)
                readOnly: page.multi
                text: page.multi ? page.selCount + " blocks selected" : (page.sel.name || "")
                color: Theme.text
                font.pixelSize: 21
                font.weight: Font.Bold
                leftPadding: 4
                selectByMouse: true
                background: Rectangle {
                    color: blockName.activeFocus && !page.multi ? Theme.field : "transparent"
                    radius: 6
                    border.width: blockName.activeFocus && !page.multi ? 1 : 0
                    border.color: Theme.accentLight
                }
                onEditingFinished: if (!page.multi) Builder.setBlockName(page.selIndex, text)
            }
            Text {
                x: 26
                y: 30
                width: parent.width - 26
                elide: Text.ElideRight
                text: {
                    if (page.multi)
                        return "Total " + Theme.clock(page.selDuration) + "  ·  changes apply to all"
                    switch (page.sel.type) {
                    case "warmup": return "Gradually raise the effort"
                    case "cooldown": return "Bring the effort down gently"
                    case "recovery": return page.rowing ? "Easy paddling between efforts" : "Easy spinning between efforts"
                    case "ramp": return page.sel.endPercent < page.sel.percent ? "Steadily decreasing power" : "Steadily increasing power"
                    case "interval": return "Build sustainable power"
                    default: return "Constant effort"
                    }
                }
                color: page.multi ? Theme.accentLight : Theme.textSecondary
                font.pixelSize: 15
            }

            ColumnLayout {
                y: 80
                width: parent.width
                spacing: 14

                // With several blocks selected: typed values are set on every
                // selected block, −/+ change every block by the same step.
                StepperField {
                    Layout.fillWidth: true
                    iconName: "clock"
                    label: "Duration"
                    sublabel: page.multi ? "each block" : ""
                    value: Theme.clock(page.sel.duration || 0)
                    onDecrement: page.forSelected(function(i, b) { Builder.setDuration(i, Theme.stepDuration(b.duration, -1)) })
                    onIncrement: page.forSelected(function(i, b) { Builder.setDuration(i, Theme.stepDuration(b.duration, 1)) })
                    onEdited: (text) => {
                        var ok = true
                        page.forSelected(function(i, b) { ok = Builder.setDurationText(i, text) && ok })
                        if (!ok)
                            page.message("Use mm:ss, e.g. 12:00")
                    }
                }
                StepperField {
                    Layout.fillWidth: true
                    iconName: "bolt"
                    iconColor: Theme.warning
                    label: page.showEndPower ? "Start Power" : "Target Power"
                    sublabel: page.sel.watts !== undefined ? "≈ " + page.sel.watts + " W" : ""
                    unit: "% " + page.ftpLabel
                    value: page.sel.percent !== undefined ? page.sel.percent : ""
                    onDecrement: page.forSelected(function(i, b) { Builder.setPercent(i, b.percent - 1) })
                    onIncrement: page.forSelected(function(i, b) { Builder.setPercent(i, b.percent + 1) })
                    onEdited: (text) => {
                        var v = parseInt(text)
                        if (!isNaN(v))
                            page.forSelected(function(i, b) { Builder.setPercent(i, v) })
                    }
                }
                StepperField {
                    Layout.fillWidth: true
                    visible: page.showEndPower
                    iconName: "trendUp"
                    iconColor: Theme.warning
                    label: "End Power"
                    sublabel: page.sel.endWatts !== undefined ? "≈ " + page.sel.endWatts + " W" : ""
                    unit: "% " + page.ftpLabel
                    value: page.sel.endPercent !== undefined ? page.sel.endPercent : ""
                    // only ramps (and warm ups / cool downs) of a multi-selection have an end power
                    function rampLike(b) {
                        return b.isRamp || b.type === "ramp" || b.type === "warmup" || b.type === "cooldown"
                    }
                    onDecrement: page.forSelected(function(i, b) { if (rampLike(b)) Builder.setEndPercent(i, b.endPercent - 1) })
                    onIncrement: page.forSelected(function(i, b) { if (rampLike(b)) Builder.setEndPercent(i, b.endPercent + 1) })
                    onEdited: (text) => {
                        var v = parseInt(text)
                        if (!isNaN(v))
                            page.forSelected(function(i, b) { if (rampLike(b)) Builder.setEndPercent(i, v) })
                    }
                }
                StepperField {
                    Layout.fillWidth: true
                    iconName: "target"
                    label: page.rowing ? "Stroke Rate (SPM)" : "Cadence (RPM)"
                    unit: page.rowing ? "SPM" : "RPM"
                    value: page.sel.cadence > 0 ? page.sel.cadence : "–"
                    onDecrement: page.forSelected(function(i, b) { Builder.setCadence(i, b.cadence > (page.rowing ? 16 : 40) ? b.cadence - 1 : 0) })
                    onIncrement: page.forSelected(function(i, b) { Builder.setCadence(i, b.cadence > 0 ? b.cadence + 1 : (page.rowing ? 22 : Profiles.preferredCadence)) })
                    onEdited: (text) => {
                        var v = parseInt(text)
                        page.forSelected(function(i, b) { Builder.setCadence(i, isNaN(v) ? 0 : v) })
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Icon { name: "heart"; color: Theme.danger; size: 26 }
                    Column {
                        Layout.preferredWidth: 86
                        Text { text: "RPE"; color: Theme.textSecondary; font.pixelSize: 16 }
                        Text { text: "(Perceived Effort)"; color: Theme.textMuted; font.pixelSize: 11 }
                    }
                    AppComboBox {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        model: Builder.rpeOptions
                        currentIndex: (page.sel.rpe !== undefined ? page.sel.rpe : -1) + 1
                        displayText: page.sel.rpe >= 0 ? Builder.rpeOptions[page.sel.rpe + 1]
                                                       : "Auto · RPE " + Theme.rpeLabels[page.sel.zone || 0]
                        accentColor: Theme.zoneColor(page.sel.zone || 0)
                        fontSize: 16
                        onActivated: (index) => page.forSelected(function(i, b) { Builder.setRpe(i, index - 1) })
                    }
                }
            }
        }

        Text {
            visible: !page.hasSel
            anchors.centerIn: parent
            text: "Select a block to edit it"
            color: Theme.textSecondary
            font.pixelSize: 17
        }

        // summary
        Rectangle {
            x: 20
            width: parent.width - 40
            height: 1
            anchors.bottom: summaryTitle.top
            anchors.bottomMargin: 14
            color: Theme.cardBorder
        }
        Text {
            id: summaryTitle
            x: 20
            anchors.bottom: summaryRow.top
            anchors.bottomMargin: 12
            text: "Workout Summary"
            color: Theme.text
            font.pixelSize: 19
            font.weight: Font.Bold
        }
        Text {
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.baseline: summaryTitle.baseline
            anchors.left: summaryTitle.right
            anchors.leftMargin: 10
            horizontalAlignment: Text.AlignRight
            // "% Rowing FTP" does not fit next to the title: shorter label for rowing
            text: Builder.intensity + "%" + (page.rowing ? "" : " " + page.ftpLabel) + "  •  TSS " + Builder.tss
            color: Theme.textSecondary
            font.pixelSize: 14
            elide: Text.ElideLeft
        }
        RowLayout {
            id: summaryRow
            x: 20
            width: parent.width - 40
            height: 78
            anchors.bottom: saveButton.top
            anchors.bottomMargin: 20
            spacing: 10
            StatTile {
                Layout.fillWidth: true
                Layout.fillHeight: true
                iconName: "clock"
                caption: "Total duration"
                value: Math.round(Builder.totalDurationSec / 60) + " min"
            }
            StatTile {
                Layout.fillWidth: true
                Layout.fillHeight: true
                iconName: "dashboard"
                iconColor: Theme.accentLight
                caption: "Blocks"
                value: Builder.blockCount + " blocks"
            }
        }
        AppButton {
            id: saveButton
            x: 20
            width: parent.width - 40
            height: 62
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            iconName: "save"
            text: Builder.dirty ? "Save Workout •" : "Save Workout"
            fontSize: 20
            onClicked: page.save()
        }
    }

    // --- preview dialog -------------------------------------------------------
    AppDialog {
        id: previewDialog
        title: Builder.name
        panelWidth: 980
        panelHeight: 560

        Text {
            x: 28
            y: 72
            text: Math.round(Builder.totalDurationSec / 60) + " min  •  " + Builder.blockCount + " blocks  •  "
                  + Builder.intensity + "% " + page.ftpLabel + "  •  estimated TSS " + Builder.tss + "  •  " + page.ftpLabel + " " + Builder.ftp + " W"
            color: Theme.textSecondary
            font.pixelSize: 17
        }
        ZoneLegend {
            x: 28
            y: 108
            showHeartRate: false
        }
        WorkoutChart {
            x: 14
            y: 146
            width: previewDialog.panelWidth - 40
            height: 300
            segments: Builder.segments
            totalSec: Builder.totalDurationSec
            showHeartRate: false
            ftp: Builder.ftp
        }
        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 24
            Item { Layout.fillWidth: true }
            AppButton {
                kind: "secondary"
                text: "Close"
                onClicked: previewDialog.close()
            }
            AppButton {
                iconName: "save"
                text: "Save Workout"
                onClicked: {
                    previewDialog.close()
                    page.save()
                }
            }
        }
    }

    // --- add ramp / interval set, delete a multi-selection -----------------------
    RampDialog {
        id: rampDialog
        onMessage: (text) => page.message(text)
    }
    IntervalSetDialog {
        id: intervalSetDialog
        onMessage: (text) => page.message(text)
    }
    ConfirmDialog {
        id: deleteGroupDialog
        title: "Delete " + page.selCount + " blocks?"
        message: "The " + page.selCount + " selected blocks (" + Theme.clock(page.selDuration) + ") are removed from the workout."
        acceptText: "Delete"
        acceptKind: "danger"
        onAccepted: Builder.removeSelection()
    }
}
