#include "session_controller.hpp"

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("GolarionHelper"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    const auto dataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    // Capture the existing campaign location before setting the profile's settings scope.
    application.setOrganizationName(QStringLiteral("GolarionHelper"));
    SessionController sessionController(dataDirectory.isEmpty() ? QString{} : QDir(dataDirectory).filePath(QStringLiteral("campaigns")));
    QQmlApplicationEngine engine;
    engine.setInitialProperties({{QStringLiteral("sessionController"), QVariant::fromValue(&sessionController)}});
    engine.loadFromModule("Golarion.App", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;

    return application.exec();
}
