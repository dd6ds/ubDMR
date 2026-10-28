// UI language selection, see languagemanager.h.

#include "languagemanager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QLocale>
#include <QQmlEngine>
#include <QSettings>

namespace {

const QString kSettingsKey = QStringLiteral("language");
const QString kTranslationDir = QStringLiteral(":/i18n");
const QString kPrefix = QStringLiteral("ubdmr_");
// English is the source language and needs no translation file.
const QString kSourceLanguage = QStringLiteral("en");

QString nativeName(const QString &code)
{
    // QLocale("en") would say "American English".
    QString name = code == kSourceLanguage ? QStringLiteral("English")
                                           : QLocale(code).nativeLanguageName();
    if (!name.isEmpty())
        name[0] = name[0].toUpper();
    return name;
}

} // namespace

LanguageManager::LanguageManager(QObject *parent) : QObject(parent)
{
    QStringList codes{kSourceLanguage};
    const QStringList files = QDir(kTranslationDir).entryList({kPrefix + QStringLiteral("*.qm")});
    for (const QString &file : files)
        codes << file.mid(kPrefix.size()).section(QLatin1Char('.'), 0, 0);
    for (const QString &code : qAsConst(codes))
        m_languages << QVariantMap{{QStringLiteral("code"), code},
                                   {QStringLiteral("name"), nativeName(code)}};
}

void LanguageManager::applyStoredLanguage()
{
    m_language = QSettings().value(kSettingsKey).toString();
    install();
}

void LanguageManager::setLanguage(const QString &code)
{
    if (code == m_language)
        return;
    m_language = code;
    QSettings().setValue(kSettingsKey, code);
    install();
    emit languageChanged();
}

void LanguageManager::install()
{
    QCoreApplication::removeTranslator(&m_translator);
    const QLocale locale = m_language.isEmpty() ? QLocale::system() : QLocale(m_language);
    const bool loaded = m_translator.load(locale, QStringLiteral("ubdmr"), QStringLiteral("_"),
                                          kTranslationDir);
    if (loaded)
        QCoreApplication::installTranslator(&m_translator);
    qDebug() << "LanguageManager:" << (m_language.isEmpty() ? "system" : m_language) << "->"
             << locale.name() << (loaded ? "(translated)" : "(English)");
    if (m_engine)
        m_engine->retranslate();
}
