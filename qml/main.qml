// Ubuntu Touch 20.04 ships Qt 5.12 - importing 2.15 makes the file fail to
// load. 2.12 is the highest version available on the device.
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import Qt.labs.settings 1.0

// Application window: scaling, the stored settings, the state the pages share
// (where a transmission goes, favourite talkgroups, echo services, contacts,
// the open chat) and the page stack with its tab bar.
ApplicationWindow {
    id: rootWindow
    visible: true
    width: Screen.width
    height: Screen.height
    title: "ubDMR"
    color: colorBackground

    // --- Scaling -----------------------------------------------------------
    readonly property real gu: gridUnitPx
    readonly property int fontTitle:     Math.round(gu * 2.0)
    readonly property int fontBody:      Math.round(gu * 1.6)
    readonly property int fontSecondary: Math.round(gu * 1.3)
    readonly property int spacingUnit:   Math.round(gu * 1.5)
    font.pixelSize: fontBody

    // --- Colours -----------------------------------------------------------
    // settings.themeMode: "system" follows the phone's Lomiri theme, "light"
    // and "dark" are fixed. Every colour in the app comes from here; the
    // palette below hands them to the stock controls (buttons, tabs, lists).
    readonly property bool darkMode: settings.themeMode === "dark"
                                     || (settings.themeMode === "system" && SystemTheme.dark)
    readonly property color colorBackground:      darkMode ? "#121212" : "#ffffff"
    readonly property color colorSurface:         darkMode ? "#1f2326" : "#eceff1"
    readonly property color colorFieldBackground: darkMode ? "#262a2d" : "#ffffff"
    readonly property color colorText:            darkMode ? "#eeeeee" : "#111111"
    readonly property color colorSecondaryText:   darkMode ? "#a8a8a8" : "#555555"
    readonly property color colorPlaceholder:     darkMode ? "#777777" : "#999999"
    readonly property color colorBorder:          darkMode ? "#4a4f53" : "#bbbbbb"
    readonly property color colorAccent:          darkMode ? "#4aa3f0" : "#0078d7"
    readonly property color colorReceive:         darkMode ? "#388e3c" : "#2e7d32"
    readonly property color colorTransmit:        darkMode ? "#d32f2f" : "#c62828"
    readonly property color colorWarning:         "#f9a825"
    readonly property color colorDisabled:        darkMode ? "#455a64" : "#b0bec5"

    palette.window: colorBackground
    palette.windowText: colorText
    palette.base: colorFieldBackground
    palette.alternateBase: colorSurface
    palette.text: colorText
    palette.button: colorSurface
    palette.buttonText: colorText
    palette.brightText: "#ffffff"
    palette.highlight: colorAccent
    palette.highlightedText: "#ffffff"
    palette.light: darkMode ? "#2c3134" : "#f5f5f5"
    palette.midlight: darkMode ? "#33383b" : "#e0e0e0"
    palette.mid: colorBorder
    palette.dark: darkMode ? "#5c6266" : "#9e9e9e"
    palette.shadow: "#000000"

    // --- Navigation --------------------------------------------------------
    // Position of each page in mainStack below.
    readonly property int pageRadio:      0
    readonly property int pageTalkgroups: 1
    readonly property int pageEcho:       2
    readonly property int pageChat:       3
    readonly property int pageContacts:   4
    readonly property int pageLastHeard:  5
    readonly property int pageSettings:   6

    function showPage(page) {
        tabBar.currentIndex = page
    }

    // --- Settings ----------------------------------------------------------
    Settings {
        id: settings
        property string host: "2621.master.brandmeister.network"
        property int port: 54006
        property string dmrId: ""
        property string password: ""
        property string callsign: ""
        property string vocoder: "builtin"
        property string vocoderHost: ""
        property int vocoderPort: 2460
        property bool autoConnect: false
        property int talkgroup: 262
        property string favorites: "91,92,262,2629,9990"
        property bool privateMode: false
        property int privateTarget: 0
        property int echoService: 0
        property string themeMode: "system"
        // JSON array of saved stations: [{ id, callsign, name }].
        property string contacts: "[]"
        // Text messages and unread counts, see MessageStore.
        property string messages: "[]"
        property string unreadMessages: "{}"
    }
    property alias settings: settings

    function connectNow() {
        Radio.connectToNetwork({
            host: settings.host, port: settings.port, dmrId: settings.dmrId,
            password: settings.password, callsign: settings.callsign,
            vocoder: settings.vocoder, vocoderHost: settings.vocoderHost,
            vocoderPort: settings.vocoderPort
        })
    }

    // --- Echo services --------------------------------------------------------
    // Called privately; tested on BrandMeister to echo the transmission back
    // unchanged.
    readonly property var echoServices: [
        { id: 9990, name: qsTr("BrandMeister parrot (worldwide)") },
        { id: 262997, name: qsTr("Parrot Germany") }
    ]

    function echoServiceName(id) {
        for (var i = 0; i < echoServices.length; ++i)
            if (echoServices[i].id === id)
                return echoServices[i].name
        return ""
    }

    // --- Text messages --------------------------------------------------------
    MessageStore {
        id: messageStore
        app: rootWindow
        settings: settings
    }
    property alias messages: messageStore

    // Conversation open on the chat page ("" = the list of conversations).
    property string chatKey: ""
    property int chatPeer: 0
    property bool chatGroup: false

    function openChat(peer, group) {
        chatPeer = peer
        chatGroup = group
        chatKey = messageStore.keyFor(peer, group)
        showPage(pageChat)
        messageStore.markRead(chatKey)
    }

    function closeChat() {
        chatKey = ""
    }

    function isReadingChat(key) {
        return tabBar.currentIndex === pageChat && chatKey === key
                && Qt.application.state === Qt.ApplicationActive
    }

    // Coming back to an open conversation marks it read.
    function markChatReadIfVisible() {
        if (chatKey.length && isReadingChat(chatKey))
            messageStore.markRead(chatKey)
    }

    Connections {
        target: Qt.application
        onStateChanged: rootWindow.markChatReadIfVisible()
    }

    // --- Contacts -------------------------------------------------------------
    readonly property var contactList: {
        try {
            var list = JSON.parse(settings.contacts)
            return Array.isArray(list) ? list : []
        } catch (e) {
            return []
        }
    }

    function isContact(id) {
        return contactList.some(function (c) { return c.id === id })
    }

    // station: { id, callsign, name } as IdLookup.searchFinished delivers it.
    function addContact(station) {
        if (station.id > 0 && !isContact(station.id)) {
            var list = contactList.concat([{ id: station.id, callsign: station.callsign,
                                              name: station.name }])
            list.sort(function (a, b) {
                return (a.callsign || "").localeCompare(b.callsign || "")
            })
            settings.contacts = JSON.stringify(list)
        }
    }

    function removeContact(id) {
        settings.contacts = JSON.stringify(contactList.filter(function (c) {
            return c.id !== id
        }))
    }

    // --- Favourite talkgroups ----------------------------------------------
    // Stored as "91,262,…" in settings.favorites, in the user's order.
    readonly property var favoriteList: settings.favorites.split(",").map(function (s) {
        return parseInt(s, 10)
    }).filter(function (tg) {
        return tg > 0
    })

    function isFavorite(tg) {
        return favoriteList.indexOf(tg) >= 0
    }

    function addFavorite(tg) {
        if (tg > 0 && !isFavorite(tg))
            settings.favorites = favoriteList.concat([tg]).join(",")
    }

    function removeFavorite(tg) {
        settings.favorites = favoriteList.filter(function (t) { return t !== tg }).join(",")
    }

    function listenTo(tg) {
        settings.talkgroup = tg
        Radio.setTalkgroup(tg)
    }

    // Private call target picked on another page (last heard, reply).
    function callPrivately(id) {
        settings.privateTarget = id
        settings.privateMode = true
        showPage(pageRadio)
    }

    // "DL5HZ" if known, else the bare ID.
    function stationName(id, call) {
        if (call && call.length)
            return call
        var service = echoServiceName(id)
        if (service.length)
            return service
        var contact = contactList.filter(function (c) { return c.id === id })[0]
        if (contact && contact.callsign)
            return contact.callsign
        var known = Radio.lookup.revision >= 0 ? Radio.lookup.callsign(id) : ""
        return known.length ? known : (id > 0 ? String(id) : qsTr("Unknown"))
    }

    // "TG 262 Deutschland"
    function talkgroupText(tg) {
        var name = Radio.lookup.revision >= 0 ? Radio.lookup.talkgroupName(tg) : ""
        return qsTr("TG %1").arg(tg) + (name.length ? " " + name : "")
    }

    Component.onCompleted: {
        Radio.setTalkgroup(settings.talkgroup)
        if (settings.autoConnect)
            connectNow()
        else
            showPage(pageSettings)
    }

    // --- Pages -------------------------------------------------------------
    footer: TabBar {
        id: tabBar
        // Seven tabs share the width: smaller text than the pages, short
        // labels and a gear for the settings.
        font.pixelSize: rootWindow.fontSecondary
        TabButton { text: qsTr("Radio") }
        TabButton { text: qsTr("TGs") }
        TabButton { text: qsTr("Echo") }
        TabButton {
            text: messageStore.unreadTotal > 0 ? qsTr("Chat (%1)").arg(messageStore.unreadTotal)
                                               : qsTr("Chat")
            font.bold: messageStore.unreadTotal > 0
        }
        TabButton { text: qsTr("Contacts") }
        TabButton { text: qsTr("Heard") }
        TabButton { text: "\u2699"; font.pixelSize: rootWindow.fontTitle }
    }

    // Order must match the page* constants above.
    StackLayout {
        anchors.fill: parent
        currentIndex: tabBar.currentIndex
        onCurrentIndexChanged: rootWindow.markChatReadIfVisible()

        RadioPage { app: rootWindow }
        TalkgroupsPage { app: rootWindow }
        EchoPage { app: rootWindow }
        ChatPage { app: rootWindow }
        ContactsPage { app: rootWindow }
        LastHeardPage { app: rootWindow }
        SettingsPage { app: rootWindow }
    }
}
