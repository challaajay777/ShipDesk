#pragma once

#include "domain/types.h"
#include <string>

namespace shipdesk::domain {

enum class DestinationKind {
    LocalFolder,
    GitHubReleases,
    CustomNetworkShare
};

inline std::string destinationKindToString(DestinationKind kind) {
    switch (kind) {
        case DestinationKind::LocalFolder: return "LocalFolder";
        case DestinationKind::GitHubReleases: return "GitHubReleases";
        case DestinationKind::CustomNetworkShare: return "CustomNetworkShare";
    }
    return "LocalFolder";
}

inline DestinationKind destinationKindFromString(const std::string& str) {
    if (str == "GitHubReleases") return DestinationKind::GitHubReleases;
    if (str == "CustomNetworkShare") return DestinationKind::CustomNetworkShare;
    return DestinationKind::LocalFolder;
}

struct DestinationRecord {
    std::string id;
    std::string workspace_id;
    DestinationKind kind{DestinationKind::LocalFolder};
    std::string display_name;
    std::string safe_configuration_json{"{}"}; // Never contains secrets!
    std::string created_at_utc{currentUtcTimestamp()};
};

struct PublishAttemptRecord {
    std::string id;
    std::string artifact_id;
    std::string destination_id;
    std::string status{"Started"}; // "Started", "Uploaded", "Verified", "Failed"
    std::string external_id;
    std::string external_url;
    std::string started_at_utc{currentUtcTimestamp()};
    std::string finished_at_utc;
    std::string result_json{"{}"};
    std::string error_json{"{}"};
};

} // namespace shipdesk::domain
