#include "app/game_view_model.h"
#include "app/isometric_board.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSettings>
#include <QTimer>

int main(int argc, char *argv[])
{
    bool smokeTest = false;
    QString screenshotPath;
    for (int i = 1; i < argc; ++i) {
        if (QByteArray(argv[i]) == "--smoke-test") smokeTest = true;
        if (QByteArray(argv[i]) == "--screenshot" && i + 1 < argc) {
            screenshotPath = QString::fromLocal8Bit(argv[++i]);
            smokeTest = true;
        }
    }
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("NeonTycoon"));
    QCoreApplication::setApplicationName(QStringLiteral("Neon Tycoon"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("霓城大亨 · Neon Tycoon"));

    qmlRegisterType<neon::IsometricBoard>("NeonTycoon", 1, 0, "IsometricBoard");
    neon::GameViewModel viewModel;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("game"), &viewModel);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("NeonTycoon", "Main");
    if (engine.rootObjects().isEmpty()) return -1;

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    QSettings settings;
    const QString mode = settings.value(QStringLiteral("windowMode"), QStringLiteral("maximized")).toString();
    if (window) {
        if (mode == QStringLiteral("fullscreen")) window->showFullScreen();
        else if (mode == QStringLiteral("windowed")) window->showNormal();
        else window->showMaximized();
    }
    if (smokeTest) {
        QTimer::singleShot(1200, &app, [window, screenshotPath, &app] {
            if (window && !screenshotPath.isEmpty()) window->grabWindow().save(screenshotPath);
            app.quit();
        });
    }
    return app.exec();
}
