#include "application/release/release_service.h"
#include "application/inspection/source_inspection_service.h"
#include "application/staging/staging_service.h"
#include "application/packaging/packaging_service.h"
#include "application/verification/verification_service.h"
#include "domain/validation/validator.h"
#include "infrastructure/database/database_manager.h"
#include "infrastructure/filesystem/path_utils.h"
#include "domain/common/json_util.h"
#include <QUuid>

namespace shipdesk::application {

domain::ReleaseDraft ReleaseService::createDraft(
    const std::string& workspaceId,
    const std::string& profileId,
    const std::string& title,
    const std::string& version,
    domain::ReleaseChannel channel,
    const std::string& description,
    const std::string& releaseNotes)
{
    std::string id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    domain::ReleaseDraft draft(id, workspaceId, profileId, title, version, channel);
    draft.setDescription(description);
    draft.setReleaseNotes(releaseNotes);

    infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);

    domain::EventLogRecord ev;
    ev.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    ev.workspace_id = workspaceId;
    ev.severity = "Info";
    ev.event_type = "ReleaseDraftCreated";
    ev.message = "Created release draft '" + title + "' v" + version;
    infrastructure::DatabaseManager::instance().logEvent(ev);

    return draft;
}

bool ReleaseService::updateDraft(const domain::ReleaseDraft& draft) {
    return infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);
}

std::optional<domain::ReleaseDraft> ReleaseService::getDraft(const std::string& draftId) {
    return infrastructure::DatabaseManager::instance().getReleaseDraft(draftId);
}

std::vector<domain::ReleaseDraft> ReleaseService::listDrafts(const std::string& workspaceId) {
    return infrastructure::DatabaseManager::instance().listReleaseDrafts(workspaceId);
}

VerticalSliceResult ReleaseService::executeVerticalSlice(
    const std::string& draftId,
    const std::filesystem::path& baseStagingDir,
    const std::filesystem::path& outputDirectory,
    domain::PackageFormat packageFormat,
    std::atomic<bool>* cancelFlag,
    std::function<void(const std::string& phase, int progressPercent, const std::string& detail)> progress)
{
    VerticalSliceResult sliceResult;
    sliceResult.release_draft_id = draftId;

    auto optDraft = infrastructure::DatabaseManager::instance().getReleaseDraft(draftId);
    if (!optDraft) {
        sliceResult.success = false;
        sliceResult.error_message = "Release draft not found: " + draftId;
        return sliceResult;
    }

    auto draft = *optDraft;
    auto optProfile = infrastructure::DatabaseManager::instance().getReleaseProfile(draft.profileId());
    domain::ReleaseProfile profile = optProfile ? *optProfile : domain::ReleaseProfile::createDefault("General File Bundle", draft.workspaceId());

    auto roots = infrastructure::DatabaseManager::instance().getSourceRoots(draft.workspaceId());
    if (roots.empty()) {
        sliceResult.success = false;
        sliceResult.error_message = "No source roots registered in this workspace.";
        return sliceResult;
    }

    // 1. Create Release Run
    std::string runId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    sliceResult.release_run_id = runId;

    domain::ReleaseRunRecord runRecord;
    runRecord.id = runId;
    runRecord.release_draft_id = draftId;
    runRecord.status = domain::ReleaseState::Inspecting;
    runRecord.started_at_utc = domain::currentUtcTimestamp();
    runRecord.profile_snapshot_json = profile.serializeSettingsJson();
    infrastructure::DatabaseManager::instance().saveReleaseRun(runRecord);

    draft.transitionTo(domain::ReleaseState::Inspecting);
    infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);

    if (progress) progress("Inspecting", 10, "Scanning source roots...");

    // 2. Inspection Phase
    SourceInspectionService inspectionService;
    std::vector<domain::SourceInventoryItem> allInventory;

    for (const auto& r : roots) {
        if (cancelFlag && cancelFlag->load()) {
            draft.transitionTo(domain::ReleaseState::Cancelled);
            infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);
            runRecord.status = domain::ReleaseState::Cancelled;
            infrastructure::DatabaseManager::instance().saveReleaseRun(runRecord);
            sliceResult.cancelled = true;
            sliceResult.final_state = domain::ReleaseState::Cancelled;
            return sliceResult;
        }

        auto inspectRes = inspectionService.inspectRoot(
            r.configured_path, r.id, profile, cancelFlag,
            [&progress](uint64_t count, const std::string& path) {
                if (progress) progress("Inspecting", 20, "Scanned " + std::to_string(count) + " files (" + path + ")");
            }
        );

        if (!inspectRes.success) {
            draft.transitionTo(domain::ReleaseState::NeedsReview);
            infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);
            runRecord.status = domain::ReleaseState::NeedsReview;
            runRecord.error_json = "{\"inspection_error\": \"" + domain::json::escapeString(inspectRes.error_message) + "\"}";
            infrastructure::DatabaseManager::instance().saveReleaseRun(runRecord);
            sliceResult.success = false;
            sliceResult.final_state = domain::ReleaseState::NeedsReview;
            sliceResult.error_message = inspectRes.error_message;
            return sliceResult;
        }

        allInventory.insert(allInventory.end(), inspectRes.items.begin(), inspectRes.items.end());
    }

    draft.transitionTo(domain::ReleaseState::Inspected);
    infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);

    // 3. Validation Phase
    draft.transitionTo(domain::ReleaseState::Validating);
    infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);
    if (progress) progress("Validating", 35, "Running rule validation...");

    auto findings = domain::ReleaseValidator::validateInventory(allInventory, profile, draft);
    sliceResult.validation_findings = findings;

    bool hasBlocker = false;
    for (const auto& f : findings) {
        if (f.isBlocker()) {
            hasBlocker = true;
            break;
        }
    }

    if (hasBlocker) {
        draft.transitionTo(domain::ReleaseState::NeedsReview);
        infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);
        runRecord.status = domain::ReleaseState::NeedsReview;
        runRecord.error_json = "{\"validation\": \"Blocker findings encountered\"}";
        infrastructure::DatabaseManager::instance().saveReleaseRun(runRecord);
        sliceResult.success = false;
        sliceResult.final_state = domain::ReleaseState::NeedsReview;
        sliceResult.error_message = "Validation found blocking errors preventing packaging.";
        return sliceResult;
    }

    draft.transitionTo(domain::ReleaseState::Validated);
    infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);

    // 4. Staging Phase (Read-only copy with simultaneous hashing)
    draft.transitionTo(domain::ReleaseState::Staging);
    infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);
    if (progress) progress("Staging", 50, "Copying files to private staging...");

    StagingService stagingService;
    auto stagingRes = stagingService.stageItems(
        baseStagingDir, runId, allInventory, profile.maxTotalBytes(), cancelFlag,
        [&progress](size_t idx, size_t total, uint64_t bytes, const std::string& path) {
            int pct = 50 + static_cast<int>(25.0 * idx / (total ? total : 1));
            if (progress) {
                progress("Staging", pct, "Staged " + std::to_string(idx) + "/" + std::to_string(total) +
                         " (" + infrastructure::PathUtils::formatByteSize(bytes) + "): " + path);
            }
        }
    );

    if (!stagingRes.success) {
        draft.transitionTo(domain::ReleaseState::NeedsReview);
        infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);
        runRecord.status = domain::ReleaseState::NeedsReview;
        runRecord.error_json = "{\"staging_error\": \"" + domain::json::escapeString(stagingRes.error_message) + "\"}";
        infrastructure::DatabaseManager::instance().saveReleaseRun(runRecord);
        sliceResult.success = false;
        sliceResult.final_state = domain::ReleaseState::NeedsReview;
        sliceResult.error_message = stagingRes.error_message;
        return sliceResult;
    }

    draft.transitionTo(domain::ReleaseState::Staged);
    infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);

    // 5. Packaging Phase
    draft.transitionTo(domain::ReleaseState::Packaging);
    infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);
    if (progress) progress("Packaging", 75, "Creating package and manifest...");

    domain::ReleaseManifest manifest;
    manifest.product_name = draft.title();
    manifest.workspace_id = draft.workspaceId();
    manifest.release_id = draft.id();
    manifest.version = draft.version();
    manifest.channel = domain::releaseChannelToString(draft.channel());
    manifest.created_at_utc = domain::currentUtcTimestamp();
    manifest.platform =
#if defined(_WIN32)
        "Windows-x64";
#elif defined(__linux__)
        "Linux-x64";
#else
        "Unknown";
#endif

    PackagingService packagingService;
    std::string basePackageName = draft.title() + "-v" + draft.version();
    auto packRes = packagingService.packageRelease(
        stagingRes, outputDirectory, basePackageName, packageFormat, manifest, cancelFlag,
        [&progress](size_t idx, size_t total, const std::string& path) {
            int pct = 75 + static_cast<int>(15.0 * idx / (total ? total : 1));
            if (progress) progress("Packaging", pct, "Packaging " + path);
        }
    );

    sliceResult.packaging_result = packRes;
    if (!packRes.success) {
        draft.transitionTo(domain::ReleaseState::NeedsReview);
        infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);
        runRecord.status = domain::ReleaseState::NeedsReview;
        runRecord.error_json = "{\"packaging_error\": \"" + domain::json::escapeString(packRes.error_message) + "\"}";
        infrastructure::DatabaseManager::instance().saveReleaseRun(runRecord);
        sliceResult.success = false;
        sliceResult.final_state = domain::ReleaseState::NeedsReview;
        sliceResult.error_message = packRes.error_message;
        return sliceResult;
    }

    draft.transitionTo(domain::ReleaseState::Packaged);
    infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);

    // 6. Verification Phase
    draft.transitionTo(domain::ReleaseState::Verifying);
    infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);
    if (progress) progress("Verifying", 90, "Verifying checksums and archive integrity...");

    auto verifyReport = VerificationService::verifyArtifact(packRes.artifact_path, packRes.manifest);
    sliceResult.verification_report = verifyReport;

    if (!verifyReport.is_valid) {
        draft.transitionTo(domain::ReleaseState::NeedsReview);
        infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);
        runRecord.status = domain::ReleaseState::NeedsReview;
        runRecord.error_json = "{\"verification_error\": \"Checksum or file count mismatch in packaged artifact\"}";
        infrastructure::DatabaseManager::instance().saveReleaseRun(runRecord);
        sliceResult.success = false;
        sliceResult.final_state = domain::ReleaseState::NeedsReview;
        sliceResult.error_message = "Package verification failed.";
        return sliceResult;
    }

    draft.transitionTo(domain::ReleaseState::Verified);
    draft.transitionTo(domain::ReleaseState::ReadyToPublish);
    infrastructure::DatabaseManager::instance().saveReleaseDraft(draft);

    runRecord.status = domain::ReleaseState::ReadyToPublish;
    runRecord.finished_at_utc = domain::currentUtcTimestamp();
    runRecord.summary_json = "{\"verified\": true, \"size\": " + std::to_string(packRes.artifact_size_bytes) +
                             ", \"sha256\": \"" + packRes.artifact_sha256 + "\"}";
    infrastructure::DatabaseManager::instance().saveReleaseRun(runRecord);

    // 7. Save Immutable Release Record
    std::string recordId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    domain::ImmutableReleaseRecord releaseRecord;
    releaseRecord.id = recordId;
    releaseRecord.release_run_id = runId;
    releaseRecord.manifest_path = packRes.manifest_path.string();
    releaseRecord.completed_at = domain::currentUtcTimestamp();
    releaseRecord.workspace_snapshot_json = "{\"workspace_id\": \"" + draft.workspaceId() + "\"}";
    releaseRecord.metadata_snapshot_json = "{\"version\": \"" + draft.version() + "\", \"title\": \"" + domain::json::escapeString(draft.title()) + "\"}";
    infrastructure::DatabaseManager::instance().saveReleaseRecord(releaseRecord);

    sliceResult.release_record_id = recordId;
    sliceResult.final_state = domain::ReleaseState::ReadyToPublish;
    sliceResult.success = true;

    // Clean up temporary owned staging directory
    StagingService::cleanupOwnedStaging(stagingRes.staging_job_dir, runId);

    if (progress) progress("Complete", 100, "Release packaged and verified successfully!");

    return sliceResult;
}

int ReleaseService::recoverInterruptedRuns() {
    return infrastructure::DatabaseManager::instance().recoverInterruptedRuns();
}

} // namespace shipdesk::application
