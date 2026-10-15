#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>

#include "wjs/version.hpp"

int main(int argc, char* argv[]) {
    QGuiApplication application(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("WordJewelsSolver"));
    QGuiApplication::setOrganizationName(QStringLiteral("Iain Chesworth Labs"));
    QGuiApplication::setApplicationVersion(QString::fromUtf8(
        wjs::project_version.data(), static_cast<qsizetype>(wjs::project_version.size())));

    QQmlApplicationEngine engine;
    engine.loadFromModule("WordJewelsSolver", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    return application.exec();
}
