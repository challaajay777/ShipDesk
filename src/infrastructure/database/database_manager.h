#pragma once

#include "domain/workspace/workspace.h"
#include "domain/profile/release_profile.h"
#include "domain/release/release_draft.h"
#include "domain/release/release_record.h"
#include "domain/destination/destination.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <mutex>

namespace shipdesk::infrastructure {

class DatabaseManager {
public:
    static DatabaseManager& instance();

    // Initializes SQLite database at path, runs migrations, enables WAL, foreign keys, busy timeout.
    // Use in-memory for testing: ":memory:"
    bool initialize(const std::string& dbPath);
    void close();

    bool isInitialized() const;
    std::string databasePath() const { return m_dbPath; }

    // Runs PRAGMA integrity_check, returns true if "ok"
    bool runIntegrityCheck(std::string& outDetails);

    // Workspace Operations
    bool saveWorkspace(const domain::Workspace& ws);
    std::optional<domain::Workspace> getWorkspace(const std::string& id);
    std::vector<domain::Workspace> listWorkspaces(bool includeArchived = false);
    bool deleteWorkspace(const std::string& id);

    // Source Root Operations
    bool saveSourceRoot(const domain::SourceRoot& root);
    std::vector<domain::SourceRoot> getSourceRoots(const std::string& workspaceId);
    bool deleteSourceRoot(const std::string& rootId);

    // Release Profile Operations
    bool saveReleaseProfile(const domain::ReleaseProfile& profile);
    std::optional<domain::ReleaseProfile> getReleaseProfile(const std::string& id);
    std::vector<domain::ReleaseProfile> listReleaseProfiles(const std::string& workspaceId);

    // Release Draft Operations
    bool saveReleaseDraft(const domain::ReleaseDraft& draft);
    std::optional<domain::ReleaseDraft> getReleaseDraft(const std::string& id);
    std::vector<domain::ReleaseDraft> listReleaseDrafts(const std::string& workspaceId);

    // Release Run & Record Operations
    bool saveReleaseRun(const domain::ReleaseRunRecord& run);
    std::optional<domain::ReleaseRunRecord> getReleaseRun(const std::string& id);
    std::vector<domain::ReleaseRunRecord> listReleaseRuns(const std::string& releaseDraftId);

    bool saveReleaseRecord(const domain::ImmutableReleaseRecord& record);
    std::optional<domain::ImmutableReleaseRecord> getReleaseRecord(const std::string& id);
    std::vector<domain::ImmutableReleaseRecord> listReleaseRecords();

    // Event Logging
    bool logEvent(const domain::EventLogRecord& event);
    std::vector<domain::EventLogRecord> getEvents(const std::string& workspaceId, size_t limit = 100);

    // Interrupted job recovery: marks transient runs as Interrupted on startup
    int recoverInterruptedRuns();

private:
    DatabaseManager() = default;
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool runMigrations();

    std::string m_dbPath;
    std::string m_connectionName{"shipdesk_db"};
    std::mutex m_mutex;
    bool m_initialized{false};
};

} // namespace shipdesk::infrastructure
