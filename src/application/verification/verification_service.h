#pragma once

#include "domain/artifact/verification_report.h"
#include "domain/artifact/manifest.h"
#include <filesystem>
#include <string>

namespace shipdesk::application {

class VerificationService {
public:
    VerificationService() = default;

    // Verifies an artifact against its manifest or expected files
    static domain::VerificationReport verifyArtifact(
        const std::filesystem::path& artifactPath,
        const domain::ReleaseManifest& manifest
    );
};

} // namespace shipdesk::application
