import QtQuick 2.12
import QtQuick.Controls 2.12

// Round push-to-talk button: transmits while held. While transmitting, a
// ring around it shrinks with the time left before the transmit time-out
// cuts the transmission off, and the time left is shown inside.
Item {
    id: ptt
    property var app
    property bool ready: false
    signal pressedDown()
    signal releasedUp()

    readonly property real ringWidth: app.gu * 0.9
    // 1 = whole time-out left, 0 = transmission is cut off.
    property real remaining: 1

    implicitWidth: app.gu * 16
    implicitHeight: implicitWidth

    function remainingText() {
        var secs = Math.ceil(remaining * Radio.transmitLimitMs / 1000)
        return Math.floor(secs / 60) + ":" + ("0" + secs % 60).slice(-2)
    }

    Timer {
        interval: 200
        repeat: true
        running: Radio.transmitting
        triggeredOnStart: true
        onTriggered: {
            var used = (Date.now() - Radio.callStartedMs) / Radio.transmitLimitMs
            ptt.remaining = Math.max(0, 1 - used)
            ring.requestPaint()
        }
        onRunningChanged: if (!running) ptt.remaining = 1
    }

    Canvas {
        id: ring
        anchors.fill: parent
        visible: Radio.transmitting
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var r = (width - ptt.ringWidth) / 2
            ctx.lineWidth = ptt.ringWidth
            ctx.strokeStyle = app.colorBorder
            ctx.beginPath()
            ctx.arc(width / 2, height / 2, r, 0, 2 * Math.PI)
            ctx.stroke()
            ctx.strokeStyle = ptt.remaining > 0.5 ? app.colorReceive
                            : ptt.remaining > 0.2 ? app.colorWarning : app.colorTransmit
            ctx.beginPath()
            // From 12 o'clock clockwise; the end moves back towards 12 o'clock.
            ctx.arc(width / 2, height / 2, r, -Math.PI / 2,
                    -Math.PI / 2 + 2 * Math.PI * ptt.remaining)
            ctx.stroke()
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: ptt.ringWidth * 1.5
        radius: width / 2
        color: !ptt.ready ? app.colorDisabled
                          : Radio.transmitting ? app.colorTransmit : "#37474f"
        border.width: app.gu * 0.4
        border.color: Radio.transmitting ? "#ff8a80" : "#90a4ae"

        Column {
            anchors.centerIn: parent
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Radio.transmitting ? qsTr("TX") : qsTr("PTT")
                color: "#ffffff"
                font.pixelSize: app.fontTitle * 1.6
                font.bold: true
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: Radio.transmitting
                text: ptt.remainingText()
                color: "#ffffff"
                font.pixelSize: app.fontSecondary
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: ptt.ready
        onPressed: ptt.pressedDown()
        onReleased: ptt.releasedUp()
        onCanceled: ptt.releasedUp()
    }
}
