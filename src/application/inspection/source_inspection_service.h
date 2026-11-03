#pragma once

#include "domain/source/inventory_item.h"
#include "domain/profile/release_profile.h"
#include <vector>
#include <string>
#include <filesystem>
#include <functional>
#include <atomic>

namespace shipdesk::application {

struct InspectionResult {
    bool success{false};
    bool cancelled{false};
    std::vector<domain::SourceInventoryItem> items;
    uint64_t total_files_count{0};
    uint64_t total_directories_count{0};
    uint64_t included_files_count{0};
    uint64_t included_bytes{0};
    uint64_t skipped_count{0};
    uint64_t error_count{0};
    std::vector<std::string> warnings;
    std::string error_message;
    int64_t duration_ms{0};
};

class SourceInspectionService {
public:
    SourceInspectionService() = default;

    // Scans the given source root safely without any write operations.
    // Respects cancellation flag, profile rules, symlink policies, and depth/size limits.
    InspectionResult inspectRoot(
        const std::filesystem::path& rootPath,
        const std::string& sourceRootId,
        const domain::ReleaseProfile& profile,
        std::atomic<bool>* cancelFlag = nullptr,
        std::function<void(uint64_t scannedCount, const std::string& currentPath)> progress = nullptr
    );

    // Compares two inventory snapshots
    static void compareSnapshots(
        const std::vector<domain::SourceInventoryItem>& oldItems,
        const std::vector<domain::SourceInventoryItem>& newItems,
        std::vector<std::string>& added,
        std::vector<std::string>& removed,
        std::vector<std::string>& changed
    );
};

} // namespace shipdesk::application
