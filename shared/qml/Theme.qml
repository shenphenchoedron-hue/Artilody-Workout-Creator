pragma Singleton
import QtQuick

// Central design tokens taken from the mockups (docs/mockups).
QtObject {
    id: theme

    property bool dark: true

    // --- surfaces ---------------------------------------------------------
    readonly property color bgTop: dark ? "#1d2c74" : "#e9eefb"
    readonly property color bgBottom: dark ? "#25398f" : "#f5f7fd"
    readonly property color sidebar: dark ? "#18245e" : "#dfe6f8"
    readonly property color card: dark ? Qt.rgba(1, 1, 1, 0.055) : "#ffffff"
    readonly property color cardStrong: dark ? Qt.rgba(1, 1, 1, 0.09) : "#f1f4fc"
    readonly property color cardBorder: dark ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0.1, 0.16, 0.4, 0.12)
    readonly property color field: dark ? Qt.rgba(0.06, 0.1, 0.3, 0.45) : "#f4f6fd"
    readonly property color fieldBorder: dark ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(0.1, 0.16, 0.4, 0.18)
    readonly property color navSelected: dark ? Qt.rgba(0.27, 0.45, 1.0, 0.22) : Qt.rgba(0.16, 0.42, 1.0, 0.14)
    readonly property color hover: dark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0.1, 0.2, 0.6, 0.06)
    readonly property color grid: dark ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(0.1, 0.16, 0.4, 0.14)
    readonly property color dim: Qt.rgba(0.02, 0.04, 0.15, 0.55)

    // --- text ---------------------------------------------------------------
    readonly property color text: dark ? "#ffffff" : "#16214f"
    readonly property color textSecondary: dark ? Qt.rgba(1, 1, 1, 0.72) : Qt.rgba(0.09, 0.13, 0.31, 0.72)
    readonly property color textMuted: dark ? Qt.rgba(1, 1, 1, 0.5) : Qt.rgba(0.09, 0.13, 0.31, 0.5)
    readonly property color textOnColor: "#141a46"

    // --- accents ------------------------------------------------------------
    readonly property color accent: "#2178ff"
    readonly property color accentLight: "#4da3ff"
    readonly property color danger: "#f4513b"
    readonly property color success: "#3ddc5c"
    readonly property color warning: "#ffd21f"
    readonly property color orange: "#ff8a26"
    readonly property color purple: "#a259f7"
    readonly property color cyan: "#22d6e6"
    readonly property color heartRate: "#ff2fb4"
    readonly property color actualPower: "#38f2ff"
    readonly property color ghost: "#ff3fc8"        // ghost boat / lane (rowing)
    readonly property color pace: "#38d9f5"         // pace curve (rowing)
    readonly property color rowingColor: "#22d6e6"  // rowing sport accent
    readonly property color cyclingColor: accent    // cycling sport accent

    // Text-safe variants of the accents: the bright accents suit the dark
    // surfaces but are too pale for (small) text on the white light-theme cards.
    readonly property color warningText: dark ? warning : "#8a6400"
    readonly property color successText: dark ? success : "#17803a"
    readonly property color accentText: dark ? accentLight : "#1a5fd6"
    // Canvas strokes that are plain white on the dark theme (cursor, target line,
    // start / finish posts, lane markers): strong foreground on either theme.
    readonly property color strongLine: dark ? Qt.rgba(1, 1, 1, 0.9) : Qt.rgba(0.09, 0.13, 0.31, 0.85)
    readonly property color softLine: dark ? Qt.rgba(1, 1, 1, 0.45) : Qt.rgba(0.09, 0.13, 0.31, 0.4)
    readonly property color faintLine: dark ? Qt.rgba(1, 1, 1, 0.22) : Qt.rgba(0.09, 0.13, 0.31, 0.2)

    function sportColor(s) {
        return s === "rowing" ? rowingColor : cyclingColor
    }

    // Six power / RPE zones (Z1 … Z6)
    readonly property var zoneColors: ["#3d86fb", "#25d3e3", "#45d659", "#ffd21f", "#ff8a26", "#ff3b30"]
    readonly property var rpeLabels: ["1", "2–3", "4–6", "7–8", "9", "10"]
    readonly property var hrZoneColors: ["#7c8db5", "#3d86fb", "#45d659", "#ffd21f", "#ff8a26", "#ff3b30"]

    function zoneColor(z) {
        return zoneColors[Math.max(0, Math.min(5, z | 0))]
    }

    function categoryColor(category) {
        switch (category) {
        case "Endurance": return "#2f6ff0"
        case "Intervals": return "#f4513b"
        case "Recovery": return "#1fc8d8"
        case "FTP Tests": return "#a259f7"
        default: return "#2fbf71"
        }
    }

    // --- metrics ------------------------------------------------------------
    readonly property int radius: 14
    readonly property int radiusSmall: 9
    readonly property int spacing: 14
    readonly property int pageMargin: 22
    readonly property string fontFamily: Qt.application.font.family

    // --- menus (AppMenu / AppMenuItem, AppComboBox popups) ---------------------
    readonly property int menuFontSize: 16
    readonly property int menuItemHeight: 44
    readonly property color menuBackground: dark ? "#22327c" : "#ffffff"
    readonly property color menuBorder: dark ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(0.1, 0.16, 0.4, 0.14)

    // --- helpers ------------------------------------------------------------
    function clock(sec) {
        sec = Math.max(0, Math.floor(sec))
        var h = Math.floor(sec / 3600)
        var m = Math.floor((sec % 3600) / 60)
        var s = sec % 60
        var ss = s < 10 ? "0" + s : "" + s
        if (h > 0)
            return h + ":" + (m < 10 ? "0" + m : m) + ":" + ss
        return m + ":" + ss
    }

    function thousands(n) {
        return Math.round(n).toString().replace(/\B(?=(\d{3})+(?!\d))/g, ",")
    }

    // --- icons (24×24 SVG path data, drawn by Icon.qml) ---------------------
    readonly property var icons: ({
        "dashboard": { d: "M4 20h3V11H4z M10.5 20h3V4h-3z M17 20h3V8h-3z", fill: true },
        "calendar": "M4 6h16v14H4z M4 10.5h16 M8.5 3v4.5 M15.5 3v4.5",
        "plus": "M12 4.5v15 M4.5 12h15",
        "minus": "M5 12h14",
        "history": "M3 12a9 9 0 1 0 18 0a9 9 0 1 0 -18 0 M12 7v5l3.2 2",
        "clock": "M3.5 12a8.5 8.5 0 1 0 17 0a8.5 8.5 0 1 0 -17 0 M12 7.5v4.5l3 2",
        "bluetooth": "M7 7.5l10 9-5 4.5V3l5 4.5-10 9",
        "profile": "M3 12a9 9 0 1 0 18 0a9 9 0 1 0 -18 0 M9 10a3 3 0 1 0 6 0a3 3 0 1 0 -6 0 M6.3 18.4a7 7 0 0 1 11.4 0",
        "sun": "M8 12a4 4 0 1 0 8 0a4 4 0 1 0 -8 0 M12 2.5v2 M12 19.5v2 M4.9 4.9l1.4 1.4 M17.7 17.7l1.4 1.4 M2.5 12h2 M19.5 12h2 M4.9 19.1l1.4-1.4 M17.7 6.3l1.4-1.4",
        "moon": "M20.5 13.5A8.5 8.5 0 1 1 10.5 3.5a6.6 6.6 0 0 0 10 10z",
        "settings": "M9 12a3 3 0 1 0 6 0a3 3 0 1 0 -6 0 M12 2.8v2.4 M12 18.8v2.4 M2.8 12h2.4 M18.8 12h2.4 M5.5 5.5l1.7 1.7 M16.8 16.8l1.7 1.7 M5.5 18.5l1.7-1.7 M16.8 7.2l1.7-1.7 M5.8 12a6.2 6.2 0 1 0 12.4 0a6.2 6.2 0 1 0 -12.4 0",
        "chevronDown": "M6 9.5l6 6 6-6",
        "chevronUp": "M6 14.5l6-6 6 6",
        "chevronRight": "M9.5 6l6 6-6 6",
        "chevronLeft": "M14.5 6l-6 6 6 6",
        "bolt": { d: "M13.5 2L4.5 13.5h6.5L10 22l9-11.5h-6.5z", fill: true },
        "cadence": "M3.5 16.5a3 3 0 1 0 6 0a3 3 0 1 0 -6 0 M14.5 7.5a3 3 0 1 0 6 0a3 3 0 1 0 -6 0 M8.6 14.4l6.8-4.8",
        "heart": { d: "M12 21l-1.4-1.3C5.4 15 2 11.9 2 8.2 2 5.2 4.4 2.8 7.4 2.8c1.7 0 3.4.8 4.6 2.1 1.2-1.3 2.9-2.1 4.6-2.1 3 0 5.4 2.4 5.4 5.4 0 3.7-3.4 6.8-8.6 11.5z", fill: true },
        "heartOutline": "M12 20.5l-1.3-1.2C5.6 14.7 2.5 11.8 2.5 8.3c0-2.8 2.2-5 5-5 1.6 0 3.2.8 4.5 2.1 1.3-1.3 2.9-2.1 4.5-2.1 2.8 0 5 2.2 5 5 0 3.5-3.1 6.4-8.2 11z",
        "heartPulse": "M12 20.5l-1.3-1.2C5.6 14.7 2.5 11.8 2.5 8.3c0-2.8 2.2-5 5-5 1.6 0 3.2.8 4.5 2.1 1.3-1.3 2.9-2.1 4.5-2.1 2.8 0 5 2.2 5 5 0 3.5-3.1 6.4-8.2 11z M6 11.5h3l1.5-2.5 2 4.5 1.5-2h4",
        "timer": "M5 13.5a7 7 0 1 0 14 0a7 7 0 1 0 -14 0 M12 13.5V9.5 M9.5 2.5h5 M18 6.5l1.5-1.5",
        "document": "M6 2.5h9l4.5 4.5v14.5H6z M14.5 2.5V7.5h5 M9 12.5h7 M9 16.5h7",
        "trainer": "M5 16a3.5 3.5 0 1 0 7 0a3.5 3.5 0 1 0 -7 0 M8.5 16l3-7h4 M10 6h3 M14 9l3.5 7 M15.5 9l1-3h2 M4 21h16",
        "bike": "M2.5 16.5a3.5 3.5 0 1 0 7 0a3.5 3.5 0 1 0 -7 0 M14.5 16.5a3.5 3.5 0 1 0 7 0a3.5 3.5 0 1 0 -7 0 M6 16.5l3.5-7h5l3.5 7 M9.5 9.5L8 6H6 M12 16.5l2.5-7 M13.5 5.5a1.5 1.5 0 1 0 3 0a1.5 1.5 0 1 0 -3 0",
        "pulse": "M2 12h4.5l2.5-6.5 4 13 3-8.5 1.5 2H22",
        "pause": { d: "M6.5 4.5h3.5v15H6.5z M14 4.5h3.5v15H14z", fill: true },
        "play": { d: "M7 4.2l13 7.8-13 7.8z", fill: true },
        "stop": { d: "M6 6h12v12H6z", fill: true },
        "skipBack": { d: "M18.5 5L8.5 12l10 7z M5 5h2.5v14H5z", fill: true },
        "skipForward": { d: "M5.5 5l10 7-10 7z M16.5 5H19v14h-2.5z", fill: true },
        "music": "M9 18V5.5l11-2v12.5 M3.5 18a2.75 2.75 0 1 0 5.5 0a2.75 2.75 0 1 0 -5.5 0 M14.5 16a2.75 2.75 0 1 0 5.5 0a2.75 2.75 0 1 0 -5.5 0",
        "volume": "M4 9.5h4l5-4.5v14l-5-4.5H4z M16.5 9a4.5 4.5 0 0 1 0 6 M19.3 6.3a8.4 8.4 0 0 1 0 11.4",
        "mute": "M4 9.5h4l5-4.5v14l-5-4.5H4z M16.5 9.5l5 5 M21.5 9.5l-5 5",
        "dots": { d: "M3.5 12a1.75 1.75 0 1 0 3.5 0a1.75 1.75 0 1 0 -3.5 0 M10.25 12a1.75 1.75 0 1 0 3.5 0a1.75 1.75 0 1 0 -3.5 0 M17 12a1.75 1.75 0 1 0 3.5 0a1.75 1.75 0 1 0 -3.5 0", fill: true },
        "search": "M4 10.5a6.5 6.5 0 1 0 13 0a6.5 6.5 0 1 0 -13 0 M15.5 15.5l5 5",
        "check": "M5 12.5l4.5 4.5 9.5-10",
        "edit": "M4 20h4L19.5 8.5l-4-4L4 16z M13.5 6.5l4 4",
        "save": "M5 3.5h11l3.5 3.5v13.5H5z M8 3.5v5h7.5v-5 M8 20.5v-7h8v7",
        "trash": "M4 7h16 M9.5 7V4h5v3 M6.2 7l1 13.5h9.6l1-13.5 M10 11v6 M14 11v6",
        "copy": "M8.5 8.5h11.5v11.5H8.5z M4 15.5V4h11.5",
        "grip": { d: "M8 6a1.5 1.5 0 1 0 3 0a1.5 1.5 0 1 0 -3 0 M13 6a1.5 1.5 0 1 0 3 0a1.5 1.5 0 1 0 -3 0 M8 12a1.5 1.5 0 1 0 3 0a1.5 1.5 0 1 0 -3 0 M13 12a1.5 1.5 0 1 0 3 0a1.5 1.5 0 1 0 -3 0 M8 18a1.5 1.5 0 1 0 3 0a1.5 1.5 0 1 0 -3 0 M13 18a1.5 1.5 0 1 0 3 0a1.5 1.5 0 1 0 -3 0", fill: true },
        "refresh": "M20 12a8 8 0 1 1-2.6-5.9 M20.5 3.5v5h-5",
        "download": "M12 3.5v12 M7 10.5l5 5 5-5 M4.5 20h15",
        "import": "M12 3v11 M7.5 9.5l4.5 4.5 4.5-4.5 M4 13.5v6.5h16v-6.5",
        "export": "M12 14V3 M7.5 7.5L12 3l4.5 4.5 M4 13.5v6.5h16v-6.5",
        "share": "M12 14.5V3.5 M8 7.5l4-4 4 4 M8.5 10H6v10.5h12V10h-2.5",
        "link": "M10 14a4.5 4.5 0 0 0 6.4 0l3.2-3.2a4.5 4.5 0 0 0-6.4-6.4L12 5.6 M14 10a4.5 4.5 0 0 0-6.4 0l-3.2 3.2a4.5 4.5 0 0 0 6.4 6.4l1.2-1.2",
        "flame": { d: "M12 2c1.2 3.6 6.5 6 6.5 12a6.5 6.5 0 0 1-13 0c0-3.2 1.8-5.4 3.2-7.2.2 2.2 1.2 3.4 2.3 3.6-.3-3.2-.4-5.6 1-8.4z", fill: true },
        "info": "M3 12a9 9 0 1 0 18 0a9 9 0 1 0 -18 0 M12 11v6 M12 7.5v.2",
        "camera": "M3 8h4l2-3h6l2 3h4v11.5H3z M9 13.5a3 3 0 1 0 6 0a3 3 0 1 0 -6 0",
        "close": "M6 6l12 12 M18 6L6 18",
        "target": "M3.5 12a8.5 8.5 0 1 0 17 0a8.5 8.5 0 1 0 -17 0 M7.5 12a4.5 4.5 0 1 0 9 0a4.5 4.5 0 1 0 -9 0 M12 12h.1",
        "trendUp": "M3 17l6-6 4 4 8-8 M15 7h6v6",
        "signal": { d: "M3 20h3.5v-5H3z M9 20h3.5v-9H9z M15 20h3.5V5H15z", fill: true },
        "folder": "M3 6h6l2 2h10v11.5H3z",
        "playlist": "M4 6h12 M4 11h12 M4 16h7 M15.5 18.5V12l5-1",
        "shuffle": "M3 7h4l10 10h4 M3 17h4l3-3 M14 10l3-3h4 M18.5 4.5L21 7l-2.5 2.5 M18.5 14.5L21 17l-2.5 2.5",
        "repeat": "M4 11V9a2 2 0 0 1 2-2h13 M16 4l3 3-3 3 M20 13v2a2 2 0 0 1-2 2H5 M8 20l-3-3 3-3",
        "skipNext": "M5 5l9 7-9 7z M18 5v14",
        "user": "M8 8a4 4 0 1 0 8 0a4 4 0 1 0 -8 0 M4.5 20.5a7.5 7.5 0 0 1 15 0",
        "logout": "M15 4h4v16h-4 M10 8l-4 4 4 4 M6 12h10",
        "chart": "M4 4v16h16 M8 15l3-4 3 2 4-6",
        "calendarDays": "M4 6h16v14H4z M4 10.5h16 M8.5 3v4.5 M15.5 3v4.5 M8 14h.01 M12 14h.01 M16 14h.01 M8 17.2h.01 M12 17.2h.01",
        "rowing": "M4 20l9.5-9.5 M12 9l3 3 M15.2 5.8a2.6 2.6 0 0 1 3.7 0l.3.3a2.6 2.6 0 0 1 0 3.7L16.5 12.5 12.5 8.5z",
        "waves": "M3 9.5c2-2 4-2 6 0s4 2 6 0 4-2 6 0 M3 15c2-2 4-2 6 0s4 2 6 0 4-2 6 0",
        "ghost": "M5 20V10.5a7 7 0 0 1 14 0V20l-2.3-1.8-2.4 1.8-2.3-1.8-2.3 1.8-2.4-1.8z M9.5 11v.5 M14.5 11v.5",
        "flag": "M5 21V4 M5 4.5h12l-2.5 4 2.5 4H5",
        "mapPin": "M12 21s-6.5-6.2-6.5-11a6.5 6.5 0 0 1 13 0c0 4.8-6.5 11-6.5 11z M9.7 10a2.3 2.3 0 1 0 4.6 0a2.3 2.3 0 1 0-4.6 0",
        "gauge": "M4 17a8 8 0 1 1 16 0 M12 17l4-5.5 M7.2 10.6l1.2 1 M12 8v1.5 M16.8 10.6l-1.2 1",
        "trophy": "M8 4h8v6a4 4 0 0 1-8 0z M8 6H4.5a3.5 3.5 0 0 0 3.8 4 M16 6h3.5a3.5 3.5 0 0 1-3.8 4 M12 14v3.5 M8.5 20.5h7 M9.5 17.5h5v3h-5z"
    })
}
