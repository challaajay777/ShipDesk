#include "ui/windows/main_window.h"
#include "ui/themes/theme_manager.h"
#include "ui/pages/overview_page.h"
#include "ui/pages/workspaces_page.h"
#include "ui/pages/releases_page.h"
#include "ui/pages/activity_page.h"
#include "ui/pages/source_inspector_page.h"
#include "ui/pages/package_verifier_page.h"
#include "ui/pages/diagnostics_page.h"
#include "ui/pages/settings_page.h"
#include "ui/dialogs/release_wizard_dialog.h"
#include "ui/dialogs/publish_dialog.h"
#include <QSplitter>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QMessageBox>
#include <QApplication>

namespace shipdesk::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("ShipDesk — Native Release Workspace (v0.1.0)");
    resize(1024, 680);
    setMinimumSize(800, 500);

    setupUi();
    setupMenus();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter);

    // Left Navigation Sidebar
    m_navList = new QListWidget(splitter);
    m_navList->setFixedWidth(180);
    m_navList->addItem("Overview");
    m_navList->addItem("Workspaces");
    m_navList->addItem("Releases");
    m_navList->addItem("Activity");
    m_navList->addItem("Source Inspector");
    m_navList->addItem("Package Verifier");
    m_navList->addItem("Diagnostics");
    m_navList->addItem("Settings");
    connect(m_navList, &QListWidget::currentRowChanged, this, &MainWindow::onNavChanged);

    splitter->addWidget(m_navList);

    // Central Content Stack
    m_pageStack = new QStackedWidget(splitter);

    m_overviewPage = new OverviewPage(m_pageStack);
    m_workspacesPage = new WorkspacesPage(m_pageStack);
    m_releasesPage = new ReleasesPage(m_pageStack);
    m_activityPage = new ActivityPage(m_pageStack);
    m_inspectorPage = new SourceInspectorPage(m_pageStack);
    m_verifierPage = new PackageVerifierPage(m_pageStack);
    m_diagnosticsPage = new DiagnosticsPage(m_pageStack);
    m_settingsPage = new SettingsPage(m_pageStack);

    m_pageStack->addWidget(m_overviewPage);
    m_pageStack->addWidget(m_workspacesPage);
    m_pageStack->addWidget(m_releasesPage);
    m_pageStack->addWidget(m_activityPage);
    m_pageStack->addWidget(m_inspectorPage);
    m_pageStack->addWidget(m_verifierPage);
    m_pageStack->addWidget(m_diagnosticsPage);
    m_pageStack->addWidget(m_settingsPage);

    splitter->addWidget(m_pageStack);
    splitter->setCollapsible(0, false);
    splitter->setStretchFactor(1, 1);

    // Connect cross-page signals
    connect(m_overviewPage, &OverviewPage::createReleaseRequested, this, &MainWindow::onCreateReleaseTriggered);
    connect(m_overviewPage, &OverviewPage::newWorkspaceRequested, this, &MainWindow::onNewWorkspaceTriggered);
    connect(m_overviewPage, &OverviewPage::openWorkspaceRequested, this, &MainWindow::onOpenWorkspaceRequested);
    connect(m_overviewPage, &OverviewPage::openReleaseRequested, this, &MainWindow::onOpenReleaseRequested);

    connect(m_workspacesPage, &WorkspacesPage::createReleaseForWorkspace, [this](const QString& wsId) {
        ReleaseWizardDialog dlg(wsId, this);
        if (dlg.exec() == QDialog::Accepted) {
            m_releasesPage->refreshData();
            m_overviewPage->refreshData();
            m_activityPage->refreshData();
        }
    });

    connect(m_releasesPage, &ReleasesPage::publishRequested, this, &MainWindow::onPublishRequested);
    connect(m_settingsPage, &SettingsPage::themeChanged, this, &MainWindow::onThemeChanged);

    // Status Bar
    m_statusLabel = new QLabel("Ready", this);
    m_dbStatusLabel = new QLabel("SQLite: Connected | WAL Mode | Foreign Keys ON", this);
    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addPermanentWidget(m_dbStatusLabel);

    m_navList->setCurrentRow(0);
}

void MainWindow::setupMenus() {
    auto* mb = menuBar();

    // File Menu
    auto* fileMenu = mb->addMenu("&File");
    auto* newWsAction = fileMenu->addAction("&New Workspace...");
    newWsAction->setShortcut(QKeySequence::New);
    connect(newWsAction, &QAction::triggered, this, &MainWindow::onNewWorkspaceTriggered);

    auto* createRelAction = fileMenu->addAction("&Create Release (Guided)...");
    createRelAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(createRelAction, &QAction::triggered, this, &MainWindow::onCreateReleaseTriggered);

    fileMenu->addSeparator();
    auto* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    // View Menu
    auto* viewMenu = mb->addMenu("&View");
    auto* viewOverview = viewMenu->addAction("&Overview");
    connect(viewOverview, &QAction::triggered, [this]() { m_navList->setCurrentRow(0); });
    auto* viewWorkspaces = viewMenu->addAction("&Workspaces");
    connect(viewWorkspaces, &QAction::triggered, [this]() { m_navList->setCurrentRow(1); });
    auto* viewReleases = viewMenu->addAction("&Releases");
    connect(viewReleases, &QAction::triggered, [this]() { m_navList->setCurrentRow(2); });
    auto* viewActivity = viewMenu->addAction("&Activity");
    connect(viewActivity, &QAction::triggered, [this]() { m_navList->setCurrentRow(3); });

    // Tools Menu
    auto* toolsMenu = mb->addMenu("&Tools");
    auto* toolInspector = toolsMenu->addAction("Source &Inspector");
    connect(toolInspector, &QAction::triggered, [this]() { m_navList->setCurrentRow(4); });
    auto* toolVerifier = toolsMenu->addAction("Package &Verifier");
    connect(toolVerifier, &QAction::triggered, [this]() { m_navList->setCurrentRow(5); });
    auto* toolDiag = toolsMenu->addAction("&Diagnostics");
    connect(toolDiag, &QAction::triggered, [this]() { m_navList->setCurrentRow(6); });

    // Help Menu
    auto* helpMenu = mb->addMenu("&Help");
    auto* aboutAction = helpMenu->addAction("&About ShipDesk");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAboutTriggered);
}

void MainWindow::onNavChanged(int row) {
    if (row >= 0 && row < m_pageStack->count()) {
        m_pageStack->setCurrentIndex(row);
        if (row == 0) m_overviewPage->refreshData();
        else if (row == 1) m_workspacesPage->refreshData();
        else if (row == 2) m_releasesPage->refreshData();
        else if (row == 3) m_activityPage->refreshData();
        else if (row == 6) m_diagnosticsPage->refreshDiagnostics();
    }
}

void MainWindow::onThemeChanged(int themeIndex) {
    auto mode = ThemeManager::ThemeMode::System;
    if (themeIndex == 1) mode = ThemeManager::ThemeMode::Light;
    else if (themeIndex == 2) mode = ThemeManager::ThemeMode::Dark;

    ThemeManager::applyTheme(*static_cast<QApplication*>(QApplication::instance()), mode);
}

void MainWindow::onCreateReleaseTriggered() {
    ReleaseWizardDialog dlg("", this);
    if (dlg.exec() == QDialog::Accepted) {
        m_overviewPage->refreshData();
        m_releasesPage->refreshData();
        m_activityPage->refreshData();
    }
}

void MainWindow::onNewWorkspaceTriggered() {
    m_navList->setCurrentRow(1);
    m_workspacesPage->refreshData();
}

void MainWindow::onOpenWorkspaceRequested(const QString& workspaceId) {
    m_navList->setCurrentRow(1);
    m_workspacesPage->selectWorkspace(workspaceId);
}

void MainWindow::onOpenReleaseRequested(const QString& releaseId) {
    m_navList->setCurrentRow(2);
    m_releasesPage->selectRelease(releaseId);
}

void MainWindow::onPublishRequested(const QString& releaseRecordId) {
    PublishDialog dlg(releaseRecordId, this);
    dlg.exec();
    m_releasesPage->refreshData();
    m_activityPage->refreshData();
}

void MainWindow::onAboutTriggered() {
    QMessageBox::about(
        this, "About ShipDesk",
        "<h3>ShipDesk v0.1.0</h3>"
        "<p><b>Prepare, check, package, verify and deliver digital work without losing control of originals.</b></p>"
        "<p>Built with C++20, Qt 6 Widgets, SQLite, and CMake.<br>"
        "Local-first release workspace designed for maximum reliability and privacy.</p>"
        "<p>&copy; 2026 Challa Ajay. Released under the MIT License.</p>"
    );
}

} // namespace shipdesk::ui
