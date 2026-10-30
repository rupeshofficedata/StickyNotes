import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import StickyNotes

// Notes folder. The app follows the system color scheme and has no color setting.
Item {
    id: root

    property string error
    property string candidate

    FolderDialog {
        id: picker
        title: qsTr("Choose the notes folder")
        currentFolder: "file://" + NoteStore.folder
        onAccepted: {
            root.error = ""
            root.candidate = selectedFolder.toString().replace(/^file:\/\//, "")
            confirm.open()
        }
    }

    Dialog {
        id: confirm
        anchors.centerIn: parent
        modal: true
        title: qsTr("Use this folder?")
        standardButtons: Dialog.Cancel | Dialog.Ok
        ColumnLayout {
            Label { text: root.candidate; font.bold: true; wrapMode: Text.WrapAnywhere; Layout.maximumWidth: 280 }
            Label { text: qsTr("No notes are copied or moved. Notes in the old folder stay where they are."); wrapMode: Text.Wrap; Layout.maximumWidth: 280 }
        }
        onAccepted: {
            // Open notes save and close first, then the app switches folders.
            Lifecycle.closeAllNotes()
            if (NoteStore.changeFolder(root.candidate)) {
                NoteIndex.rescan()
                BinIndex.rescan()
            } else {
                root.error = qsTr("Cannot use this folder. It must exist and be writable.")
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label { text: qsTr("Notes folder"); font.bold: true }
        Label { Layout.fillWidth: true; text: NoteStore.folder; wrapMode: Text.WrapAnywhere }
        ErrorBanner { Layout.fillWidth: true; message: root.error }
        ErrorBanner {
            Layout.fillWidth: true
            message: NoteStore.available ? "" : qsTr("This folder is not available.")
        }
        RowLayout {
            Button { text: qsTr("Change folder…"); onClicked: picker.open() }
            Button { text: qsTr("Open notes folder"); onClicked: Qt.openUrlExternally("file://" + NoteStore.folder) }
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            opacity: 0.7
            text: qsTr("Changing the folder does not move any notes. Move the files in your file manager, then press Refresh.")
        }
        Item { Layout.fillHeight: true }
    }
}
