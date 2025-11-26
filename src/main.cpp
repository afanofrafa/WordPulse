#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include "src/wordpulseviewmodel.h"
#include "src/logger.h"

int main(int argc, char *argv[])
{
    Logger::init("application.log");
    qInfo() << "=== Application Started ===";

    QApplication app(argc, argv);

    try {
        QQuickStyle::setStyle("Material");

        QQmlApplicationEngine engine;
        qmlRegisterType<WordPulseViewModel>("WordPulse", 1, 0, "WordPulseViewModel");

        QObject::connect(
            &engine,
            &QQmlApplicationEngine::objectCreationFailed,
            &app,
            []() {
                qCritical() << "QML Object creation failed. Exiting.";
                QCoreApplication::exit(-1);
            },
            Qt::QueuedConnection);

        engine.loadFromModule("untitled", "Main");

        int result = app.exec();
        qInfo() << "=== Application Exit Code:" << result << "===";
        return result;

    } catch (const std::exception &e) {
        qCritical() << "Unhandled C++ exception in main:" << e.what();
        return -1;
    } catch (...) {
        qCritical() << "Unknown unhandled exception in main";
        return -1;
    }
}
