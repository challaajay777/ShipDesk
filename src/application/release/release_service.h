#pragma once

#include "domain/release/release_draft.h"
#include "domain/release/release_record.h"
#include "domain/validation/validation_finding.h"
#include "domain/artifact/verification_report.h"
#include "application/staging/staging_service.h"
#include "application/packaging/packaging_service.h"
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <atomic>

namespace shipdesk::application {

struct VerticalSliceResult {
    bool success{false};
    bool cancelled{false};
    std::string release_draft_id;
    std::string release_run_id;
    std::string release_record_id;
    domain::ReleaseState final_state{domain::ReleaseState::Draft};
    std::vector<domain::ValidationFinding> validation_findings;
    PackagingResult packaging_result;
    domain::VerificationReport verification_report;
    std::string error_message;
};

class ReleaseService {
public:
    ReleaseService() = default;

    domain::ReleaseDraft createDraft(
        const std::string& workspaceId,
        const std::string& profileId,
        const std::string& title,
        const std::string& version,
        domain::ReleaseChannel channel = domain::ReleaseChannel::Draft,
        const std::string& description = "",
        const std::string& releaseNotes = ""
    );

    bool updateDraft(const domain::ReleaseDraft& draft);
    std::optional<domain::ReleaseDraft> getDraft(const std::string& draftId);
    std::vector<domain::ReleaseDraft> listDrafts(const std::string& workspaceId);

    // Executes a complete vertical slice end-to-end:
    // Inspect -> Validate -> Stage -> Package -> Manifest/Hash -> Verify -> Save Immutable Record!
    VerticalSliceResult executeVerticalSlice(
        const std::string& draftId,
        const std::filesystem::path& baseStagingDir,
        const std::filesystem::path& outputDirectory,
        domain::PackageFormat packageFormat,
        std::atomic<bool>* cancelFlag = nullptr,
        std::function<void(const std::string& phase, int progressPercent, const std::string& detail)> progress = nullptr
    );

    // Recovers any transient runs that were abandoned due to prior app restart
    int recoverInterruptedRuns();
};

} // namespace shipdesk::application
