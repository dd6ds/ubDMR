import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

// Main page: link status, the monitored talkgroup with favourites, the call
// in progress and push-to-talk to the talkgroup or a private station.
Flickable {
    id: page
    property var app

    contentHeight: column.height + 2 * app.spacingUnit
    clip: true

    readonly property int txDestination: app.settings.privateMode ? app.settings.privateTarget
                                                                   : Radio.talkgroup

    function statusLine() {
        var link = Radio.online ? qsTr("Online") : Radio.connecting ? qsTr("Connecting…")
                                                                    : qsTr("Offline")
        if (app.settings.vocoder !== "ambeserver")
            return link
        return link + " · " + (Radio.vocoderReady ? qsTr("AMBEserver ready")
                                                  : qsTr("no AMBEserver"))
    }

    function applyTalkgroup() {
        var tg = parseInt(tgField.text, 10)
        if (tg > 0)
            app.listenTo(tg)
    }

    ColumnLayout {
        id: column
        x: app.spacingUnit
        y: app.spacingUnit
        width: page.width - 2 * app.spacingUnit
        spacing: app.spacingUnit

        RowLayout {
            Layout.fillWidth: true
            spacing: app.gu

            Rectangle {
                width: app.gu * 1.2
                height: width
                radius: width / 2
                color: Radio.online && Radio.vocoderReady ? app.colorReceive
                       : Radio.online || Radio.connecting ? app.colorWarning : app.colorTransmit
            }
            AppLabel {
                Layout.fillWidth: true
                text: page.statusLine()
                font.pixelSize: app.fontSecondary
            }
        }

        AppLabel {
            Layout.fillWidth: true
            visible: Radio.statusText.length > 0
            text: Radio.statusText
            color: app.colorTransmit
            font.pixelSize: app.fontSecondary
        }

        AppLabel {
            text: qsTr("Listening to %1").arg(app.talkgroupText(Radio.talkgroup))
            Layout.fillWidth: true
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            AppField {
                id: tgField
                Layout.fillWidth: true
                placeholderText: qsTr("Talkgroup")
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator { bottom: 1 }
                text: String(Radio.talkgroup)
                onAccepted: page.applyTalkgroup()
            }
            Button {
                text: qsTr("Listen")
                onClicked: page.applyTalkgroup()
            }
            ToolButton {
                // ★ / ☆: keep the current talkgroup among the favourites or not.
                text: app.isFavorite(Radio.talkgroup) ? "\u2605" : "\u2606"
                font.pixelSize: app.fontTitle
                palette.buttonText: app.colorWarning
                enabled: Radio.talkgroup > 0
                onClicked: app.isFavorite(Radio.talkgroup) ? app.removeFavorite(Radio.talkgroup)
                                                           : app.addFavorite(Radio.talkgroup)
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: app.gu
            Repeater {
                model: app.favoriteList
                Button {
                    text: String(modelData)
                    highlighted: modelData === Radio.talkgroup
                    onClicked: app.listenTo(modelData)
                }
            }
        }

        ActivityCard {
            app: page.app
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            CheckBox {
                id: privateBox
                checked: app.settings.privateMode
                onToggled: app.settings.privateMode = checked
            }
            AppLabel {
                text: qsTr("Private call to")
            }
            AppField {
                Layout.fillWidth: true
                enabled: privateBox.checked
                placeholderText: qsTr("DMR ID")
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator { bottom: 1 }
                text: app.settings.privateTarget > 0 ? String(app.settings.privateTarget) : ""
                onTextChanged: app.settings.privateTarget = parseInt(text, 10) || 0
            }
        }

        Button {
            Layout.fillWidth: true
            visible: Radio.lastPrivateCaller > 0
            text: qsTr("Reply to %1").arg(app.stationName(Radio.lastPrivateCaller, ""))
            onClicked: app.callPrivately(Radio.lastPrivateCaller)
        }

        AppLabel {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: app.fontSecondary
            color: app.colorSecondaryText
            text: page.txDestination > 0
                  ? (app.settings.privateMode
                     ? qsTr("Talk to %1").arg(app.stationName(page.txDestination, ""))
                     : qsTr("Talk on %1").arg(app.talkgroupText(page.txDestination)))
                  : qsTr("Choose where to talk")
        }

        PttButton {
            app: page.app
            Layout.alignment: Qt.AlignHCenter
            ready: Radio.online && Radio.vocoderReady && page.txDestination > 0
            onPressedDown: Radio.startTransmit(page.txDestination, app.settings.privateMode)
            onReleasedUp: Radio.stopTransmit()
        }

        MicMeter {
            app: page.app
            Layout.fillWidth: true
            Layout.leftMargin: app.gu * 6
            Layout.rightMargin: app.gu * 6
            opacity: Radio.transmitting ? 1 : 0
        }
    }
}
