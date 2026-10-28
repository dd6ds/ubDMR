import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

// Station search and saved contacts. The search asks radioid.net for a
// callsign (DL5U* finds all starting with DL5U) or a DMR ID; results can be
// saved. Tapping a station prepares a private call to it, ✉ writes it a
// message.
Flickable {
    id: page
    property var app

    contentHeight: column.height + 2 * app.spacingUnit
    clip: true

    property var results: []
    property bool searching: false
    property string searchError: ""

    function startSearch() {
        if (queryField.text.trim().length === 0)
            return
        searching = true
        searchError = ""
        results = []
        Radio.lookup.search(queryField.text)
    }

    Connections {
        target: Radio.lookup
        onSearchFinished: {
            page.searching = false
            page.results = results
            page.searchError = error
        }
    }

    ColumnLayout {
        id: column
        x: app.spacingUnit
        y: app.spacingUnit
        width: page.width - 2 * app.spacingUnit
        spacing: app.gu * 0.6

        RowLayout {
            Layout.fillWidth: true
            AppField {
                id: queryField
                Layout.fillWidth: true
                placeholderText: qsTr("Callsign or DMR ID")
                inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhNoPredictiveText
                onAccepted: page.startSearch()
            }
            Button {
                text: qsTr("Search")
                enabled: !page.searching && queryField.text.trim().length > 0
                onClicked: page.startSearch()
            }
        }

        AppLabel {
            Layout.fillWidth: true
            visible: text.length > 0
            color: page.searchError.length ? app.colorTransmit : app.colorSecondaryText
            font.pixelSize: app.fontSecondary
            text: page.searching ? qsTr("Searching…")
                  : page.searchError.length ? page.searchError : ""
        }

        Repeater {
            model: page.results
            StationRow {
                app: page.app
                Layout.fillWidth: true
                station: modelData
                actionText: app.isContact(modelData.id) ? qsTr("Saved") : qsTr("Save")
                actionEnabled: !app.isContact(modelData.id)
                onActionClicked: app.addContact(modelData)
                onClicked: app.callPrivately(modelData.id)
            }
        }

        AppLabel {
            Layout.topMargin: app.spacingUnit
            text: qsTr("Contacts")
            font.bold: true
            font.pixelSize: app.fontTitle
        }

        AppLabel {
            Layout.fillWidth: true
            visible: app.contactList.length === 0
            text: qsTr("No contacts yet. Search for a callsign above and save it.")
            color: app.colorSecondaryText
        }

        Repeater {
            model: app.contactList
            StationRow {
                app: page.app
                Layout.fillWidth: true
                station: modelData
                extraText: "\u2709"
                onExtraClicked: app.openChat(modelData.id, false)
                actionText: "\u2715"
                onActionClicked: app.removeContact(modelData.id)
                onClicked: app.callPrivately(modelData.id)
            }
        }
    }
}
