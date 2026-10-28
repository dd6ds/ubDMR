import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

// One station in a list: callsign, name/location and DMR ID, with an
// optional extra button (e.g. write a message) and one action button on the
// right (save, remove).
ItemDelegate {
    id: row
    property var app
    // { id, callsign, name }
    property var station
    property string actionText
    property bool actionEnabled: true
    // Empty = no extra button.
    property string extraText
    signal actionClicked()
    signal extraClicked()

    contentItem: RowLayout {
        spacing: app.spacingUnit

        Column {
            Layout.fillWidth: true
            AppLabel {
                width: parent.width
                text: (row.station.callsign || "?") + "  ·  " + row.station.id
                font.bold: true
            }
            AppLabel {
                width: parent.width
                visible: text.length > 0
                text: row.station.name || ""
                font.pixelSize: app.fontSecondary
                color: app.colorSecondaryText
            }
        }
        Button {
            visible: row.extraText.length > 0
            text: row.extraText
            onClicked: row.extraClicked()
        }
        Button {
            text: row.actionText
            enabled: row.actionEnabled
            onClicked: row.actionClicked()
        }
    }
}
