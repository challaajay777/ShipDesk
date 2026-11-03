#include "domain/release/release_draft.h"
#include "domain/release/release_state_machine.h"

namespace shipdesk::domain {

ReleaseDraft::ReleaseDraft(std::string id, std::string workspace_id, std::string profile_id,
                           std::string title, std::string version, ReleaseChannel channel)
    : m_id(std::move(id))
    , m_workspaceId(std::move(workspace_id))
    , m_profileId(std::move(profile_id))
    , m_title(std::move(title))
    , m_version(std::move(version))
    , m_channel(channel)
    , m_state(ReleaseState::Draft)
    , m_createdAtUtc(currentUtcTimestamp())
    , m_updatedAtUtc(m_createdAtUtc)
{
    if (m_title.empty()) {
        throw std::invalid_argument("Release title cannot be empty");
    }
}

void ReleaseDraft::setTitle(const std::string& title) {
    if (title.empty()) {
        throw std::invalid_argument("Release title cannot be empty");
    }
    m_title = title;
    m_updatedAtUtc = currentUtcTimestamp();
}

void ReleaseDraft::setVersion(const std::string& ver) {
    if (ver.empty()) {
        throw std::invalid_argument("Release version cannot be empty");
    }
    m_version = ver;
    m_updatedAtUtc = currentUtcTimestamp();
}

void ReleaseDraft::setChannel(ReleaseChannel ch) {
    m_channel = ch;
    m_updatedAtUtc = currentUtcTimestamp();
}

void ReleaseDraft::setDescription(const std::string& desc) {
    m_description = desc;
    m_updatedAtUtc = currentUtcTimestamp();
}

void ReleaseDraft::setReleaseNotes(const std::string& notes) {
    m_releaseNotes = notes;
    m_updatedAtUtc = currentUtcTimestamp();
}

void ReleaseDraft::setMetadataJson(const std::string& json) {
    m_metadataJson = json.empty() ? "{}" : json;
    m_updatedAtUtc = currentUtcTimestamp();
}

void ReleaseDraft::transitionTo(ReleaseState newState) {
    ReleaseStateMachine::validateTransition(m_state, newState);
    m_state = newState;
    m_updatedAtUtc = currentUtcTimestamp();
}

void ReleaseDraft::setStateDirect(ReleaseState state) {
    m_state = state;
}

void ReleaseDraft::setTimestamps(const std::string& created, const std::string& updated) {
    m_createdAtUtc = created;
    m_updatedAtUtc = updated;
}

} // namespace shipdesk::domain
