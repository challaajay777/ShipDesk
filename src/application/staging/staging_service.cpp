#include "application/staging/staging_service.h"
#include "infrastructure/filesystem/path_utils.h"
#include "infrastructure/hashing/hasher.h"
#include "domain/common/json_util.h"
#include <QCryptographicHash>
#include <QByteArrayView>
#include <QFile>
#include <fstream>
#include <sstream>
#include <set>

namespace shipdesk::application {

StagingResult StagingService::stageItems(
    const std::filesystem::path& baseStagingRoot,
    const std::string& releaseRunId,
    const std::vector<domain::SourceInventoryItem>& itemsToStage,
    uint64_t maxTotalBytes,
    std::atomic<bool>* cancelFlag,
    std::function<void(size_t index, size_t total, uint64_t bytesDone, const std::string& currentPath)> progress)
{
    StagingResult result;
    result.job_id = releaseRunId;

    std::filesystem::path jobDir = baseStagingRoot / ("stage_" + releaseRunId);
    std::filesystem::path contentDir = jobDir / "content";
    result.staging_job_dir = jobDir;

    std::error_code ec;
    std::filesystem::create_directories(contentDir, ec);
    if (ec) {
        result.success = false;
        result.error_message = "Failed to create staging directory: " + jobDir.string() + " (" + ec.message() + ")";
        return result;
    }

    // Write ownership marker file
    std::filesystem::path ownerFile = jobDir / "shipdesk.owner";
    {
        std::ofstream ofs(ownerFile);
        if (ofs.is_open()) {
            ofs << "{\n";
            ofs << "  \"job_id\": \"" << domain::json::escapeString(releaseRunId) << "\",\n";
            ofs << "  \"created_at\": \"" << domain::currentUtcTimestamp() << "\",\n";
            ofs << "  \"owner\": \"ShipDesk Staging Engine\"\n";
            ofs << "}\n";
        }
    }

    // Filter included regular files
    std::vector<const domain::SourceInventoryItem*> filesToCopy;
    uint64_t expectedTotalBytes = 0;
    std::set<std::string> seenRelPaths;

    for (const auto& item : itemsToStage) {
        if (!item.included || !item.isFile()) continue;

        std::string normRel = infrastructure::PathUtils::normalizeRelativePath(item.relative_path);
        if (!infrastructure::PathUtils::isSafeRelativePath(normRel)) {
            result.success = false;
            result.error_message = "Path traversal attempted in item: " + item.relative_path;
            cleanupOwnedStaging(jobDir, releaseRunId);
            return result;
        }

        if (seenRelPaths.find(normRel) != seenRelPaths.end()) {
            result.success = false;
            result.error_message = "Duplicate normalized path in staging list: " + normRel;
            cleanupOwnedStaging(jobDir, releaseRunId);
            return result;
        }
        seenRelPaths.insert(normRel);

        filesToCopy.push_back(&item);
        expectedTotalBytes += item.size_bytes;
    }

    // Check disk space
    uint64_t freeBytes = infrastructure::PathUtils::getAvailableDiskSpace(jobDir);
    if (freeBytes > 0 && freeBytes < (expectedTotalBytes + (10 * 1024 * 1024))) { // +10MB safety margin
        result.success = false;
        result.error_message = "Insufficient disk space for staging. Needed: " +
            infrastructure::PathUtils::formatByteSize(expectedTotalBytes) +
            ", Available: " + infrastructure::PathUtils::formatByteSize(freeBytes);
        cleanupOwnedStaging(jobDir, releaseRunId);
        return result;
    }

    uint64_t bytesStagedSoFar = 0;

    for (size_t i = 0; i < filesToCopy.size(); ++i) {
        if (cancelFlag && cancelFlag->load()) {
            result.cancelled = true;
            result.success = false;
            result.error_message = "Staging cancelled by user.";
            cleanupOwnedStaging(jobDir, releaseRunId);
            return result;
        }

        const auto* item = filesToCopy[i];
        std::string relPath = infrastructure::PathUtils::normalizeRelativePath(item->relative_path);
        std::filesystem::path targetPath = contentDir / relPath;

        // Path containment security check
        if (!infrastructure::PathUtils::isContainedWithin(targetPath, contentDir)) {
            result.success = false;
            result.error_message = "Target path escapes content directory: " + targetPath.string();
            cleanupOwnedStaging(jobDir, releaseRunId);
            return result;
        }

        // Create parent directories inside content
        if (targetPath.has_parent_path()) {
            std::filesystem::create_directories(targetPath.parent_path(), ec);
        }

        // Capture source file modified time before copy
        std::filesystem::path srcPath(item->absolute_path);
        auto mtimeBefore = std::filesystem::last_write_time(srcPath, ec);

        // Stream copy & hash simultaneously
        QFile srcFile(QString::fromStdWString(srcPath.wstring()));
        if (!srcFile.open(QIODevice::ReadOnly)) {
            result.success = false;
            result.error_message = "Failed to open source file for reading: " + srcPath.string();
            cleanupOwnedStaging(jobDir, releaseRunId);
            return result;
        }

        QFile dstFile(QString::fromStdWString(targetPath.wstring()));
        if (!dstFile.open(QIODevice::WriteOnly)) {
            srcFile.close();
            result.success = false;
            result.error_message = "Failed to open staging destination for writing: " + targetPath.string();
            cleanupOwnedStaging(jobDir, releaseRunId);
            return result;
        }

        QCryptographicHash hash(QCryptographicHash::Sha256);
        constexpr qint64 bufSize = 64 * 1024;
        char buffer[bufSize];
        uint64_t fileBytesWritten = 0;

        while (!srcFile.atEnd()) {
            if (cancelFlag && cancelFlag->load()) {
                srcFile.close();
                dstFile.close();
                result.cancelled = true;
                result.success = false;
                result.error_message = "Staging cancelled.";
                cleanupOwnedStaging(jobDir, releaseRunId);
                return result;
            }

            qint64 readBytes = srcFile.read(buffer, bufSize);
            if (readBytes < 0) {
                srcFile.close();
                dstFile.close();
                result.success = false;
                result.error_message = "Read failure during staging: " + srcPath.string();
                cleanupOwnedStaging(jobDir, releaseRunId);
                return result;
            }

            if (readBytes > 0) {
                qint64 written = dstFile.write(buffer, readBytes);
                if (written != readBytes) {
                    srcFile.close();
                    dstFile.close();
                    result.success = false;
                    result.error_message = "Short write / disk full during staging: " + targetPath.string();
                    cleanupOwnedStaging(jobDir, releaseRunId);
                    return result;
                }
                hash.addData(QByteArrayView(buffer, readBytes));
                fileBytesWritten += written;
                bytesStagedSoFar += written;

                if (bytesStagedSoFar > maxTotalBytes) {
                    srcFile.close();
                    dstFile.close();
                    result.success = false;
                    result.error_message = "Staging exceeded maximum total size limit of " +
                        infrastructure::PathUtils::formatByteSize(maxTotalBytes);
                    cleanupOwnedStaging(jobDir, releaseRunId);
                    return result;
                }
            }
        }

        dstFile.flush();
        dstFile.close();
        srcFile.close();

        // Check if source was mutated during copy
        auto mtimeAfter = std::filesystem::last_write_time(srcPath, ec);
        bool sourceMutated = (mtimeBefore != mtimeAfter);
        if (sourceMutated) {
            result.warnings.push_back("Source file modified while copying: " + relPath);
        }

        std::string fileSha256 = hash.result().toHex().toStdString();

        StagedItem stagedItem;
        stagedItem.relative_path = relPath;
        stagedItem.staged_disk_path = targetPath;
        stagedItem.size_bytes = fileBytesWritten;
        stagedItem.sha256_hash = fileSha256;
        stagedItem.source_mutated_during_copy = sourceMutated;

        result.staged_items.push_back(stagedItem);
        result.total_staged_bytes += fileBytesWritten;
        result.total_staged_files++;

        if (progress) {
            progress(i + 1, filesToCopy.size(), bytesStagedSoFar, relPath);
        }
    }

    result.success = true;
    return result;
}

bool StagingService::cleanupOwnedStaging(const std::filesystem::path& stagingJobDir, const std::string& expectedJobId) {
    std::filesystem::path ownerFile = stagingJobDir / "shipdesk.owner";
    if (!std::filesystem::exists(ownerFile)) {
        // Ownership uncertain: retain directory for safety!
        return false;
    }

    // Verify ownership marker contains expectedJobId
    std::ifstream ifs(ownerFile);
    if (!ifs.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    if (content.find(expectedJobId) == std::string::npos) {
        // Mismatch: ownership belongs to another job!
        return false;
    }

    std::error_code ec;
    std::filesystem::remove_all(stagingJobDir, ec);
    return !ec;
}

} // namespace shipdesk::application
