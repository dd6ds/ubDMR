import QtQuick 2.12

// Text messages: storage, conversations and unread counts. Messages are kept
// as JSON in settings.messages (newest last, at most kMaxMessages), unread
// counts per conversation in settings.unreadMessages. A conversation is a
// station (private) or a talkgroup (group), keyed "p<id>" / "g<id>".
QtObject {
    id: store
    property var app
    property var settings

    readonly property int kMaxMessages: 500

    readonly property var messages: parse(settings.messages, [])
    readonly property var unread: parse(settings.unreadMessages, {})

    // Newest conversation first: [{ key, peer, group, lastText, lastTime, unread }].
    readonly property var conversations: {
        var byKey = {}
        for (var i = 0; i < messages.length; ++i) {
            var m = messages[i]
            byKey[m.key] = { key: m.key, peer: m.peer, group: m.group, lastText: m.text,
                             lastTime: m.time, unread: unread[m.key] || 0 }
        }
        var list = Object.keys(byKey).map(function (k) { return byKey[k] })
        list.sort(function (a, b) { return b.lastTime - a.lastTime })
        return list
    }

    readonly property int unreadTotal: {
        var n = 0
        for (var k in unread)
            n += unread[k]
        return n
    }

    function parse(json, fallback) {
        try {
            var value = JSON.parse(json)
            return value !== null && typeof value === "object" ? value : fallback
        } catch (e) {
            return fallback
        }
    }

    function keyFor(peer, group) {
        return (group ? "g" : "p") + peer
    }

    function messagesOf(key) {
        return messages.filter(function (m) { return m.key === key })
    }

    function add(message) {
        var list = messages.concat([message])
        settings.messages = JSON.stringify(list.slice(-kMaxMessages))
    }

    function markRead(key) {
        if (!unread[key])
            return
        var u = Object.assign({}, unread)
        delete u[key]
        settings.unreadMessages = JSON.stringify(u)
    }

    function deleteConversation(key) {
        settings.messages = JSON.stringify(messages.filter(function (m) { return m.key !== key }))
        markRead(key)
    }

    // False when the radio is offline (nothing is stored then).
    function send(peer, group, text) {
        if (!Radio.sendText(peer, group, text))
            return false
        add({ key: keyFor(peer, group), peer: peer, group: group, outgoing: true,
              from: 0, text: text.trim().substring(0, Radio.maxTextLength),
              time: Date.now(), status: group ? "" : "sent" })
        return true
    }

    function setStatusOfLastSent(peer, status) {
        var list = messages.slice()
        for (var i = list.length - 1; i >= 0; --i) {
            if (list[i].key === keyFor(peer, false) && list[i].status === "sent") {
                list[i] = Object.assign({}, list[i], { status: status })
                settings.messages = JSON.stringify(list)
                return
            }
        }
    }

    property Connections radioEvents: Connections {
        target: Radio

        onTextReceived: {
            // Group messages belong to the talkgroup, private ones to the sender.
            var peer = group ? destination : source
            var key = store.keyFor(peer, group)
            store.add({ key: key, peer: peer, group: group, outgoing: false, from: source,
                        text: text, time: Date.now(), status: "" })
            if (!store.app.isReadingChat(key)) {
                var u = Object.assign({}, store.unread)
                u[key] = (u[key] || 0) + 1
                store.settings.unreadMessages = JSON.stringify(u)
            }
        }

        onTextDelivered: store.setStatusOfLastSent(destination, success ? "delivered" : "failed")
    }
}
