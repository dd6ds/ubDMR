import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

// One conversation: the field to write a new message at the top, where the
// on-screen keyboard cannot cover it, and the messages below it, newest
// first, as bubbles (own ones on the right, ✓ sent / ✓✓ delivered).
ColumnLayout {
    id: view
    property var app
    property string title

    spacing: 0

    // Newest first, so the latest messages sit right under the input field.
    readonly property var shown: app.messages.messagesOf(app.chatKey).reverse()

    property string sendError: ""

    // The on-screen keyboard keeps the word being typed as uncommitted
    // pre-edit text; it belongs to the message too.
    readonly property string typed: input.text + input.preeditText

    function sendNow() {
        if (!Radio.online) {
            sendError = qsTr("Offline: connect to send messages.")
            return
        }
        if (app.messages.send(app.chatPeer, app.chatGroup, typed)) {
            Qt.inputMethod.reset()
            input.text = ""
            sendError = ""
        } else {
            sendError = qsTr("The message could not be sent.")
        }
    }

    function statusMark(status) {
        return status === "delivered" ? " ✓✓" : status === "sent" ? " ✓"
             : status === "failed" ? " ✗" : ""
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.margins: app.gu * 0.5
        ToolButton {
            text: "‹"
            font.pixelSize: app.fontTitle * 1.4
            onClicked: app.closeChat()
        }
        AppLabel {
            Layout.fillWidth: true
            text: view.title
            font.bold: true
            elide: Text.ElideRight
            wrapMode: Text.NoWrap
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.margins: app.gu
        AppField {
            id: input
            Layout.fillWidth: true
            placeholderText: qsTr("Message")
            maximumLength: Radio.maxTextLength
            onAccepted: view.sendNow()
            onTextChanged: view.sendError = ""
        }
        AppLabel {
            text: Radio.maxTextLength - view.typed.length
            font.pixelSize: app.fontSecondary
            color: app.colorSecondaryText
        }
        Button {
            text: qsTr("Send")
            enabled: view.typed.trim().length > 0
            onClicked: view.sendNow()
        }
    }

    AppLabel {
        Layout.fillWidth: true
        Layout.leftMargin: app.spacingUnit
        visible: text.length > 0
        text: view.sendError.length ? view.sendError
                                    : !Radio.online ? qsTr("Offline: connect to send messages.") : ""
        color: app.colorTransmit
        font.pixelSize: app.fontSecondary
    }

    ListView {
        id: list
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: app.gu * 0.6
        model: view.shown
        onCountChanged: positionViewAtBeginning()

        delegate: Item {
            width: list.width
            height: bubble.height

            Rectangle {
                id: bubble
                x: modelData.outgoing ? parent.width - width - app.spacingUnit : app.spacingUnit
                width: Math.min(list.width * 0.8, body.implicitWidth + 2 * app.gu)
                height: body.implicitHeight + app.gu
                radius: app.gu
                color: modelData.outgoing ? app.colorAccent : app.colorSurface

                Column {
                    id: body
                    x: app.gu
                    y: app.gu * 0.5
                    width: bubble.width - 2 * app.gu
                    AppLabel {
                        visible: view.app.chatGroup && !modelData.outgoing
                        text: app.stationName(modelData.from, "")
                        font.bold: true
                        font.pixelSize: app.fontSecondary
                        color: app.colorAccent
                    }
                    AppLabel {
                        width: Math.min(implicitWidth, list.width * 0.8 - 2 * app.gu)
                        text: modelData.text
                        color: modelData.outgoing ? "#ffffff" : app.colorText
                    }
                    AppLabel {
                        anchors.right: parent.right
                        text: Qt.formatDateTime(new Date(modelData.time), "dd.MM. hh:mm")
                              + view.statusMark(modelData.status)
                        font.pixelSize: app.fontSecondary * 0.85
                        color: modelData.outgoing ? "#e0e0e0" : app.colorSecondaryText
                    }
                }
            }
        }
    }
}
