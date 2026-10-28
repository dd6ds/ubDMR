import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

// Favourite talkgroups: add by number, tap to listen, ✕ to remove. The
// favourites also appear as quick buttons on the radio page.
ColumnLayout {
    id: page
    property var app

    spacing: app.spacingUnit

    function addEntered() {
        var tg = parseInt(tgInput.text, 10)
        if (tg > 0) {
            app.addFavorite(tg)
            tgInput.text = ""
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.margins: app.spacingUnit
        Layout.bottomMargin: 0

        AppField {
            id: tgInput
            Layout.fillWidth: true
            placeholderText: qsTr("Talkgroup number")
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator { bottom: 1 }
            onAccepted: page.addEntered()
        }
        Button {
            text: qsTr("Add")
            enabled: parseInt(tgInput.text, 10) > 0
            onClicked: page.addEntered()
        }
    }

    ListView {
        id: list
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: app.favoriteList

        AppLabel {
            anchors.centerIn: parent
            width: parent.width - 2 * app.spacingUnit
            horizontalAlignment: Text.AlignHCenter
            visible: list.count === 0
            text: qsTr("No favourite talkgroups yet. Add one above or with ☆ on the radio page.")
            color: app.colorSecondaryText
        }

        delegate: ItemDelegate {
            width: list.width
            onClicked: {
                app.listenTo(modelData)
                app.showPage(app.pageRadio)
            }

            contentItem: RowLayout {
                spacing: app.spacingUnit

                AppLabel {
                    Layout.fillWidth: true
                    text: app.talkgroupText(modelData)
                    font.bold: modelData === Radio.talkgroup
                    color: modelData === Radio.talkgroup ? app.colorReceive : app.colorText
                }
                ToolButton {
                    text: "✕"
                    font.pixelSize: app.fontTitle
                    palette.buttonText: app.colorSecondaryText
                    onClicked: app.removeFavorite(modelData)
                }
            }
        }
    }
}
