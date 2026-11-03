#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace shipdesk::infrastructure {

struct GitRepositoryStatus {
    bool is_git_repository{false};
    std::string branch_name;
    std::string head_commit_hash;
    bool is_dirty{false};
    std::vector<std::string> remotes; // Safe redacted remote URLs
    std::string error_message;
};

class GitInspector {
public:
    // Safely inspects Git repository at targetPath without mutating anything.
    // Redacts credentials from remote URLs.
    static GitRepositoryStatus inspect(const std::filesystem::path& targetPath);

    // Redacts user:password from git remote URLs
    static std::string redactRemoteUrl(const std::string& url);
};

} // namespace shipdesk::infrastructure
