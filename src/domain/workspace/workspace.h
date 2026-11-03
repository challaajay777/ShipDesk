#pragma once

#include "domain/types.h"
#include <string>
#include <vector>
#include <optional>

namespace shipdesk::domain {

struct SourceRoot {
    std::string id;
    std::string workspace_id;
    std::string display_name;
    std::string configured_path;
    std::string path_kind{"Directory"}; // "Directory" or "File"
    bool follow_symlinks{false};
    std::string created_at_utc;
};

class Workspace {
public:
    Workspace() = default;
    Workspace(std::string id, std::string name, std::string description = "", std::string category = "General");

    const std::string& id() const { return m_id; }
    const std::string& name() const { return m_name; }
    const std::string& description() const { return m_description; }
    const std::string& category() const { return m_category; }
    const std::string& status() const { return m_status; }
    const std::string& createdAtUtc() const { return m_createdAtUtc; }
    const std::string& updatedAtUtc() const { return m_updatedAtUtc; }
    const std::string& archivedAtUtc() const { return m_archivedAtUtc; }

    void setName(const std::string& name);
    void setDescription(const std::string& desc);
    void setCategory(const std::string& cat);
    void setTimestamps(const std::string& created, const std::string& updated, const std::string& archived = "");

    void archive();
    void restore();
    bool isArchived() const { return m_status == "Archived"; }

    const std::vector<SourceRoot>& sourceRoots() const { return m_sourceRoots; }
    void addSourceRoot(const SourceRoot& root);
    bool removeSourceRoot(const std::string& rootId);
    void clearSourceRoots();

private:
    std::string m_id;
    std::string m_name;
    std::string m_description;
    std::string m_category{"General"};
    std::string m_status{"Active"};
    std::string m_createdAtUtc;
    std::string m_updatedAtUtc;
    std::string m_archivedAtUtc;
    std::vector<SourceRoot> m_sourceRoots;
};

} // namespace shipdesk::domain
