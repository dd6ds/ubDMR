import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

// Text messages: a contact picker to start writing and the list of
// conversations, or the open conversation (ChatView) when app.chatKey is set.
Item {
    id: page
    property var app

    function conversationTitle(peer, group) {
        return group ? app.talkgroupText(peer) : app.stationName(peer, "")
    }

    ChatView {
        anchors.fill: parent
        visible: app.chatKey.length > 0
        app: page.app
        title: page.conversationTitle(app.chatPeer, app.chatGroup)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: app.spacingUnit
        visible: app.chatKey.length === 0
        spacing: app.gu * 0.6

        ComboBox {
            id: contactBox
            Layout.fillWidth: true
            visible: app.contactList.length > 0
            model: app.contactList
            // Nothing chosen: the box is only a way to start a conversation.
            currentIndex: -1
            displayText: qsTr("Write to a contact…")
            onActivated: {
                app.openChat(app.contactList[index].id, false)
                currentIndex = -1
            }

            contentItem: AppLabel {
                leftPadding: app.gu
                text: contactBox.displayText
                wrapMode: Text.NoWrap
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                implicitHeight: app.gu * 4
                color: app.colorFieldBackground
                border.color: contactBox.activeFocus ? app.colorAccent : app.colorBorder
                radius: 4
            }
            delegate: ItemDelegate {
                width: contactBox.width
                highlighted: contactBox.highlightedIndex === index
                contentItem: Column {
                    AppLabel {
                        text: (modelData.callsign || "?") + "  ·  " + modelData.id
                        font.bold: true
                    }
                    AppLabel {
                        visible: text.length > 0
                        text: modelData.name || ""
                        font.pixelSize: app.fontSecondary
                        color: app.colorSecondaryText
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: app.contactList.length === 0
            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Save stations on the Contacts page to write to them.")
                color: app.colorSecondaryText
            }
            Button {
                text: qsTr("Contacts")
                onClicked: app.showPage(app.pageContacts)
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: app.messages.conversations

            AppLabel {
                anchors.centerIn: parent
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                visible: list.count === 0
                text: qsTr("No messages yet.")
                color: app.colorSecondaryText
            }

            delegate: ItemDelegate {
                width: list.width
                onClicked: app.openChat(modelData.peer, modelData.group)

                contentItem: RowLayout {
                    spacing: app.spacingUnit
                    Column {
                        Layout.fillWidth: true
                        AppLabel {
                            width: parent.width
                            text: page.conversationTitle(modelData.peer, modelData.group)
                            font.bold: modelData.unread > 0
                            elide: Text.ElideRight
                            wrapMode: Text.NoWrap
                        }
                        AppLabel {
                            width: parent.width
                            text: modelData.lastText
                            color: app.colorSecondaryText
                            font.pixelSize: app.fontSecondary
                            elide: Text.ElideRight
                            wrapMode: Text.NoWrap
                        }
                    }
                    Rectangle {
                        visible: modelData.unread > 0
                        width: Math.max(height, unreadText.implicitWidth + app.gu)
                        height: app.gu * 2.2
                        radius: height / 2
                        color: app.colorAccent
                        Label {
                            id: unreadText
                            anchors.centerIn: parent
                            text: modelData.unread
                            color: "#ffffff"
                            font.pixelSize: app.fontSecondary
                        }
                    }
                    ToolButton {
                        text: "✕"
                        palette.buttonText: app.colorSecondaryText
                        onClicked: app.messages.deleteConversation(modelData.key)
                    }
                }
            }
        }
    }
}
