#include "launcher/update_controller.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>

int main(int argc, char *argv[])
{
    bool smokeTest = false;
    for (int i = 1; i < argc; ++i) if (QByteArray(argv[i]) == "--smoke-test") smokeTest = true;
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("NeonTycoon"));
    QCoreApplication::setApplicationName(QStringLiteral("Neon Tycoon Launcher"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("盛世百业启动器"));
    neon::launcher::UpdateController updater;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("updater"), &updater);
    engine.loadFromModule("NeonTycoonLauncher", "Launcher");
    if (engine.rootObjects().isEmpty()) return -1;
    if (smokeTest) QTimer::singleShot(1200, &app, &QCoreApplication::quit);
    return app.exec();
}
