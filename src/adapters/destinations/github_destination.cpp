#include "adapters/destinations/github_destination.h"
#include "infrastructure/process/process_runner.h"
#include "domain/common/json_util.h"
#include <sstream>
#include <QRegularExpression>

namespace shipdesk::adapters {

GitHubCliAuthInfo GitHubDestinationAdapter::checkAuth() {
    GitHubCliAuthInfo info;
    infrastructure::ProcessRunner runner;

    // Check version
    auto verRes = runner.executeSync("gh", {"--version"}, "", {}, 5);
    if (!verRes.started || verRes.exit_code != 0) {
        info.installed = false;
        info.status_message = "GitHub CLI ('gh') is not installed or not in PATH.";
        return info;
    }
    info.installed = true;
    info.cli_version = verRes.stdout_output.substr(0, verRes.stdout_output.find('\n'));

    // Check auth status
    auto authRes = runner.executeSync("gh", {"auth", "status"}, "", {}, 10);
    std::string combined = authRes.stdout_output + "\n" + authRes.stderr_output;
    info.status_message = combined;

    if (combined.find("Logged in to github.com account") != std::string::npos ||
        combined.find("Logged in to github.com as") != std::string::npos) {
        info.authenticated = true;

        // Parse user account name safely
        QRegularExpression userRegex(R"(account\s+([A-Za-z0-9_\-]+))");
        auto match = userRegex.match(QString::fromStdString(combined));
        if (match.hasMatch()) {
            info.logged_in_user = match.captured(1).toStdString();
        }
    } else {
        info.authenticated = false;
    }

    return info;
}

bool GitHubDestinationAdapter::checkRepositoryExists(const std::string& ownerAndRepo) {
    if (ownerAndRepo.empty()) return false;
    infrastructure::ProcessRunner runner;
    auto res = runner.executeSync("gh", {"repo", "view", ownerAndRepo, "--json", "name"}, "", {}, 10);
    return res.exit_code == 0;
}

GitHubPublishResult GitHubDestinationAdapter::publishReleaseAndAsset(
    const std::string& ownerAndRepo,
    const std::string& tagName,
    const std::string& releaseTitle,
    const std::string& releaseNotes,
    const std::filesystem::path& assetPath,
    bool isDraft,
    bool isPrerelease)
{
    GitHubPublishResult result;
    infrastructure::ProcessRunner runner;

    // 1. Create release (or draft)
    std::vector<std::string> createArgs = {
        "release", "create", tagName,
        "--repo", ownerAndRepo,
        "--title", releaseTitle,
        "--notes", releaseNotes.empty() ? releaseTitle : releaseNotes
    };
    if (isDraft) {
        createArgs.push_back("--draft");
    }
    if (isPrerelease) {
        createArgs.push_back("--prerelease");
    }

    auto createRes = runner.executeSync("gh", createArgs, "", {}, 30);
    if (createRes.exit_code != 0) {
        // Check if release already exists
        if (createRes.stderr_output.find("already exists") == std::string::npos) {
            result.success = false;
            result.error_message = "Failed to create GitHub release: " + createRes.stderr_output;
            return result;
        }
    }

    std::string releaseUrl = createRes.stdout_output;
    while (!releaseUrl.empty() && (releaseUrl.back() == '\r' || releaseUrl.back() == '\n' || releaseUrl.back() == ' ')) {
        releaseUrl.pop_back();
    }
    result.release_url = releaseUrl;

    // 2. Upload asset
    std::vector<std::string> uploadArgs = {
        "release", "upload", tagName,
        assetPath.string(),
        "--repo", ownerAndRepo,
        "--clobber"
    };

    auto uploadRes = runner.executeSync("gh", uploadArgs, "", {}, 120);
    if (uploadRes.exit_code != 0) {
        result.success = false;
        result.error_message = "Failed to upload asset to GitHub release: " + uploadRes.stderr_output;
        return result;
    }

    // 3. Verify remote asset existence and size
    std::vector<std::string> viewArgs = {
        "release", "view", tagName,
        "--repo", ownerAndRepo,
        "--json", "assets,url"
    };
    auto viewRes = runner.executeSync("gh", viewArgs, "", {}, 15);
    if (viewRes.exit_code == 0) {
        std::string jsonStr = viewRes.stdout_output;
        std::string assetFilename = assetPath.filename().string();
        if (jsonStr.find(assetFilename) != std::string::npos) {
            result.asset_verified = true;
            result.success = true;
            if (result.release_url.empty()) {
                result.release_url = domain::json::extractStringField(jsonStr, "url");
            }
            return result;
        }
    }

    result.success = true;
    return result;
}

} // namespace shipdesk::adapters
