#pragma once

#include "domain/destination/destination.h"
#include <filesystem>
#include <string>

namespace shipdesk::adapters {

struct LocalPublishResult {
    bool success{false};
    std::filesystem::path destination_path;
    uint64_t copied_bytes{0};
    std::string sha256_hash;
    std::string error_message;
};

class LocalFolderDestinationAdapter {
public:
    static LocalPublishResult publishArtifact(
        const std::filesystem::path& artifactPath,
        const std::filesystem::path& targetFolder,
        bool overwriteConfirmed = false
    );
};

} // namespace shipdesk::adapters
