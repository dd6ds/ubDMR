// Entry point: sets up logging, the UI language and the QML engine, and
// exposes the radio backend (RadioController) and the language choice
// (LanguageManager) and the phone's light/dark theme (SystemTheme) to QML
// as "Radio", "Language" and "SystemTheme".

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <QTextStream>
#include <QtQml>

#include <cstdio>

#include "languagemanager.h"
#include "radiocontroller.h"
#include "systemtheme.h"

// Confined click apps may only write below ~/.local/share/<package-name> and
// friends, so the log lives in the app's own data directory.
static void logToFile(QtMsgType, const QMessageLogContext &, const QString &message)
{
    static const QString path = [] {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        return dir + QStringLiteral("/ubdmr.log");
    }();
    QFile file(path);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream(&file) << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
                           << ' ' << message << '\n';
    }
    // Also to the journal: journalctl --user -f on the phone.
    fprintf(stderr, "%s\n", qPrintable(message));
}

// Ubuntu Touch exports GRID_UNIT_PX (typically 18-27 on a phone). All QML
// sizes are multiples of it so text stays readable on high-DPI screens.
static int gridUnitPx()
{
    bool ok = false;
    const int px = qEnvironmentVariableIntValue("GRID_UNIT_PX", &ok);
    return (ok && px > 0) ? px : 8;
}

int main(int argc, char *argv[])
{
    // The platform style brings its own palette, and controls without
    // explicit colours then render dark text on a dark background.
    qputenv("QT_QUICK_CONTROLS_STYLE", "Default");

    QGuiApplication app(argc, argv);
    // Organisation = click package name: QSettings and AppDataLocation then
    // land in the directories AppArmor lets the app write.
    app.setOrganizationName(QStringLiteral("ubdmr"));
    app.setApplicationName(QStringLiteral("ubDMR"));
    qInstallMessageHandler(logToFile);
    qDebug() << "=== ubDMR starting ===";

    QQmlApplicationEngine engine;
    auto *language = new LanguageManager(&engine);
    language->applyStoredLanguage();
    language->setEngine(&engine);
    engine.rootContext()->setContextProperty(QStringLiteral("Language"), language);
    engine.rootContext()->setContextProperty(QStringLiteral("SystemTheme"),
                                             new SystemTheme(&engine));
    // Owned by the engine so it outlives every QML binding during shutdown.
    auto *radio = new RadioController(&engine);
    qmlRegisterUncreatableType<LastHeardModel>("ubDMR", 1, 0, "LastHeardModel",
                                               QStringLiteral("Provided by Radio.lastHeard"));
    qmlRegisterUncreatableType<IdLookup>("ubDMR", 1, 0, "IdLookup",
                                         QStringLiteral("Provided by Radio.lookup"));
    engine.rootContext()->setContextProperty(QStringLiteral("Radio"), radio);
    engine.rootContext()->setContextProperty(QStringLiteral("gridUnitPx"), gridUnitPx());

    QObject::connect(&app, &QGuiApplication::applicationStateChanged, radio,
                     [radio](Qt::ApplicationState state) {
                         if (state == Qt::ApplicationActive)
                             radio->onApplicationActive();
                     });

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        qDebug() << "ERROR: failed to load QML";
        return -1;
    }
    return app.exec();
}
