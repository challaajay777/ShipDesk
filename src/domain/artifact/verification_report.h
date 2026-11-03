#pragma once

#include "domain/types.h"
#include <string>
#include <vector>
#include <utility>
#include <cstdint>

namespace shipdesk::domain {

struct VerificationReport {
    std::string artifact_id;
    std::string artifact_path;
    bool is_valid{false};
    bool archive_integrity_ok{true};
    uint64_t expected_files_count{0};
    uint64_t actual_files_count{0};
    std::vector<std::string> missing_files;
    std::vector<std::string> unexpected_files;
    std::vector<std::pair<std::string, std::string>> mismatched_checksums; // {path, detail}
    std::vector<std::string> messages;
    std::string checked_at_utc{currentUtcTimestamp()};

    bool hasErrors() const {
        return !is_valid || !archive_integrity_ok || !missing_files.empty() || !mismatched_checksums.empty();
    }
};

} // namespace shipdesk::domain
