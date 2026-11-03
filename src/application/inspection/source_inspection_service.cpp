#include "application/inspection/source_inspection_service.h"
#include "infrastructure/filesystem/path_utils.h"
#include <chrono>
#include <map>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace shipdesk::application {

InspectionResult SourceInspectionService::inspectRoot(
    const std::filesystem::path& rootPath,
    const std::string& sourceRootId,
    const domain::ReleaseProfile& profile,
    std::atomic<bool>* cancelFlag,
    std::function<void(uint64_t scannedCount, const std::string& currentPath)> progress)
{
    InspectionResult result;
    auto startTime = std::chrono::steady_clock::now();

    std::error_code ec;
    if (!std::filesystem::exists(rootPath, ec) || ec) {
        result.success = false;
        result.error_message = "Source root path does not exist: " + rootPath.string();
        return result;
    }

    // Single file root handling
    if (!std::filesystem::is_directory(rootPath, ec)) {
        domain::SourceInventoryItem item;
        item.source_root_id = sourceRootId;
        item.relative_path = rootPath.filename().string();
        item.absolute_path = std::filesystem::canonical(rootPath, ec).string();
        item.item_type = domain::SourceItemType::RegularFile;
        item.size_bytes = std::filesystem::file_size(rootPath, ec);
        item.included = true;

        result.items.push_back(item);
        result.total_files_count = 1;
        result.included_files_count = 1;
        result.included_bytes = item.size_bytes;
        result.success = true;
        result.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        return result;
    }

    uint64_t count = 0;
    constexpr int maxDepth = 64;

    try {
        std::filesystem::recursive_directory_iterator iter(
            rootPath,
            std::filesystem::directory_options::skip_permission_denied,
            ec
        );
        std::filesystem::recursive_directory_iterator end;

        while (iter != end && !ec) {
            if (cancelFlag && cancelFlag->load()) {
                result.cancelled = true;
                result.success = false;
                result.error_message = "Inspection cancelled by user.";
                return result;
            }

            const auto& entry = *iter;
            int currentDepth = iter.depth();
            if (currentDepth > maxDepth) {
                iter.pop();
                continue;
            }

            count++;
            if (progress && (count % 25 == 0)) {
                progress(count, entry.path().filename().string());
            }

            std::filesystem::path rel = std::filesystem::relative(entry.path(), rootPath, ec);
            std::string relStr = infrastructure::PathUtils::normalizeRelativePath(rel.string());

            domain::SourceInventoryItem item;
            item.source_root_id = sourceRootId;
            item.relative_path = relStr;
            item.absolute_path = entry.path().string();

            // Detect hidden
            std::string filenameStr = entry.path().filename().string();
            bool isHidden = (!filenameStr.empty() && filenameStr[0] == '.');
#if defined(_WIN32)
            DWORD attrs = GetFileAttributesW(entry.path().wstring().c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_HIDDEN)) {
                isHidden = true;
            }
#endif
            item.is_hidden = isHidden;

            // Check symlink / reparse point
            bool isLink = infrastructure::PathUtils::isSymlinkOrReparsePoint(entry.path());
            if (isLink) {
                item.item_type = domain::SourceItemType::Symlink;
                // Check if target exists
                std::error_code linkEc;
                if (!std::filesystem::exists(entry.path(), linkEc)) {
                    item.item_type = domain::SourceItemType::BrokenLink;
                }

                if (!profile.followSymlinks()) {
                    // Do not recurse into directory symlinks
                    if (entry.is_directory(ec)) {
                        iter.disable_recursion_pending();
                    }
                }
            } else if (entry.is_directory(ec)) {
                item.item_type = domain::SourceItemType::Directory;
                result.total_directories_count++;
            } else if (entry.is_regular_file(ec)) {
                item.item_type = domain::SourceItemType::RegularFile;
                item.size_bytes = entry.file_size(ec);
                result.total_files_count++;
            } else {
                item.item_type = domain::SourceItemType::Special;
            }

            // Inclusion / Exclusion logic
            bool matchedInclude = false;
            for (const auto& pat : profile.includePatterns()) {
                if (infrastructure::PathUtils::matchesGlob(relStr, pat) ||
                    infrastructure::PathUtils::matchesGlob(filenameStr, pat)) {
                    matchedInclude = true;
                    break;
                }
            }

            bool matchedExclude = false;
            std::string reason;
            for (const auto& pat : profile.excludePatterns()) {
                if (infrastructure::PathUtils::matchesGlob(relStr, pat) ||
                    infrastructure::PathUtils::matchesGlob(filenameStr, pat)) {
                    matchedExclude = true;
                    reason = "Matched exclusion rule: " + pat;
                    break;
                }
            }

            if (isHidden && !profile.includeHiddenFiles()) {
                matchedExclude = true;
                reason = "Hidden file (excluded by default profile)";
            }

            if (item.isSymlink() && !profile.followSymlinks()) {
                matchedExclude = true;
                reason = "Symlink (skipped by safe default)";
            }

            if (matchedExclude) {
                item.included = false;
                item.exclusion_reason = reason;
                result.skipped_count++;
                if (item.isDirectory()) {
                    iter.disable_recursion_pending();
                }
            } else if (matchedInclude) {
                item.included = true;
                if (item.isFile()) {
                    result.included_files_count++;
                    result.included_bytes += item.size_bytes;
                }
            } else {
                item.included = false;
                item.exclusion_reason = "Did not match include patterns";
                result.skipped_count++;
            }

            result.items.push_back(item);

            iter.increment(ec);
            if (ec) {
                result.error_count++;
                result.warnings.push_back("Error accessing entry: " + ec.message());
                ec.clear();
            }
        }
    } catch (const std::exception& ex) {
        result.warnings.push_back(std::string("Inspection exception: ") + ex.what());
    }

    result.success = true;
    result.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();

    if (progress) {
        progress(count, "Done");
    }

    return result;
}

void SourceInspectionService::compareSnapshots(
    const std::vector<domain::SourceInventoryItem>& oldItems,
    const std::vector<domain::SourceInventoryItem>& newItems,
    std::vector<std::string>& added,
    std::vector<std::string>& removed,
    std::vector<std::string>& changed)
{
    std::map<std::string, const domain::SourceInventoryItem*> oldMap;
    for (const auto& item : oldItems) {
        oldMap[item.relative_path] = &item;
    }

    std::map<std::string, const domain::SourceInventoryItem*> newMap;
    for (const auto& item : newItems) {
        newMap[item.relative_path] = &item;
    }

    for (const auto& [path, pNew] : newMap) {
        auto it = oldMap.find(path);
        if (it == oldMap.end()) {
            added.push_back(path);
        } else {
            const auto* pOld = it->second;
            if (pOld->size_bytes != pNew->size_bytes || pOld->modified_at_utc != pNew->modified_at_utc) {
                changed.push_back(path);
            }
        }
    }

    for (const auto& [path, _] : oldMap) {
        if (newMap.find(path) == newMap.end()) {
            removed.push_back(path);
        }
    }
}

} // namespace shipdesk::application
