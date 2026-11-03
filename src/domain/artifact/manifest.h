#pragma once

#include "domain/types.h"
#include <string>
#include <vector>
#include <cstdint>

namespace shipdesk::domain {

struct ManifestArtifactEntry {
    std::string artifact_id;
    std::string filename;
    std::string output_path;
    uint64_t size_bytes{0};
    std::string sha256;
    std::string package_format;
    std::string created_at_utc;
};

struct ManifestFileEntry {
    std::string relative_path;
    uint64_t size_bytes{0};
    std::string sha256;
    bool is_executable{false};
    std::string source_relative_path;
    std::string classification{"regular"};
    std::string inclusion_reason{"explicit_or_profile_rule"};
};

class ReleaseManifest {
public:
    ReleaseManifest() = default;

    std::string format{"shipdesk-release-manifest"};
    std::string schema_version{"1.0.0"};
    std::string product_name;
    std::string workspace_id;
    std::string release_id;
    std::string version;
    std::string channel{"Draft"};
    std::string created_at_utc;
    std::string platform;
    std::string source_snapshot_json{"{}"};
    std::string profile_snapshot_json{"{}"};
    std::string validation_summary_json{"{}"};
    std::string build_summary_json{"{}"};
    std::string test_summary_json{"{}"};
    std::string provenance_reference;

    std::vector<ManifestArtifactEntry> artifacts;
    std::vector<ManifestFileEntry> files;

    std::string toJsonString() const;
    static ReleaseManifest fromJsonString(const std::string& json);
};

} // namespace shipdesk::domain
