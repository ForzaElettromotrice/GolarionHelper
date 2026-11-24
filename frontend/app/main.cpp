#include "controllers/session_controller.hpp"
#include "controllers/sheet_controller.hpp"
#include "golarion/catalog/catalog_reader.hpp"

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QtQml/QQmlExtensionPlugin>

Q_IMPORT_QML_PLUGIN(GolarionSheetPlugin)

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("GolarionHelper"));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.golarionhelper.GolarionHelper"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    const QDir executableDirectory(QCoreApplication::applicationDirPath());
    auto catalogPath = executableDirectory.filePath(QStringLiteral("catalogs"));
#ifdef Q_OS_WIN
    golarion::catalog::setDirectory(catalogPath.toStdWString());
#else
    if (!QDir(catalogPath).exists())
        catalogPath = executableDirectory.filePath(QStringLiteral(GOLARION_INSTALLED_CATALOGS));
    golarion::catalog::setDirectory(catalogPath.toStdString());
#endif

    const auto dataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    // Capture the existing campaign location before setting the profile's settings scope.
    application.setOrganizationName(QStringLiteral("GolarionHelper"));
    SessionController sessionController(dataDirectory.isEmpty() ? QString{} : QDir(dataDirectory).filePath(QStringLiteral("campaigns")));
    SheetController sheetController(nullptr, dataDirectory.isEmpty() ? QString{} : QDir(dataDirectory).filePath(QStringLiteral("sheets")));
    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("sessionController"), QVariant::fromValue(&sessionController)},
        {QStringLiteral("sheetController"), QVariant::fromValue(&sheetController)}
    });
    engine.loadFromModule("Golarion.App", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;
    sheetController.preloadCatalogs();

    return application.exec();
}
