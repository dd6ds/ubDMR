import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

// Recent calls, newest first. Tapping a station prepares a private call to it.
ListView {
    id: list
    property var app

    clip: true
    model: Radio.lastHeard

    AppLabel {
        anchors.centerIn: parent
        visible: list.count === 0
        text: qsTr("Nothing heard yet")
        color: app.colorSecondaryText
    }

    delegate: ItemDelegate {
        width: list.width
        enabled: !model.outgoing && model.sourceId > 0
        onClicked: app.callPrivately(model.sourceId)

        contentItem: RowLayout {
            spacing: app.spacingUnit

            Rectangle {
                width: app.gu
                Layout.fillHeight: true
                color: model.outgoing ? app.colorTransmit
                                      : model.active ? app.colorReceive : "transparent"
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                AppLabel {
                    Layout.fillWidth: true
                    text: (model.outgoing ? "↑ " : "") + app.stationName(model.sourceId,
                                                                         model.sourceCall)
                    font.bold: true
                }
                AppLabel {
                    Layout.fillWidth: true
                    visible: text.length > 0
                    text: Radio.lookup.revision >= 0
                          ? Radio.lookup.operatorText(model.sourceId) : ""
                    font.pixelSize: app.fontSecondary
                    color: app.colorSecondaryText
                }
                AppLabel {
                    Layout.fillWidth: true
                    text: model.isPrivate ? qsTr("Private call")
                                          : app.talkgroupText(model.destinationId)
                    font.pixelSize: app.fontSecondary
                    color: app.colorSecondaryText
                }
            }

            ColumnLayout {
                spacing: 0
                AppLabel {
                    Layout.alignment: Qt.AlignRight
                    text: model.time
                    font.pixelSize: app.fontSecondary
                }
                AppLabel {
                    Layout.alignment: Qt.AlignRight
                    text: model.active ? qsTr("live") : qsTr("%1 s").arg(model.duration)
                    font.pixelSize: app.fontSecondary
                    color: model.active ? app.colorReceive : app.colorSecondaryText
                }
            }
        }
    }
}
