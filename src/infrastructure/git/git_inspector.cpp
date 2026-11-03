#include "infrastructure/git/git_inspector.h"
#include "infrastructure/process/process_runner.h"
#include <fstream>
#include <sstream>
#include <QRegularExpression>

namespace shipdesk::infrastructure {

std::string GitInspector::redactRemoteUrl(const std::string& url) {
    QString qUrl = QString::fromStdString(url);
    // Mask https://username:password@host
    static const QRegularExpression credRegex(R"(([a-zA-Z]+://[^:]+:)([^@]+)(@))");
    qUrl.replace(credRegex, "\\1[REDACTED_PASSWORD]\\3");
    return qUrl.toStdString();
}

GitRepositoryStatus GitInspector::inspect(const std::filesystem::path& targetPath) {
    GitRepositoryStatus status;

    std::filesystem::path gitDir = targetPath / ".git";
    if (!std::filesystem::exists(gitDir)) {
        status.is_git_repository = false;
        return status;
    }

    status.is_git_repository = true;

    // Use read-only git commands
    ProcessRunner runner;

    // 1. Get branch name
    auto branchRes = runner.executeSync(
        "git", {"rev-parse", "--abbrev-ref", "HEAD"},
        targetPath.string(), {}, 5
    );
    if (branchRes.exit_code == 0) {
        std::string b = branchRes.stdout_output;
        while (!b.empty() && (b.back() == '\r' || b.back() == '\n' || b.back() == ' ')) b.pop_back();
        status.branch_name = b;
    }

    // 2. Get HEAD commit hash
    auto headRes = runner.executeSync(
        "git", {"rev-parse", "HEAD"},
        targetPath.string(), {}, 5
    );
    if (headRes.exit_code == 0) {
        std::string h = headRes.stdout_output;
        while (!h.empty() && (h.back() == '\r' || h.back() == '\n' || h.back() == ' ')) h.pop_back();
        status.head_commit_hash = h;
    }

    // 3. Check dirty state (read-only)
    auto dirtyRes = runner.executeSync(
        "git", {"status", "--porcelain"},
        targetPath.string(), {}, 5
    );
    if (dirtyRes.exit_code == 0) {
        std::string out = dirtyRes.stdout_output;
        while (!out.empty() && (out.back() == '\r' || out.back() == '\n' || out.back() == ' ')) out.pop_back();
        status.is_dirty = !out.empty();
    }

    // 4. Get remotes and redact
    auto remoteRes = runner.executeSync(
        "git", {"remote", "-v"},
        targetPath.string(), {}, 5
    );
    if (remoteRes.exit_code == 0) {
        std::istringstream iss(remoteRes.stdout_output);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty()) {
                status.remotes.push_back(redactRemoteUrl(line));
            }
        }
    }

    return status;
}

} // namespace shipdesk::infrastructure
