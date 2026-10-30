import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StickyNotes

// Search, note list, New note and Refresh. Settings and the Bin arrive in Stage 3.
Window {
    id: win

    width: 360
    height: 480
    title: qsTr("StickyNotes")
    color: palette.window

    // Refresh: look for notes and files changed by outside tools.
    function refresh() {
        NoteStore.checkOpenNotes()
        NoteIndex.rescan()
        BinIndex.rescan()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        TabBar {
            id: tabs
            Layout.fillWidth: true
            TabButton { text: qsTr("Notes") }
            TabButton { text: qsTr("Bin") }
            TabButton { text: qsTr("Settings") }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

        ColumnLayout {
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                TextField {
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search notes")
                    onTextChanged: NoteIndex.setFilter(text)
                }
                Button { text: qsTr("New note"); onClicked: Lifecycle.newNote() }
                Button { text: qsTr("Refresh"); onClicked: win.refresh() }
            }

            ErrorBanner {
                id: trayNotice
                property bool dismissed: false
                Layout.fillWidth: true
                message: dismissed || Lifecycle.trayPresent ? "" : qsTr("No system tray found. The app quits when you close the last window.")
                actions: [qsTr("OK")]
                onActionTriggered: dismissed = true
            }

            ErrorBanner {
                Layout.fillWidth: true
                message: NoteStore.available ? "" : qsTr("Notes folder unavailable: %1").arg(NoteStore.folder)
            }

            Label {
                Layout.fillWidth: true
                visible: NoteStore.available && list.count === 0
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("No notes in this folder")
                opacity: 0.6
            }

            ListView {
                id: list
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 4
                model: NoteIndex

                delegate: Rectangle {
                    id: row
                    required property string noteId
                    required property string title
                    required property string snippet
                    required property string colorToken
                    required property bool isError

                    width: ListView.view.width
                    height: 54
                    radius: 4
                    color: isError ? "#B3261E" : Theme.background(colorToken)

                    Column {
                        anchors { fill: parent; margins: 6 }
                        Label { width: parent.width; text: row.title; elide: Text.ElideRight; font.bold: true; color: row.isError ? "white" : Theme.foreground() }
                        Label { width: parent.width; text: row.snippet; elide: Text.ElideRight; font.pixelSize: 11; color: row.isError ? "white" : Theme.foreground(); opacity: 0.8 }
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton)
                                menu.popup()
                            else if (!row.isError)
                                Lifecycle.openNote(row.noteId)
                        }
                    }
                    Menu {
                        id: menu
                        MenuItem { text: qsTr("Delete"); onTriggered: { NoteStore.moveToBin(row.noteId); NoteIndex.rescan() } }
                    }
                }
            }
        }


            BinView {}
            SettingsPage {}
        }
    }

    // The window hides instead of closing. AppLifecycle decides whether the app quits.
    onClosing: (close) => {
        close.accepted = false
        Lifecycle.primaryCloseRequested()
    }
    onVisibleChanged: visible ? Lifecycle.windowShown("primary", "") : Lifecycle.windowHidden("primary", "")
}
