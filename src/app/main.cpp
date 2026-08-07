#include "app/game_view_model.h"
#include "app/procedural_audio.h"
#include "app/top_down_city_view.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSettings>
#include <QTimer>

int main(int argc, char *argv[])
{
    bool smokeTest = false;
    bool showGameForScreenshot = false;
    QString screenshotPath;
    for (int i = 1; i < argc; ++i) {
        if (QByteArray(argv[i]) == "--smoke-test") smokeTest = true;
        if (QByteArray(argv[i]) == "--game-screenshot") { smokeTest = true; showGameForScreenshot = true; }
        if (QByteArray(argv[i]) == "--screenshot" && i + 1 < argc) {
            screenshotPath = QString::fromLocal8Bit(argv[++i]);
            smokeTest = true;
        }
    }
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("NeonTycoon"));
    QCoreApplication::setApplicationName(QStringLiteral("Neon Tycoon"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("盛世百业"));

    qmlRegisterType<neon::TopDownCityView>("NeonTycoon", 1, 0, "TopDownCityView");
    neon::GameViewModel viewModel;
    neon::ProceduralAudio audio;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("game"), &viewModel);
    engine.rootContext()->setContextProperty(QStringLiteral("audio"), &audio);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("NeonTycoon", "Main");
    if (engine.rootObjects().isEmpty()) return -1;

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (showGameForScreenshot && window) window->setProperty("page", QStringLiteral("game"));
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
