#include "ui/windows/main_window.h"
#include "ui/themes/theme_manager.h"
#include "infrastructure/database/database_manager.h"
#include <QApplication>
#include <QIcon>
#include <QStandardPaths>
#include <QDir>
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("ShipDesk");
    app.setOrganizationName("ShipDesk");
    app.setApplicationVersion("0.1.0");

    QIcon appIcon(":/icons/shipdesk.png");
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }

    // Initialize database in standard user app data location
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QString dbPath = dataDir + "/shipdesk.db";

    if (!shipdesk::infrastructure::DatabaseManager::instance().initialize(dbPath.toStdString())) {
        std::cerr << "Failed to initialize ShipDesk SQLite database at: " << dbPath.toStdString() << std::endl;
        return 1;
    }

    // Startup Crash Recovery: detect and recover any abandoned jobs from prior ungraceful exits
    int recoveredCount = shipdesk::infrastructure::DatabaseManager::instance().recoverInterruptedRuns();
    if (recoveredCount > 0) {
        std::cout << "Startup recovery: Marked " << recoveredCount << " abandoned transient run(s) as Interrupted." << std::endl;
    }

    // Apply native desktop theme styling
    shipdesk::ui::ThemeManager::applyTheme(app, shipdesk::ui::ThemeManager::ThemeMode::System);

    shipdesk::ui::MainWindow mainWindow;
    if (!appIcon.isNull()) {
        mainWindow.setWindowIcon(appIcon);
    }
    mainWindow.show();

    int exitCode = app.exec();

    shipdesk::infrastructure::DatabaseManager::instance().close();
    return exitCode;
}
