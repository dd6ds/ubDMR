import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

// Echo test: pick one of the echo services (app.echoServices), hold PTT and
// speak; the service plays the transmission back so level and sound can be
// checked. The reply shows up in the activity card like any other call.
Flickable {
    id: page
    property var app

    contentHeight: column.height + 2 * app.spacingUnit
    clip: true

    readonly property var selected: app.echoServices[Math.min(app.settings.echoService,
                                                              app.echoServices.length - 1)]

    ColumnLayout {
        id: column
        x: app.spacingUnit
        y: app.spacingUnit
        width: page.width - 2 * app.spacingUnit
        spacing: app.spacingUnit

        AppLabel {
            Layout.fillWidth: true
            text: qsTr("Hold PTT, say a few words and release. The echo plays your "
                       + "transmission back a moment later.")
            color: app.colorSecondaryText
        }

        Repeater {
            model: app.echoServices
            ItemDelegate {
                Layout.fillWidth: true
                highlighted: index === app.settings.echoService
                onClicked: app.settings.echoService = index

                contentItem: Column {
                    AppLabel {
                        width: parent.width
                        text: modelData.id + "  " + modelData.name
                        font.bold: index === app.settings.echoService
                        color: index === app.settings.echoService ? app.colorAccent : app.colorText
                    }
                    AppLabel {
                        width: parent.width
                        text: qsTr("Private call")
                        font.pixelSize: app.fontSecondary
                        color: app.colorSecondaryText
                    }
                }
            }
        }

        ActivityCard {
            app: page.app
            Layout.fillWidth: true
        }

        PttButton {
            app: page.app
            Layout.alignment: Qt.AlignHCenter
            ready: Radio.online && Radio.vocoderReady
            onPressedDown: Radio.startTransmit(page.selected.id, true)
            onReleasedUp: Radio.stopTransmit()
        }

        MicGainControl {
            app: page.app
            Layout.fillWidth: true
            Layout.topMargin: app.spacingUnit
        }
    }
}
