#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <cstdint>

namespace shipdesk::infrastructure {

class PathUtils {
public:
    // Normalizes path: converts all backslashes to forward slashes,
    // removes redundant slashes, and removes leading/trailing slashes.
    static std::string normalizeRelativePath(const std::string& input);

    // Component-aware containment check.
    // Returns true if child is strictly inside parent (or equal to parent if allowEqual=true).
    static bool isContainedWithin(const std::filesystem::path& child,
                                 const std::filesystem::path& parent,
                                 bool allowEqual = true);

    // Checks if the given relative path is safe:
    // It must not be absolute, must not contain '..' components, and must not escape root.
    static bool isSafeRelativePath(const std::string& relativePath);

    // Checks if two paths collide in a case-insensitive manner.
    static bool hasCaseCollision(const std::string& path1, const std::string& path2);

    // Checks available disk space on the drive hosting the given path.
    // Returns free bytes, or 0 on error.
    static uint64_t getAvailableDiskSpace(const std::filesystem::path& path);

    // Checks if a path is a symlink or Windows reparse point/junction.
    static bool isSymlinkOrReparsePoint(const std::filesystem::path& path);

    // Formats byte count into human-readable string (e.g. "12.4 MB", "512 KB")
    static std::string formatByteSize(uint64_t bytes);

    // Glob pattern matching (supports *, ?, and directory matching)
    static bool matchesGlob(const std::string& path, const std::string& pattern);
};

} // namespace shipdesk::infrastructure
