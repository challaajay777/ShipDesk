#include "infrastructure/archives/zip_archive.h"
#include "infrastructure/archives/miniz.h"
#include "infrastructure/filesystem/path_utils.h"
#include "infrastructure/hashing/hasher.h"
#include <fstream>
#include <sstream>
#include <set>
#include <cstring>
#include <stdexcept>

namespace shipdesk::infrastructure {

void ZipArchiveService::createZip(
    const std::filesystem::path& zipFilePath,
    const std::vector<ZipEntryInput>& entries,
    std::function<void(size_t index, size_t total, const std::string& currentPath)> progress)
{
    // Ensure parent directory exists
    if (zipFilePath.has_parent_path()) {
        std::filesystem::create_directories(zipFilePath.parent_path());
    }

    mz_zip_archive zipArchive;
    std::memset(&zipArchive, 0, sizeof(zipArchive));

    std::string zipPathStr = zipFilePath.string();
    if (!mz_zip_writer_init_file(&zipArchive, zipPathStr.c_str(), 0)) {
        throw std::runtime_error("Failed to initialize zip writer for: " + zipPathStr);
    }

    std::set<std::string> addedPaths;

    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        std::string relPath = PathUtils::normalizeRelativePath(entry.archive_relative_path);

        if (!PathUtils::isSafeRelativePath(relPath)) {
            mz_zip_writer_end(&zipArchive);
            throw std::runtime_error("Refusing to add unsafe archive path: " + relPath);
        }

        if (addedPaths.find(relPath) != addedPaths.end()) {
            mz_zip_writer_end(&zipArchive);
            throw std::runtime_error("Duplicate relative path detected in archive input: " + relPath);
        }
        addedPaths.insert(relPath);

        std::string diskPathStr = entry.disk_path.string();
        if (!std::filesystem::exists(entry.disk_path)) {
            mz_zip_writer_end(&zipArchive);
            throw std::runtime_error("Disk file missing when adding to archive: " + diskPathStr);
        }

        if (progress) {
            progress(i, entries.size(), relPath);
        }

        // Add file with standard Deflate compression (level 6)
        if (!mz_zip_writer_add_file(&zipArchive, relPath.c_str(), diskPathStr.c_str(), nullptr, 0, MZ_DEFAULT_LEVEL)) {
            mz_zip_writer_end(&zipArchive);
            throw std::runtime_error("Failed to add file to zip archive: " + diskPathStr + " as " + relPath);
        }
    }

    if (!mz_zip_writer_finalize_archive(&zipArchive)) {
        mz_zip_writer_end(&zipArchive);
        throw std::runtime_error("Failed to finalize zip archive: " + zipPathStr);
    }

    if (!mz_zip_writer_end(&zipArchive)) {
        throw std::runtime_error("Failed to close zip archive: " + zipPathStr);
    }

    if (progress) {
        progress(entries.size(), entries.size(), "Completed");
    }
}

std::vector<ZipEntryInfo> ZipArchiveService::inspectZip(const std::filesystem::path& zipFilePath) {
    std::vector<ZipEntryInfo> results;
    mz_zip_archive zipArchive;
    std::memset(&zipArchive, 0, sizeof(zipArchive));

    std::string zipPathStr = zipFilePath.string();
    if (!mz_zip_reader_init_file(&zipArchive, zipPathStr.c_str(), 0)) {
        throw std::runtime_error("Failed to open zip archive for reading: " + zipPathStr);
    }

    mz_uint numFiles = mz_zip_reader_get_num_files(&zipArchive);
    for (mz_uint i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat fileStat;
        if (mz_zip_reader_file_stat(&zipArchive, i, &fileStat)) {
            ZipEntryInfo info;
            info.relative_path = PathUtils::normalizeRelativePath(fileStat.m_filename);
            info.uncompressed_size = fileStat.m_uncomp_size;
            info.compressed_size = fileStat.m_comp_size;
            info.crc32 = fileStat.m_crc32;
            info.is_directory = mz_zip_reader_is_file_a_directory(&zipArchive, i);
            results.push_back(info);
        }
    }

    mz_zip_reader_end(&zipArchive);
    return results;
}

domain::VerificationReport ZipArchiveService::verifyZip(
    const std::filesystem::path& zipFilePath,
    const std::map<std::string, std::pair<uint64_t, std::string>>& expectedFiles)
{
    domain::VerificationReport report;
    report.artifact_path = zipFilePath.string();
    report.archive_integrity_ok = true;
    report.is_valid = true;
    report.expected_files_count = expectedFiles.size();

    mz_zip_archive zipArchive;
    std::memset(&zipArchive, 0, sizeof(zipArchive));

    std::string zipPathStr = zipFilePath.string();
    if (!mz_zip_reader_init_file(&zipArchive, zipPathStr.c_str(), 0)) {
        report.archive_integrity_ok = false;
        report.is_valid = false;
        report.messages.push_back("Archive failed to open or header is corrupted.");
        return report;
    }

    mz_uint numFiles = mz_zip_reader_get_num_files(&zipArchive);
    report.actual_files_count = 0;

    std::set<std::string> foundInArchive;

    for (mz_uint i = 0; i < numFiles; ++i) {
        if (mz_zip_reader_is_file_a_directory(&zipArchive, i)) {
            continue; // Skip directory entries in file count
        }

        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(&zipArchive, i, &fileStat)) {
            report.archive_integrity_ok = false;
            report.messages.push_back("Failed to read file stat for entry index " + std::to_string(i));
            continue;
        }

        std::string rawName = fileStat.m_filename;
        std::string normPath = PathUtils::normalizeRelativePath(rawName);

        // Security check: absolute path or traversal
        if (!PathUtils::isSafeRelativePath(normPath)) {
            report.archive_integrity_ok = false;
            report.messages.push_back("Archive contains unsafe path traversal entry: " + rawName);
            report.is_valid = false;
        }

        bool isManifest = (normPath == "shipdesk-release-manifest.json");
        bool isExpected = (!expectedFiles.empty() && expectedFiles.find(normPath) != expectedFiles.end());

        if (expectedFiles.empty() || isExpected || !isManifest) {
            report.actual_files_count++;
            foundInArchive.insert(normPath);
        }

        // Extract and test entry data integrity (CRC32 checked by miniz)
        size_t uncompSize = 0;
        void* pData = mz_zip_reader_extract_to_heap(&zipArchive, i, &uncompSize, 0);
        if (!pData) {
            report.archive_integrity_ok = false;
            report.messages.push_back("Corrupted data or CRC32 mismatch on entry: " + normPath);
            report.is_valid = false;
            continue;
        }

        // If expectedFiles is provided, check size and SHA-256
        if (!expectedFiles.empty()) {
            if (!isExpected) {
                if (!isManifest) {
                    report.unexpected_files.push_back(normPath);
                }
            } else {
                auto it = expectedFiles.find(normPath);
                uint64_t expectedSize = it->second.first;
                const std::string& expectedSha = it->second.second;

                if (uncompSize != expectedSize) {
                    report.mismatched_checksums.push_back({
                        normPath,
                        "Size mismatch: expected " + std::to_string(expectedSize) +
                        " but got " + std::to_string(uncompSize)
                    });
                    report.is_valid = false;
                }

                if (!expectedSha.empty()) {
                    std::string actualSha = Hasher::sha256String(
                        std::string(static_cast<const char*>(pData), uncompSize)
                    );
                    if (actualSha != expectedSha) {
                        report.mismatched_checksums.push_back({
                            normPath,
                            "SHA-256 mismatch: expected " + expectedSha + " but got " + actualSha
                        });
                        report.is_valid = false;
                    }
                }
            }
        }

        mz_free(pData);
    }

    mz_zip_reader_end(&zipArchive);

    // Check for missing expected files
    for (const auto& [expectedPath, _] : expectedFiles) {
        if (foundInArchive.find(expectedPath) == foundInArchive.end()) {
            report.missing_files.push_back(expectedPath);
            report.is_valid = false;
        }
    }

    if (!report.missing_files.empty() || !report.unexpected_files.empty() ||
        !report.mismatched_checksums.empty() || !report.archive_integrity_ok) {
        report.is_valid = false;
    }

    return report;
}

void ZipArchiveService::extractZip(
    const std::filesystem::path& zipFilePath,
    const std::filesystem::path& destinationDir,
    uint64_t maxTotalBytes,
    size_t maxFiles)
{
    mz_zip_archive zipArchive;
    std::memset(&zipArchive, 0, sizeof(zipArchive));

    std::string zipPathStr = zipFilePath.string();
    if (!mz_zip_reader_init_file(&zipArchive, zipPathStr.c_str(), 0)) {
        throw std::runtime_error("Failed to open zip archive for extraction: " + zipPathStr);
    }

    mz_uint numFiles = mz_zip_reader_get_num_files(&zipArchive);
    if (numFiles > maxFiles) {
        mz_zip_reader_end(&zipArchive);
        throw std::runtime_error("Archive entry count exceeds maximum limit of " + std::to_string(maxFiles));
    }

    uint64_t totalBytesExtracted = 0;
    std::filesystem::create_directories(destinationDir);

    for (mz_uint i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(&zipArchive, i, &fileStat)) {
            continue;
        }

        std::string relPath = PathUtils::normalizeRelativePath(fileStat.m_filename);
        if (!PathUtils::isSafeRelativePath(relPath)) {
            mz_zip_reader_end(&zipArchive);
            throw std::runtime_error("Extraction rejected: entry attempts path traversal: " + std::string(fileStat.m_filename));
        }

        std::filesystem::path outPath = destinationDir / relPath;
        if (!PathUtils::isContainedWithin(outPath, destinationDir)) {
            mz_zip_reader_end(&zipArchive);
            throw std::runtime_error("Extraction rejected: target path escapes destination root: " + outPath.string());
        }

        if (mz_zip_reader_is_file_a_directory(&zipArchive, i)) {
            std::filesystem::create_directories(outPath);
            continue;
        }

        if (outPath.has_parent_path()) {
            std::filesystem::create_directories(outPath.parent_path());
        }

        totalBytesExtracted += fileStat.m_uncomp_size;
        if (totalBytesExtracted > maxTotalBytes) {
            mz_zip_reader_end(&zipArchive);
            throw std::runtime_error("Extraction aborted: uncompressed size exceeds limit of " + std::to_string(maxTotalBytes));
        }

        if (!mz_zip_reader_extract_to_file(&zipArchive, i, outPath.string().c_str(), 0)) {
            mz_zip_reader_end(&zipArchive);
            throw std::runtime_error("Failed to extract file from zip: " + outPath.string());
        }
    }

    mz_zip_reader_end(&zipArchive);
}

std::string ZipArchiveService::readEntryString(
    const std::filesystem::path& zipFilePath,
    const std::string& entryRelativePath)
{
    mz_zip_archive zipArchive;
    std::memset(&zipArchive, 0, sizeof(zipArchive));

    std::string zipPathStr = zipFilePath.string();
    if (!mz_zip_reader_init_file(&zipArchive, zipPathStr.c_str(), 0)) {
        return "";
    }

    std::string targetNorm = PathUtils::normalizeRelativePath(entryRelativePath);
    mz_uint numFiles = mz_zip_reader_get_num_files(&zipArchive);
    std::string result;

    for (mz_uint i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat fileStat;
        if (mz_zip_reader_file_stat(&zipArchive, i, &fileStat)) {
            std::string curNorm = PathUtils::normalizeRelativePath(fileStat.m_filename);
            if (curNorm == targetNorm) {
                size_t uncompSize = 0;
                void* pData = mz_zip_reader_extract_to_heap(&zipArchive, i, &uncompSize, 0);
                if (pData) {
                    result.assign(static_cast<const char*>(pData), uncompSize);
                    mz_free(pData);
                }
                break;
            }
        }
    }

    mz_zip_reader_end(&zipArchive);
    return result;
}

} // namespace shipdesk::infrastructure
