import QtQuick 2.12

// Microphone level bar: -60 dB full scale on the left, 0 dB on the right.
// Green is the range speech peaks should reach (-20 … -3 dB), red means the
// gain drives the limiter. The thin marks sit at -20 and -3 dB.
Item {
    id: meter
    property var app

    readonly property real floorDb: -60
    function position(db) { return Math.max(0, Math.min(1, (db - floorDb) / -floorDb)) }

    implicitHeight: app.gu * 1.6

    Rectangle {
        id: track
        anchors.fill: parent
        radius: height / 4
        color: app.colorSurface
        border.color: app.colorBorder
        border.width: 1
        clip: true

        Rectangle {
            height: parent.height
            width: parent.width * meter.position(Radio.micLevelDb)
            radius: track.radius
            color: Radio.micLimited || Radio.micLevelDb > -3 ? app.colorTransmit
                 : Radio.micLevelDb > -20 ? app.colorReceive : app.colorWarning
        }

        Repeater {
            model: [-20, -3]
            Rectangle {
                x: track.width * meter.position(modelData)
                width: 1
                height: track.height
                color: app.colorSecondaryText
            }
        }
    }
}
