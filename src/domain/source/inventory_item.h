#pragma once

#include "domain/types.h"
#include <string>
#include <cstdint>

namespace shipdesk::domain {

enum class SourceItemType {
    RegularFile,
    Directory,
    Symlink,
    ReparsePoint,
    BrokenLink,
    Special
};

inline std::string sourceItemTypeToString(SourceItemType type) {
    switch (type) {
        case SourceItemType::RegularFile: return "File";
        case SourceItemType::Directory: return "Directory";
        case SourceItemType::Symlink: return "Symlink";
        case SourceItemType::ReparsePoint: return "ReparsePoint";
        case SourceItemType::BrokenLink: return "BrokenLink";
        case SourceItemType::Special: return "Special";
    }
    return "File";
}

struct SourceInventoryItem {
    std::string source_root_id;
    std::string relative_path;       // Normalized, forward slashes, no leading/trailing slashes
    std::string absolute_path;
    SourceItemType item_type{SourceItemType::RegularFile};
    uint64_t size_bytes{0};
    std::string modified_at_utc;
    bool is_hidden{false};
    bool included{true};
    std::string exclusion_reason;
    std::string sha256_hash;

    bool isFile() const { return item_type == SourceItemType::RegularFile; }
    bool isDirectory() const { return item_type == SourceItemType::Directory; }
    bool isSymlink() const { return item_type == SourceItemType::Symlink || item_type == SourceItemType::ReparsePoint; }
};

} // namespace shipdesk::domain
