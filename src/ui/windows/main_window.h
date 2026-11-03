#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QLabel>
#include <QAction>

namespace shipdesk::ui {

class OverviewPage;
class WorkspacesPage;
class ReleasesPage;
class ActivityPage;
class SourceInspectorPage;
class PackageVerifierPage;
class DiagnosticsPage;
class SettingsPage;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNavChanged(int row);
    void onThemeChanged(int themeIndex);
    void onCreateReleaseTriggered();
    void onNewWorkspaceTriggered();
    void onOpenWorkspaceRequested(const QString& workspaceId);
    void onOpenReleaseRequested(const QString& releaseId);
    void onPublishRequested(const QString& releaseRecordId);
    void onAboutTriggered();

private:
    void setupUi();
    void setupMenus();

    QListWidget* m_navList{nullptr};
    QStackedWidget* m_pageStack{nullptr};

    OverviewPage* m_overviewPage{nullptr};
    WorkspacesPage* m_workspacesPage{nullptr};
    ReleasesPage* m_releasesPage{nullptr};
    ActivityPage* m_activityPage{nullptr};
    SourceInspectorPage* m_inspectorPage{nullptr};
    PackageVerifierPage* m_verifierPage{nullptr};
    DiagnosticsPage* m_diagnosticsPage{nullptr};
    SettingsPage* m_settingsPage{nullptr};

    QLabel* m_statusLabel{nullptr};
    QLabel* m_dbStatusLabel{nullptr};
};

} // namespace shipdesk::ui
