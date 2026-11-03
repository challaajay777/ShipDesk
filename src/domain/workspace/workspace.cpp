#include "domain/workspace/workspace.h"

namespace shipdesk::domain {

Workspace::Workspace(std::string id, std::string name, std::string description, std::string category)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_description(std::move(description))
    , m_category(std::move(category))
    , m_status("Active")
    , m_createdAtUtc(currentUtcTimestamp())
    , m_updatedAtUtc(m_createdAtUtc)
{
    if (m_name.empty()) {
        throw std::invalid_argument("Workspace name cannot be empty");
    }
}

void Workspace::setName(const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("Workspace name cannot be empty");
    }
    m_name = name;
    m_updatedAtUtc = currentUtcTimestamp();
}

void Workspace::setDescription(const std::string& desc) {
    m_description = desc;
    m_updatedAtUtc = currentUtcTimestamp();
}

void Workspace::setCategory(const std::string& cat) {
    m_category = cat;
    m_updatedAtUtc = currentUtcTimestamp();
}

void Workspace::setTimestamps(const std::string& created, const std::string& updated, const std::string& archived) {
    m_createdAtUtc = created;
    m_updatedAtUtc = updated;
    m_archivedAtUtc = archived;
    if (!m_archivedAtUtc.empty()) {
        m_status = "Archived";
    }
}

void Workspace::archive() {
    m_status = "Archived";
    m_archivedAtUtc = currentUtcTimestamp();
    m_updatedAtUtc = m_archivedAtUtc;
}

void Workspace::restore() {
    m_status = "Active";
    m_archivedAtUtc.clear();
    m_updatedAtUtc = currentUtcTimestamp();
}

void Workspace::addSourceRoot(const SourceRoot& root) {
    m_sourceRoots.push_back(root);
    m_updatedAtUtc = currentUtcTimestamp();
}

bool Workspace::removeSourceRoot(const std::string& rootId) {
    for (auto it = m_sourceRoots.begin(); it != m_sourceRoots.end(); ++it) {
        if (it->id == rootId) {
            m_sourceRoots.erase(it);
            m_updatedAtUtc = currentUtcTimestamp();
            return true;
        }
    }
    return false;
}

void Workspace::clearSourceRoots() {
    m_sourceRoots.clear();
}

} // namespace shipdesk::domain
