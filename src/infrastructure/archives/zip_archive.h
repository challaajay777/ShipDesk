#pragma once

#include "domain/artifact/verification_report.h"
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <functional>
#include <cstdint>

namespace shipdesk::infrastructure {

struct ZipEntryInput {
    std::filesystem::path disk_path;
    std::string archive_relative_path;
};

struct ZipEntryInfo {
    std::string relative_path;
    uint64_t uncompressed_size{0};
    uint64_t compressed_size{0};
    uint32_t crc32{0};
    bool is_directory{false};
};

class ZipArchiveService {
public:
    // Creates a ZIP archive containing the specified file entries.
    // Throws std::runtime_error on failure.
    static void createZip(
        const std::filesystem::path& zipFilePath,
        const std::vector<ZipEntryInput>& entries,
        std::function<void(size_t index, size_t total, const std::string& currentPath)> progress = nullptr
    );

    // Reads the list of entries present in the ZIP archive
    static std::vector<ZipEntryInfo> inspectZip(const std::filesystem::path& zipFilePath);

    // Verifies the archive structure and optionally checks entries against expected relative paths and digests.
    // Returns a comprehensive VerificationReport.
    static domain::VerificationReport verifyZip(
        const std::filesystem::path& zipFilePath,
        const std::map<std::string, std::pair<uint64_t, std::string>>& expectedFiles = {} // path -> {size, sha256}
    );

    // Reads a single entry from the archive into a string
    static std::string readEntryString(
        const std::filesystem::path& zipFilePath,
        const std::string& entryRelativePath
    );

    // Extracts the ZIP archive into a designated destination directory with path traversal protection
    static void extractZip(
        const std::filesystem::path& zipFilePath,
        const std::filesystem::path& destinationDir,
        uint64_t maxTotalBytes = 10ULL * 1024 * 1024 * 1024,
        size_t maxFiles = 100000
    );
};

} // namespace shipdesk::infrastructure
