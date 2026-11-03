#include "application/publishing/publishing_service.h"
#include "adapters/destinations/local_folder_destination.h"
#include "adapters/destinations/github_destination.h"
#include "infrastructure/database/database_manager.h"
#include "domain/common/json_util.h"
#include <QUuid>

namespace shipdesk::application {

PublishResult PublishingService::publishArtifact(
    const std::string& destinationId,
    const std::string& artifactId,
    const std::filesystem::path& artifactPath,
    domain::DestinationKind kind,
    const std::string& destinationConfigJson,
    const std::string& releaseTag,
    const std::string& releaseTitle,
    const std::string& releaseNotes,
    bool overwriteConfirmed)
{
    PublishResult result;
    result.destination_id = destinationId;

    domain::PublishAttemptRecord attempt;
    attempt.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    attempt.artifact_id = artifactId;
    attempt.destination_id = destinationId;
    attempt.status = "Started";
    attempt.started_at_utc = domain::currentUtcTimestamp();

    if (kind == domain::DestinationKind::LocalFolder) {
        std::string targetDir = domain::json::extractStringField(destinationConfigJson, "target_directory");
        if (targetDir.empty()) {
            targetDir = destinationConfigJson; // direct path
        }

        auto localRes = adapters::LocalFolderDestinationAdapter::publishArtifact(
            artifactPath, targetDir, overwriteConfirmed
        );

        attempt.finished_at_utc = domain::currentUtcTimestamp();
        if (localRes.success) {
            attempt.status = "Published";
            attempt.external_url = localRes.destination_path.string();
            result.success = true;
            result.external_url = localRes.destination_path.string();
        } else {
            attempt.status = "PublishFailed";
            attempt.error_json = "{\"error\": \"" + domain::json::escapeString(localRes.error_message) + "\"}";
            result.success = false;
            result.error_message = localRes.error_message;
        }

    } else if (kind == domain::DestinationKind::GitHubReleases) {
        std::string repo = domain::json::extractStringField(destinationConfigJson, "repository");
        bool isDraft = domain::json::extractBoolField(destinationConfigJson, "is_draft", true);
        bool isPrerelease = domain::json::extractBoolField(destinationConfigJson, "is_prerelease", false);

        auto ghRes = adapters::GitHubDestinationAdapter::publishReleaseAndAsset(
            repo, releaseTag, releaseTitle, releaseNotes, artifactPath, isDraft, isPrerelease
        );

        attempt.finished_at_utc = domain::currentUtcTimestamp();
        if (ghRes.success) {
            attempt.status = "Published";
            attempt.external_url = ghRes.release_url;
            result.success = true;
            result.external_url = ghRes.release_url;
        } else {
            attempt.status = "PublishFailed";
            attempt.error_json = "{\"error\": \"" + domain::json::escapeString(ghRes.error_message) + "\"}";
            result.success = false;
            result.error_message = ghRes.error_message;
        }
    }

    return result;
}

} // namespace shipdesk::application
