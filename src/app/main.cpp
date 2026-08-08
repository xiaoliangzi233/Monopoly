#include "app/game_view_model.h"
#include "app/procedural_audio.h"
#include "app/release_notes.h"
#include "app/top_down_city_view.h"

#include <QGuiApplication>
#include <QDebug>
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
    neon::ReleaseNotes releaseNotes;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("game"), &viewModel);
    engine.rootContext()->setContextProperty(QStringLiteral("audio"), &audio);
    engine.rootContext()->setContextProperty(QStringLiteral("releaseNotes"), &releaseNotes);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    if (showGameForScreenshot)
        engine.setInitialProperties({{QStringLiteral("page"), QStringLiteral("game")},
                                     {QStringLiteral("suppressAutoReleaseNotes"), true}});
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
            if (window && !screenshotPath.isEmpty()) {
                const QImage capture = window->grabWindow();
                if (capture.isNull() || !capture.save(screenshotPath))
                    qWarning() << "Unable to save smoke-test screenshot to" << screenshotPath;
            }
            app.quit();
        });
    }
    return app.exec();
}
