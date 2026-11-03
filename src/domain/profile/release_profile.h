#pragma once

#include "domain/types.h"
#include <string>
#include <vector>
#include <cstdint>

namespace shipdesk::domain {

class ReleaseProfile {
public:
    ReleaseProfile() = default;
    ReleaseProfile(std::string id, std::string workspace_id, std::string name, std::string category = "General File Bundle");

    static ReleaseProfile createDefault(const std::string& category, const std::string& workspaceId = "");

    const std::string& id() const { return m_id; }
    const std::string& workspaceId() const { return m_workspaceId; }
    const std::string& name() const { return m_name; }
    const std::string& category() const { return m_category; }
    const std::string& schemaVersion() const { return m_schemaVersion; }

    const std::vector<std::string>& includePatterns() const { return m_includePatterns; }
    const std::vector<std::string>& excludePatterns() const { return m_excludePatterns; }
    const std::string& namingTemplate() const { return m_namingTemplate; }
    PackageFormat defaultFormat() const { return m_defaultFormat; }

    bool includeHiddenFiles() const { return m_includeHiddenFiles; }
    bool followSymlinks() const { return m_followSymlinks; }
    uint64_t maxTotalBytes() const { return m_maxTotalBytes; }
    uint64_t maxFileCount() const { return m_maxFileCount; }
    uint64_t maxPerFileBytes() const { return m_maxPerFileBytes; }

    void setName(const std::string& name) { m_name = name; }
    void setCategory(const std::string& cat) { m_category = cat; }
    void setIncludePatterns(std::vector<std::string> patterns) { m_includePatterns = std::move(patterns); }
    void setExcludePatterns(std::vector<std::string> patterns) { m_excludePatterns = std::move(patterns); }
    void setNamingTemplate(const std::string& tmpl) { m_namingTemplate = tmpl; }
    void setDefaultFormat(PackageFormat fmt) { m_defaultFormat = fmt; }
    void setIncludeHiddenFiles(bool val) { m_includeHiddenFiles = val; }
    void setFollowSymlinks(bool val) { m_followSymlinks = val; }
    void setMaxTotalBytes(uint64_t bytes) { m_maxTotalBytes = bytes; }
    void setMaxFileCount(uint64_t count) { m_maxFileCount = count; }
    void setMaxPerFileBytes(uint64_t bytes) { m_maxPerFileBytes = bytes; }

    std::string serializeSettingsJson() const;
    void deserializeSettingsJson(const std::string& json);

private:
    std::string m_id;
    std::string m_workspaceId;
    std::string m_name;
    std::string m_category{"General File Bundle"};
    std::string m_schemaVersion{"1.0.0"};
    std::vector<std::string> m_includePatterns{"*"};
    std::vector<std::string> m_excludePatterns{
        "*.tmp", "*.bak", "*.swp", "*~",
        ".git*", ".svn*", ".hg*",
        "Thumbs.db", ".DS_Store", "desktop.ini",
        "__pycache__*", "*.pyc", "*.pyo"
    };
    std::string m_namingTemplate{"{product}-{version}"};
    PackageFormat m_defaultFormat{PackageFormat::ZipArchive};
    bool m_includeHiddenFiles{false};
    bool m_followSymlinks{false};
    uint64_t m_maxTotalBytes{10ULL * 1024 * 1024 * 1024}; // 10 GB limit default
    uint64_t m_maxFileCount{500000};
    uint64_t m_maxPerFileBytes{4ULL * 1024 * 1024 * 1024}; // 4 GB per file default
    std::string m_createdAtUtc;
    std::string m_updatedAtUtc;
};

} // namespace shipdesk::domain
