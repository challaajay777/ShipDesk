#pragma once

#include "domain/types.h"
#include <string>

namespace shipdesk::domain {

struct ReleaseRunRecord {
    std::string id;
    std::string release_draft_id;
    ReleaseState status{ReleaseState::Draft};
    std::string source_snapshot_json{"{}"};
    std::string profile_snapshot_json{"{}"};
    std::string started_at_utc{currentUtcTimestamp()};
    std::string finished_at_utc;
    std::string summary_json{"{}"};
    std::string error_json{"{}"};
};

struct ImmutableReleaseRecord {
    std::string id;
    std::string release_run_id;
    std::string workspace_snapshot_json{"{}"};
    std::string metadata_snapshot_json{"{}"};
    std::string manifest_path;
    std::string completed_at{currentUtcTimestamp()};
};

struct EventLogRecord {
    std::string id;
    std::string workspace_id;
    std::string release_run_id;
    std::string severity{"Info"};
    std::string event_type;
    std::string message;
    std::string safe_details_json{"{}"};
    std::string created_at_utc{currentUtcTimestamp()};
};

} // namespace shipdesk::domain
