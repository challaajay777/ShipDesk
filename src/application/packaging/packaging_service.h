#pragma once

#include "domain/types.h"
#include "domain/artifact/manifest.h"
#include "application/staging/staging_service.h"
#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <atomic>

namespace shipdesk::application {

struct PackagingResult {
    bool success{false};
    bool cancelled{false};
    std::filesystem::path artifact_path;
    std::string artifact_filename;
    domain::PackageFormat package_format{domain::PackageFormat::ZipArchive};
    uint64_t artifact_size_bytes{0};
    std::string artifact_sha256;
    std::filesystem::path manifest_path;
    domain::ReleaseManifest manifest;
    std::string error_message;
};

class PackagingService {
public:
    PackagingService() = default;

    // Packages staged items into either a Directory Package or a ZIP Archive.
    // Automatically creates and embeds the ReleaseManifest.
    PackagingResult packageRelease(
        const StagingResult& staging,
        const std::filesystem::path& destinationDirectory,
        const std::string& packageBaseName,
        domain::PackageFormat format,
        domain::ReleaseManifest manifest,
        std::atomic<bool>* cancelFlag = nullptr,
        std::function<void(size_t index, size_t total, const std::string& currentPath)> progress = nullptr
    );
};

} // namespace shipdesk::application
