#include "application/packaging/packaging_service.h"
#include "infrastructure/archives/zip_archive.h"
#include "infrastructure/filesystem/path_utils.h"
#include "infrastructure/hashing/hasher.h"
#include <fstream>
#include <QUuid>

namespace shipdesk::application {

PackagingResult PackagingService::packageRelease(
    const StagingResult& staging,
    const std::filesystem::path& destinationDirectory,
    const std::string& packageBaseName,
    domain::PackageFormat format,
    domain::ReleaseManifest manifest,
    std::atomic<bool>* cancelFlag,
    std::function<void(size_t index, size_t total, const std::string& currentPath)> progress)
{
    PackagingResult result;
    result.package_format = format;

    std::error_code ec;
    std::filesystem::create_directories(destinationDirectory, ec);
    if (ec) {
        result.success = false;
        result.error_message = "Cannot create destination directory: " + destinationDirectory.string();
        return result;
    }

    // Populate manifest files from staged items
    manifest.files.clear();
    for (const auto& item : staging.staged_items) {
        domain::ManifestFileEntry entry;
        entry.relative_path = item.relative_path;
        entry.size_bytes = item.size_bytes;
        entry.sha256 = item.sha256_hash;
        entry.source_relative_path = item.relative_path;
        entry.classification = "regular";
        entry.inclusion_reason = "profile_rule_or_user_selection";
        manifest.files.push_back(entry);
    }

    // Write manifest to staging area first
    std::filesystem::path stagingManifestPath = staging.staging_job_dir / "shipdesk-release-manifest.json";
    {
        std::ofstream ofs(stagingManifestPath);
        if (!ofs.is_open()) {
            result.success = false;
            result.error_message = "Failed to write release manifest to staging";
            return result;
        }
        ofs << manifest.toJsonString();
    }

    if (format == domain::PackageFormat::ZipArchive) {
        std::string zipFileName = packageBaseName + ".zip";
        std::filesystem::path finalZipPath = destinationDirectory / zipFileName;
        std::filesystem::path tempZipPath = destinationDirectory / (packageBaseName + ".tmp.zip");

        std::vector<infrastructure::ZipEntryInput> zipEntries;
        // Add content files
        for (const auto& item : staging.staged_items) {
            infrastructure::ZipEntryInput input;
            input.disk_path = item.staged_disk_path;
            input.archive_relative_path = item.relative_path;
            zipEntries.push_back(input);
        }

        // Add manifest inside ZIP
        infrastructure::ZipEntryInput manifestInput;
        manifestInput.disk_path = stagingManifestPath;
        manifestInput.archive_relative_path = "shipdesk-release-manifest.json";
        zipEntries.push_back(manifestInput);

        try {
            infrastructure::ZipArchiveService::createZip(tempZipPath, zipEntries, progress);
        } catch (const std::exception& ex) {
            std::filesystem::remove(tempZipPath, ec);
            result.success = false;
            result.error_message = std::string("ZIP packaging failed: ") + ex.what();
            return result;
        }

        if (cancelFlag && cancelFlag->load()) {
            std::filesystem::remove(tempZipPath, ec);
            result.cancelled = true;
            result.success = false;
            result.error_message = "Packaging cancelled.";
            return result;
        }

        // Atomic rename / finalize
        if (std::filesystem::exists(finalZipPath)) {
            std::filesystem::remove(finalZipPath, ec);
        }
        std::filesystem::rename(tempZipPath, finalZipPath, ec);
        if (ec) {
            // Fallback to copy and remove if cross-device
            std::filesystem::copy_file(tempZipPath, finalZipPath, std::filesystem::copy_options::overwrite_existing, ec);
            std::filesystem::remove(tempZipPath, ec);
        }

        result.artifact_path = finalZipPath;
        result.artifact_filename = zipFileName;
        result.artifact_size_bytes = std::filesystem::file_size(finalZipPath, ec);
        result.artifact_sha256 = infrastructure::Hasher::sha256File(finalZipPath);

        // Record artifact metadata into manifest
        domain::ManifestArtifactEntry artEntry;
        artEntry.artifact_id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        artEntry.filename = zipFileName;
        artEntry.output_path = finalZipPath.string();
        artEntry.size_bytes = result.artifact_size_bytes;
        artEntry.sha256 = result.artifact_sha256;
        artEntry.package_format = "ZIP";
        artEntry.created_at_utc = domain::currentUtcTimestamp();
        manifest.artifacts = {artEntry};

        // Write finalized sidecar manifest alongside zip
        std::filesystem::path sidecarManifest = destinationDirectory / (packageBaseName + ".manifest.json");
        {
            std::ofstream ofs(sidecarManifest);
            if (ofs.is_open()) {
                ofs << manifest.toJsonString();
            }
        }
        result.manifest_path = sidecarManifest;
        result.manifest = manifest;

        result.success = true;
        return result;

    } else {
        // Directory package format
        std::filesystem::path targetDir = destinationDirectory / packageBaseName;
        std::filesystem::create_directories(targetDir, ec);

        uint64_t totalBytes = 0;
        for (size_t i = 0; i < staging.staged_items.size(); ++i) {
            if (cancelFlag && cancelFlag->load()) {
                result.cancelled = true;
                result.success = false;
                result.error_message = "Packaging cancelled.";
                return result;
            }

            const auto& item = staging.staged_items[i];
            std::filesystem::path destFile = targetDir / item.relative_path;
            if (destFile.has_parent_path()) {
                std::filesystem::create_directories(destFile.parent_path(), ec);
            }
            std::filesystem::copy_file(item.staged_disk_path, destFile, std::filesystem::copy_options::overwrite_existing, ec);
            totalBytes += item.size_bytes;

            if (progress) {
                progress(i + 1, staging.staged_items.size(), item.relative_path);
            }
        }

        // Record artifact metadata into manifest
        domain::ManifestArtifactEntry artEntry;
        artEntry.artifact_id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        artEntry.filename = packageBaseName;
        artEntry.output_path = targetDir.string();
        artEntry.size_bytes = totalBytes;
        artEntry.sha256 = "";
        artEntry.package_format = "Directory";
        artEntry.created_at_utc = domain::currentUtcTimestamp();
        manifest.artifacts = {artEntry};

        // Write manifest into directory
        std::filesystem::path destManifest = targetDir / "shipdesk-release-manifest.json";
        {
            std::ofstream ofs(destManifest);
            if (ofs.is_open()) {
                ofs << manifest.toJsonString();
            }
        }

        result.artifact_path = targetDir;
        result.artifact_filename = packageBaseName;
        result.artifact_size_bytes = totalBytes;
        result.manifest_path = destManifest;
        result.manifest = manifest;
        result.success = true;
        return result;
    }
}

} // namespace shipdesk::application
