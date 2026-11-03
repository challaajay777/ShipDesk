#include "domain/validation/validator.h"
#include <set>
#include <algorithm>
#include <cctype>

namespace shipdesk::domain {

static std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}

std::vector<ValidationFinding> ReleaseValidator::validateInventory(
    const std::vector<SourceInventoryItem>& inventory,
    const ReleaseProfile& profile,
    const ReleaseDraft& draft)
{
    std::vector<ValidationFinding> findings;

    uint64_t includedFilesCount = 0;
    uint64_t totalBytes = 0;
    std::set<std::string> seenPaths;
    std::set<std::string> seenLowerPaths;
    bool hasReadme = false;
    bool hasLicense = false;

    for (const auto& item : inventory) {
        if (!item.included) {
            continue;
        }

        // Check path traversal & absolute paths
        if (item.relative_path.empty() ||
            item.relative_path.find("..") != std::string::npos ||
            item.relative_path.front() == '/' ||
            item.relative_path.front() == '\\' ||
            (item.relative_path.size() >= 2 && item.relative_path[1] == ':'))
        {
            ValidationFinding f;
            f.rule_id = "VAL-007";
            f.severity = FindingSeverity::Blocker;
            f.title = "Unsafe Relative Path Traversal";
            f.explanation = "A relative path contains parent traversal ('..') or absolute elements.";
            f.evidence = "Path: " + item.relative_path;
            f.suggested_action = "Exclude or sanitize this entry before packaging.";
            f.blocks_packaging = true;
            f.blocks_publishing = true;
            findings.push_back(f);
        }

        std::string lowerPath = toLower(item.relative_path);

        // Check duplicate normalized paths
        bool isDuplicate = (seenPaths.find(item.relative_path) != seenPaths.end());
        if (isDuplicate) {
            ValidationFinding f;
            f.rule_id = "VAL-008";
            f.severity = FindingSeverity::Blocker;
            f.title = "Duplicate Output Path";
            f.explanation = "Multiple entries resolve to the exact same normalized output path.";
            f.evidence = "Path: " + item.relative_path;
            f.suggested_action = "Ensure all included relative paths are unique.";
            f.blocks_packaging = true;
            findings.push_back(f);
        } else {
            seenPaths.insert(item.relative_path);

            // Case collision check (only evaluated for distinct paths)
            if (seenLowerPaths.find(lowerPath) != seenLowerPaths.end()) {
                ValidationFinding f;
                f.rule_id = "VAL-009";
                f.severity = FindingSeverity::Warning;
                f.title = "Case-Only Path Collision";
                f.explanation = "Two entries differ only by casing. This causes collisions on case-insensitive filesystems (Windows/macOS).";
                f.evidence = "Path: " + item.relative_path;
                f.suggested_action = "Rename one of the colliding files to prevent destination overwrites.";
                findings.push_back(f);
            } else {
                seenLowerPaths.insert(lowerPath);
            }
        }

        if (item.isFile()) {
            includedFilesCount++;
            totalBytes += item.size_bytes;

            // Check per-file size limit
            if (profile.maxPerFileBytes() > 0 && item.size_bytes > profile.maxPerFileBytes()) {
                ValidationFinding f;
                f.rule_id = "VAL-003";
                f.severity = FindingSeverity::Warning;
                f.title = "Large File Exceeds Recommended Limit";
                f.explanation = "File size exceeds the configured profile threshold.";
                f.evidence = "File: " + item.relative_path + " (" + std::to_string(item.size_bytes / (1024 * 1024)) + " MB)";
                f.suggested_action = "Review if this large file should be distributed or compressed.";
                findings.push_back(f);
            }

            // Check hidden file
            if (item.is_hidden && !profile.includeHiddenFiles()) {
                ValidationFinding f;
                f.rule_id = "VAL-005";
                f.severity = FindingSeverity::Warning;
                f.title = "Hidden File Included";
                f.explanation = "File is marked as hidden or starts with a dot.";
                f.evidence = "File: " + item.relative_path;
                f.suggested_action = "Exclude hidden files or enable hidden file inclusion in profile settings.";
                findings.push_back(f);
            }

            // Software checklist recommendations
            if (lowerPath.find("readme") != std::string::npos) hasReadme = true;
            if (lowerPath.find("license") != std::string::npos || lowerPath.find("copying") != std::string::npos) hasLicense = true;
        }

        if (item.item_type == SourceItemType::BrokenLink) {
            ValidationFinding f;
            f.rule_id = "VAL-006";
            f.severity = FindingSeverity::Error;
            f.title = "Broken Symlink or Inaccessible Target";
            f.explanation = "A symbolic link or junction points to a non-existent target.";
            f.evidence = "Path: " + item.relative_path;
            f.suggested_action = "Remove or exclude the broken link from the release.";
            f.blocks_packaging = true;
            findings.push_back(f);
        }
    }

    // Check empty package
    if (includedFilesCount == 0) {
        ValidationFinding f;
        f.rule_id = "VAL-001";
        f.severity = FindingSeverity::Blocker;
        f.title = "No Files Selected for Release";
        f.explanation = "The release inventory does not contain any included regular files.";
        f.evidence = "Included files count: 0";
        f.suggested_action = "Adjust inclusion/exclusion filters or select at least one source file.";
        f.blocks_packaging = true;
        findings.push_back(f);
    }

    // Check total count limit
    if (profile.maxFileCount() > 0 && includedFilesCount > profile.maxFileCount()) {
        ValidationFinding f;
        f.rule_id = "VAL-004";
        f.severity = FindingSeverity::Error;
        f.title = "File Count Exceeds Profile Limit";
        f.explanation = "Total number of included files exceeds profile threshold.";
        f.evidence = "Count: " + std::to_string(includedFilesCount) + " > " + std::to_string(profile.maxFileCount());
        f.suggested_action = "Exclude unnecessary assets or increase the limit in profile settings.";
        f.blocks_packaging = true;
        findings.push_back(f);
    }

    // Check total bytes limit
    if (profile.maxTotalBytes() > 0 && totalBytes > profile.maxTotalBytes()) {
        ValidationFinding f;
        f.rule_id = "VAL-002";
        f.severity = FindingSeverity::Error;
        f.title = "Total Size Exceeds Profile Limit";
        f.explanation = "Total uncompressed size of included files exceeds profile threshold.";
        f.evidence = "Size: " + std::to_string(totalBytes / (1024 * 1024)) + " MB";
        f.suggested_action = "Exclude heavy files or increase threshold.";
        f.blocks_packaging = true;
        findings.push_back(f);
    }

    // Software application recommendations (never universal blockers!)
    if (profile.category() == "Software Application" || profile.category() == "Source Code Release") {
        if (!hasReadme) {
            ValidationFinding f;
            f.rule_id = "VAL-010-README";
            f.severity = FindingSeverity::Info;
            f.title = "README Recommendation";
            f.explanation = "No README file detected in the release inventory.";
            f.evidence = "Missing README.md or README.txt";
            f.suggested_action = "Consider providing a README to guide recipients.";
            f.blocks_packaging = false;
            f.blocks_publishing = false;
            findings.push_back(f);
        }
        if (!hasLicense) {
            ValidationFinding f;
            f.rule_id = "VAL-010-LICENSE";
            f.severity = FindingSeverity::Info;
            f.title = "License File Recommendation";
            f.explanation = "No LICENSE or COPYING file detected in release inventory.";
            f.evidence = "Missing LICENSE";
            f.suggested_action = "Consider including a license document to clarify usage terms.";
            f.blocks_packaging = false;
            f.blocks_publishing = false;
            findings.push_back(f);
        }
    }

    // Version sanity check
    if (draft.version().empty()) {
        ValidationFinding f;
        f.rule_id = "VAL-011";
        f.severity = FindingSeverity::Blocker;
        f.title = "Release Version Required";
        f.explanation = "A release must have a non-empty version label.";
        f.evidence = "Version is empty";
        f.suggested_action = "Specify a version label (e.g. 1.0.0).";
        f.blocks_packaging = true;
        findings.push_back(f);
    }

    return findings;
}

} // namespace shipdesk::domain
