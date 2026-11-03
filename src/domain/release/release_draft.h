#pragma once

#include "domain/types.h"
#include <string>

namespace shipdesk::domain {

class ReleaseDraft {
public:
    ReleaseDraft() = default;
    ReleaseDraft(std::string id, std::string workspace_id, std::string profile_id,
                 std::string title, std::string version, ReleaseChannel channel = ReleaseChannel::Draft);

    const std::string& id() const { return m_id; }
    const std::string& workspaceId() const { return m_workspaceId; }
    const std::string& profileId() const { return m_profileId; }
    const std::string& title() const { return m_title; }
    const std::string& version() const { return m_version; }
    ReleaseChannel channel() const { return m_channel; }
    const std::string& description() const { return m_description; }
    const std::string& releaseNotes() const { return m_releaseNotes; }
    const std::string& metadataJson() const { return m_metadataJson; }
    ReleaseState state() const { return m_state; }
    const std::string& createdAtUtc() const { return m_createdAtUtc; }
    const std::string& updatedAtUtc() const { return m_updatedAtUtc; }

    void setTitle(const std::string& title);
    void setVersion(const std::string& ver);
    void setChannel(ReleaseChannel ch);
    void setDescription(const std::string& desc);
    void setReleaseNotes(const std::string& notes);
    void setMetadataJson(const std::string& json);
    void transitionTo(ReleaseState newState);
    void setStateDirect(ReleaseState state); // Used for database loading

    void setTimestamps(const std::string& created, const std::string& updated);

private:
    std::string m_id;
    std::string m_workspaceId;
    std::string m_profileId;
    std::string m_title;
    std::string m_version{"0.1.0"};
    ReleaseChannel m_channel{ReleaseChannel::Draft};
    std::string m_description;
    std::string m_releaseNotes;
    std::string m_metadataJson{"{}"};
    ReleaseState m_state{ReleaseState::Draft};
    std::string m_createdAtUtc;
    std::string m_updatedAtUtc;
};

} // namespace shipdesk::domain
