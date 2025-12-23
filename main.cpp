#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "aircraftmodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<AircraftModel>("NoFlyZone", 1, 0, "AircraftModel");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("noflyzone", "Main");

    return app.exec();
}
