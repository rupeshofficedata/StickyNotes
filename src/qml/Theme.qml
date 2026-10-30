pragma Singleton
import QtQuick

// Note colors are stored as tokens. Each token has a fixed background for light and dark;
// the text color only depends on the scheme.
QtObject {
    readonly property bool dark: Qt.styleHints.colorScheme === Qt.ColorScheme.Dark
    readonly property var tokens: ["default", "blue", "green", "pink"]
    readonly property var table: ({
        "default": { light: "#FFF2A8", dark: "#1C1C1C" },
        "blue":    { light: "#CFE8FF", dark: "#1F2A36" },
        "green":   { light: "#D4F3D0", dark: "#1F2D20" },
        "pink":    { light: "#FFD6E5", dark: "#36222B" }
    })

    function background(token) {
        const e = table[token] || table["default"]
        return dark ? e.dark : e.light
    }
    function foreground() {
        return dark ? "#EDEDED" : "#1E1E1E"
    }
}
