#include "application/verification/verification_service.h"
#include "infrastructure/archives/zip_archive.h"
#include "infrastructure/filesystem/path_utils.h"
#include "infrastructure/hashing/hasher.h"
#include <map>
#include <set>
#include <fstream>

namespace shipdesk::application {

domain::VerificationReport VerificationService::verifyArtifact(
    const std::filesystem::path& artifactPath,
    const domain::ReleaseManifest& manifest)
{
    domain::ReleaseManifest effectiveManifest = manifest;

    // If no manifest files were passed, try auto-discovering embedded manifest
    if (effectiveManifest.files.empty()) {
        if (!std::filesystem::is_directory(artifactPath)) {
            // Check inside ZIP
            std::string embeddedJson = infrastructure::ZipArchiveService::readEntryString(
                artifactPath, "shipdesk-release-manifest.json"
            );
            if (!embeddedJson.empty()) {
                effectiveManifest = domain::ReleaseManifest::fromJsonString(embeddedJson);
            }
        } else {
            // Check inside directory
            std::filesystem::path manFile = artifactPath / "shipdesk-release-manifest.json";
            if (std::filesystem::exists(manFile)) {
                std::ifstream ifs(manFile);
                if (ifs.is_open()) {
                    std::string embeddedJson((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                    effectiveManifest = domain::ReleaseManifest::fromJsonString(embeddedJson);
                }
            }
        }
    }

    std::map<std::string, std::pair<uint64_t, std::string>> expectedFiles;
    for (const auto& f : effectiveManifest.files) {
        expectedFiles[infrastructure::PathUtils::normalizeRelativePath(f.relative_path)] = {f.size_bytes, f.sha256};
    }

    if (!std::filesystem::is_directory(artifactPath)) {
        // ZIP archive artifact
        auto report = infrastructure::ZipArchiveService::verifyZip(artifactPath, expectedFiles);
        return report;
    }

    // Directory package verification
    domain::VerificationReport report;
    report.artifact_path = artifactPath.string();
    report.expected_files_count = expectedFiles.size();
    report.archive_integrity_ok = true;
    report.is_valid = true;

    std::set<std::string> actualFiles;
    std::error_code ec;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(artifactPath, ec)) {
        if (entry.is_regular_file(ec)) {
            std::filesystem::path rel = std::filesystem::relative(entry.path(), artifactPath, ec);
            std::string normRel = infrastructure::PathUtils::normalizeRelativePath(rel.string());

            if (normRel == "shipdesk-release-manifest.json") {
                continue; // Skip embedded manifest from payload file list
            }

            actualFiles.insert(normRel);
            report.actual_files_count++;

            auto it = expectedFiles.find(normRel);
            if (!expectedFiles.empty() && it == expectedFiles.end()) {
                report.unexpected_files.push_back(normRel);
            } else if (it != expectedFiles.end()) {
                uint64_t actualSize = entry.file_size(ec);
                if (actualSize != it->second.first) {
                    report.mismatched_checksums.push_back({
                        normRel,
                        "Size mismatch: expected " + std::to_string(it->second.first) + " but got " + std::to_string(actualSize)
                    });
                    report.is_valid = false;
                }

                if (!it->second.second.empty()) {
                    std::string actualSha = infrastructure::Hasher::sha256File(entry.path());
                    if (actualSha != it->second.second) {
                        report.mismatched_checksums.push_back({
                            normRel,
                            "SHA-256 mismatch: expected " + it->second.second + " but got " + actualSha
                        });
                        report.is_valid = false;
                    }
                }
            }
        }
    }

    for (const auto& [path, _] : expectedFiles) {
        if (actualFiles.find(path) == actualFiles.end()) {
            report.missing_files.push_back(path);
            report.is_valid = false;
        }
    }

    if (!report.missing_files.empty() || !report.unexpected_files.empty() || !report.mismatched_checksums.empty()) {
        report.is_valid = false;
    }

    return report;
}

} // namespace shipdesk::application