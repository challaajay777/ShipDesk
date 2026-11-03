#pragma once

#include "domain/workspace/workspace.h"
#include <string>
#include <vector>
#include <optional>

namespace shipdesk::application {

class WorkspaceService {
public:
    WorkspaceService() = default;

    domain::Workspace createWorkspace(const std::string& name, const std::string& description = "", const std::string& category = "General");
    bool updateWorkspace(const domain::Workspace& ws);
    bool archiveWorkspace(const std::string& workspaceId);
    bool restoreWorkspace(const std::string& workspaceId);
    bool deleteWorkspace(const std::string& workspaceId);
    std::optional<domain::Workspace> getWorkspace(const std::string& workspaceId);
    std::vector<domain::Workspace> listWorkspaces(bool includeArchived = false);

    bool addSourceRoot(const std::string& workspaceId, const std::string& path, const std::string& displayName = "");
    bool removeSourceRoot(const std::string& rootId);
    std::vector<domain::SourceRoot> getSourceRoots(const std::string& workspaceId);
};

} // namespace shipdesk::application
