#include "adapters/destinations/local_folder_destination.h"
#include "infrastructure/hashing/hasher.h"
#include <system_error>

namespace shipdesk::adapters {

LocalPublishResult LocalFolderDestinationAdapter::publishArtifact(
    const std::filesystem::path& artifactPath,
    const std::filesystem::path& targetFolder,
    bool overwriteConfirmed)
{
    LocalPublishResult result;
    std::error_code ec;

    if (!std::filesystem::exists(artifactPath, ec)) {
        result.success = false;
        result.error_message = "Source artifact does not exist: " + artifactPath.string();
        return result;
    }

    std::filesystem::create_directories(targetFolder, ec);
    if (ec) {
        result.success = false;
        result.error_message = "Failed to create target folder: " + targetFolder.string();
        return result;
    }

    std::filesystem::path destPath = targetFolder / artifactPath.filename();
    result.destination_path = destPath;

    if (std::filesystem::exists(destPath, ec)) {
        if (!overwriteConfirmed) {
            result.success = false;
            result.error_message = "Destination already exists and overwrite was not confirmed: " + destPath.string();
            return result;
        }
        // Ensure write permissions on existing target before overwrite
        std::filesystem::permissions(destPath, std::filesystem::perms::owner_write, std::filesystem::perm_options::add, ec);
        ec.clear();
    }

    if (std::filesystem::is_directory(artifactPath, ec)) {
        std::filesystem::copy(
            artifactPath, destPath,
            std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing,
            ec
        );
        if (ec) {
            result.success = false;
            result.error_message = "Directory copy failed: " + ec.message();
            return result;
        }
        result.success = true;
        return result;
    }

    // File copy
    std::filesystem::copy_file(
        artifactPath, destPath,
        std::filesystem::copy_options::overwrite_existing,
        ec
    );
    if (ec) {
        result.success = false;
        result.error_message = "File copy failed: " + ec.message();
        return result;
    }

    result.copied_bytes = std::filesystem::file_size(destPath, ec);
    // Re-verify digest after final copy
    result.sha256_hash = infrastructure::Hasher::sha256File(destPath);
    result.success = true;
    return result;
}

} // namespace shipdesk::adapters
