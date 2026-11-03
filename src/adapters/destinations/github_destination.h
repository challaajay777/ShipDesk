#pragma once

#include "domain/destination/destination.h"
#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>

namespace shipdesk::adapters {

struct GitHubCliAuthInfo {
    bool installed{false};
    bool authenticated{false};
    std::string cli_version;
    std::string logged_in_user;
    std::string git_protocol;
    std::string scopes;
    std::string status_message;
};

struct GitHubPublishResult {
    bool success{false};
    std::string release_url;
    std::string release_id;
    std::string asset_url;
    uint64_t remote_asset_size{0};
    bool asset_verified{false};
    std::string error_message;
};

class GitHubDestinationAdapter {
public:
    static GitHubCliAuthInfo checkAuth();

    static bool checkRepositoryExists(const std::string& ownerAndRepo);

    static GitHubPublishResult publishReleaseAndAsset(
        const std::string& ownerAndRepo,
        const std::string& tagName,
        const std::string& releaseTitle,
        const std::string& releaseNotes,
        const std::filesystem::path& assetPath,
        bool isDraft = true,
        bool isPrerelease = false
    );
};

} // namespace shipdesk::adapters
