import QtQuick 2.12
import QtQuick.Controls 2.12

// Picks the UI language: the phone's own, or one of the languages the app
// ships. Texts and colours are explicit, see AppLabel.
ComboBox {
    id: box
    property var app

    // Entry 0 follows the system; the rest come from LanguageManager.
    readonly property var entries: [{ code: "", name: qsTr("System language") }]
                                   .concat(Language.languages)

    function indexOfCode(code) {
        for (var i = 0; i < entries.length; ++i)
            if (entries[i].code === code)
                return i
        return 0
    }

    model: entries
    currentIndex: indexOfCode(Language.language)
    displayText: entries[currentIndex].name
    onActivated: Language.setLanguage(entries[index].code)

    contentItem: AppLabel {
        leftPadding: app.gu
        text: box.displayText
        wrapMode: Text.NoWrap
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        implicitHeight: app.gu * 4
        color: app.colorFieldBackground
        border.color: box.activeFocus ? app.colorAccent : app.colorBorder
        radius: 4
    }

    delegate: ItemDelegate {
        width: box.width
        highlighted: box.highlightedIndex === index
        contentItem: AppLabel {
            text: modelData.name
            font.bold: index === box.currentIndex
        }
    }
}
