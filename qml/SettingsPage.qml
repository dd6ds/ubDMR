import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

// Language, light/dark appearance, login data for BrandMeister and the
// vocoder, plus connect/disconnect.
Flickable {
    id: page
    property var app

    contentHeight: column.height + 2 * app.spacingUnit
    clip: true

    ColumnLayout {
        id: column
        x: app.spacingUnit
        y: app.spacingUnit
        width: page.width - 2 * app.spacingUnit
        spacing: app.gu * 0.6

        AppLabel { text: qsTr("Language"); font.bold: true; font.pixelSize: app.fontTitle }
        LanguageBox {
            app: page.app
            Layout.fillWidth: true
        }

        AppLabel { text: qsTr("Appearance"); font.bold: true; font.pixelSize: app.fontTitle }
        Repeater {
            model: [
                { mode: "system", label: qsTr("Like the phone") },
                { mode: "light", label: qsTr("Light") },
                { mode: "dark", label: qsTr("Dark") }
            ]
            RadioButton {
                id: themeChoice
                Layout.fillWidth: true
                // text must be set: without it the style centres the indicator.
                text: modelData.label
                checked: app.settings.themeMode === modelData.mode
                onToggled: if (checked) app.settings.themeMode = modelData.mode
                contentItem: AppLabel {
                    leftPadding: themeChoice.indicator.width + themeChoice.spacing
                    text: themeChoice.text
                }
            }
        }

        AppLabel { text: qsTr("BrandMeister"); font.bold: true; font.pixelSize: app.fontTitle }

        AppLabel { text: qsTr("Master server") }
        AppField {
            Layout.fillWidth: true
            text: app.settings.host
            inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
            onEditingFinished: app.settings.host = text.trim()
        }

        AppLabel { text: qsTr("Port") }
        AppField {
            Layout.fillWidth: true
            text: String(app.settings.port)
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator { bottom: 1; top: 65535 }
            onEditingFinished: app.settings.port = parseInt(text, 10)
        }

        AppLabel { text: qsTr("DMR ID") }
        AppField {
            Layout.fillWidth: true
            text: app.settings.dmrId
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator { bottom: 1 }
            onEditingFinished: app.settings.dmrId = text.trim()
        }

        AppLabel { text: qsTr("Callsign") }
        AppField {
            Layout.fillWidth: true
            text: app.settings.callsign
            inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhNoPredictiveText
            onEditingFinished: app.settings.callsign = text.trim().toUpperCase()
        }

        AppLabel { text: qsTr("Hotspot password (BrandMeister SelfCare)") }
        AppField {
            Layout.fillWidth: true
            text: app.settings.password
            echoMode: TextInput.Password
            inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
            onEditingFinished: app.settings.password = text
        }

        AppLabel { text: qsTr("Vocoder"); font.bold: true; font.pixelSize: app.fontTitle }

        RadioButton {
            id: builtinVocoder
            Layout.fillWidth: true
            text: qsTr("Built in (no extra hardware)")
            checked: app.settings.vocoder !== "ambeserver"
            onToggled: if (checked) app.settings.vocoder = "builtin"
            contentItem: AppLabel {
                leftPadding: builtinVocoder.indicator.width + builtinVocoder.spacing
                text: builtinVocoder.text
            }
        }
        RadioButton {
            id: serverVocoder
            Layout.fillWidth: true
            text: qsTr("AMBEserver (DV3000/ThumbDV, better sound)")
            checked: app.settings.vocoder === "ambeserver"
            onToggled: if (checked) app.settings.vocoder = "ambeserver"
            contentItem: AppLabel {
                leftPadding: serverVocoder.indicator.width + serverVocoder.spacing
                text: serverVocoder.text
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: serverVocoder.checked
            spacing: app.gu * 0.6

            AppLabel { text: qsTr("AMBEserver host") }
            AppField {
                Layout.fillWidth: true
                text: app.settings.vocoderHost
                placeholderText: "192.168.1.20"
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
                onEditingFinished: app.settings.vocoderHost = text.trim()
            }

            AppLabel { text: qsTr("UDP port") }
            AppField {
                Layout.fillWidth: true
                text: String(app.settings.vocoderPort)
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator { bottom: 1; top: 65535 }
                onEditingFinished: app.settings.vocoderPort = parseInt(text, 10)
            }
        }

        RowLayout {
            CheckBox {
                checked: app.settings.autoConnect
                onToggled: app.settings.autoConnect = checked
            }
            AppLabel { text: qsTr("Connect when the app starts") }
        }

        AppLabel {
            Layout.fillWidth: true
            visible: Radio.statusText.length > 0
            text: Radio.statusText
            color: app.colorTransmit
        }

        Button {
            Layout.fillWidth: true
            text: Radio.online || Radio.connecting ? qsTr("Disconnect") : qsTr("Connect")
            onClicked: {
                if (Radio.online || Radio.connecting) {
                    Radio.disconnectFromNetwork()
                } else {
                    app.connectNow()
                    app.showPage(app.pageRadio)
                }
            }
        }

        AppLabel {
            Layout.fillWidth: true
            Layout.topMargin: app.spacingUnit
            font.pixelSize: app.fontSecondary
            color: app.colorSecondaryText
            text: qsTr("Transmitting needs a valid amateur radio licence. "
                       + "Ubuntu Touch pauses apps in the background; to keep listening "
                       + "with the screen off, exempt ubDMR from suspension "
                       + "(UT Tweak Tool → Apps & Scopes).")
        }
    }
}
