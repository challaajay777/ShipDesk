#pragma once

#include "domain/source/inventory_item.h"
#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <atomic>
#include <cstdint>

namespace shipdesk::application {

struct StagedItem {
    std::string relative_path;
    std::filesystem::path staged_disk_path;
    uint64_t size_bytes{0};
    std::string sha256_hash;
    bool source_mutated_during_copy{false};
};

struct StagingResult {
    bool success{false};
    bool cancelled{false};
    std::filesystem::path staging_job_dir;
    std::string job_id;
    std::vector<StagedItem> staged_items;
    uint64_t total_staged_bytes{0};
    size_t total_staged_files{0};
    std::string error_message;
    std::vector<std::string> warnings;
};

class StagingService {
public:
    StagingService() = default;

    // Creates managed staging directory, writes ownership marker, copies included inventory items,
    // streaming and calculating SHA-256 simultaneously, verifies metadata, and enforces limits.
    StagingResult stageItems(
        const std::filesystem::path& baseStagingRoot,
        const std::string& releaseRunId,
        const std::vector<domain::SourceInventoryItem>& itemsToStage,
        uint64_t maxTotalBytes = 10ULL * 1024 * 1024 * 1024,
        std::atomic<bool>* cancelFlag = nullptr,
        std::function<void(size_t index, size_t total, uint64_t bytesDone, const std::string& currentPath)> progress = nullptr
    );

    // Cleans up a staging directory ONLY IF ownership marker matches the job ID
    static bool cleanupOwnedStaging(const std::filesystem::path& stagingJobDir, const std::string& expectedJobId);
};

} // namespace shipdesk::application
