#pragma once

#include "domain/destination/destination.h"
#include <filesystem>
#include <string>

namespace shipdesk::application {

struct PublishResult {
    bool success{false};
    std::string destination_id;
    std::string external_id;
    std::string external_url;
    std::string error_message;
};

class PublishingService {
public:
    PublishingService() = default;

    // Publishes a verified artifact to a destination
    static PublishResult publishArtifact(
        const std::string& destinationId,
        const std::string& artifactId,
        const std::filesystem::path& artifactPath,
        domain::DestinationKind kind,
        const std::string& destinationConfigJson,
        const std::string& releaseTag,
        const std::string& releaseTitle,
        const std::string& releaseNotes,
        bool overwriteConfirmed = false
    );
};

} // namespace shipdesk::application
