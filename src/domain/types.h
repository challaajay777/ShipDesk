#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace shipdesk::domain {

enum class ReleaseState {
    Draft,
    Inspecting,
    Inspected,
    Validating,
    Validated,
    BuildPending,
    Building,
    BuildSucceeded,
    BuildFailed,
    Staging,
    Staged,
    Packaging,
    Packaged,
    Verifying,
    Verified,
    ReadyToPublish,
    Publishing,
    Published,
    PublishFailed,
    Cancelled,
    Archived,
    Interrupted,
    NeedsReview
};

inline std::string releaseStateToString(ReleaseState state) {
    switch (state) {
        case ReleaseState::Draft: return "Draft";
        case ReleaseState::Inspecting: return "Inspecting";
        case ReleaseState::Inspected: return "Inspected";
        case ReleaseState::Validating: return "Validating";
        case ReleaseState::Validated: return "Validated";
        case ReleaseState::BuildPending: return "BuildPending";
        case ReleaseState::Building: return "Building";
        case ReleaseState::BuildSucceeded: return "BuildSucceeded";
        case ReleaseState::BuildFailed: return "BuildFailed";
        case ReleaseState::Staging: return "Staging";
        case ReleaseState::Staged: return "Staged";
        case ReleaseState::Packaging: return "Packaging";
        case ReleaseState::Packaged: return "Packaged";
        case ReleaseState::Verifying: return "Verifying";
        case ReleaseState::Verified: return "Verified";
        case ReleaseState::ReadyToPublish: return "ReadyToPublish";
        case ReleaseState::Publishing: return "Publishing";
        case ReleaseState::Published: return "Published";
        case ReleaseState::PublishFailed: return "PublishFailed";
        case ReleaseState::Cancelled: return "Cancelled";
        case ReleaseState::Archived: return "Archived";
        case ReleaseState::Interrupted: return "Interrupted";
        case ReleaseState::NeedsReview: return "NeedsReview";
    }
    return "Draft";
}

inline ReleaseState releaseStateFromString(const std::string& str) {
    if (str == "Inspecting") return ReleaseState::Inspecting;
    if (str == "Inspected") return ReleaseState::Inspected;
    if (str == "Validating") return ReleaseState::Validating;
    if (str == "Validated") return ReleaseState::Validated;
    if (str == "BuildPending") return ReleaseState::BuildPending;
    if (str == "Building") return ReleaseState::Building;
    if (str == "BuildSucceeded") return ReleaseState::BuildSucceeded;
    if (str == "BuildFailed") return ReleaseState::BuildFailed;
    if (str == "Staging") return ReleaseState::Staging;
    if (str == "Staged") return ReleaseState::Staged;
    if (str == "Packaging") return ReleaseState::Packaging;
    if (str == "Packaged") return ReleaseState::Packaged;
    if (str == "Verifying") return ReleaseState::Verifying;
    if (str == "Verified") return ReleaseState::Verified;
    if (str == "ReadyToPublish") return ReleaseState::ReadyToPublish;
    if (str == "Publishing") return ReleaseState::Publishing;
    if (str == "Published") return ReleaseState::Published;
    if (str == "PublishFailed") return ReleaseState::PublishFailed;
    if (str == "Cancelled") return ReleaseState::Cancelled;
    if (str == "Archived") return ReleaseState::Archived;
    if (str == "Interrupted") return ReleaseState::Interrupted;
    if (str == "NeedsReview") return ReleaseState::NeedsReview;
    return ReleaseState::Draft;
}

enum class FindingSeverity {
    Info,
    Warning,
    Error,
    Blocker
};

inline std::string findingSeverityToString(FindingSeverity s) {
    switch (s) {
        case FindingSeverity::Info: return "Info";
        case FindingSeverity::Warning: return "Warning";
        case FindingSeverity::Error: return "Error";
        case FindingSeverity::Blocker: return "Blocker";
    }
    return "Info";
}

inline FindingSeverity findingSeverityFromString(const std::string& str) {
    if (str == "Warning") return FindingSeverity::Warning;
    if (str == "Error") return FindingSeverity::Error;
    if (str == "Blocker") return FindingSeverity::Blocker;
    return FindingSeverity::Info;
}

enum class ReleaseChannel {
    Draft,
    Preview,
    Alpha,
    Beta,
    Stable,
    Archived
};

inline std::string releaseChannelToString(ReleaseChannel ch) {
    switch (ch) {
        case ReleaseChannel::Draft: return "Draft";
        case ReleaseChannel::Preview: return "Preview";
        case ReleaseChannel::Alpha: return "Alpha";
        case ReleaseChannel::Beta: return "Beta";
        case ReleaseChannel::Stable: return "Stable";
        case ReleaseChannel::Archived: return "Archived";
    }
    return "Draft";
}

inline ReleaseChannel releaseChannelFromString(const std::string& str) {
    if (str == "Preview") return ReleaseChannel::Preview;
    if (str == "Alpha") return ReleaseChannel::Alpha;
    if (str == "Beta") return ReleaseChannel::Beta;
    if (str == "Stable") return ReleaseChannel::Stable;
    if (str == "Archived") return ReleaseChannel::Archived;
    return ReleaseChannel::Draft;
}

enum class PackageFormat {
    Directory,
    ZipArchive,
    TarArchive
};

inline std::string packageFormatToString(PackageFormat fmt) {
    switch (fmt) {
        case PackageFormat::Directory: return "Directory";
        case PackageFormat::ZipArchive: return "ZIP";
        case PackageFormat::TarArchive: return "TAR";
    }
    return "Directory";
}

inline PackageFormat packageFormatFromString(const std::string& str) {
    if (str == "ZIP" || str == "Zip" || str == "ZipArchive") return PackageFormat::ZipArchive;
    if (str == "TAR" || str == "Tar" || str == "TarArchive") return PackageFormat::TarArchive;
    return PackageFormat::Directory;
}

inline std::string currentUtcTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tmUtc{};
#if defined(_WIN32)
    gmtime_s(&tmUtc, &timeT);
#else
    gmtime_r(&timeT, &tmUtc);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tmUtc, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

} // namespace shipdesk::domain
