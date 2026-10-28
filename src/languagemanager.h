// The UI language as QML sees it ("Language"): follows the phone's language
// unless the user picked another one, and switches the running UI at once.
// Translations are embedded as :/i18n/ubdmr_<code>.qm.

#ifndef LANGUAGEMANAGER_H
#define LANGUAGEMANAGER_H

#include <QObject>
#include <QTranslator>
#include <QVariantList>

class QQmlEngine;

class LanguageManager : public QObject
{
    Q_OBJECT
    // Code of the chosen language, "" when following the system.
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    // [{ code, name }] of every language the app ships, names in that language.
    Q_PROPERTY(QVariantList languages READ languages CONSTANT)

public:
    explicit LanguageManager(QObject *parent = nullptr);

    QString language() const { return m_language; }
    QVariantList languages() const { return m_languages; }

    // Installs the stored choice; call before QML is loaded.
    void applyStoredLanguage();
    // Re-evaluates every qsTr() in QML after a switch.
    void setEngine(QQmlEngine *engine) { m_engine = engine; }

    Q_INVOKABLE void setLanguage(const QString &code);

signals:
    void languageChanged();

private:
    void install();

    QQmlEngine *m_engine = nullptr;
    QTranslator m_translator;
    QString m_language;
    QVariantList m_languages;
};

#endif
