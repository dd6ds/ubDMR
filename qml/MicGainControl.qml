import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

// Microphone gain slider with level meter, an assessment of the level and a
// button that runs the microphone without transmitting.
ColumnLayout {
    id: control
    property var app
    spacing: app.gu * 0.6

    // Peak of the last few seconds, falling slowly, for the assessment.
    property real recentPeakDb: -60
    readonly property bool active: Radio.micTesting || Radio.transmitting
    Connections {
        target: Radio
        onMicLevelChanged: control.recentPeakDb = Math.max(Radio.micLevelDb,
                                                           control.recentPeakDb - 0.15)
        onMicTestingChanged: control.recentPeakDb = -60
    }

    // The test runs only while the page that started it is shown.
    onVisibleChanged: if (!visible && Radio.micTesting) Radio.stopMicTest()

    function gainText(db) {
        return (db > 0 ? "+" : db < 0 ? "−" : "") + Math.abs(Math.round(db)) + " dB"
    }

    function assessment() {
        if (!active)
            return qsTr("Speak as you would on air: the bar should reach green, but not red.")
        if (Radio.micLimited || recentPeakDb > -3)
            return qsTr("Too loud: lower the gain.")
        if (recentPeakDb > -20)
            return qsTr("Good level.")
        if (recentPeakDb > -45)
            return qsTr("Too quiet: raise the gain.")
        return qsTr("Hardly any signal: speak, or check the microphone.")
    }

    RowLayout {
        Layout.fillWidth: true
        AppLabel {
            Layout.fillWidth: true
            text: qsTr("Microphone gain")
            font.bold: true
        }
        // Current gain as a badge in the accent colour.
        Rectangle {
            implicitWidth: gainLabel.implicitWidth + app.gu * 1.6
            implicitHeight: gainLabel.implicitHeight + app.gu * 0.6
            radius: height / 2
            color: app.colorAccent
            AppLabel {
                id: gainLabel
                anchors.centerIn: parent
                text: control.gainText(gainSlider.value)
                color: "#ffffff"
                font.bold: true
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: app.gu * 0.6

        RoundButton {
            text: "−"
            font.pixelSize: app.fontTitle
            font.bold: true
            enabled: gainSlider.value > gainSlider.from
            onClicked: Radio.micGainDb = gainSlider.value - 1
            autoRepeat: true
        }

        Slider {
            id: gainSlider
            Layout.fillWidth: true
            from: Radio.micGainMinDb
            to: Radio.micGainMaxDb
            stepSize: 1
            snapMode: Slider.SnapAlways
            value: Radio.micGainDb
            onMoved: Radio.micGainDb = value
            implicitHeight: app.gu * 4.5

            // Thick track, filled in the accent colour up to the handle.
            background: Rectangle {
                x: gainSlider.leftPadding
                y: gainSlider.topPadding + gainSlider.availableHeight / 2 - height / 2
                width: gainSlider.availableWidth
                height: app.gu * 1.2
                radius: height / 2
                color: app.colorSurface
                border.color: app.colorBorder
                border.width: 1

                Rectangle {
                    width: gainSlider.visualPosition * parent.width
                    height: parent.height
                    radius: parent.radius
                    color: app.colorAccent
                }
                // Mark at 0 dB (no change to the microphone signal).
                Rectangle {
                    x: parent.width * (0 - gainSlider.from) / (gainSlider.to - gainSlider.from)
                    width: 2
                    height: parent.height + app.gu
                    anchors.verticalCenter: parent.verticalCenter
                    color: app.colorSecondaryText
                }
            }

            // Big round handle with grip lines.
            handle: Rectangle {
                x: gainSlider.leftPadding + gainSlider.visualPosition
                   * (gainSlider.availableWidth - width)
                y: gainSlider.topPadding + gainSlider.availableHeight / 2 - height / 2
                width: app.gu * 3.6
                height: width
                radius: width / 2
                color: gainSlider.pressed ? Qt.darker(app.colorAccent, 1.2) : app.colorAccent
                border.color: "#ffffff"
                border.width: app.gu * 0.35

                Row {
                    anchors.centerIn: parent
                    spacing: app.gu * 0.35
                    Repeater {
                        model: 3
                        Rectangle {
                            width: Math.max(1, app.gu * 0.2)
                            height: app.gu * 1.2
                            color: "#ffffff"
                        }
                    }
                }
            }
        }

        RoundButton {
            text: "+"
            font.pixelSize: app.fontTitle
            font.bold: true
            enabled: gainSlider.value < gainSlider.to
            onClicked: Radio.micGainDb = gainSlider.value + 1
            autoRepeat: true
        }
    }

    // Scale under the slider: quieter … louder.
    RowLayout {
        Layout.fillWidth: true
        AppLabel {
            text: qsTr("quieter")
            font.pixelSize: app.fontSecondary
            color: app.colorSecondaryText
        }
        Item { Layout.fillWidth: true }
        AppLabel {
            text: qsTr("louder")
            font.pixelSize: app.fontSecondary
            color: app.colorSecondaryText
        }
    }

    AppLabel {
        Layout.fillWidth: true
        Layout.topMargin: app.gu * 0.4
        text: qsTr("Level")
        font.pixelSize: app.fontSecondary
        color: app.colorSecondaryText
    }

    MicMeter {
        app: control.app
        Layout.fillWidth: true
        opacity: control.active ? 1 : 0.4
    }

    AppLabel {
        Layout.fillWidth: true
        text: control.assessment()
        font.pixelSize: app.fontSecondary
        color: control.active && (Radio.micLimited || control.recentPeakDb > -3)
               ? app.colorTransmit : app.colorSecondaryText
    }

    Button {
        Layout.fillWidth: true
        enabled: !Radio.transmitting
        text: Radio.micTesting ? qsTr("Stop microphone test")
                               : qsTr("Test microphone (no transmission)")
        onClicked: Radio.micTesting ? Radio.stopMicTest() : Radio.startMicTest()
    }
}
