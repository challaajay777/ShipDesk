#include "infrastructure/filesystem/path_utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace shipdesk::infrastructure {

std::string PathUtils::normalizeRelativePath(const std::string& input) {
    if (input.empty()) return "";

    std::string result;
    result.reserve(input.size());

    // Convert backslashes to forward slashes and collapse duplicate slashes
    bool lastWasSlash = false;
    for (char c : input) {
        if (c == '\\' || c == '/') {
            if (!lastWasSlash) {
                result.push_back('/');
                lastWasSlash = true;
            }
        } else {
            result.push_back(c);
            lastWasSlash = false;
        }
    }

    // Strip leading slash
    while (!result.empty() && result.front() == '/') {
        result.erase(result.begin());
    }

    // Strip trailing slash
    while (!result.empty() && result.back() == '/') {
        result.pop_back();
    }

    return result;
}

bool PathUtils::isSafeRelativePath(const std::string& relativePath) {
    if (relativePath.empty()) return false;

    // Check for drive letter or leading slashes
    if (relativePath.front() == '/' || relativePath.front() == '\\') return false;
    if (relativePath.size() >= 2 && relativePath[1] == ':') return false;

    // Component-wise check for '..' or '.'
    std::string comp;
    std::istringstream iss(relativePath);
    while (std::getline(iss, comp, '/')) {
        if (comp == "..") return false;
    }
    // Also check backslashes just in case
    std::istringstream issBack(relativePath);
    while (std::getline(issBack, comp, '\\')) {
        if (comp == "..") return false;
    }

    return true;
}

bool PathUtils::isContainedWithin(const std::filesystem::path& child,
                                  const std::filesystem::path& parent,
                                  bool allowEqual)
{
    std::error_code ec;
    auto canonicalParent = std::filesystem::weakly_canonical(parent, ec);
    if (ec) return false;

    auto canonicalChild = std::filesystem::weakly_canonical(child, ec);
    if (ec) return false;

    auto pIt = canonicalParent.begin();
    auto cIt = canonicalChild.begin();

    while (pIt != canonicalParent.end() && cIt != canonicalChild.end()) {
        std::string pStr = pIt->string();
        std::string cStr = cIt->string();
#if defined(_WIN32)
        std::transform(pStr.begin(), pStr.end(), pStr.begin(), ::tolower);
        std::transform(cStr.begin(), cStr.end(), cStr.begin(), ::tolower);
#endif
        if (pStr != cStr) {
            return false;
        }
        ++pIt;
        ++cIt;
    }

    if (pIt != canonicalParent.end()) {
        // Parent has more components than child
        return false;
    }

    if (cIt == canonicalChild.end()) {
        // Equal paths
        return allowEqual;
    }

    return true;
}

bool PathUtils::hasCaseCollision(const std::string& path1, const std::string& path2) {
    if (path1 == path2) return false;
    if (path1.size() != path2.size()) return false;

    for (size_t i = 0; i < path1.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(path1[i])) !=
            std::tolower(static_cast<unsigned char>(path2[i]))) {
            return false;
        }
    }
    return true;
}

uint64_t PathUtils::getAvailableDiskSpace(const std::filesystem::path& path) {
    std::error_code ec;
    auto spaceInfo = std::filesystem::space(path, ec);
    if (!ec) {
        return spaceInfo.available;
    }
    // If path doesn't exist yet, try parent directory
    if (path.has_parent_path()) {
        spaceInfo = std::filesystem::space(path.parent_path(), ec);
        if (!ec) {
            return spaceInfo.available;
        }
    }
    return 0;
}

bool PathUtils::isSymlinkOrReparsePoint(const std::filesystem::path& path) {
    std::error_code ec;
    auto status = std::filesystem::symlink_status(path, ec);
    if (!ec && std::filesystem::is_symlink(status)) {
        return true;
    }
#if defined(_WIN32)
    DWORD attrs = GetFileAttributesW(path.wstring().c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_REPARSE_POINT)) {
        return true;
    }
#endif
    return false;
}

std::string PathUtils::formatByteSize(uint64_t bytes) {
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;
    constexpr double GB = MB * 1024.0;

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);

    if (bytes >= GB) {
        ss << (bytes / GB) << " GB";
    } else if (bytes >= MB) {
        ss << (bytes / MB) << " MB";
    } else if (bytes >= KB) {
        ss << (bytes / KB) << " KB";
    } else {
        ss << bytes << " B";
    }
    return ss.str();
}

bool PathUtils::matchesGlob(const std::string& path, const std::string& pattern) {
    // Basic recursive wildcard matching
    size_t pIdx = 0, patIdx = 0;
    size_t pStar = std::string::npos, patStar = std::string::npos;

    while (pIdx < path.size()) {
        if (patIdx < pattern.size() && (pattern[patIdx] == '?' ||
            std::tolower(static_cast<unsigned char>(pattern[patIdx])) ==
            std::tolower(static_cast<unsigned char>(path[pIdx]))))
        {
            ++pIdx;
            ++patIdx;
        } else if (patIdx < pattern.size() && pattern[patIdx] == '*') {
            patStar = patIdx++;
            pStar = pIdx;
        } else if (patStar != std::string::npos) {
            patIdx = patStar + 1;
            pIdx = ++pStar;
        } else {
            return false;
        }
    }

    while (patIdx < pattern.size() && pattern[patIdx] == '*') {
        ++patIdx;
    }

    return patIdx == pattern.size();
}

} // namespace shipdesk::infrastructure
