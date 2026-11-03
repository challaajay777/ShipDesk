#include "application/workspace/workspace_service.h"
#include "infrastructure/database/database_manager.h"
#include <QUuid>
#include <filesystem>

namespace shipdesk::application {

domain::Workspace WorkspaceService::createWorkspace(const std::string& name, const std::string& description, const std::string& category) {
    std::string id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    domain::Workspace ws(id, name, description, category);

    infrastructure::DatabaseManager::instance().saveWorkspace(ws);

    // Create default release profiles
    auto defaultProf = domain::ReleaseProfile::createDefault(category, id);
    infrastructure::DatabaseManager::instance().saveReleaseProfile(defaultProf);

    domain::EventLogRecord ev;
    ev.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    ev.workspace_id = id;
    ev.severity = "Info";
    ev.event_type = "WorkspaceCreated";
    ev.message = "Created workspace '" + name + "'";
    infrastructure::DatabaseManager::instance().logEvent(ev);

    return ws;
}

bool WorkspaceService::updateWorkspace(const domain::Workspace& ws) {
    return infrastructure::DatabaseManager::instance().saveWorkspace(ws);
}

bool WorkspaceService::archiveWorkspace(const std::string& workspaceId) {
    auto opt = infrastructure::DatabaseManager::instance().getWorkspace(workspaceId);
    if (!opt) return false;

    auto ws = *opt;
    ws.archive();
    bool ok = infrastructure::DatabaseManager::instance().saveWorkspace(ws);
    if (ok) {
        domain::EventLogRecord ev;
        ev.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        ev.workspace_id = workspaceId;
        ev.severity = "Info";
        ev.event_type = "WorkspaceArchived";
        ev.message = "Archived workspace '" + ws.name() + "'";
        infrastructure::DatabaseManager::instance().logEvent(ev);
    }
    return ok;
}

bool WorkspaceService::restoreWorkspace(const std::string& workspaceId) {
    auto opt = infrastructure::DatabaseManager::instance().getWorkspace(workspaceId);
    if (!opt) return false;

    auto ws = *opt;
    ws.restore();
    bool ok = infrastructure::DatabaseManager::instance().saveWorkspace(ws);
    if (ok) {
        domain::EventLogRecord ev;
        ev.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        ev.workspace_id = workspaceId;
        ev.severity = "Info";
        ev.event_type = "WorkspaceRestored";
        ev.message = "Restored workspace '" + ws.name() + "'";
        infrastructure::DatabaseManager::instance().logEvent(ev);
    }
    return ok;
}

bool WorkspaceService::deleteWorkspace(const std::string& workspaceId) {
    return infrastructure::DatabaseManager::instance().deleteWorkspace(workspaceId);
}

std::optional<domain::Workspace> WorkspaceService::getWorkspace(const std::string& workspaceId) {
    return infrastructure::DatabaseManager::instance().getWorkspace(workspaceId);
}

std::vector<domain::Workspace> WorkspaceService::listWorkspaces(bool includeArchived) {
    return infrastructure::DatabaseManager::instance().listWorkspaces(includeArchived);
}

bool WorkspaceService::addSourceRoot(const std::string& workspaceId, const std::string& path, const std::string& displayName) {
    domain::SourceRoot root;
    root.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    root.workspace_id = workspaceId;
    root.configured_path = path;
    root.display_name = displayName.empty() ? std::filesystem::path(path).filename().string() : displayName;
    root.path_kind = std::filesystem::is_directory(path) ? "Directory" : "File";
    root.follow_symlinks = false;
    root.created_at_utc = domain::currentUtcTimestamp();

    bool ok = infrastructure::DatabaseManager::instance().saveSourceRoot(root);
    if (ok) {
        domain::EventLogRecord ev;
        ev.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        ev.workspace_id = workspaceId;
        ev.severity = "Info";
        ev.event_type = "SourceRootAdded";
        ev.message = "Added source root '" + root.display_name + "' (" + path + ")";
        infrastructure::DatabaseManager::instance().logEvent(ev);
    }
    return ok;
}

bool WorkspaceService::removeSourceRoot(const std::string& rootId) {
    return infrastructure::DatabaseManager::instance().deleteSourceRoot(rootId);
}

std::vector<domain::SourceRoot> WorkspaceService::getSourceRoots(const std::string& workspaceId) {
    return infrastructure::DatabaseManager::instance().getSourceRoots(workspaceId);
}

} // namespace shipdesk::application
