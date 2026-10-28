import QtQuick 2.12
import QtQuick.Layouts 1.12

// The call in progress (or the last one): who is talking, where to and for
// how long. Green while receiving, red while transmitting.
Rectangle {
    id: card
    property var app

    readonly property bool active: Radio.receiving || Radio.transmitting
    property int elapsedSecs: 0

    radius: app.gu
    color: Radio.transmitting ? app.colorTransmit
                              : Radio.receiving ? app.colorReceive : app.colorSurface
    implicitHeight: content.implicitHeight + 2 * app.spacingUnit

    readonly property color textColor: active ? "#ffffff" : app.colorText

    function destinationText() {
        if (Radio.callDestination <= 0)
            return ""
        return Radio.callPrivate ? qsTr("Private call to %1").arg(Radio.callDestination)
                                 : app.talkgroupText(Radio.callDestination)
    }

    Timer {
        interval: 500
        repeat: true
        running: card.active
        triggeredOnStart: true
        onTriggered: card.elapsedSecs = Math.floor((Date.now() - Radio.callStartedMs) / 1000)
    }

    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: app.spacingUnit
        spacing: app.gu * 0.4

        AppLabel {
            Layout.fillWidth: true
            visible: !Radio.callSourceId && !card.active
            text: qsTr("No activity yet")
            color: app.colorSecondaryText
            horizontalAlignment: Text.AlignHCenter
        }

        AppLabel {
            Layout.fillWidth: true
            visible: Radio.callSourceId > 0 || card.active
            text: app.stationName(Radio.callSourceId, Radio.callSourceCall)
            font.pixelSize: app.fontTitle * 1.5
            font.bold: true
            color: card.textColor
            horizontalAlignment: Text.AlignHCenter
        }

        AppLabel {
            Layout.fillWidth: true
            visible: text.length > 0
            text: Radio.lookup.revision >= 0 ? Radio.lookup.operatorText(Radio.callSourceId) : ""
            color: card.textColor
            horizontalAlignment: Text.AlignHCenter
        }

        AppLabel {
            Layout.fillWidth: true
            visible: text.length > 0
            text: card.destinationText()
            font.pixelSize: app.fontSecondary
            color: card.textColor
            horizontalAlignment: Text.AlignHCenter
        }

        AppLabel {
            Layout.fillWidth: true
            visible: card.active
            text: (Radio.transmitting ? qsTr("Transmitting") : qsTr("Receiving"))
                  + "  " + Math.floor(card.elapsedSecs / 60) + ":"
                  + ("0" + card.elapsedSecs % 60).slice(-2)
            font.pixelSize: app.fontSecondary
            color: card.textColor
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
